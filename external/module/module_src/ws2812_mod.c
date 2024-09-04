#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/export.h>

#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/spi/spi.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#include "module_config.h"
#include "debug_ctrl.h"


#include "ws2812_common.h"

// MODULE PARAMETERS
unsigned x_panel_len = 8;
unsigned y_panel_len = 8;
unsigned x_virtual_len = 16;
unsigned y_virtual_len = 8;
unsigned x_pan_step = 1;
unsigned y_pan_step = 1;

unsigned short color_bits = 8;
unsigned short g_offset = 0;
unsigned short r_offset = 8;
unsigned short b_offset = 16;

module_param(x_panel_len, uint, 0444); // SETTING IT AS "WORLD-READABLE" for better debugging
module_param(y_panel_len, uint, 0444);
module_param(x_virtual_len, uint, 0444);
module_param(y_virtual_len, uint, 0444);
module_param(x_pan_step, uint, 0444);
module_param(y_pan_step, uint, 0444);
module_param(color_bits, ushort, 0444);
module_param(g_offset, ushort, 0444);
module_param(r_offset, ushort, 0444);
module_param(b_offset, ushort, 0444);

struct WS2812_module_info *module_info = NULL;

static int WS2812_init(void);
static void WS2812_uninit(void);

static void fb_fillrect(struct fb_info* info, const struct fb_fillrect* area);
static void fb_copyarea(struct fb_info* info, const struct fb_copyarea* area);
static void fb_imageblit(struct fb_info* info, const struct fb_image* area);

int WS2812_spi_init(struct WS2812_module_info* info);

/**
 * @brief Framebuffer operations structure
 *
 */

static const struct fb_ops WS2812_fb_ops = {
  .owner = THIS_MODULE,
  //.fb_read = fb_sys_read,
  //.fb_write = fb_sys_write,
  //.fb_fillrect = sys_fillrect, /* obligatory fun (using sys_XX functions for now)*/
  //.fb_copyarea = sys_copyarea, /* obligatory fun (using sys_XX functions for now)*/
  #ifndef NO_DEVICE_TESTING
    .fb_fillrect = cfb_fillrect, /* obligatory fun (using sys_XX functions for now)*/
    .fb_copyarea = cfb_copyarea, /* obligatory fun (using sys_XX functions for now)*/
    .fb_imageblit = cfb_imageblit, /* obligatory fun (using sys_XX functions for now)*/
  #else
    .fb_fillrect = fb_fillrect,
    .fb_copyarea = fb_copyarea,
    .fb_imageblit = fb_imageblit,
  #endif
  .fb_mmap = WS2812_map,
  // FB_WAIT_FOR_VSYNC is called every draw execution
  // should we use that to update display?
  .fb_ioctl = WS_fb_ioctl,
  .fb_pan_display = WS2812_fb_pan_display
};

/**
 * @brief SPI initalization function
 *
 * @param info Module info struct
 * @return int Returns 0 on success
 */

int WS2812_spi_init(struct WS2812_module_info* info) {
  //fill the info structure
  // info->spi_device_info.modalias = "WS"
  struct spi_board_info dummy_spi = {
    .modalias = "WS2812_panel_simple",
    .max_speed_hz = WS2812_SPI_MAX_SPEED_HZ,
    .bus_num = WS2812_SPI_BUS_NUM,
    .chip_select = 0,
    .mode = 0,
  };
  int ret = 0;


  info->WS2812_spi_master = spi_busnum_to_master(WS2812_SPI_BUS_NUM);
  if(!info->WS2812_spi_master) {
    PRINT_ERR_FA("Cannot get spi_master (no bus %d?)\n", WS2812_SPI_BUS_NUM);
    return (module_errno = -ENODEV);
  }

  //spooky scary
  memcpy(&(info->spi_device_info), &dummy_spi, sizeof(struct spi_board_info));
  info->WS2812_spi_dev = spi_new_device(info->WS2812_spi_master, &(info->spi_device_info));
  if(!info->WS2812_spi_dev) {
    PRINT_ERR_FA("Cannot create spi_device\n");
    return (module_errno = -ENODEV);
  }
  info->WS2812_spi_dev->bits_per_word = color_bits;

  if((ret = spi_setup(info->WS2812_spi_dev))) {
    PRINT_ERR_FA("Cannot setup spi device\n");
    spi_unregister_device(info->WS2812_spi_dev);
    return (module_errno = ret);
  }

  // some driver specific data
  info->spi_transfer_in_progress = false;
  info->spi_transfer_continous = run_continously;

  return 0;
}

/**
 * @brief Module initalization function
 *
 * @return int Returns 0 on success
 */

static int __init WS2812_init(void) {
  struct fb_init_values fb_init_vals = {
    .x_panel_length = x_panel_len,
    .y_panel_length = y_panel_len,
    .x_virtual_length = x_virtual_len,
    .y_virtual_length = y_virtual_len,
    .x_pan_step = x_pan_step,
    .y_pan_step = y_pan_step,
    .color_bits = color_bits,
    .green_offset = g_offset,
    .red_offset = r_offset,
    .blue_offset = b_offset,
    .prep_fb_ops = &WS2812_fb_ops,
  };
  struct WS2812_module_info* ret = NULL;

  module_info = vmalloc(sizeof(struct WS2812_module_info));

  ret = frame_buffer_init(module_info, &fb_init_vals);
  if(!ret) {
    PRINT_LOG("ERROR after frame buffer init");
    return module_errno;
  }

  if(WS2812_work_init(module_info)) {
    goto framebuffer_initialized;
  }

  #ifndef NO_DEVICE_TESTING
  if(WS2812_spi_init(module_info)) {
    goto work_initialized;
  }
  #endif

  WS2812_spi_setup_message(module_info);
  PRINT_LOG("WS2812 has succesfully initialise module\n");
  return 0;

work_initialized:
  WS2812_uninit_work(module_info);
framebuffer_initialized:
  WS2812_uninit_framebuffer(module_info);
  PRINT_ERR("Module exit (code %d)!\n", module_errno);
  return module_errno;
}

/**
 * @brief Module deitialization function
 *
 */

static void __exit WS2812_uninit(void) {
  if(!module_info) {
    PRINT_LOG("module_info is NULL. Module didn't initialise correctly?\n");
    return;
  }
  WS2812_uninit_spi(module_info);
  WS2812_uninit_work(module_info);
  WS2812_uninit_framebuffer(module_info);

  printk("Goodbye from WS2812 module\n");
}

static void fb_fillrect(struct fb_info* info, const struct fb_fillrect* area) {return;}
static void fb_copyarea(struct fb_info* info, const struct fb_copyarea* area) {return;}
static void fb_imageblit(struct fb_info* info, const struct fb_image* area) {return;}

module_init(WS2812_init);
module_exit(WS2812_uninit);

MODULE_DESCRIPTION("WS2812 stip driver (non-dts version) over SPI");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Smolinski");
