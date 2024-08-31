#ifndef WS2812_CONFIG_H
#define WS2812_CONFIG_H

#include "linux/spi/spi.h"
#include "linux/workqueue.h"
#include <linux/types.h>
#include "linux/fb.h" // IWYU pragma: keep

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
  u16 *spi_buffer;
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


#endif //WS2812_CONFIG_H
