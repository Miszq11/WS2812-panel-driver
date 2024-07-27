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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Smolinski");
MODULE_DESCRIPTION("WS2812 stip driver (non-dts version) over SPI");

#define WS2812_SPI_TRUE 0b11111000 /* check that!*/
#define WS2812_SPI_FALSE 0b11000000 /* check that as well!*/

#define WS2812_SPI_BUS_NUM 0
#define WS2812_SPI_MAX_SPEED_HZ 10000000
#define WS2812_SPI_TARGET_HZ 8000000
#define WS2812_ZERO_PAADING_SIZE 50*WS2812_SPI_TARGET_HZ/8000000+10

// MODULE PARAMETERS

unsigned x_panel_len = 8;
unsigned y_panel_len = 8;
//unsigned short colors    = 3;
unsigned short color_bits = 8;
unsigned short g_offset = 0;
unsigned short r_offset = 8;
unsigned short b_offset = 16;
bool run_continously = false;

module_param(x_panel_len, uint, 0444); // SETTING IT AS "WORLD-READABLE" for better debugging
module_param(y_panel_len, uint, 0444);
module_param(color_bits, ushort, 0444);
module_param(g_offset, ushort, 0444);
module_param(r_offset, ushort, 0444);
module_param(b_offset, ushort, 0444);

static int module_errno = 0;
struct WS2812_module_info *module_info = NULL;

static int WS2812_init(void);
static void WS2812_uninit(void);

static int WS2812_map(struct fb_info* info, struct vm_area_struct* vma);
int WS_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);

static void WS2812_convert_work_fun(struct work_struct* work_str);
int WS2812_work_init(struct WS2812_module_info* info);
int WS2812_spi_init(struct WS2812_module_info* info);
struct WS2812_module_info* frame_buffer_init(void);

void WS2812_uninit_framebuffer(struct WS2812_module_info* info);
void WS2812_uninit_work(struct WS2812_module_info* info);
void WS2812_uninit_spi(struct WS2812_module_info* info);

static void WS2712_spi_completion(void* arg);
void WS2812_spi_setup_message(struct WS2812_module_info* info);
void WS2812_spi_transfer_begin(struct WS2812_module_info* info);

static const struct fb_ops WS2812_fb_ops = {
  .owner = THIS_MODULE,
  //.fb_read = fb_sys_read,
  //.fb_write = fb_sys_write,
  //.fb_fillrect = sys_fillrect, /* obligatory fun (using sys_XX functions for now)*/
  //.fb_copyarea = sys_copyarea, /* obligatory fun (using sys_XX functions for now)*/
  .fb_fillrect = cfb_fillrect, /* obligatory fun (using sys_XX functions for now)*/
  .fb_copyarea = cfb_copyarea, /* obligatory fun (using sys_XX functions for now)*/
  .fb_imageblit = cfb_imageblit, /* obligatory fun (using sys_XX functions for now)*/
  .fb_mmap = WS2812_map,
  // FB_WAIT_FOR_VSYNC is called every draw execution
  // should we use that to update display?
  .fb_ioctl = WS_fb_ioctl,
};

struct WS2812_module_info* frame_buffer_init(void /* for now? */) {
    //out of the tree module starting?
  struct fb_info *info;
  struct WS2812_module_info *mod_info;
  int ret = -ENOMEM;
  unsigned pixel_buffor_len;

  PRINT_LOG("Module Init call with params: \n\tx_panel_len = %d \n\ty_panel_len = %d\
    \n\tcolor_bits = %u \n\tg_offset = %u \n\tr_offset = %u \n\tb_offset = %u \n",
    x_panel_len, y_panel_len, /*colors,*/ color_bits, g_offset, r_offset, b_offset);

  if(!(info = framebuffer_alloc(sizeof(struct WS2812_module_info), NULL))) {
    PRINT_ERR_FA("frame buffer allocation failed! (private data struc size %u)\n", sizeof(struct WS2812_module_info));
    goto err_framebuffer_alloc;
  }
  PRINT_LOG("framebuffer alloc'ed\n", NULL);
  //set the address of module assigned private data
  mod_info = info->par; // all module info contained in fb private data
  mod_info->info = info;

