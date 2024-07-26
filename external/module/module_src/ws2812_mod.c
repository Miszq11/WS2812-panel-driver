#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/export.h>

// CLEAN HEADERS BELOW!
#include "asm-generic/errno-base.h"
#include "asm/memory.h"
#include "asm/page.h"
#include "linux/fb.h"
#include "linux/gfp.h"
#include "linux/mm.h"
#include "linux/moduleparam.h"
#include "linux/platform_device.h"
#include "linux/slab.h"
#include "linux/stddef.h"
#include "linux/types.h"
#include "linux/vmalloc.h"
#include "module_config.h"
#include "debug_ctrl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MS");
MODULE_DESCRIPTION("WS2812 stip driver over SPI");

// MODULE PARAMETERS

unsigned xPanelLen = 8;
unsigned yPanelLen = 8;
//unsigned short colors    = 3;
unsigned short colorBits = 8;
unsigned short gOffset = 0;
unsigned short rOffset = 8;
unsigned short bOffset = 16;

module_param(xPanelLen, uint, 0444); // SETTING IT AS "WORLD-READABLE" for better debugging
module_param(yPanelLen, uint, 0444);
module_param(colorBits, ushort, 0444);
module_param(gOffset, ushort, 0444);
module_param(rOffset, ushort, 0444);
module_param(bOffset, ushort, 0444);

static int module_errno = 0;
struct WS2812_module_fb_info *module_info = NULL;

static int WS2812_init(void);
static void WS2812_uninit(void);

static int WS2812_map(struct fb_info* info, struct vm_area_struct* vma);

static const struct fb_ops WS2812_fb_ops = {
  .owner = THIS_MODULE,
  //.fb_read = fb_sys_read,
  //.fb_write = fb_sys_write,
  //.fb_fillrect = sys_fillrect, /* obligatory fun (using sys_XX functions for now)*/
  //.fb_copyarea = sys_copyarea, /* obligatory fun (using sys_XX functions for now)*/
  //.fb_imageblit = sys_imageblit, /* obligatory fun (using sys_XX functions for now) sig-segv? memory issue*/
  .fb_fillrect = cfb_fillrect, /* obligatory fun (using sys_XX functions for now)*/
  .fb_copyarea = cfb_copyarea, /* obligatory fun (using sys_XX functions for now)*/
  .fb_imageblit = cfb_imageblit, /* obligatory fun (using sys_XX functions for now) sig-segv? memory issue*/
  //.fb_fillrect = fb_fillrect,
  //.fb_copyarea = fb_copyarea,
  //.fb_imageblit = fb_imageblit,
  .fb_mmap = WS2812_map,
};

struct WS2812_module_fb_info* frame_buffer_init(void /* for now? */) {
    //out of the tree module starting?
  struct fb_info *info;
  struct WS2812_module_fb_info *mod_info;
  int ret = -ENOMEM;
  unsigned pixel_buffor_len;

  PRINT_LOG("Module Init call with params: \n\txPanelLen = %d \n\tyPanelLen = %d\
    \n\tcolorBits = %u \n\tgOffset = %u \n\trOffset = %u \n\tbOffset = %u \n",
    xPanelLen, yPanelLen, /*colors,*/ colorBits, gOffset, rOffset, bOffset);

  if(!(info = framebuffer_alloc(sizeof(struct WS2812_module_fb_info), NULL))) {
    PRINT_ERR_FA("frame buffer allocation failed! (private data struc size %u)\n", sizeof(struct WS2812_module_fb_info));
    goto err_framebuffer_alloc;
  }
  PRINT_LOG("framebuffer alloc'ed\n", NULL);
  //set the address of module assigned private data
  mod_info = info->par; // all module info contained in fb private data
  mod_info->info = info;

  // ALLOCATE PIXEL BUFFOR
  pixel_buffor_len = xPanelLen*yPanelLen*(colorBits*3/8);
  mod_info->fb_virt = (u8*) vmalloc(pixel_buffor_len);
  if(!mod_info->fb_virt)  {
    PRINT_ERR_FA("fb_virt alloc failed with %u\n", (unsigned)(mod_info->fb_virt));
    goto err_fb_virt_alloc;
  }
  mod_info->fb_virt_size = pixel_buffor_len;

