#ifndef __DEBUG_UTILS_H__
#define __DEBUG_UTILS_H__

#include "nrf51.h"
#include "nrf51_bitfields.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "boards.h"

typedef void (*uart_rx_handler_t) (uint8_t byte);

#if defined(DEBUG) || defined(DEBUG_VERBOSE)
#define DEBUG_PRINT printf

uint32_t current_time_get(void);
void current_time_start(void);
void uart_init(uart_rx_handler_t p_rx_handler);
#else
#define DEBUG_PRINT(...)
#define current_time_get 0
#define current_time_start(...)
#endif /* defined(DEBUG) || defined(DEBUG_VERBOSE) */

#if defined(DEBUG_VERBOSE)
#define VERBOSE_PRINT printf
#else
#define VERBOSE_PRINT(...)
#endif

#endif /* __DEBUG_UTILS_H__ */
