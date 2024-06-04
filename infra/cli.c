/**
  ******************************************************************************
  * @file    cli.c
 *  @brief   The brain behind this awesome CLI module.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "cli.h" /* Module local include */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include "llist.h" 	/* Basic lists manipulation */

/** @defgroup CLI CLI
  * @brief CLI module
  * @{
  */

/* Private define ------------------------------------------------------------*/
/** @defgroup CLI_Private_define CLI Private Define
  * @{
  */

/*!************************************************************************/ /**
  * @brief
  *   Escape sequence processing stuff
  *
  *****************************************************************************/
#define CLI_ARROW_UP         200
#define CLI_ARROW_DOWN       201
#define CLI_TAB              202
#define CLI_ARROW_RIGHT      203
#define CLI_ARROW_LEFT       204
#define CLI_MAX_ESCAPE       10
#define CLI_MIN(a, b)        (((a) < (b)) ? (a) : (b))
#define CLI_DELIMIT          " "
#define CLI_MAX_PASSWORD_LEN 12

/* Send carriage return line feed sequence */
#define CLI_SEND_CRLF(val) CLI_Print("\r\n", 2)

/* ASCII control characters */
#define ASCII_STX (0x02)
#define ASCII_ETX (0x03)
#define ASCII_ACK (0x06)
#define ASCII_NAK (0x15)

/**
  * @}
  */

/* Private typedef -----------------------------------------------------------*/
/** @defgroup CLI_Private_Typedef CLI Private Typedef
  * @{
  */

/**
  * @brief
  *  Local types and structures.
  */

/* Escape sequence:
 * https://en.wikipedia.org/wiki/Escape_sequences_in_C#:~:text=An%20escape%20sequence%20is%20a,or%20impossible%20to%20represent%20directly.
 */

typedef struct __CLI_EscTypeDef
{
    const char *string;
    uint8_t     value;

} CLI_EscTypeDef;

typedef enum __CLI_EexecTypeDef
{
    CLI_Exec_Nothing = 0,
    CLI_Exec_AutoComplete,
    CLI_Exec_SearchAndExec,
    CLI_Exec_RetrieveHistory,

} CLI_ExecTypeDef;

/**
  * @brief  A single table instance (list node).
  */
typedef struct __CLI_TableNode_TypeDef
{
    const CLI_CmdTypeDef *          table; /* pointer to a commands table instance */
    uint32_t                        items; /* Count of table elements */
    struct __CLI_TableNode_TypeDef *next;  /* Next pointer to the other siblings nodes */

} CLI_TableNode_TypeDef;

/**
 * @brief
 *   The CLI module locals.
 */

typedef struct __CLI_DataTypeDef
{

    char                      line[CLI_MAX_HISTORY_LINES][CLI_MAX_LINE_LENGTH + 16]; /* Command buffer. */
    char                      argvBuf[CLI_MAX_LINE_LENGTH + 16];     /* Command line is copied here before execution; then it will be tokenized. */
    char                      prompt[CLI_MAX_PROMPT + 2];            /* Prompt textual buffer. */
    CLI_CmdTypeDef *          cmnds;                                 /* Commands array. */
    CLI_TableNode_TypeDef *   cmndsTableHead;                        /* Multiple linked CLI tables */
    char *                    completion[CLI_MAX_COMPLETIONS];       /* Command completion */
    CLI_InitTypeDef           cliInitData;                           /* CLI configuration provided when initialized. */
    CLI_ExecTypeDef           execType;                              /* What to do when we're being triggered from a task context. */
    uint16_t                  cmndsCount;                            /* Count of loaded commands. */
    uint8_t                   lineIdx;                               /* Index in the history array. */
    uint8_t                   lineCurrent;                           /* Where current command is stored. */
    uint8_t                   LineCount;                             /* How many command stored at all */
    uint8_t                   LineBack;                              /* Index of command when walking through history */
    uint32_t                  cmndEvent;                             /* Event to raise  when a command is pending execution. */
    uint8_t                   prmpSize;                              /* Prompt length. */
    CLI_EscTypeDef            escapeSequence[CLI_MAX_ESCAPE];        /* Escape sequence container for arrow up and arrow down. */
    bool                      receivingEscapeSequence;               /* Escape sequence. */
    char                      CurrentEscapeSequence[CLI_MAX_ESCAPE]; /* Escape sequence. */
    uint8_t                   CurrentEscapeSequenceCount;            /* Escape sequence. */
    bool                      initialized;                           /* Module initialization flag. */
    bool                      echo;                                  /* Do we have to echo back to the terminal? */
    bool                      locked;                                /* Locks the CLI. */
    bool                      autoLowerCase;                         /* Force lower case input. */
    bool                      commandsSorted;                        /*Use binary searching. */

} CLI_DataTypeDef;

