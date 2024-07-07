#ifndef WS2812_DEBUG_CTRL_H
#define WS2812_DEBUG_CTRL_H
#include <linux/printk.h>

#define WS2812_DEBUG

  #ifdef WS2812_DEBUG
    #define PRINT_LOG(frmt, ...) printk("[LOG_WS2812]: " frmt, __VA_ARGS__)
    #define PRINT_ERR(frmt, ...) printk("\033[1;31m[ERR_WS2812]:\033[0m " frmt, __VA_ARGS__)
    #define PRINT_ERR_FA(frmt, ...) printk("\033[1;31m[CRITICAL ERR_WS2812]:\033[0m " frmt, __VA_ARGS__)
  #endif

  #ifndef WS2812_DEBUG
    #define PRINT_LOG(frmt, ...)
    #define PRINT_ERR(frmt, ...)
    #define PRINT_ERR_FA(frmt, ...)
  #endif
#endif