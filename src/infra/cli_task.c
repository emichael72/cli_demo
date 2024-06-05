
/**
  ******************************************************************************
  * 
  * @file    cli_task.c
  * @brief   CLI RTOS Auxillry task.
  * 
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "cli.h"
#include "infra.h"

/* Private define ------------------------------------------------------------*/
/** @defgroup CLI_Task_Private_define CLI_Task Private Define
  * @{
  */

/* RTOS Events */
#define CLI_TASK_EVENT_INIT        (uint32_t)(1 << 0) /*!< Task initialization */
#define CLI_TASK_EVENT_CLI_CMD_REQ (uint32_t)(1 << 1) /*!< Terminal mode: new command line request */
#define CLI_TASK_EVENT_CLI_POLL_RX (uint32_t)(1 << 2) /*!< Periodic poll for user input */
#define CLI_TASK_EVENT_SIGTERM     (uint32_t)(1 << 3) /*!< Terminate task  */

/** @} */

/* Private typedef -----------------------------------------------------------*/
/** @defgroup CLI_Task_Private_Typedef CLI_Task Private Typedef
  * @{
  */

/**
 * @brief The main session information for this module
 */
typedef struct
{
    pthread_t thread; /*!< Thread handle */
    struct
    {
        pthread_mutex_t mutex;       /*!< Mutex for event handling */
        pthread_cond_t  cond;        /*!< Condition variable for event signaling */
        int             event_flags; /*!< Event flags */
    } event;
    bool           initialized;   /*!< Flag indicating if the task is initialized */
    struct termios original_term; /*!< Original terminal settings */
} CLITask_Data_TypeDef;

/** @} */

/* Private variables ---------------------------------------------------------*/
/** @defgroup CLI_Task_Private_Variables CLI_Task Private Variables
  * @{
  */

static CLITask_Data_TypeDef gTaskCli = {0};

/** @} */

/* Private functions ---------------------------------------------------------*/
/** @defgroup CLI_Task_Private_Functions CLI_Task Private Functions
  * @{
  */

/**
 * @brief Set terminal to non-blocking mode.
 * @retval EXIT_SUCCESS on success, EXIT_FAILURE on error.
 */

static int CLI_Terminal_SetNB(void)
{
    struct termios term;
    int            flags;

    /* Get the current terminal attributes */
    if ( tcgetattr(STDIN_FILENO, &term) < 0 )
        return EXIT_FAILURE;

    /* Save the original terminal attributes */
    gTaskCli.original_term = term;

    /* Set the terminal to raw mode */
    term.c_lflag &= ~(ICANON | ECHO | ISIG);
    term.c_iflag &= ~(IXON | ICRNL);
    term.c_oflag &= ~(OPOST);
    term.c_cc[VMIN]  = 0;
    term.c_cc[VTIME] = 1; // 100ms timeout

    if ( tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0 )
        return EXIT_FAILURE;

    /* Set the file descriptor to non-blocking mode */
    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if ( flags < 0 )
        return EXIT_FAILURE;

    if ( fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) < 0 )
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

/**
 * @brief Restore terminal to normal blocking mode.
 * @retval EXIT_SUCCESS on success, EXIT_FAILURE on error.
 */

int CLI_Terminal_SetNormal(void)
{
    int flags;

    /* Restore the terminal to its original mode */
    if ( tcsetattr(STDIN_FILENO, TCSANOW, &gTaskCli.original_term) < 0 )
        return EXIT_FAILURE;

    /* Set the file descriptor to blocking mode */
    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if ( flags < 0 )
        return EXIT_FAILURE;

    if ( fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK) < 0 )
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

/**
 * @brief Signal an event to the CLI task.
 * @param event_flag The event flag to signal.
 */

static void CLI_SignalEvent(int event_flag)
{
    pthread_mutex_lock(&gTaskCli.event.mutex);
    gTaskCli.event.event_flags |= event_flag;
    pthread_cond_signal(&gTaskCli.event.cond);
    pthread_mutex_unlock(&gTaskCli.event.mutex);
}

/**
 * @brief Timer handler function to signal CLI poll event.
 * @param arg Not used.
 */

static void CLI_TimerHandler(union sigval arg)
{
    CLI_SignalEvent(CLI_TASK_EVENT_CLI_POLL_RX);
}

/**
 * @brief Set up a periodic timer.
 * @param interval The timer interval in milliseconds.
 * @retval true on success, false on error.
 */

static bool CLI_Timer_Set(uint32_t interval)
{
    /* Create and start a periodic timer */
    struct sigevent   sev;
    struct itimerspec its;
    timer_t           timerid;

    sev.sigev_notify            = SIGEV_THREAD;
    sev.sigev_value.sival_ptr   = &gTaskCli.event;
    sev.sigev_notify_function   = CLI_TimerHandler;
    sev.sigev_notify_attributes = NULL;

    if ( timer_create(CLOCK_REALTIME, &sev, &timerid) == -1 )
        return false;

    its.it_value.tv_sec     = interval / 1000;
    its.it_value.tv_nsec    = (interval % 1000) * 1000000; // Convert milliseconds to nanoseconds
    its.it_interval.tv_sec  = interval / 1000;
    its.it_interval.tv_nsec = (interval % 1000) * 1000000; // Convert milliseconds to nanoseconds

    if ( timer_settime(timerid, 0, &its, NULL) == -1 )
        return false;

    return true;
}