/**
  * @}
  */

/* Private variables ---------------------------------------------------------*/
/** @defgroup CLI_Private_Variables CLI Private Variables
  * @{
  */

/*! Global instance of the CLI data structure, some elements must be initialized at compile stage. */
static CLI_DataTypeDef gCliData = {0};

/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/
/** @defgroup CLI_Private_Functions CLI Private Functions
  * @{
  */


/**
 * @brief
 *   STD C: Simple byte by byte terminal printer. */

static void CLI_PutByte(char c)
{
	if(gCliData.cliInitData.handlers.putc)
	{
		putchar(c);
		fflush(stdout);
	}
}

/**
 * @brief
 *   STD C: Simple byte by byte terminal printer. */

static void CLI_Print(char *s, int len)
{
    if ( s )
    {
        if ( len )
        {
            /* GCC pedantic: array subscript 1 is outside array bounds of 1 */
            if ( len == 1 )
            {
            	CLI_PutByte(*s);
                return;
            }

            /* Print while not NULL and length is not 0 */
            while ( *s && len )
            {
                CLI_PutByte(*s);
                s++;
                len--;
            }
        }
        else
        {
            /* No length, print up to the first NUL occurrence  */
            while ( *s )
            {
                CLI_PutByte(*s);
                s++;
            }
        }
    }
}

/**
 * @brief
 * Simple iterative binary search.
 *  Return -1 or the index at which we found the byte in question.
*/

static int32_t CLI_SearchChar(CLI_CmdTypeDef *pCommand, int32_t startIndex, int32_t count, char *searched, bool matchAll, bool sorted)
{
    int32_t middle = -1, i = 0, found = 0;

    if ( ! searched || ! count || startIndex > count || ! pCommand || *searched == 0 )
        return -1;

    if ( sorted == true )
    {
        while ( startIndex <= count )
        {
            middle = startIndex + (count - startIndex) / 2;
            if ( pCommand[middle].Name[0] == *searched )
            {
                if ( matchAll == false )
                    return middle;
                else
                    break;
            }
            if ( pCommand[middle].Name[0] < *searched )
                startIndex = middle + 1;
            else
                count = middle - 1;
        }
    }

    /* The array was sorted and we've used the binary search, in this case we
     * ignore 'matchAll' and return. */
    if ( sorted == true )
        return middle;

    if ( matchAll == true )
    {
        found = -1;

        /* Iterate through the command structure looking for a match on the first
         * parameter (the command).
         * Check that argument count is OK then call the function. */

        while ( i < gCliData.cmndsCount )
        {
            if ( gCliData.cliInitData.handlers.stristr((const char *) searched, (const char *) pCommand[i].Name) != 0 )
            {
                found = i;
                break;
            }

            i++;
        }
    }

    return found;
}

/**
 * @brief
 * Find the number of consecutive matching characters in two strings.
 *  return Number of common character
*/

static uint8_t CLI_MatchChars(char *a, char *b)
{
    uint8_t al = (uint8_t) strlen(a);
    uint8_t bl = (uint8_t) strlen(b);
    uint8_t i;

    int32_t ml = CLI_MIN(al, bl);

    for ( i = 0; i < ml; i++ )
        if ( a[i] != b[i] )
            return i;

    return 0;
}

/**
 * @brief
 *  Implements a simple tab completer that matches on
 *  commands available.
*/

