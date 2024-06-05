
/**
 *******************************************************************************
 * 
 * @file   infra.h
 * @brief  Place older for a more comprehensive project main header file.
 *
 *******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __INFRA_H__
#define __INFRA_H__

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

/** @addtogroup CORE_LIB
  * @{
  */

/** @addtogroup CORE
 * @{
 */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup CORE_Exported_Macros CORE Exported Macros
 * @{
 */

#ifndef NULL
#define NULL (void *) 0
#endif

#ifndef _IO
#define __IO volatile
#endif

/**
  * @brief  HAL Status structures definition
  */
typedef enum
{
    HAL_OK      = 0x00U,
    HAL_ERROR   = 0x01U,
    HAL_BUSY    = 0x02U,
    HAL_TIMEOUT = 0x03U

} HAL_StatusTypeDef;

#endif /* __INFRA_H__ */
