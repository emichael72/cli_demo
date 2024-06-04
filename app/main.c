/**
  ******************************************************************************
  * @file    main.c
  * @brief   CLI Engine demo.
  ******************************************************************************
  */

#include "main.h"
#include "cli.h"
#include "text_utils.h"

/**
  * @brief  Initialize and start the CLI.
  * @retval bool - true if initialization is successful, false otherwise.
  */
static bool CLI_Start(void)
{
    CLI_InitTypeDef cliInit = {0};

    cliInit.autoLowerCase = false;
    cliInit.echo = true;

    /* Set the prompt */
    strncpy(cliInit.prompt, "Intel", sizeof(cliInit.prompt) - 1);
    cliInit.printPrompt = true;

    /* Set the handler functions */
    cliInit.handlers.itoa = __itoa;
    cliInit.handlers.free = free;
    cliInit.handlers.malloc = malloc;
    cliInit.handlers.putc = ( __cli_putc)putc;
    cliInit.handlers.stricmp = __stricmp;
    cliInit.handlers.stristr = __stristr;
    cliInit.handlers.strlwr = __strlwr;
    cliInit.handlers.strtrim = __strtrim;

    return CLI_Init(&cliInit);
}

/**
  * @brief  Main function to start the CLI demo.
  * @retval int - EXIT_SUCCESS on successful execution.
  */
  
int main(void)
{
    /* Start the CLI */
    if (!CLI_Start())
    {
	   printf("Error: copul start CLI Demo.\n");
	   return EXIT_FAILURE;
    }

    /* Add few commands and build the CLI table */
    cli_addCommands();
    CLI_BuildTable();

    /* Main loop, the can exit ny typing 'exit' */
    while (1)
    {
        sleep(1);
    }

    return EXIT_SUCCESS;
}
