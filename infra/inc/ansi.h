
/**
  ******************************************************************************
  *
  * @file    ansi.h
  * @brief   ANSI terminal codes.
  *  
  ******************************************************************************
  */

#ifndef __ANSI_H__
#define __ANSI_H__

#define ANSI_CLS         "\033[2J"
#define ANSI_CLR         "\033[K"
#define ANSI_CURSOR_OFF  "\033[?25l"
#define ANSI_CURSOR_ON   "\033[?25h"
#define ANSI_BLACK       "\033[30m"          /* Black */
#define ANSI_RED         "\033[1;31m"        /* Red */
#define ANSI_GREEN       "\033[1;32m"        /* Green */
#define ANSI_YELLOW      "\033[1;33m"        /* Yellow */
#define ANSI_BLUE        "\033[1;34m"        /* Blue */
#define ANSI_MAGENTA     "\033[1;35m"        /* Magenta */
#define ANSI_CYAN        "\033[1;36m"        /* Cyan  */
#define ANSI_WHITE       "\033[37m"          /* White */
#define ANSI_BOLDBLACK   "\033[1m\033[30m"   /* Bold Black */
#define ANSI_BOLDRED     "\033[1m\033[1;31m" /* Bold Red */
#define ANSI_BOLDGREEN   "\033[1m\033[1;32m" /* Bold Green */
#define ANSI_BOLDYELLOW  "\033[1m\033[1;33m" /* Bold Yellow */
#define ANSI_BOLDBLUE    "\033[1m\033[1;34m" /* Bold Blue */
#define ANSI_BOLDMAGENTA "\033[1m\033[1;35m" /* Bold Magenta */
#define ANSI_BOLDCYAN    "\033[1m\033[1;36m" /* Bold Cyan */
#define ANSI_BOLDWHITE   "\033[1m\033[37m"   /* Bold White */
#define ANSI_BG_BLACK    "\033[0;40m"
#define ANSI_FG_BLACK    "\033[1;30m"
#define ANSI_BG_WHITE    "\033[0;47m"
#define ANSI_FG_WHITE    "\033[1;37m"
#define ANSI_BG_CYAN     "\033[0;46m"
#define ANSI_BG_YELLOW   "\033[0;43m"
#define ANSI_FG_DEFAULT  "\033[0;39m"
#define ANSI_BG_DEFAULT  "\033[0;49m"
#define ANSI_MODE        ANSI_CURSOR_ON ANSI_BG_DEFAULT ANSI_FG_DEFAULT
#define ANSI_SPACES "                                " /* 32 Spaces */

#endif /* __ANSI_H__  */
