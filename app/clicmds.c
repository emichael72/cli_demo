
/**
  ******************************************************************************
  * @file    clicmds.c
  * @brief   This module contains all of the basic terminal commands
  *          that this product supports.
  ******************************************************************************
  */

#include "cli.h"  /* Command line interface task */
#include "ansi.h"
#include <stdio.h>

/**
 * @brief Dumps out the commands list along with each command's help string.
 * @param argc Argument count
 * @param argv Argument vector
 * @return EXIT_SUCCESS on success
 */
 
static int cli_help(int argc, char **argv)
{
    int i = 0;
    static char *p_arg = "@";
    CLI_CmdTypeDef *p_command = CLI_GetCommandsPtr();

    /* Dump help and exit */
    CLI_SHOW_HELP("List commands.");

    printf("\r\n");
    while (p_command && i < CLI_GetCommandCnt())
    {
        printf(ANSI_CYAN "%-20s " ANSI_MODE, p_command->Name);

        /* Invoke the command with the fixed predefined symbol "@" that should instruct the
         * command to dump its help string and exit. */
        p_command->pHandler(2, &p_arg);
        printf("\r\n");

        p_command++;
        i++;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Shutdown the MCU.
 * @param argc Argument count
 * @param argv Argument vector
 * @return EXIT_SUCCESS on success
 */
 
int cli_exit(int argc, char **argv)
{
    /* Dump help and exit */
    CLI_SHOW_HELP("Terminate CLI.");

    printf("Shutting down..\n");

    /* Gracefully terminate the CLI task */
    CLI_TaskTerminate();

    exit(1);

    return EXIT_SUCCESS;
}

/**
 * @brief Reset the MCU.
 * @param argc Argument count
 * @param argv Argument vector
 * @return EXIT_SUCCESS on success
 */
 
int cli_mcuReset(int argc, char **argv)
{
    /* Dump help and exit */
    CLI_SHOW_HELP("MCU reset.");

    printf("Restarting..\n");

    return EXIT_SUCCESS;
}

/**
 * @brief Dumps out the product version.
 * @param argc Argument count
 * @param argv Argument vector
 * @return EXIT_SUCCESS on success
 */
 
int cli_ver(int argc, char **argv)
{
    /* Dump help and exit */
    CLI_SHOW_HELP("Show version.");

    printf("Version 1.1\n");
    return EXIT_SUCCESS;
}

/**
 * @brief Registers the CLI commands in this module with the CLI engine.
 */
 
void cli_addCommands(void)
{
    /* clang-format off */
    /* Keep all those CLI tables in a fixed global context (aka 'static const') */
    static const CLI_CmdTypeDef gCliBaseCommands[] =
    {
        // Handler                    Name
        //-----------------------------------------------
        { cli_help,                  "?"                },
        { cli_help,                  "help"             },
        { cli_exit,                  "exit"             },
        { cli_mcuReset,              "reset"            },
        { cli_ver,                   "version"          },
    };

    /* Inject all of the commands found in this module.
     * First argument of type 'CLI_CmdTypeDef*' must be global (static) so its
     * pointer will remain at all times. */
    CLI_InjectCommands(gCliBaseCommands, sizeof(gCliBaseCommands) / sizeof(gCliBaseCommands[0]));
}
