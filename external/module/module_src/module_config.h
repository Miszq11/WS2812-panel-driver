#ifndef WS2812_CONFIG_H
#define WS2812_CONFIG_H

#ifdef __KERNEL__
#include "linux/spi/spi.h"
#include "linux/workqueue.h"
#include <linux/types.h>
#include "linux/fb.h" // IWYU pragma: keep
#endif // __KERNEL

//IOCTL CODES
/// @brief Test ioctl code
#define WS_IO_DUMMY _IO('x', 1)
/// @brief Process pixel buffor and start spitransfer ioctl code
#define WS_IO_PROCESS_AND_SEND _IO('x', 2)

#ifdef __KERNEL__
// Kernel space specific configuration
  /// @brief Process workqueue name
  #define WS2812_WORKQUEUE_NAME "WS2812_simple_module"
  #define BITS_PER_WORD 16

  #if BITS_PER_WORD == 8
  /// @brief True value of one bit in spi transfer
  #define WS2812_SPI_TRUE 0b11111100
  /// @brief False value of one bit in spi transfer
  #define WS2812_SPI_FALSE 0b11000000

  #define WS2812_SPI_BUS_NUM 0
  #define WS2812_SPI_MAX_SPEED_HZ 10000000
  #define WS2812_SPI_TARGET_HZ 8000000
  #define WS2812_ZERO_PAADING_SIZE 50*WS2812_SPI_TARGET_HZ/8000000+10
  #define WS2812_SPI_BUFF_TYPE u8
  #endif

  #if BITS_PER_WORD == 16
  /// @brief True value of one bit in spi transfer
  #define WS2812_SPI_TRUE  0b0111111111000000
  /// @brief False value of one bit in spi transfer
  #define WS2812_SPI_FALSE 0b0111100000000000
  #define WS2812_SPI_MAX_SPEED_HZ 32000000
  #define WS2812_SPI_TARGET_HZ 15000000
  #define WS2812_ZERO_PAADING_SIZE 50*WS2812_SPI_TARGET_HZ/16000000+10
  #define WS2812_SPI_BUFF_TYPE u16
  #endif

  #define WS2812_SPI_BUS_NUM 0

  struct WS2812_module_info {
    // spi
    // dma...
    // workqueue...
    u8 *fb_virt;
    size_t fb_virt_size;
    //work buffer input
    u8 *work_buffer_input;
    size_t work_buffer_input_size;
    //work buffer output -- SPI input
    WS2812_SPI_BUFF_TYPE *spi_buffer;
    size_t spi_buffer_size; // should be 8 times bigger than fb_virt_size!
    struct fb_info* info;
    struct work_struct WS2812_work;
    struct workqueue_struct *convert_workqueue;
    struct spi_master* WS2812_spi_master;
    struct spi_device* WS2812_spi_dev;
    struct spi_board_info spi_device_info;

    struct spi_message WS2812_message;
    struct spi_transfer WS2812_xfer;

    bool spi_transfer_in_progress;
    bool spi_transfer_continous;
    //struct device *dev;
  };



  static const struct fb_fix_screeninfo WS_fb_fix = {
    .id         = "WS2812 Panel fb",
    .type       = FB_TYPE_PACKED_PIXELS, // ??
    .visual     = FB_VISUAL_PSEUDOCOLOR, // ??
    .xpanstep   = 0, // ??
    .ypanstep   = 0, // ??
    .ywrapstep  = 0, // ??
    .accel      = FB_ACCEL_NONE,
  };

  #endif //__KERNEL__
#endif //WS2812_CONFIG_H
