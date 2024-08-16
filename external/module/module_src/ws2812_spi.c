#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/export.h>

#include "linux/workqueue.h"
#include "module_config.h"
#include "def_msg.h"

#include "linux/fb.h"
#include "linux/gfp.h"
#include "linux/spi/spi.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Przemyslaw Zeranski");
MODULE_DESCRIPTION("WS2812 stip driver (dts version) over SPI. Based on non-dts version.");

#define MY_BUS_NUM 0

struct spi_master *master;


/**
 * @brief SPI device info struct, contains device informations
 *
 */

struct spi_board_info spi_device_info = {
	.modalias = "WS2812_panel",
	.max_speed_hz = 10000000,
	.bus_num = MY_BUS_NUM,
	.chip_select = 0,
	.mode = 0,
};

static struct spi_device *WS2812_panel;

/**
 * @brief SPI board info struct, contains board informations
 *
 */

struct spi_board_info WS2812_board_info =
{
  .modalias     = "WS2812-board",
  .max_speed_hz = 10000000,
  .bus_num      = MY_BUS_NUM,
  .chip_select  = 0,
  .mode         = SPI_MODE_0
};


/*****************************************/
/*************** SPI layer ***************/
/*****************************************/


static struct WS2812_module_info *WS2812_spi_mod;



//! Structure for device names
static struct of_device_id WS2812_dt_match_id[] ={
  {
    .compatible = "WS2812_panel",
  },
  {}
};
MODULE_DEVICE_TABLE(of, WS2812_dt_match_id);

//! Structure for device id
static const struct spi_device_id WS2812_dt_spi_id[] = {
	{"WS2812_panel", 0},
	{ }
};
MODULE_DEVICE_TABLE(spi, WS2812_dt_spi_id);

/**
 * @brief Probe function, runs at driver initialization
 *
 * @param mod_info
 * @return int
 */
static int WS2812_spi_probe(struct spi_device *spi){
  printk("Starting probe function\n");
  int ret = 0;

  struct fb_info *info;
  struct WS2812_module_info *ws2812_spi_mod;
  //struct spi_device spi;

  ws2812_spi_mod = devm_kzalloc(&spi->dev, sizeof(struct WS2812_module_info*),GFP_KERNEL);
  if(!ws2812_spi_mod)
    return -ENOMEM;
  else
    printk("devm_kzalloc success\n");

  //spi->mode = SPI_MODE_1;
 // ws2812_spi_mod->WS2812_spi_dev->bits_per_word = 8;

	ws2812_spi_mod->WS2812_spi_master = spi_busnum_to_master(MY_BUS_NUM);
	ws2812_spi_mod->WS2812_spi_master -> max_transfer_size = NULL;
	if(!ws2812_spi_mod->WS2812_spi_master) {
		printk("There is no spi bus with Nr. %d\n", MY_BUS_NUM);
		return -1;
	}


  if(spi_setup(ws2812_spi_mod->WS2812_spi_dev)==0){
    printk("SPI setup success\n");
  }else{
    printk("SPI setup error\n");
    return -1;
  }

printk("Sending short default message with size: %d\n",sizeof(short_msg));
	if(spi_write(ws2812_spi_mod->WS2812_spi_dev, short_msg, sizeof(short_msg)/sizeof(u8))==0){
		printk("Def short msg sent, sucess!\n");
	}else{
		printk("FAIL!\n");
	}
return ret;
}

/**
 * @brief Remove function, runs when driver is unitialized
 *
 * @param spi
 * @return int
 */
static int WS2812_remove(struct spi_device *spi){
return 0;
}




/**
 * @brief SPI device driver structure, constains driver compatibility list
 *
 */
static struct spi_driver WS2812_spi_driver = {
  .probe = WS2812_spi_probe,
  .remove = WS2812_remove,
  .id_table = WS2812_dt_spi_id,
  .driver = {
    .name = "WS2812_panel",
    //.owner = THIS_MODULE,
    //.of_match_table = WS2812_dt_spi_id,
    //.of_match_table = WS2812_dt_match_id,
    },
};
module_spi_driver(WS2812_spi_driver);