/**
 * @brief Wait for events and return the event flags.
 * @retval The event flags.
 */

static int CLI_WaitEvents(void)
{
    int events = 0;
    pthread_mutex_lock(&gTaskCli.event.mutex);
    while ( gTaskCli.event.event_flags == 0 )
    {
        pthread_cond_wait(&gTaskCli.event.cond, &gTaskCli.event.mutex);
    }

    events                     = gTaskCli.event.event_flags;
    gTaskCli.event.event_flags = 0; // Reset the event flags after being signaled
    pthread_mutex_unlock(&gTaskCli.event.mutex);

    return events;
}

/**
 * @brief The main CLI task function,
 *        Simiar to a typocal RTOS task.
 * @param arg Not used.
 * @retval NULL
 */

static void *CLI_Task(void *arg)
{
    for ( ;; )
    {
        int cli_events = CLI_WaitEvents(); /* Block indefinitely*/

        /*!****************************************************************/
        /**
         * @brief
         *   Task inner initialization step.
         *
         **********************************************************************/

        if ( (cli_events & CLI_TASK_EVENT_INIT) != 0 )
        {
            /* If we're here it is safe to assume we're initialized. */
            gTaskCli.initialized = true;

            /* Linux: need to switch to RAW terminal so getch()  
             * will not block. */

            CLI_Terminal_SetNB();

            /* Start the periodic 10 milliseconds timer. */
            CLI_Timer_Set(5);
        }

        /*!****************************************************************/
        /**
         * @brief
         *   CLI event handler.
         *   If CLI is enabled and its internal engine received a complete
         *   command, this handler will take care of the actual command execution.
         *
         **********************************************************************/

        if ( (cli_events & CLI_TASK_EVENT_CLI_CMD_REQ) != 0 )
        {
            CLI_ProcessState();
        }

        /*!****************************************************************/ /**
         * @brief
        *   Poll and read the standard input.
        *
        ***********************************************************************/

        if ( (cli_events & CLI_TASK_EVENT_CLI_POLL_RX) != 0 )
        {
            char    c;
            ssize_t n = read(STDIN_FILENO, &c, 1);

            if ( n > 0 )
            {
                /* Pass to the CLI engine */
                CLI_ProcessChar(c);
            }
        }

        /*!****************************************************************/ /**
         * @brief
        *   Task termination event.
        *
        ***********************************************************************/

        if ( (cli_events & CLI_TASK_EVENT_SIGTERM) != 0 )
        {
            break;
        }
    }

    return NULL;
}

/**
 * @brief Alert the CLI task to process a nre CLI command.
 */
void CLI_TaskAlert(void)
{
    if ( gTaskCli.initialized == true )
    {
        CLI_SignalEvent(CLI_TASK_EVENT_CLI_CMD_REQ);
    }
}

/**
 * @brief Terminate the CLI task.
 */

void CLI_TaskTerminate(void)
{
    if ( gTaskCli.initialized == true )
    {

        CLI_SignalEvent(CLI_TASK_EVENT_SIGTERM);

        /* Wait for the thread to finish. */
        pthread_join(gTaskCli.thread, NULL);

        /* Cleanup (this will not be reached in this example, as the loop is infinite). */
        pthread_mutex_destroy(&gTaskCli.event.mutex);
        pthread_cond_destroy(&gTaskCli.event.cond);

        CLI_Terminal_SetNormal();
    }
}

/**
 * @brief Initializes the CLI task.
 */

bool CLI_InitTask(void)
{
    bool success = true;
    int  ret;

    do
    {
        ret = pthread_mutex_init(&gTaskCli.event.mutex, NULL);
        if ( ret != 0 )
        {
            success = false;
            break;
        }

        ret = pthread_cond_init(&gTaskCli.event.cond, NULL);
        if ( ret != 0 )
        {
            success = false;
            break;
        }

        gTaskCli.event.event_flags = 0;

        /* Create a new thread */
        ret = pthread_create(&gTaskCli.thread, NULL, CLI_Task, NULL);
        if ( ret != 0 )
        {
            success = false;
            break;
        }

        /* Make sure we execute the task init event handler. */
        CLI_SignalEvent(CLI_TASK_EVENT_INIT);

    } while ( 0 );

    if ( ! success )
    {
        pthread_cond_destroy(&gTaskCli.event.cond);
        pthread_mutex_destroy(&gTaskCli.event.mutex);
    }

    return success;
}
/** @} */
