#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <linux/export.h>
//#include <linux/kernel.h>
//#include <linux/uaccess.h>

//#include <linux/mm_types.h>
#include "asm-generic/errno-base.h"
#include "asm/memory.h"
#include "linux/fb.h"
#include "linux/mm.h"
#include "linux/moduleparam.h"
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
//module_param(colors, ushort, 0444);
module_param(colorBits, ushort, 0444);
module_param(gOffset, ushort, 0444);
module_param(rOffset, ushort, 0444);
module_param(bOffset, ushort, 0444);


static int WS2812_init(void);
static void WS2812_uninit(void);

static int WS2812_map(struct fb_info* info, struct vm_area_struct* vma);
static void fb_fillrect(struct fb_info* info, const struct fb_fillrect* rect);
static void fb_copyarea(struct fb_info* info, const struct fb_copyarea* area);
static void fb_imageblit(struct fb_info* info, const struct fb_image* area);

//static struct fb_info info;

static const struct fb_ops WS2812_fb_ops = {
  .owner = THIS_MODULE,
  //.fb_read = fb_sys_read,
  //.fb_write = fb_sys_write,
  //.fb_fillrect = sys_fillrect, /* obligatory fun (using sys_XX functions for now)*/
  //.fb_copyarea = sys_copyarea, /* obligatory fun (using sys_XX functions for now)*/
  //.fb_imageblit = sys_imageblit, /* obligatory fun (using sys_XX functions for now)*/
  .fb_fillrect = fb_fillrect,
  .fb_copyarea = fb_copyarea,
  .fb_imageblit = fb_imageblit,
  .fb_mmap = WS2812_map,
};

static int __init WS2812_init(void) {
  //NOTE THIS ALL WILL BE MOVED TO SPI DRIVER INIT FUNCTION
  struct fb_info *info;
  struct WS2812_module_fb_info *mod_info;
  int ret = -ENOMEM;

  PRINT_LOG("Module Init call with params: \n\txPanelLen = %d \n\tyPanelLen = %d\
    \n\tcolorBits = %u \n\tgOffset = %u \n\trOffset = %u \n\tbOffset = %u \n",
    xPanelLen, yPanelLen, /*colors,*/ colorBits, gOffset, rOffset, bOffset);

  if(!(info = framebuffer_alloc(sizeof(struct WS2812_module_fb_info), NULL))) {
    PRINT_ERR_FA("frame buffer allocation failed! (private data struc size %u)\n", sizeof(struct WS2812_module_fb_info));
    goto err_framebuffer_alloc;
  }
  PRINT_LOG("framebuffer alloc'ed\n", NULL);
  //set the address of module assigned private data
  mod_info = info->par;
  // ALLOCATE PIXEL BUFFOR
  unsigned pixel_buffor_len = xPanelLen*yPanelLen*(colorBits*3/8);
  mod_info->fb_virt = vzalloc(pixel_buffor_len);
  if(!mod_info->fb_virt)  {
    PRINT_ERR_FA("fb_virt alloc failed with %u\n", (unsigned)(mod_info->fb_virt));
    goto err_fb_virt_alloc;
  }
  PRINT_LOG("log1\n", NULL);

  // fill the info structure...
  info->screen_base = (char __iomem *)mod_info->fb_virt;
  info->fbops = &WS2812_fb_ops;
  info->fix = WS_fb_fix;
  info->fix.line_length = yPanelLen*(colorBits*3/8);
  info->fix.smem_len = pixel_buffor_len;
  info->fix.smem_start = virt_to_phys(mod_info->fb_virt);

  PRINT_LOG("fb_virt at %x, smem_start at: %x\n", mod_info->fb_virt, info->fix.smem_start);

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

  PRINT_LOG("log3\n", NULL);
  ret = register_framebuffer(info);
  PRINT_LOG("log4\n", NULL);
  if(ret < 0){
    PRINT_ERR_FA("Framebuffer register failed!", NULL);
    goto err_fb_register;
  }
  printk("WS2812 has succesfully initialise module\n");
  return 0;

  err_fb_register:
  framebuffer_release(info);

  err_fb_virt_alloc:
  if(mod_info->fb_virt)
    vfree(mod_info->fb_virt);

  err_framebuffer_alloc:
  printk("WS2812 module did not initialise\n");
  return ret;
}

static void __exit WS2812_uninit(void) {
  printk("Goodbye from WS2812 module\n");
}

static int WS2812_map(struct fb_info* info, struct vm_area_struct* vma) {
  unsigned long offset  = vma->vm_pgoff << PAGE_SHIFT;
  unsigned long start   = (unsigned long)info->fix.smem_start;
  if(offset >= PAGE_ALIGN((start & ~PAGE_MASK) + info->fix.smem_len)) {
    PRINT_ERR("%s", "Tried to reach too far in memory");
    return -EINVAL;
  }
  return 0;
}

static void fb_fillrect(struct fb_info* info, const struct fb_fillrect* area) {
  PRINT_LOG("fillrect Called\n", NULL);
}

static void fb_copyarea(struct fb_info* info, const struct fb_copyarea* area) {
  PRINT_LOG("copyarea Called\n", NULL);
}

static void fb_imageblit(struct fb_info* info, const struct fb_image* area) {
  //PRINT_LOG("imageblit Called\n", NULL);
}


module_init(WS2812_init);
module_exit(WS2812_uninit);