static uint8_t CLI_TabCompleter(char *cmpLine, uint8_t cmpLen)
{
    uint8_t i               = 0;
    uint8_t completionCount = 0;
    char    formatted[64]   = {0};
    int     flen            = 0;

    if ( ! gCliData.cmnds ) /* No commands loaded. */
        return 0;

    if ( gCliData.commandsSorted == true )
    {
        /* Sorted array can optimize and search for the first letter in O(log N)
         * instead of the hideous linear O(N), there are duplicates so this is
         * as far as we go.
         */

        if ( CLI_SearchChar(gCliData.cmnds, 0, gCliData.cmndsCount, cmpLine, false, gCliData.commandsSorted) == -1 )
            return completionCount;
    }

    while ( i < gCliData.cmndsCount && completionCount < CLI_MAX_COMPLETIONS )
    {
        /* Parasoft : The size_t argument passed to any function in string.h shall have an appropriate value [BD-API-STRSIZE] */
        if ( (cmpLen > 0) && (strncmp(gCliData.cmnds[i].Name, cmpLine, cmpLen) == 0) )
        {
            gCliData.completion[completionCount] = (char *) gCliData.cmnds[i].Name;
            completionCount++;
        }

        i++;
    }

    if ( completionCount > 0 )
    {
        char *  lcd  = gCliData.completion[0] + cmpLen;
        uint8_t plen = (uint8_t) strlen(lcd);
        uint8_t nlen = 0;
        char *  line = gCliData.line[gCliData.lineCurrent];

        for ( i = 1; i < completionCount; i++ )
        {
            nlen = CLI_MatchChars(lcd, gCliData.completion[i] + cmpLen);
            if ( nlen < plen )
                plen = nlen;
        }

        gCliData.lineIdx = cmpLen + plen;
        memcpy(line, gCliData.completion[0], gCliData.lineIdx);
        line[gCliData.lineIdx] = '\0';

        if ( completionCount == 1 )
        {
            line[gCliData.lineIdx++] = ' ';
            line[gCliData.lineIdx]   = '\0';
        }

        if ( plen != 0 )
            CLI_Print(line + cmpLen, 0);
        else
        {
            uint8_t display = 0;
            if ( gCliData.echo == true )
                CLI_SEND_CRLF();
            for ( i = 0; i < completionCount; i++ )
            {
                flen = snprintf(formatted, (sizeof(formatted) - 1), "%-19s", gCliData.completion[i]);
                if ( flen > 0 )
                    CLI_Print(formatted, flen);

                display++;
                if ( display == 3 && i != (completionCount - 1) )
                {
                    if ( gCliData.echo == true )
                        CLI_SEND_CRLF();
                    display = 0;
                }
                else
                    printf(" ");
            }

            if ( gCliData.echo == true )
                CLI_SEND_CRLF();
            CLI_PrintPrompt(1);
            CLI_Print(line, 0);
        }
    }

    return completionCount;
}

/**
 * @brief
 * called in escape sequence mode
 * Checking if received char yields exact escape sequence match
 * if it does, return matching sequence code, returns zero otherwise
 * it is assumed that valid escape sequence can't be prefix of
 * another valid escape sequence.
 */

static uint8_t CLI_ProcessEscapeSequnceChar(char c)
{
    uint8_t idx        = 0;
    uint8_t matchCount = 0;
    uint8_t matchIdx   = 0;

    gCliData.CurrentEscapeSequence[gCliData.CurrentEscapeSequenceCount++] = c;

    /* Look for matching escape sequences. */
    for ( idx = 0; gCliData.escapeSequence[idx].string; idx++ )
    {

        if ( strncmp(gCliData.CurrentEscapeSequence, gCliData.escapeSequence[idx].string, gCliData.CurrentEscapeSequenceCount) == 0 )
        {
            matchCount++;
            matchIdx = idx;
        }
    }

    /* Check matches. */
    switch ( matchCount )
    {
        case 0:
            /* No match, discard escape sequence, finish escape mode. */
            gCliData.receivingEscapeSequence = false;
            return 0;

        case 1:
            /* Unique match, process the sequence, finish escape mode. */
            gCliData.receivingEscapeSequence = false;
            return gCliData.escapeSequence[matchIdx].value;

        default:
            /* Multiple matches, continue reading. */
            return 0;
    }
}

/**
 * @brief
 *  STDC qsort required comparator, used only when dynamic memory is available.
 *  Compare command a and b alphabetically according to it's name property.
 */

static int CLI_Compare(const void *a, const void *b)
{
    return strcmp(((CLI_CmdTypeDef *) a)->Name, ((CLI_CmdTypeDef *) b)->Name);
}

/**
 * @brief
 *  Use ANSI codes to erase a single char.
 */

static void CLI_EraseChar(void)
{
    if ( gCliData.lineIdx != 0 )
    {
        gCliData.line[gCliData.lineCurrent][--gCliData.lineIdx] = '\0';
        CLI_Print("\b \b", 3);
    }
}

/**
 * @brief
 *  Use ANSI codes to erase current line.
 */

