#ifndef WS2812_CONFIG_H
#define WS2812_CONFIG_H

#include "linux/fb.h"
#include "linux/workqueue.h"
#include <linux/types.h>

#define WS_IO_DUMMY _IO('x', 1)
#define WS_IO_PROCESS_AND_SEND _IO('x', 2)

#define WS2812_WORKQUEUE_NAME "WS2812_simple_module"

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
  u8 *spi_buffer;
  size_t spi_buffer_size; // should be 8 times bigger than fb_virt_size!
  struct fb_info* info;
  struct work_struct WS2812_work;
  struct workqueue_struct *convert_workqueue;
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