  // ALLOCATE PIXEL BUFFOR
  pixel_buffor_len = x_panel_len*y_panel_len*(color_bits*3/8);
  mod_info->fb_virt = (u8*) vmalloc(pixel_buffor_len);
  if(!mod_info->fb_virt)  {
    PRINT_ERR_FA("fb_virt alloc failed with %u\n", (unsigned)(mod_info->fb_virt));
    goto err_fb_virt_alloc;
  }
  mod_info->fb_virt_size = pixel_buffor_len;

  // fill the info structure... can be moved to another fun?
  info->screen_base = (char __iomem *)mod_info->fb_virt;
  info->fbops = &WS2812_fb_ops;
  info->fix = WS_fb_fix;
  info->fix.line_length = y_panel_len*(color_bits*3/8);
  info->fix.smem_len = pixel_buffor_len;
  info->fix.smem_start = virt_to_phys(mod_info->fb_virt);

  PRINT_LOG("fb_virt at %px, smem_start at: %px\n", mod_info->fb_virt, info->fix.smem_start);

  info->var.xres = x_panel_len;
  info->var.yres = y_panel_len;
  info->var.xres_virtual = x_panel_len;
  info->var.yres_virtual = y_panel_len;
  info->var.bits_per_pixel = color_bits*3;
  info->var.red.length = color_bits;
  info->var.red.offset = r_offset;
  info->var.green.length = color_bits;
  info->var.green.offset = g_offset;
  info->var.blue.length = color_bits;
  info->var.blue.offset = b_offset;
  info->var.transp.length = 0;
  info->var.transp.offset = 0;
  info->var.activate = FB_ACTIVATE_NOW;

  ret = register_framebuffer(info);
  if(ret < 0){
    PRINT_ERR_FA("Framebuffer register failed!", NULL);
    goto err_fb_register;
  }
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
    return -ENODEV;
  }

  //spooky scary
  memcpy(&(info->spi_device_info), &dummy_spi, sizeof(struct spi_board_info));
  info->WS2812_spi_dev = spi_new_device(info->WS2812_spi_master, &(info->spi_device_info));
  if(!info->WS2812_spi_dev) {
    PRINT_ERR_FA("Cannot create spi_device\n", NULL);
    return -ENODEV;
  }
  info->WS2812_spi_dev->bits_per_word = color_bits;

  if((ret = spi_setup(info->WS2812_spi_dev))) {
    PRINT_ERR_FA("Cannot setup spi device\n", NULL);
    spi_unregister_device(info->WS2812_spi_dev);
    module_errno = ret;
    return ret;
  }

  // some driver specific data
  info->spi_transfer_in_progress = false;
  info->spi_transfer_continous = run_continously;

  return 0;
}

int WS2812_work_init(struct WS2812_module_info* info) {
  int ret = 0;

  info->work_buffer_input = vmalloc(info->fb_virt_size);
  if(!info->work_buffer_input) {
    PRINT_ERR_FA("vmalloc (work_buffer_input) failed", NULL);
    ret = -ENOMEM;
    goto vmalloc_err;
  }

  // initialise work output buffer;
  info->spi_buffer_size = 8*info->fb_virt_size + WS2812_ZERO_PAADING_SIZE;
  if(!(info->spi_buffer = vmalloc(info->spi_buffer_size))) {
    PRINT_ERR_FA("vmalloc (spi_buffer) failed", NULL);
    ret = -ENOMEM;
    goto vmalloc_err;
  }
  memset(info->spi_buffer, 0, info->spi_buffer_size);

  // idk how to check if this fails
  INIT_WORK(&(info->WS2812_work), WS2812_convert_work_fun);

  // create workqueue...
  info->convert_workqueue = create_singlethread_workqueue(WS2812_WORKQUEUE_NAME);
  if(info->convert_workqueue == NULL) {
    PRINT_ERR_FA("Cannot create workqueue named %s\n", WS2812_WORKQUEUE_NAME);
    ret = -EINVAL;
    goto work_queue_error;
  }

  work_queue_error:
  vmalloc_err:
  module_errno = ret;
  return ret;
}