static void CLI_EraseLine(void)
{

    /* Clear all characters from the cursor position to the end of the line
     * using ANSI codes. */

    char cmd[]     = {"\033["};
    char lenVal[8] = {0};

    int len = gCliData.prmpSize + gCliData.lineIdx;
    gCliData.cliInitData.handlers.itoa(len, lenVal, 10);
    len = (int) strlen(lenVal);

    CLI_Print(cmd, sizeof(cmd));
    CLI_Print(lenVal, len);
    CLI_Print("D\033[K", 4);
}

/**
 * @brief
 *  Goes LineBack command back, retrieves command from history and puts
 *  it in current slot.
 */

static bool cliRetrieveHistory(void)
{
    uint8_t history_idx = 0;
    char *  src_line    = NULL; /* Copy command line from here. */
    char *  dst_line    = NULL; /* Copy command line there. */
    int     len;

    CLI_EraseLine();

    history_idx = (gCliData.lineCurrent + CLI_MAX_HISTORY_LINES - gCliData.LineBack);
    history_idx %= CLI_MAX_HISTORY_LINES;
    src_line = gCliData.line[history_idx];
    dst_line = gCliData.line[gCliData.lineCurrent];

    /* Copy from history to current command line. */
    for ( gCliData.lineIdx = 0; src_line[gCliData.lineIdx]; gCliData.lineIdx++ )
    {
        dst_line[gCliData.lineIdx] = src_line[gCliData.lineIdx];
    }

    dst_line[gCliData.lineIdx] = '\0';

    /* Print new command line. */
    CLI_PrintPrompt(0);
    len = strlen(gCliData.line[gCliData.lineCurrent]);

    CLI_Print(gCliData.line[gCliData.lineCurrent], len);
    return true;
}

/**
 * @brief
 *  Parse the input buffer.  This will tokenize the input line buffer
 *  (destructively) and check the number of parameters.  It finds the matching
 *  command based on the first parameter and executes its associated function if
 *  Sufficient arguments are provided.
 */

static int CLI_ParseEndExec(CLI_CmdTypeDef *pCommand, char line[])
{
    /* Parameter token pointers. */
    char *  param[CLI_MAX_NUM_PARAMS];
    uint8_t paramCount = 0;
    uint8_t i          = 0;
    uint8_t handled    = 0;
    int     cmdRet     = 0;

    /* Should not ever happen but better safe than sorry. */
    if ( ! pCommand )
        return 0;

    /* '#' Comments will return immediately */
    i = 0;
    if ( '#' == line[0] )
        return -1;

    /* First call to strtok. */
    // parasoft-begin-suppress BD-PB-CHECKRETGEN "begin suppress BD-PB-CHECKRETGEN"
    param[paramCount++] = strtok(line, CLI_DELIMIT);
    if ( ! param[0] )
        return -1;
    // parasoft-end-suppress BD-PB-CHECKRETGEN "end suppress BD-PB-CHECKRETGEN"

    while ( 1 )
    {
        param[paramCount] = strtok(NULL, CLI_DELIMIT);
        if ( ! param[paramCount] )
            break;

        paramCount++;
        if ( paramCount > (CLI_MAX_NUM_PARAMS - 1) )
        {
            printf("Too many arguments");
            if ( gCliData.echo == true )
                CLI_SEND_CRLF();
            {
                return -1;
            }
        }
    }

    /* Handle empty command line. */
    if ( (paramCount == 1) && (param[0] == NULL) )
        return -1;

    /* Iterate through the command structure looking for a match on the first parameter (the command).
     * Check that argument count is OK then call the function. */

    while ( i < gCliData.cmndsCount )
    {
        if ( gCliData.cliInitData.handlers.stricmp((const unsigned char *) param[0], (const unsigned char *) pCommand[i].Name) == 0 )
        {
            handled = 1;
            if ( (paramCount - 1) >= 0 )
            {
                if ( gCliData.echo == false )
                    CLI_SEND_CRLF();

                /* Call the function pointer in the command record. */
                cmdRet = pCommand[i].pHandler(paramCount, param);
                if ( gCliData.echo == true )
                    CLI_SEND_CRLF();
                break;
            }
            else
            {
                printf("Not enough arguments");
                if ( gCliData.echo == true )
                    CLI_SEND_CRLF();
                break;
            }
        }

        i++;
    }

    if ( ! handled )
    {
        printf("'%s' is not recognized as an internal command.\r\n", line);
        cmdRet = EXIT_FAILURE;
        if ( gCliData.echo == true )
            CLI_SEND_CRLF();
    }

    return cmdRet;
}

