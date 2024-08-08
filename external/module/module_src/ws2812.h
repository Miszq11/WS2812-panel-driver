#ifndef WS2812_SPI_MOD_H
#define WS2812_SPI_MOD_H

#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/export.h>

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


/// @brief private struct for WS2812 driver

struct ws2812_spi_mod_priv{
    struct fb_ops *fb_dev;
    struct spi_device *spi_dev;
    // name, speed, reg???
};

int WS2812_probe(struct device *dev);


#endif 