static void WS2812_convert_work_fun(struct work_struct* work_str) {
  struct WS2812_module_info *priv = container_of(work_str, struct WS2812_module_info, WS2812_work);
  int x, y, color;
  unsigned long flags;

  if(!memcpy(priv->work_buffer_input, priv->fb_virt, priv->fb_virt_size)) {
    PRINT_ERR_FA("Work failed due to memcpy failure!\n", NULL);
    return;
  }

  // DEBUG PRINT (REMOVE later)
  //local_irq_save(flags);
  //for(y = 0; y < priv->info->var.yres; y++) {
  //  for(x = 0; x < priv->info->var.xres; x++) {
  //    printk(KERN_CONT "(%u, %u, %u) ",
  //        priv->work_buffer_input[3*(x + y*priv->info->var.xres)],
  //        priv->work_buffer_input[3*(x + y*priv->info->var.xres) + 1],
  //        priv->work_buffer_input[3*(x + y*priv->info->var.xres) + 2]);
  //  }
  //  printk("");
  //}
  //local_irq_restore(flags);

  for(y = 0; y < priv->info->var.yres; y++) {
    for(x = 0; x < priv->info->var.xres; x++) {
      // I had overwhelming desire to do something stupid
      // ...
      // here I go :)
      #define WS_SPI_IDX(x, y, color) 24*(x + y*priv->info->var.xres) + 8*color
      #define WS_WORK_IDX(x, y, color) 3*(x + y*priv->info->var.xres) + color

      #define WS_CHEAT(x, y, color, bit) priv->spi_buffer[ WS_SPI_IDX(x, y, color) + bit] = \
        (priv->work_buffer_input[WS_WORK_IDX(x, y, color)] & BIT(7-bit))? WS2812_SPI_TRUE : WS2812_SPI_FALSE

      for(color = 0; color <3 /*hearth for you <3*/; color++) {
        WS_CHEAT(x, y, color, 0);
        WS_CHEAT(x, y, color, 1);
        WS_CHEAT(x, y, color, 2);
        WS_CHEAT(x, y, color, 3);
        WS_CHEAT(x, y, color, 4);
        WS_CHEAT(x, y, color, 5);
        WS_CHEAT(x, y, color, 6);
        WS_CHEAT(x, y, color, 7);
      }
    }
  }
  // DEBUG PRINT (REMOVE later)
  // local_irq_save(flags);
  // for(y=0; y<3; y++){
  //   for(x=0; x < 8; x++) {
  //     printk(KERN_CONT "%u", (priv->spi_buffer[8*y+ x] == WS2812_SPI_TRUE)?1:0);
  //   }
  //   printk(KERN_CONT " ");
  // }
  // local_irq_restore(flags);
  // printk("");

  WS2812_spi_transfer_begin(priv);

  if(run_continously)
    queue_work(priv->convert_workqueue, &priv->WS2812_work);
}

void WS2812_spi_setup_message(struct WS2812_module_info* info) {
  spi_message_init(&(info->WS2812_message));
  info->WS2812_message.complete = WS2712_spi_completion;
  info->WS2812_message.context = info;

  info->WS2812_xfer.tx_buf = info->spi_buffer;
  info->WS2812_xfer.rx_buf = NULL;
  info->WS2812_xfer.speed_hz = 8000000;
  info->WS2812_xfer.bits_per_word = info->WS2812_spi_dev->bits_per_word;
  info->WS2812_xfer.len = info->spi_buffer_size;

  spi_message_add_tail(&info->WS2812_xfer, &info->WS2812_message);
}

void WS2812_spi_transfer_begin(struct WS2812_module_info* info) {
  //build message
  if(info->spi_transfer_in_progress) {
    PRINT_LOG("previous message still in progress! ABORTING\n", NULL);
    return;
  }

  info->spi_transfer_in_progress = true;
  if(spi_async(info->WS2812_spi_dev, &info->WS2812_message)) {
    PRINT_ERR_FA("could not send spi message msg_ptr: 0x%px", &info->WS2812_message);
    info->spi_transfer_in_progress = false;
  }
}

static void WS2712_spi_completion(void* arg) {
  struct WS2812_module_info *info = arg; // idk mby might be needed
  PRINT_GOOD("SPI Transfer completed and was %s with status %d (xfered: %dBytes/ %d All Bytes)!\n",
      (info->WS2812_message.status)?"\033[1;31mUNSUCCESFULL:\033[0m":"\033[1;32mSUCCESFULL:\033[0m",
      info->WS2812_message.status, info->WS2812_message.frame_length, info->WS2812_message.actual_length);
  info->spi_transfer_in_progress = false;
}

