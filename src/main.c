
/**
  ******************************************************************************
  *
  * @file    main.c
  * @brief   CLI Engine demo entry pointy.
  * 
  ******************************************************************************
  */

#include "main.h"
#include "cli.h"
#include "text_utils.h"

/**
  * @brief  Initialize and start CLI engine.
  * @retval bool - true if initialization is successful, false otherwise.
  */

static bool CLI_Start(void)
{
    CLI_InitTypeDef cliInit = {0};

    cliInit.autoLowerCase = false;
    cliInit.echo          = true;

    /* Set the prompt */
    strncpy(cliInit.prompt, "Intel", sizeof(cliInit.prompt) - 1);
    cliInit.printPrompt = true;

    /* Set the handler functions, some of which may be available
       by your compiler.*/

    cliInit.handlers.itoa    = __itoa;
    cliInit.handlers.free    = free;
    cliInit.handlers.malloc  = malloc;
    cliInit.handlers.putc    = (__cli_putc) putc;
    cliInit.handlers.stricmp = __stricmp;
    cliInit.handlers.stristr = __stristr;
    cliInit.handlers.strlwr  = __strlwr;
    cliInit.handlers.strtrim = __strtrim;

    return CLI_Init(&cliInit);
}

/**
  * @brief  Main function to start the CLI demo.
  * @retval int - EXIT_SUCCESS on successful execution.
  */

int main(void)
{

    printf("\n---------------------------------------\n");
    printf("\nGreetings!, welcome to 'CLI demo'.\n");
    printf("Type 'exit' when you're done.\n");
    printf("\n---------------------------------------\n");

    /* Start CLI.
       Note: this will spawn the an auxiliary task which will take care of 
       executing CLI command. 
    */
    if ( ! CLI_Start() )
    {
        printf("Error: Could not start CLI Demo.\n");
        return EXIT_FAILURE;
    }

    /* Add few commands and build the CLI table */
    cli_addCommands();

    /* 
     * You can 'inject' CLI commands multiple times from various modules. 
     * However, once you call 'CLI_BuildTable', you will not be able to add more 
     * commands.
     */

    CLI_BuildTable();

    /* Continue with system boot.. */
    while ( 1 )
    {
        sleep(1);
    }

    return EXIT_SUCCESS;
}
