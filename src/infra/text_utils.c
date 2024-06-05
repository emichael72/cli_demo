
/**
  ******************************************************************************
  * @file    test_utils.c
  * @brief   C misfits.
  ******************************************************************************
  */

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

/** @defgroup Utilities
  * @brief Utilities module
  * @{
  */

int __toupper(int c)
{
    if ( c >= 97 && c <= 122 )
        c -= 32;
    return (c);
}

int __isdigit(int c)
{
    if ( c >= 48 && c <= 57 )
        return (1);
    return (0);
}

int __isalpha(int c)
{
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

int __tolower(int c)
{
    if ( c >= 'A' && c <= 'Z' )
        return c + 'a' - 'A';
    else
        return c;
}

int __isspace(int c)
{
    return ((c == ' ') || (c == '\n') || (c == '\t'));
}

void __strrev(char *str)
{
    int           i;
    int           j;
    unsigned char a;
    unsigned      len = strlen((const char *) str);
    for ( i = 0, j = len - 1; i < j; i++, j-- )
    {
        a      = str[i];
        str[i] = str[j];
        str[j] = a;
    }
}

int __itoa(int num, char *str, int base)
{
    int sum = num;
    int i   = 0;
    int digit;
    int len = strlen(str);

    do
    {
        digit = sum % base;
        if ( digit < 0xA )
            str[i++] = '0' + digit;
        else
            str[i++] = 'A' + digit - 0xA;
        sum /= base;
    } while ( sum && (i < (len - 1)) );
    if ( i == (len - 1) && sum )
        return -1;
    str[i] = '\0';
    __strrev(str);
    return 0;
}

/**
 * @brief
 * Remove all leading & trailing white-spaces from input string _inplace_.
*/

char *__strtrim(char *in_str)
{
    unsigned char *str = (unsigned char *) in_str, *ptr, *str_begin, *str_end;
    unsigned int   c;

    if ( ! str )
        return 0;

    /* Remove leading spaces */
    for ( str_begin = str; (c = *str); str++ )
    {
        if ( __isspace(c) )
            continue;

        /* Copy from str_begin to end --skipping trailing white
         * spaces */
        str_end = str_begin;
        for ( ptr = str_begin; (c = *ptr = *str); str++, ptr++ )
        {
            if ( ! __isspace(c) )
                str_end = ptr;
        }

        *(str_end + 1) = 0;
        return (char *) str_begin;
    }

    *str_begin = 0;
    return (char *) str_begin;
}

/**
 * @brief
 *  Case insensitive strcmp(). Non-ISO.
*/

int __stricmp(const unsigned char *pStr1, const unsigned char *pStr2)
{
    unsigned char c1, c2;
    int           v;

    do
    {
        c1 = *pStr1++;
        c2 = *pStr2++;

        /* The casts are necessary when pStr1 is shorter & char is signed */
        v = (unsigned int) __tolower(c1) - (unsigned int) __tolower(c2);

    } while ( (v == 0) && (c1 != '\0') && (c2 != '\0') );

    return v;
}

/**
 * @brief
 *   Converts string to lower case. Non-ISO.
*/

char *__strlwr(char *str)
{
    unsigned char *p = (unsigned char *) str;

    while ( *p )
    {
        *p = __tolower((unsigned char) *p);
        p++;
    }

    return str;
}

/**
 * @brief
 *  This function is an ANSI version of strstr() with
 *     case insensitivity.
*/

char *__stristr(const char *String, const char *Pattern)
{
    char   *pptr, *sptr, *start;
    int32_t slen, plen;

    for ( start = (char *) String, pptr = (char *) Pattern, slen = strlen(String), plen = strlen(Pattern);

          /* while string length not shorter than pattern length */
          slen >= plen;

          start++, slen-- )
    {
        /* find start of pattern in string */
        while ( __toupper(*start) != __toupper(*Pattern) )
        {
            start++;
            slen--;

            /* if pattern longer than string */

            if ( slen < plen )
                return (NULL);
        }

        sptr = start;
        pptr = (char *) Pattern;

        while ( __toupper(*sptr) == __toupper(*pptr) )
        {
            sptr++;
            pptr++;

            /* if end of pattern then pattern was found */

            if ( '\0' == *pptr )
                return (start);
        }
    }

    return (NULL);
}

/**
 * @}
 */