/**
 * @brief
 *  Execute a command (once it was found).
 */

static void CLI_ExecuteCommand(void)
{

    int cmdRet = 0;

    /* No commands in memory or pending for execution. */
    if ( gCliData.cmnds != NULL )
    {

        if ( gCliData.echo == true )
            CLI_SEND_CRLF();

        /* Process command if it is not empty. */
        if ( '\0' != *gCliData.line[gCliData.lineCurrent] )
        {
            uint8_t prev_line_idx = 0;

            /* Parse and execute. */
            memset(gCliData.argvBuf, 0, sizeof(gCliData.argvBuf));
            strncpy(gCliData.argvBuf, gCliData.line[gCliData.lineCurrent], sizeof(gCliData.argvBuf) - 1);

            /* Optional non-ascii indication that a command is starting execution. */

            /* Execute! */
            cmdRet = CLI_ParseEndExec(gCliData.cmnds, gCliData.argvBuf);

            /* Check is save the command in history. */
            prev_line_idx = (gCliData.lineCurrent + CLI_MAX_HISTORY_LINES - 1);
            prev_line_idx %= CLI_MAX_HISTORY_LINES;

            if ( strcmp(gCliData.line[gCliData.lineCurrent], gCliData.line[prev_line_idx]) )
            {
                /* Last command differs from previous one, move to next
                 * slot. so the last command remain in history. */

                gCliData.lineCurrent = (gCliData.lineCurrent + CLI_MAX_HISTORY_LINES + 1);
                gCliData.lineCurrent %= CLI_MAX_HISTORY_LINES;

                if ( gCliData.LineCount < CLI_MAX_HISTORY_LINES - 1 )
                    gCliData.LineCount++;
            }

            gCliData.lineIdx                       = 0;
            gCliData.line[gCliData.lineCurrent][0] = '\0';
        }

        if ( cmdRet != CLI_RESET_CMD ) /* Reserved for reset command. */
        {
            if ( gCliData.echo == true )
                CLI_PrintPrompt(0);
            else
                CLI_PrintPrompt(1);
        }
    }
}

/**
 * @brief
 *  Scan the input buffer and figure if it marches any of the commands we have,
 *  if we find a match, invoke the command handler and execute it.
 * @retval boolean: true if a command was executed.
 */