  // fill the info structure...
  info->screen_base = (char __iomem *)mod_info->fb_virt;
  info->fbops = &WS2812_fb_ops;
  info->fix = WS_fb_fix;
  info->fix.line_length = yPanelLen*(colorBits*3/8);
  info->fix.smem_len = pixel_buffor_len;
  info->fix.smem_start = virt_to_phys(mod_info->fb_virt);

  PRINT_LOG("fb_virt at %p, smem_start at: %p\n", mod_info->fb_virt, info->fix.smem_start);

  info->var.xres = xPanelLen;
  info->var.yres = yPanelLen;
  info->var.xres_virtual = xPanelLen;
  info->var.yres_virtual = yPanelLen;
  info->var.bits_per_pixel = colorBits*3;
  info->var.red.length = colorBits;
  info->var.red.offset = rOffset;
  info->var.green.length = colorBits;
  info->var.green.offset = gOffset;
  info->var.blue.length = colorBits;
  info->var.blue.offset = bOffset;
  info->var.transp.length = 0;
  info->var.transp.offset = 0;
  info->var.activate = FB_ACTIVATE_NOW;

  ret = register_framebuffer(info);
  if(ret < 0){
    PRINT_ERR_FA("Framebuffer register failed!", NULL);
    goto err_fb_register;
  }
  printk("WS2812 has succesfully initialise module\n");
  return mod_info;

  err_fb_register:
  framebuffer_release(info);

  err_fb_virt_alloc:
  if(mod_info->fb_virt)
    vfree(mod_info->fb_virt);

  err_framebuffer_alloc:
  PRINT_ERR_FA("WS2812 module did not initialise\n", NULL);
  module_errno = ret;
  return NULL;
}

static int __init WS2812_init(void) {
  module_info = frame_buffer_init();
  if(!module_info)
    return module_errno;
  return 0;
}

static void __exit WS2812_uninit(void) {
  if(module_info) {
    unregister_framebuffer(module_info->info);
    if(module_info->info->screen_base)
      vfree(module_info->info->screen_base);
    framebuffer_release(module_info->info);
  } else {
    PRINT_LOG("module_info is NULL. Module didn't initialise correctly?\n", NULL);
  }

  printk("Goodbye from WS2812 module\n");
}

static int WS2812_map(struct fb_info* info, struct vm_area_struct* vma) {
  //TODO: test any vma->vm_ops for detecting any R/W operation?
  //This is gonna be fun
  struct WS2812_module_fb_info* priv = info->par;
  size_t page_count = priv->fb_virt_size/PAGE_SIZE + 1, iterator=0, offset=0;
  struct page** pages = kmalloc(page_count*(sizeof(struct page*)), GFP_KERNEL);
  int ret = 0;

  for(offset=0; offset < priv->fb_virt_size; offset += PAGE_SIZE) {
    pages[iterator++] = vmalloc_to_page(priv->fb_virt + offset);
  }

  ret = vm_map_pages_zero(vma, pages, page_count);
  if(ret) {
    PRINT_ERR_FA("vm_map_pages_zero ret=%d, page_count=%u", ret, page_count);
  }

  kfree(pages);
  return ret;
}

// static void fb_fillrect(struct fb_info* info, const struct fb_fillrect* area) {
//   PRINT_LOG("fillrect Called\n", NULL);
// }

// static void fb_copyarea(struct fb_info* info, const struct fb_copyarea* area) {
//   PRINT_LOG("copyarea Called\n", NULL);
// }

// static void fb_imageblit(struct fb_info* info, const struct fb_image* area) {
//   PRINT_LOG("imageblit Called\n", NULL);
// }

module_init(WS2812_init);
module_exit(WS2812_uninit);

// INTREE initialisation
// Think about splitting those files...

// static int WS2812_SPI_probe(struct platform_device *pdev) {
//   struct spi_master *master;
//   PRINT_LOG("\n\t\tWS2812_SPI_PROBE called with *pdev: %p\n\n", pdev);

//   return 0;
// }

// static int WS2812_SPI_remove(struct platform_device *pdev) {
//   return 0;
// }

// static const struct of_device_id WS2812_spi_of_match[] = {
//   {.compatible = "WS2812_panel",},
//   {}
// };

// static struct platform_driver WS2812_spi_driver = {
//   .driver = {
//     .name = "ws2812-spi",
//     .of_match_table = WS2812_spi_of_match,
//   },
//   .probe = WS2812_SPI_probe,
//   .remove = WS2812_SPI_remove,
// };

// module_platform_driver(WS2812_spi_driver);
