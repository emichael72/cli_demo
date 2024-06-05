
/**
  ******************************************************************************
  *
  * @file    text_utils.h
  * @brief   Code misfits.
  * 
  ******************************************************************************
  */

#ifndef _TEXT_UTILS_H_
#define _TEXT_UTILS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* Exported functions --------------------------------------------------------*/
/** @addtogroup Utilities C supplementary Functions
 * @{
 */

/* Local 'ctype' minimal subset, could reduce binary size, albeit
 * probably not optimized  / efficient implementation */

int   __isdigit(int c);
int   __tolower(int c);
int   __toupper(int c);
int   __isalpha(int c);
int   __isspace(int c);
int   __itoa(int num, char *str, int base);
int   __stricmp(const unsigned char *pStr1, const unsigned char *pStr2);
bool  __isnumeric(char *str);
char *__stristr(const char *String, const char *Pattern);
char *__strtrim(char *in_str);
char *__strlwr(char *str);
void  __strrev(unsigned char *str);

/**
 * @}
 */

#endif /* _TEXT_UTILS_H_ */
