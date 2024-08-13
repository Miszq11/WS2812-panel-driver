#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/export.h>

#include "ws2812.h"
#include "SPI_mod_lib.h"
#include "def_msg.h"
// CLEAN HEADERS BELOW!
#include "asm-generic/errno-base.h"
#include "asm/memory.h"
#include "asm/page.h"
#include "asm/string.h"
#include "linux/fb.h"
#include "linux/gfp.h"
#include "linux/irqflags.h"
#include "linux/kern_levels.h"
#include "linux/kernel.h"
#include "linux/mm.h"
#include "linux/moduleparam.h"
#include "linux/platform_device.h"
#include "linux/slab.h"
#include "linux/spi/spi.h"
#include "linux/stddef.h"
#include "linux/types.h"
#include "linux/vmalloc.h"
#include "linux/workqueue.h"
#include "module_config.h"
#include "debug_ctrl.h"
#include "vdso/bits.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Przemyslaw Zeranski");
MODULE_DESCRIPTION("WS2812 stip driver (dts version) over SPI. Based on non-dts version.");

#define MY_BUS_NUM 0

struct spi_master *master;

struct spi_board_info spi_device_info = {
	.modalias = "WS2812_panel",
	.max_speed_hz = 10000000,
	.bus_num = MY_BUS_NUM,
	.chip_select = 0,
	.mode = 0,
};

static struct spi_device *WS2812_panel;

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




//! Structure for device names
static struct of_device_id WS2812_dt_match_id[] ={
  {
    .compatible = "WS2812_panel",
  },
  {}
};
MODULE_DEVICE_TABLE(of, WS2812_dt_match_id);


static const struct spi_device_id WS2812_dt_spi_id[] = {
	{"WS2812_panel", 0},
	{ }
};
MODULE_DEVICE_TABLE(spi, WS2812_dt_spi_id);


static int WS2812_spi_probe(struct spi_device *spi){
  printk("Starting probe function\n");
  int ret = 0;
  
  struct device *dev;
  struct fb_ops *fb_dev;
  struct ws2812_spi_mod_priv *ws2812_spi_mod;
	

  fb_dev = devm_kzalloc(&spi->dev, sizeof(*fb_dev),GFP_KERNEL);
  if(!fb_dev)
    return -ENOMEM;
  else
    printk("devm_kzalloc success\n");

  //spi->mode = SPI_MODE_1;
  spi->bits_per_word = 8;

	master = spi_busnum_to_master(MY_BUS_NUM);
	master -> max_transfer_size = NULL;
	if(!master) {
		printk("There is no spi bus with Nr. %d\n", MY_BUS_NUM);
		return -1;
	}


  if(spi_setup(spi)==0){
    printk("SPI setup success\n");
  }else{
    printk("SPI setup error\n");
    return -1;
  }

printk("Sending short default message with size: %d\n",sizeof(short_msg));
	if(spi_write(WS2812_panel, short_msg, sizeof(short_msg)/sizeof(u8))==0){
		printk("Def short msg sent, sucess!\n");
	}else{
		printk("FAIL!\n");
	}
return ret;
}


static int WS2812_remove(struct spi_device *spi){
return 0;
}


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



