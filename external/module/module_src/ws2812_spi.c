#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/export.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>

#include "asm-generic/errno-base.h"
#include "debug_ctrl.h"
#include "linux/property.h"
#include "module_config.h"
#include "linux/gfp.h"
#include "linux/spi/spi.h"
#include <linux/string.h>
#include "linux/kern_levels.h"
#include "linux/stddef.h"

#define ELSE_RETURN_ERR else return -1;

#include "ws2812_common.h"

#define MY_BUS_NUM 0

static int WS2812_spi_probe(struct spi_device *);
static int WS2812_remove(struct spi_device *);
static int WS2812_fb_parse_dt(struct device_node* , struct fb_init_values*);


/**
 * @brief Structure for compatible SPI device name in Device Tree (DT)
 *
 */
static struct of_device_id WS2812_dt_match_id[] ={
  {
    .compatible = "swis,ws2812-panel", },
  {}
};
MODULE_DEVICE_TABLE(of, WS2812_dt_match_id);

/**
 * @brief Structure for DT names of SPI device IDs
 *
 */
static const struct spi_device_id WS2812_dt_spi_id[] = {
	{.name="ws2812-panel",0},
  {.name="ws2812-panel",1},
  {.name="ws2812-panel",2},
	{ }
};
MODULE_DEVICE_TABLE(spi, WS2812_dt_spi_id);

/**
 * @brief Framebuffer structure
 *
 */

static const struct fb_ops WS2812_fb_ops = {
  .owner = THIS_MODULE,
  .fb_fillrect = cfb_fillrect,
  .fb_copyarea = cfb_copyarea,
  .fb_imageblit = cfb_imageblit,
  .fb_mmap = WS2812_map,
  .fb_ioctl = WS_fb_ioctl,
  .fb_pan_display = WS2812_fb_pan_display,
};


/**
 * @brief Probe function, runs at driver initialization
 *
 * @param spi SPI device structure
 * @return int
 */
static int WS2812_spi_probe(struct spi_device *spi){
  struct device *dev = &spi->dev;
  struct WS2812_module_info *info;
  struct fb_init_values fb_init_values = {0};

  printk(KERN_ERR "Probing my_device with SPI ID: %s\n", spi->modalias);

  fb_init_values.prep_fb_ops = &WS2812_fb_ops;

  if(!WS2812_fb_parse_dt(dev->of_node, &fb_init_values))
    goto dt_read;

  info = devm_kzalloc(dev, sizeof(struct WS2812_module_info), GFP_KERNEL);
  if(info)
    PRINT_LOG("devm success");
  else
    return -ENOMEM;

  info->WS2812_spi_dev = spi;
  info->WS2812_spi_dev->bits_per_word = BITS_PER_WORD;

  PRINT_LOG("spi dev master bus num: %d\n", info->WS2812_spi_dev->master->bus_num);
  PRINT_LOG("spi dev bits per word: %d\n", info->WS2812_spi_dev->bits_per_word);

  info -> spi_transfer_continous = false;
  info -> spi_transfer_in_progress = false;

  if(frame_buffer_init(info, &fb_init_values)){
    PRINT_LOG("fb init done success\n");
  } else goto framebuffer_initialized;

  if(WS2812_work_init(info)==0) {
    PRINT_LOG("work init done success\n");
  } else goto work_initialized;

  WS2812_spi_setup_message(info);

  return 0;

work_initialized:
  WS2812_uninit_work(info);
framebuffer_initialized:
  WS2812_uninit_framebuffer(info);
  PRINT_ERR_FA("Critical error returning %d from driver\n", module_errno);
  return module_errno;
dt_read:
  PRINT_ERR_FA("Could not parse needed dts entries (did you miss any?)\n");
  return -EINVAL;
}

/**
 * @brief Remove function, runs when driver is unitialized
 *
 * @param spi
 * @return int
 */
static int WS2812_remove(struct spi_device *spi){
  // TODO: add proper removal of fb, spi dev, buffers!!
  int ret = 0;
  pr_info("Probe function remove %d\n",ret);
return ret;
}


/**
 * @brief SPI device driver structure, constains driver compatibility list
 *
 */
static struct spi_driver WS2812_spi_driver = {
  .driver = {
    .name = "ws2812-panel",
    .of_match_table = WS2812_dt_match_id,
  },
  .probe = WS2812_spi_probe,
  .id_table = WS2812_dt_spi_id,
  .remove = WS2812_remove,
};


/**
 * @brief Function reading all necessary properties from device tree
 *        entry, including:
 *
 *        \b Obligatory:
 *          - panel,x-visible-len - Panel x dimension
 *          - panel,y-visible-len - Panel y dimension
 *          - panel,color-bits - Bits every color takes in spi communication
 *          - panel,green-bit-offset - bit offset of green color
 *          - panel,red-bit-offset - bit offset of red color
 *          - panel,blue-bit-offset - bit offset of blue color
 *
 *        \b Optional:
 *          - panel,x-virtual-len - Framebuffer x dimension (may be only bigger than x-visible-len)
 *          - panel,y-virtual-len - Framebuffer y dimension (may be only bigger than y-visible-len)
 *          - panel,x-pan-step - Allowed x-panstep for display panning
 *          - panel,y-pan-step - Allowed y-panstep for display panning
 */

static int WS2812_fb_parse_dt(struct device_node* node, struct fb_init_values* init_vals) {
  u32 of_val32 = 0;

  if(of_property_read_u32(node, "panel,x-visible-len", &of_val32) >= 0) {
    init_vals->x_panel_length = of_val32;
  } ELSE_RETURN_ERR

  if(of_property_read_u32(node, "panel,y-visible-len", &of_val32) >= 0) {
    init_vals->y_panel_length = of_val32;
  } ELSE_RETURN_ERR

  // These 4 parameters are optional (simply set the size of panel)
  if(of_property_read_u32(node, "panel,x-virtual-len", &of_val32) >= 0) {
    init_vals->x_virtual_length = of_val32;
  } else {
    init_vals->x_virtual_length = init_vals->x_panel_length;
  }

  if(of_property_read_u32(node, "panel,y-virtual-len", &of_val32) >= 0) {
    init_vals->y_virtual_length = of_val32;
  } else {
    init_vals->y_virtual_length = init_vals->y_panel_length;
  }

  if(of_property_read_u32(node, "panel,x-pan-step", &of_val32) >= 0) {
    init_vals->y_pan_step = of_val32;
  } else {
    init_vals->y_pan_step = 1;
  }

  if(of_property_read_u32(node, "panel,y-pan-step", &of_val32) >= 0) {
    init_vals->x_pan_step = of_val32;
  } else {
    init_vals->x_pan_step = 1;
  }

  if(of_property_read_u32(node, "panel,color-bits", &of_val32) >= 0) {
    init_vals->color_bits = of_val32;
  } ELSE_RETURN_ERR

  if(of_property_read_u32(node, "panel,green-bit-offset", &of_val32) >=0) {
    init_vals->green_offset = of_val32;
  } ELSE_RETURN_ERR
  if(of_property_read_u32(node, "panel,red-bit-offset", &of_val32) >= 0) {
    init_vals->red_offset = of_val32;
  } ELSE_RETURN_ERR

  if(of_property_read_u32(node, "panel,blue-bit-offset", &of_val32) >= 0) {
    init_vals->blue_offset = of_val32;
  } ELSE_RETURN_ERR

  return 0;
}

module_spi_driver(WS2812_spi_driver);

MODULE_AUTHOR("Przemyslaw Zeranski");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("WS2812 stip driver (dts version) over SPI. Based on non-dts version.");