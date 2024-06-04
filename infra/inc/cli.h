/**
 ******************************************************************************
 * @file    cli.h
 * @brief   CLI engine provides a simple command line interface with very basic
 *          line editing, history, and command completion. It is supposed to run
 *          in a separate task context. The CLI engine uses ANSI escape sequences,
 *          so the corresponding terminal application should be configured as
 *          ANSI or VT100 terminal.
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CLI_H__
#define __CLI_H__

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "infra.h"

/** @addtogroup CLI
 * @{
 */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup CLI_Exported_Macros CLI Exported Macros
 * @{
 */

#define CLI_CONTROL_FLOW_ENABLE (CONFIG_CLI_CONTROL_FLOW_ENABLE)

/* Max number of typed CLI commands remembered by CLI engine. */
#define CLI_MAX_HISTORY_LINES 10

/* Max number of bytes for the UART printf buffer */
#define CLI_MAX_UART_BUFFER_LEN 256

/* Max CLI command line length allowed to type including delimiters,
 * includes termination zero character. */
#define CLI_MAX_LINE_LENGTH 80

/* Max number of completion suggestions supported. */
#define CLI_MAX_COMPLETIONS 32

/* Max size of CLI prompt, including termination zero character. */
#define CLI_MAX_PROMPT 10

/* Maximum bytes allowed for a command name */
#define CLI_MAX_COMMAND_NAME_LEN 12

/* Max number of CLI command parameters. */
#define CLI_MAX_NUM_PARAMS 15

/* Return value reserved for re-setting (prevents echoing the prompt) */
#define CLI_RESET_CMD -10

/* Convert commands to lower case (only in dynamic mode) */
#define CLI_FORCE_LOWER_CASE 1

/* Macro to dump help string and exit from within a CLI command
 * This assumes that you send "@" as argument 0. */
#define CLI_SHOW_HELP(str)                  \
    {                                       \
        if (argc == 2 && *argv[0] == '@')   \
        {                                   \
            printf(str);                    \
            return EXIT_SUCCESS;            \
        }                                   \
    }

#ifndef SIZEOF_ITEM
#define SIZEOF_ITEM(x) (sizeof(x) / sizeof((x)[0])) /*!< Size of a single element in an array */
#endif

/**
 * @}
 */

/* Exported types ------------------------------------------------------------*/
/** @defgroup CLI_Exported_Types CLI Exported Types
  * @{
  */

/** @brief CLI command descriptor structure. */
typedef struct {
    int (*pHandler)(int argc, char **argv); /*!< Pointer to CLI command handler function */
    char Name[CLI_MAX_COMMAND_NAME_LEN];    /*!< Command name, as it should be typed on CLI prompt */
} CLI_CmdTypeDef;

/** @defgroup CLI_ExtHandlers CLI External Handlers
  * @{
  */

/** @brief Function pointer types for external handlers used by CLI */
typedef int   (*__cli_putc)(int, void *);
typedef void* (*__cli_malloc)(size_t);
typedef void  (*__cli_free)(void *);
typedef char* (*__cli_stristr)(const char *, const char *);
typedef char* (*__cli_strtrim)(char *);
typedef char* (*__cli_strlwr)(char *);
typedef int   (*__cli_itoa)(int, char *, int);
typedef int   (*__cli_stricmp)(const unsigned char *, const unsigned char *);

/** @brief CLI external handlers structure */
typedef struct {
    __cli_putc     putc;      /*!< Function to output a character */
    __cli_malloc   malloc;    /*!< Function to allocate memory */
    __cli_free     free;      /*!< Function to free allocated memory */
    __cli_stristr  stristr;   /*!< Function to find a substring case-insensitively */
    __cli_strtrim  strtrim;   /*!< Function to trim a string */
    __cli_strlwr   strlwr;    /*!< Function to convert a string to lower case */
    __cli_itoa     itoa;      /*!< Function to convert an integer to a string */
    __cli_stricmp  stricmp;   /*!< Function to compare strings case-insensitively */
} CLI_ExtHandlersTypDef;

/** @brief CLI initialization structure */
typedef struct {
    CLI_ExtHandlersTypDef handlers; /*!< Caller implemented required API */
    bool printPrompt;               /*!< Print the CLI prompt? */
    bool autoLowerCase;             /*!< Auto set user input to lower case */
    bool echo;                      /*!< Local echo */
    char prompt[CLI_MAX_PROMPT];    /*!< Product prompt, this will prefix the prompt '>' symbol */
} CLI_InitTypeDef;

/**
 * @}
 */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup CLI CLI Exported Functions
 * @{
 */

bool CLI_Init(CLI_InitTypeDef *cliInit);
void CLI_ResetState(void);
bool CLI_ProcessChar(unsigned char c);
int CLI_InjectCommands(const CLI_CmdTypeDef *pCommand, int count);
bool CLI_BuildTable(void);
CLI_CmdTypeDef* CLI_GetCommandsPtr(void);
void CLI_PrintPrompt(int addCrLfCnt);
int CLI_GetCommandCnt(void);
bool CLI_ProcessState(void);

/* Auxiliary task interface */
void CLI_InitTask(void);
void CLI_TaskAlert(void);
void CLI_TaskTerminate(void);

/**
 * @}
 */

/**
 * @}
 */

#endif /* __CLI_H__ */