static bool CLI_SearchAndExecute(void)
{
    if ( gCliData.initialized == false )
        return false;

    bool commandTriggered = true;

    /* Fast verification that we have something to execute. */
    if ( *gCliData.line[gCliData.lineCurrent] )
    {

        /* Make sure that there something worthwhile to alert the supper loop. */
        if ( CLI_SearchChar(gCliData.cmnds, 0, gCliData.cmndsCount, gCliData.line[gCliData.lineCurrent], true, gCliData.commandsSorted) == -1 )
            commandTriggered = false;
    }

    /* Execute.. */
    if ( commandTriggered == true )
        CLI_ExecuteCommand();
    else
    {
        /* Nothing to execute, simply dump the prompt and we're done. */
        if ( *gCliData.line[gCliData.lineCurrent] )
        {
            printf("\r\n'%s' is not recognized as an internal command.\r\n", gCliData.line[gCliData.lineCurrent]);
            gCliData.lineIdx                       = 0;
            gCliData.line[gCliData.lineCurrent][0] = '\0';
        }

        /* Print the prompt. */
        CLI_PrintPrompt(1);
    }

    return commandTriggered;
}

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup CLI_Exported_Functions CLI Exported Functions
  * @{
  */

/**
 * @brief
 *   Get's a pointer to the internal stored command.
 *   Caller must validate the returned pointer prior to using it.
 * @retval Pointer to the stored commands or NULL on error.
 */

CLI_CmdTypeDef *CLI_GetCommandsPtr(void)
{
    if ( gCliData.initialized == true )
        return gCliData.cmnds;

    return NULL;
}

/**
 * @brief
 *   Print the command prompt.
 * @param addCrLfCnt: Count of "\r\n" to add before.
 */

void CLI_PrintPrompt(int addCrLfCnt)
{
    if ( gCliData.locked == false )
    {
        while ( addCrLfCnt-- > 0 ) CLI_SEND_CRLF(); /* Dump '\r\n' */
        CLI_Print(gCliData.prompt, gCliData.prmpSize);
    }
}

/**
 * @brief
 *    Gets the sorted commands count.
 * @retval Count of commands.
 */

int CLI_GetCommandCnt(void)
{

    if ( gCliData.initialized == false )
        return 0;

    return gCliData.cmndsCount;
}

/**
  * @brief Injects a table instance to be later merged with all other instances.
  * @param table: Instance to commands table, must be static so its pointer will remain
  *               valid when its host function is exited.
  * @param items: Count of elements within the table.
  *
  * @retval number of injected commands.
  */

int CLI_InjectCommands(const CLI_CmdTypeDef *table, int items)
{
    CLI_TableNode_TypeDef *instance = NULL;

    /* Sanity */
    if ( table == NULL || items == 0 || gCliData.commandsSorted == true )
        return 0;

    instance =  gCliData.cliInitData.handlers.malloc(sizeof(CLI_TableNode_TypeDef)); /* Allocate node pointer */
    if ( instance == NULL )
        return 0; /* No memory */

    instance->items = items;
    instance->table = table;
    instance->next  = NULL;

    /* Attach to the table head */
    LL_APPEND(gCliData.cmndsTableHead, instance);

    return instance->items;
}

/**
  * @brief Aggregate all commands tables, merge them to one while dropping
  *        duplicated commands. When done, sort the table to allow for fast
  *        binary searches.
  * @note  Must be called before attempting to execute any CLI command.
  * @retval boolean, true if went as expected.
  */

bool CLI_BuildTable(void)
{

    uint32_t               total_items = 0;
    uint32_t               total_mem   = 0;
    uint16_t               i           = 0;
    uint16_t               position    = 0;
    bool                   retVal      = false;
    CLI_TableNode_TypeDef *instance    = NULL;
    bool                   duplicate   = false;

    do
    {
        /* Make sure we ware not already aggregated and sorted */
        if (gCliData.commandsSorted || gCliData.cmndsTableHead == NULL )
            break; /* Must call ProtoTable_SetMemory() first */

        /* Count all entries thought all instances so we could calculate
       * the total required memory for all of them.
       */

        LL_FOREACH(gCliData.cmndsTableHead, instance)
        total_items += instance->items;

        if ( total_items == 0 )
            break;

        total_mem = ((total_items + 1) * sizeof(CLI_CmdTypeDef));

        /* Attempt to allocate */
        gCliData.cmnds =  gCliData.cliInitData.handlers.malloc(total_mem);
        if ( gCliData.cmnds == NULL )
            break;

        /* Start fresh */
        memset(gCliData.cmnds, 0, total_mem);

        /* Aggregate - merge into a single table */
        LL_FOREACH(gCliData.cmndsTableHead, instance)
        {
            for ( position = 0; position < instance->items; position++ )
            {

                duplicate = false;

                /* Search for existing duplicated item */
                for ( i = 0; i < gCliData.cmndsCount; i++ )
                {
                    if ( gCliData.cliInitData.handlers.stricmp((unsigned char *) gCliData.cmnds[i].Name, (unsigned char *) instance->table[position].Name) == 0 )
                    {
                        duplicate = true;
                        break;
                    }
                }

                /* Add to the main table if the OP code was unique */
                if ( duplicate == false )
                {
                    memcpy(&gCliData.cmnds[gCliData.cmndsCount], &instance->table[position], sizeof(CLI_CmdTypeDef));

                    /* Force commands to lower case, trim and NULL terminate */
                    gCliData.cmnds[gCliData.cmndsCount].Name[CLI_MAX_COMMAND_NAME_LEN - 1] = 0; /* Force NULL termination */
                    gCliData.cliInitData.handlers.strtrim(gCliData.cmnds[gCliData.cmndsCount].Name);
                    gCliData.cliInitData.handlers.strlwr(gCliData.cmnds[gCliData.cmndsCount].Name);

                    gCliData.cmndsCount++;
                }
            }
        }

        /* Sort */
        qsort(gCliData.cmnds, gCliData.cmndsCount, sizeof(CLI_CmdTypeDef), CLI_Compare);
        gCliData.commandsSorted = true; /* Mark as sorted and effectively disable injections from now no */

        retVal = true;

    } while ( 0 );

    return retVal;
}

/**
 * @brief
 *  Restore CLI engine state machine to its default state.
 *	Handy when the parser goes bananas after UART errors act.
 */

void CLI_ResetState(void)
{
    if ( gCliData.initialized == false )
        return;

    memset(gCliData.line, 0, sizeof(gCliData.line));
    gCliData.lineIdx     = 0;
    gCliData.lineCurrent = 0;
    gCliData.LineCount   = 0;
    gCliData.LineBack    = 0;
}

/**
 * @brief
 *  Performs state logic after an event was set from the CLI bytes processor.
 *  We're doing this to execute most of the CLI logic from a task context rather
 *  than from an interrupt.
  * @retval boolean: true if wen't OK.
  */

bool CLI_ProcessState(void)
{

    bool retVal = false;

    if ( gCliData.initialized == false )
        return false;

    switch ( gCliData.execType )
    {
        case CLI_Exec_SearchAndExec:
            retVal = CLI_SearchAndExecute();
            break;

        case CLI_Exec_AutoComplete:
            retVal = CLI_TabCompleter(gCliData.line[gCliData.lineCurrent], gCliData.lineIdx);
            break;
        case CLI_Exec_RetrieveHistory:
            retVal = cliRetrieveHistory();
            break;
        default:
            break;
    }

    gCliData.execType = CLI_Exec_Nothing;
    return retVal;
}

/**
 * @brief
 *  process a single input byte,
 *  pass the byte through the CLI state machine where we :
 *
 *  - Handle ANSI escape codes.
 *  - Recognize a Line feed carriage return (CrLf  [0x0a 0x0d] )
 *  - Validate the the command stored in the buffer is valid.
 *  - Trigger a polling loop whenever there is a command to execute .
 *
 *  Return true if a full buffer was received and a command execution was triggered.
 *  Else false.
  * @param c: Rx byte.
  * @retval boolean: true if a command is ready to be executed.
  */

bool CLI_ProcessChar(unsigned char c)
{

    bool commandTriggered = false;

    do
    {
        if ( gCliData.initialized == false || c == 0 || c >= 128 || gCliData.execType != CLI_Exec_Nothing )
            break;

        /* Exit  no registered commands */
        if ( gCliData.cmnds == NULL )
            break;

        if ( gCliData.receivingEscapeSequence )
        {
            c = CLI_ProcessEscapeSequnceChar(c);
            if ( ! c )
                break;
        }

        /* Process character */
        switch ( c )
        {
            case '\033':
                // Start of escape sequence
                gCliData.CurrentEscapeSequenceCount = 0;
                gCliData.receivingEscapeSequence    = true;
                break;

            case '\r':
                if ( gCliData.locked ) /* If we're locked, pass the buffer to the external handler */
                {
                    gCliData.lineIdx                       = 0;
                    gCliData.line[gCliData.lineCurrent][0] = '\0';
                }
                else
                {
                    /* Finally alert the super loop / task if valid command was found. */
                    gCliData.execType = CLI_Exec_SearchAndExec;
                    CLI_TaskAlert(); /* Signal an external handler to process the command. */
                }

                gCliData.LineBack = 0;
                break;

            case '\t':
                if ( gCliData.lineIdx > 0 ) /* Make sure index is grater than zero to prevent dumping data on TAB key press event. */
                {

                    /* Alert the super loop / task to execute the auto complete logic. */
                    gCliData.execType = CLI_Exec_AutoComplete;
                    CLI_TaskAlert();
                    gCliData.LineBack = 0;
                }
                break;

            case '\b':
                CLI_EraseChar();
                gCliData.LineBack = 0;
                break;

            case CLI_ARROW_DOWN:
                if ( gCliData.LineBack )
                {
                    gCliData.LineBack--;
                    /* Alert the super loop / task to execute history retrieval. */
                    gCliData.execType = CLI_Exec_RetrieveHistory;
                    CLI_TaskAlert();
                }
                break;

            case CLI_ARROW_UP:
                if ( gCliData.LineBack < gCliData.LineCount )
                {
                    gCliData.LineBack++;

                    /* Alert the super loop / task to execute history retrieval. */
                    gCliData.execType = CLI_Exec_RetrieveHistory;
                    CLI_TaskAlert();
                }
                break;

            case CLI_ARROW_RIGHT:
            {
            }
            break;
            case CLI_ARROW_LEFT:
            {
            }
            break;

            case CLI_TAB:
                if ( gCliData.lineIdx > 0 ) /* Make sure index is grater than zero to prevent dumping data on TAB key press event. */
                {
                    /* Alert the super loop / task to execute the auto complete logic. */
                    gCliData.execType = CLI_Exec_AutoComplete;
                    CLI_TaskAlert();
                    gCliData.LineBack = 0;
                }
                break;

            default:
                /* Add the RX character to the command buffer. */
                if ( gCliData.lineIdx < (CLI_MAX_LINE_LENGTH - 1) )
                {
                    /* Force input to lower case. */
                    if ( gCliData.autoLowerCase == true )
                        c = tolower(c);

                    /* No echo when locked. */
                    if ( gCliData.echo && gCliData.locked == false )
                        CLI_Print((char *) &c, 1);

                    gCliData.line[gCliData.lineCurrent][gCliData.lineIdx++] = c;
                    gCliData.line[gCliData.lineCurrent][gCliData.lineIdx]   = '\0';
                }
                else
                    gCliData.lineIdx = 0;

                gCliData.LineBack = 0;
        }
    } while ( 0 );

    return commandTriggered;
}

/**
  * @brief  initializes CLI internal processor.
  * @param[in] cliInit a pointer to a module configuration structure.
  * @retval none.
  */

bool CLI_Init(CLI_InitTypeDef *cliInit)
{
    char    Prompt[CLI_MAX_PROMPT + 1] = {0};
    uint8_t escIndex                   = 0;

    /* Sanity */
    if ( cliInit == NULL || gCliData.initialized == true )
        return false;

    /* Store configuration locally. */
    memcpy(&gCliData.cliInitData, cliInit, sizeof(CLI_InitTypeDef));

    gCliData.autoLowerCase = cliInit->autoLowerCase;
    gCliData.echo          = cliInit->echo;

    /* StoreS escape sequence values, this could be changed pending on the
     * echoing mode. */
    if ( gCliData.echo == true )
    {
        /* CLI is echoing back to the client (remote is set to echo OFF).
         * Here we only handle escape codes for UP & DOWN where tab is
         * treated as \t */

        gCliData.escapeSequence[escIndex].value    = CLI_ARROW_UP;
        gCliData.escapeSequence[escIndex++].string = "[A";
        gCliData.escapeSequence[escIndex].value    = CLI_ARROW_DOWN;
        gCliData.escapeSequence[escIndex++].string = "[B";
        gCliData.escapeSequence[escIndex].value    = CLI_ARROW_RIGHT;
        gCliData.escapeSequence[escIndex++].string = "[C";
        gCliData.escapeSequence[escIndex].value    = CLI_ARROW_LEFT;
        gCliData.escapeSequence[escIndex++].string = "[D";
    }
    else
    {
        /* CLI is not echoing back to the client (remote is set to echo ON).
         * For this to work we expect the remote to re-map the following in
         * XTerm / ANSI emulation:
         * TAB  : \033T  (Escape followed by 'T')
         * UP   : \033[A (Escape followed by 'A')
         * DOWN : \033[B (Escape followed by 'B')
         * RIGHT :\033[C (Escape followed by 'C')
         * LEFT : \033[D (Escape followed by 'D')
         */

        gCliData.escapeSequence[escIndex].value    = CLI_TAB;
        gCliData.escapeSequence[escIndex++].string = "T";
        gCliData.escapeSequence[escIndex].value    = CLI_ARROW_UP;
        gCliData.escapeSequence[escIndex++].string = "A";
        gCliData.escapeSequence[escIndex].value    = CLI_ARROW_DOWN;
        gCliData.escapeSequence[escIndex++].string = "B";
        gCliData.escapeSequence[escIndex].value    = CLI_ARROW_RIGHT;
        gCliData.escapeSequence[escIndex++].string = "C";
        gCliData.escapeSequence[escIndex].value    = CLI_ARROW_LEFT;
        gCliData.escapeSequence[escIndex++].string = "D";
    }

    gCliData.escapeSequence[escIndex].string = NULL;
    gCliData.escapeSequence[escIndex].value  = 0;

    /* Set the prompt string. */
    snprintf(Prompt, CLI_MAX_PROMPT + 1, "%s>", cliInit->prompt);
    strncpy(gCliData.prompt, Prompt, sizeof(gCliData.prompt) - 1);
    gCliData.prmpSize = (uint8_t) strlen(gCliData.prompt); /* Adjust for time stamp. */

    /* Assume highest credentials when initialized as not locked. */
    if ( cliInit->printPrompt == true )
        CLI_PrintPrompt(1);

    gCliData.initialized = true;

    /* Lastly - fore the auxiliary thread */
    CLI_InitTask();

    return true;
}


/**
  * @}
  */

/**
  * @}
  */

