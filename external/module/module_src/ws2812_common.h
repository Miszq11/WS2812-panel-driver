#ifndef WS2812_COMMON_H
#define WS2812_COMMON_H

#include "linux/fb.h"
#include "module_config.h"

//IOCTL CODES
#define WS_IO_DUMMY _IO('x', 1)
#define WS_IO_PROCESS_AND_SEND _IO('x', 2)
#define WS2812_WORKQUEUE_NAME "WS2812_simple_module"

#define WS2812_SPI_TRUE 0b11111100 /* check that!*/
#define WS2812_SPI_FALSE 0b11000000 /* check that as well!*/

#define WS2812_SPI_BUS_NUM 0
#define WS2812_SPI_MAX_SPEED_HZ 10000000
#define WS2812_SPI_TARGET_HZ 8000000
#define WS2812_ZERO_PAADING_SIZE 50*WS2812_SPI_TARGET_HZ/8000000+10


struct fb_init_values {
  unsigned x_panel_length, y_panel_length;
  unsigned short color_bits,
      green_offset, red_offset, blue_offset;
  const struct fb_ops* prep_fb_ops;
};

static int module_errno = 0;
bool run_continously = false;

int WS2812_work_init(struct WS2812_module_info* info);
struct WS2812_module_info* frame_buffer_init(struct WS2812_module_info* mod_info, struct fb_init_values* fb_init);

void WS2812_uninit_framebuffer(struct WS2812_module_info* info);
void WS2812_uninit_work(struct WS2812_module_info* info);
void WS2812_uninit_spi(struct WS2812_module_info* info);

void WS2812_spi_setup_message(struct WS2812_module_info* info);
void WS2812_spi_transfer_begin(struct WS2812_module_info* info);

static int WS2812_map(struct fb_info* info, struct vm_area_struct* vma);

#endif