static int __init WS2812_init(void) {
  module_info = frame_buffer_init();
  if(!module_info)
    return module_errno;

  if(WS2812_work_init(module_info)) {
    goto framebuffer_initialized;
  }

  if(WS2812_spi_init(module_info)) {
    goto work_initialized;
  }

  WS2812_spi_setup_message(module_info);
  PRINT_LOG("WS2812 has succesfully initialise module\n", NULL);
  return 0;

  work_initialized:
    WS2812_uninit_work(module_info);
  framebuffer_initialized:
    WS2812_uninit_framebuffer(module_info);
  return module_errno;
}

void WS2812_uninit_framebuffer(struct WS2812_module_info* info) {
  unregister_framebuffer(module_info->info);
  if(module_info->fb_virt)
    vfree(module_info->fb_virt);
  if(module_info->work_buffer_input)
    vfree(module_info->work_buffer_input);
  if(module_info->spi_buffer)
    vfree(module_info->spi_buffer);
  framebuffer_release(module_info->info);
}

void WS2812_uninit_work(struct WS2812_module_info* info) {
  flush_workqueue(module_info->convert_workqueue);
  destroy_workqueue(module_info->convert_workqueue);
}

void WS2812_uninit_spi(struct WS2812_module_info* info) {
  if(info->WS2812_spi_dev)
    spi_unregister_device(info->WS2812_spi_dev);
}

static void __exit WS2812_uninit(void) {
  if(!module_info) {
    PRINT_LOG("module_info is NULL. Module didn't initialise correctly?\n", NULL);
    return;
  }
  WS2812_uninit_spi(module_info);
  WS2812_uninit_work(module_info);
  WS2812_uninit_framebuffer(module_info);

  printk("Goodbye from WS2812 module\n");
}

static int WS2812_map(struct fb_info* info, struct vm_area_struct* vma) {
  //TODO: test any vma->vm_ops for detecting any R/W operation?
  //This is gonna be fun
  struct WS2812_module_info* priv = info->par;
  size_t page_count = priv->fb_virt_size/PAGE_SIZE + 1, iterator=0, offset=0;
  struct page** pages = kmalloc(page_count*(sizeof(struct page*)), GFP_KERNEL);
  int ret = 0;

  for(offset=0; offset < priv->fb_virt_size; offset += PAGE_SIZE) {
    pages[iterator++] = vmalloc_to_page(priv->fb_virt + offset);
  }

  PRINT_LOG("Mapping %d pages (fbvirt: 0x%px, page: 0x%px)\n", page_count, priv->fb_virt, pages[0]);
  vma->vm_pgoff = virt_to_phys(priv->fb_virt) >> PAGE_SHIFT;
  ret = vm_map_pages_zero(vma, pages, page_count);
  if(ret) {
    PRINT_ERR_FA("vm_map_pages_zero ret=%d, page_count=%u\n", ret, page_count);
  }

  kfree(pages);
  return ret;
}

int WS_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg) {
  size_t x=0, y=0;
  unsigned long flags;
  struct WS2812_module_info *priv = info->par;

  switch (cmd) {
    case FBIO_WAITFORVSYNC:
      printk_once("[INFO_WS2812] Vsync unsupported!\n");
      break;
    case WS_IO_DUMMY:
      local_irq_save(flags);
      for(y = 0; y < info->var.yres; y++) {
        for(x = 0; x < info->var.xres; x++) {
          printk(KERN_CONT "(%u, %u, %u) ",
              priv->fb_virt[3*(x + y*info->var.xres)],
              priv->fb_virt[3*(x + y*info->var.xres) + 1],
              priv->fb_virt[3*(x + y*info->var.xres) + 2]);
        }
        printk("");
      }
      local_irq_restore(flags);
      break;
    case WS_IO_PROCESS_AND_SEND:
      queue_work(priv->convert_workqueue, &(priv->WS2812_work));
      break;
    default:
      PRINT_LOG("Unknown IOCTL? cmd: 0x%px arg: 0x%px\n", cmd, arg);
      break;
  }
  return 0;
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
