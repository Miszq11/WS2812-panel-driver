#ifndef WS2812_DEBUG_CTRL_H
#define WS2812_DEBUG_CTRL_H
#include <linux/printk.h>

#define WS2812_DEBUG_STM

  #ifdef WS2812_DEBUG
    #define PRINT_LOG(frmt, ...) printk("[LOG_WS2812]: " frmt, __VA_ARGS__)
    #define PRINT_ERR(frmt, ...) printk("\033[1;31m[ERR_WS2812]:\033[0m " frmt, __VA_ARGS__)
    #define PRINT_ERR_FA(frmt, ...) printk("\033[1;31m[CRITICAL ERR_WS2812]:\033[0m " frmt, __VA_ARGS__)
    #define PRINT_GOOD(frmt, ...) printk("\033[1;32m[INFO GOOD_WS2812]:\033[0m " frmt, __VA_ARGS__)
  #endif


    #ifdef WS2812_DEBUG_STM
 #define PRINT_LOG(frmt, ...) pr_info("[LOG_WS2812]: " frmt, __VA_ARGS__)
    #define PRINT_ERR(frmt, ...) pr_err("\033[1;31m[ERR_WS2812]:\033[0m " frmt, __VA_ARGS__)
    #define PRINT_ERR_FA(frmt, ...) pr_err("\033[1;31m[CRITICAL ERR_WS2812]:\033[0m " frmt, __VA_ARGS__)
    #define PRINT_GOOD(frmt, ...) pr_info("\033[1;32m[INFO GOOD_WS2812]:\033[0m " frmt, __VA_ARGS__)
  #endif
#endif