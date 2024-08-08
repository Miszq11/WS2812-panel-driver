#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/export.h>

#include "ws2812.h"

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
  int ret = 0;
  
  struct device *dev = &spi->dev;
	struct fb_ops *fb_dev;
	

  fb_dev = devm_kzalloc(&spi->dev, sizeof(*fb_dev),GFP_KERNEL);
  if(!fb_dev)
    return -ENOMEM;
  else
    printk("devm_kzalloc success\n");

  spi->mode = SPI_MODE_1;
  spi->bits_per_word = 8;
  if(spi_setup(spi)==0){
    printk("SPI setup success\n");
  }else{
    printk("SPI setup error\n");
    return -1;
  }

return ret;
};


static int WS2812_remove(struct spi_device *spi){
return 0;
};


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



