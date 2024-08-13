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

/**
 * @brief probe function for the device
 * 
 * @param dev pointer to a device struct 
 * @return int 
 */
/*
int WS2812_probe(struct device *dev){

return 0;
}*/
//EXPORT_SYMBOL_GPL(WS2812_probe);

