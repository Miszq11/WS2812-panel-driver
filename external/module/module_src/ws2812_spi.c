#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/export.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>

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

static int WS2812_spi_probe(struct spi_device *spi);
static int WS2812_remove(struct spi_device *spi);

struct spi_master *master;

/**
 * @brief SPI device info struct, contains device informations
 *
 */
/*
struct spi_board_info spi_device_info = {
	.modalias = "ws2812-panel",
	.max_speed_hz = 10000000,
	.bus_num = MY_BUS_NUM,
	.chip_select = 0,
	.mode = 2,
};
*/
//static struct spi_device *WS2812_panel;

/**
 * @brief SPI board info struct, contains board informations
 *
 */

struct spi_board_info WS2812_board_info =
{
  .modalias     = "ws2812-board",
  .max_speed_hz = 10000000,
  .bus_num      = MY_BUS_NUM,
  .chip_select  = 1,
  .mode         = SPI_MODE_2
};
//spi_register_board_info(spi_board_info, ARRAY_SIZE(WS2812_board_info));

/*****************************************/
/*************** SPI layer ***************/
/*****************************************/



//! Structure for device names
static struct of_device_id WS2812_dt_match_id[] ={
  {
    .compatible = "swis,ws2812-panel", },
  {}
};
MODULE_DEVICE_TABLE(of, WS2812_dt_match_id);

//! Structure for device id
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
};


/**
 * @brief Probe function, runs at driver initialization
 *
 * @param mod_info
 * @return int
 */
static int WS2812_spi_probe(struct spi_device *spi){
  int ret = 0;
  struct device *dev = &spi->dev;
  struct WS2812_module_info *info;
  struct fb_init_values fb_init_values = {0};
  u32 of_val32 = 0;
  u16 of_val16 = 0;
  int num = device_get_child_node_count(dev);

  fb_init_values.prep_fb_ops = &WS2812_fb_ops;
  if(of_property_read_u32(dev->of_node, "panel,x-visible-len", &of_val32) > 0) {
    fb_init_values.x_panel_length = of_val32;
  } ELSE_RETURN_ERR
  if(of_property_read_u32(dev->of_node, "panel,y-visible-len", &of_val32) > 0) {
    fb_init_values.y_panel_length = of_val32;
  } ELSE_RETURN_ERR

  if(of_property_read_u16(dev->of_node, "panel,color-bits", &of_val16) > 0) {
    fb_init_values.color_bits = of_val16;
  } ELSE_RETURN_ERR
  if(of_property_read_u16(dev->of_node, "panel,green-bit-offset", &of_val16) > 0) {
    fb_init_values.green_offset = of_val16;
  } ELSE_RETURN_ERR
  if(of_property_read_u16(dev->of_node, "panel,red-bit-offset", &of_val16) > 0) {
    fb_init_values.red_offset = of_val16;
  } ELSE_RETURN_ERR
  if(of_property_read_u16(dev->of_node, "panel,blue-bit-offset", &of_val16) > 0) {
    fb_init_values.blue_offset = of_val16;
  } ELSE_RETURN_ERR

  printk(KERN_ERR "Probing my_device with SPI ID: %s\n", spi->modalias);
  pr_info("Child dev num: %d\n",num);

  info = devm_kzalloc(dev, sizeof(struct WS2812_module_info), GFP_KERNEL);
  if(info)
    pr_info("devm success");
  else
    return -1;

  info->WS2812_spi_dev = spi;
  pr_info("spi modalias: %s\n",info->WS2812_spi_dev->modalias);
  pr_info("spi dev master bus num: %d\n", info->WS2812_spi_dev->master->bus_num);
  //pr_info("spi master bus num: %d\n", info->WS2812_spi_master->bus_num);
  info -> spi_transfer_continous = false;
  info -> spi_transfer_in_progress = false;


  frame_buffer_init(info, &fb_init_values);


  if(WS2812_work_init(info)) {
    //goto framebuffer_initialized;
    pr_info("work init done success\n");
  }
pr_info("work init done \n");
   // work_initialized:
   // WS2812_uninit_work(info);
   // framebuffer_initialized:
   // WS2812_uninit_framebuffer(info);


return ret;
}

/**
 * @brief Remove function, runs when driver is unitialized
 *
 * @param spi
 * @return int
 */
static int WS2812_remove(struct spi_device *spi){
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
    //.owner = THIS_MODULE,
    //.of_match_table = WS2812_dt_spi_id,
    .of_match_table = WS2812_dt_match_id,
  },
  .probe = WS2812_spi_probe,
  .id_table = WS2812_dt_spi_id,
  .remove = WS2812_remove,
};

module_spi_driver(WS2812_spi_driver);

MODULE_AUTHOR("Przemyslaw Zeranski");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("WS2812 stip driver (dts version) over SPI. Based on non-dts version.");