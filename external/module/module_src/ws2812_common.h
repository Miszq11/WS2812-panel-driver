#ifndef WS2812_COMMON_H
#define WS2812_COMMON_H

#include "linux/fb.h" // IWYU pragma: keep
#include "module_config.h"


#ifdef __KERNEL__
/**
 *  @brief structure holding values for framebuffer initialisation
 *    It adds layer of abstraction for different ways of starting the modue
 *
 *  \see static int __init WS2812_init(void)
 *  \see static int WS2812_spi_probe(struct spi_device *spi)
 */
struct fb_init_values {
  /// @brief panel real size: x-pixels by y-pixels
  unsigned x_panel_length, y_panel_length;
  /// @brief virtual pixel board size (whole image)
  unsigned x_virtual_length, y_virtual_length;
  /// @brief Pan span values?
  unsigned x_pan_step, y_pan_step;
  /// @brief bitcount of one color
  unsigned color_bits;
  /// @brief offset of colors in word
  unsigned green_offset, red_offset, blue_offset;
  /// @brief combined structure of fb_ops (may differ in every module, thats why it is here)
  const struct fb_ops* prep_fb_ops;
};

int module_errno = 0;
bool run_continously = false;

int WS2812_work_init(struct WS2812_module_info* info);
struct WS2812_module_info* frame_buffer_init(struct WS2812_module_info* mod_info, struct fb_init_values* fb_init);

void WS2812_uninit_framebuffer(struct WS2812_module_info* info);
void WS2812_uninit_work(struct WS2812_module_info* info);
void WS2812_uninit_spi(struct WS2812_module_info* info);

void WS2812_spi_setup_message(struct WS2812_module_info* info);
void WS2812_spi_transfer_begin(struct WS2812_module_info* info);

int WS2812_map(struct fb_info* info, struct vm_area_struct* vma);
int WS_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);
int WS2812_fb_pan_display(struct fb_var_screeninfo*, struct fb_info*);

#endif //__KERNEL__

#endif //WS2812_COMMON_H