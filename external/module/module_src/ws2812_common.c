
#include "linux/export.h"
#include "linux/init.h"
#include "linux/module.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#pragma GCC diagnostic ignored "-Wformat"
#include "ws2812_common.h"
#include "debug_ctrl.h"
//#pragma GCC diagnostic pop

#include "linux/fb.h"  // IWYU pragma: keep

static void WS2812_convert_work_fun(struct work_struct* work_str);

/**
 * @brief frame buffer initialization function, returns NULL on success
 *
 * @param mod_info module info struct
 * @return struct WS2812_module_info* returns NULL on success
 */

struct WS2812_module_info* frame_buffer_init(struct WS2812_module_info* mod_info, struct fb_init_values* fb_init) {
  //out of the tree module starting?
  struct fb_info *info;
  //struct WS2812_module_info *mod_info;
  int ret = -ENOMEM;
  unsigned pixel_buffor_len;

  PRINT_LOG("Module Init call with params: \n\tx_panel_len = %d \n\ty_panel_len = %d\
    \n\tcolor_bits = %u \n\tg_offset = %u \n\tr_offset = %u \n\tb_offset = %u \n",
    fb_init->x_panel_length, fb_init->y_panel_length, /*colors,*/
    fb_init->color_bits, fb_init->green_offset,
    fb_init->red_offset, fb_init->blue_offset);

  if(!(info = framebuffer_alloc(sizeof(struct WS2812_module_info*), NULL))) {
    PRINT_ERR_FA("frame buffer allocation failed! (private data struc size %u)\n", sizeof(struct WS2812_module_info));
    goto err_framebuffer_alloc;
  }

  PRINT_LOG("framebuffer alloc'ed\n", NULL);
  //set the address of module assigned private data
  mod_info->info = info;
  info->par = mod_info;

  // ALLOCATE PIXEL BUFFOR
  pixel_buffor_len = fb_init->x_panel_length*fb_init->y_panel_length*(fb_init->color_bits*3/8);
  mod_info->fb_virt = (u8*) vmalloc(pixel_buffor_len); // TODO: fix for panel shifting!
  if(!mod_info->fb_virt)  {
    PRINT_ERR_FA("fb_virt alloc failed with %u\n", (unsigned)(mod_info->fb_virt));
    goto err_fb_virt_alloc;
  }
  mod_info->fb_virt_size = pixel_buffor_len;

  // fill the info structure... can be moved to another fun?
  info->screen_base = (char __iomem *)mod_info->fb_virt;
  info->fbops = fb_init->prep_fb_ops;
  info->fix = WS_fb_fix;
  info->fix.line_length = fb_init->y_panel_length*(fb_init->color_bits*3/8);
  info->fix.smem_len = pixel_buffor_len;
  info->fix.smem_start = virt_to_phys(mod_info->fb_virt);

  PRINT_LOG("fb_virt at %px, smem_start at: %px\n", mod_info->fb_virt, info->fix.smem_start);

  info->var.xres = fb_init->x_panel_length;
  info->var.yres = fb_init->y_panel_length;
  info->var.xres_virtual = fb_init->x_panel_length; // TODO: fix for panel shifting!
  info->var.yres_virtual = fb_init->y_panel_length; // TODO: fix for panel shifting!
  info->var.bits_per_pixel = fb_init->color_bits*3;

  info->var.red.length = fb_init->color_bits;
  info->var.red.offset = fb_init->red_offset;

  info->var.green.length = fb_init->color_bits;
  info->var.green.offset = fb_init->green_offset;

  info->var.blue.length = fb_init->color_bits;
  info->var.blue.offset = fb_init->blue_offset;

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
EXPORT_SYMBOL_GPL(frame_buffer_init);

/**
 * @brief Function for allocating memory for the SPI buffer
 *
 * @param info Module info struct
 * @return int Returns 0 on success
 */
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
  return (module_errno = ret);
}
EXPORT_SYMBOL_GPL(WS2812_work_init);

/**
 * @brief Allocates memory map
 *
 * @param info Frame buffer informations
 * @param vma Memory map informations
 * @return int Returns 0 on success
 */

int WS2812_map(struct fb_info* info, struct vm_area_struct* vma) {
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
EXPORT_SYMBOL_GPL(WS2812_map);

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
      #define WS_SPI_IDX(x, y, color) 24*(x + y*priv->info->var.xres) + 8*color + WS2812_ZERO_PAADING_SIZE
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

/**
 * @brief Prints out information about current pixel values
 *
 * @param info Frame buffer informations
 * @param cmd Command
 * @param arg Argument
 * @return int
 */

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
EXPORT_SYMBOL_GPL(WS_fb_ioctl);

/**
 * @brief Starts the SPI message transfer, prints out errors if they occur
 *
 * @param info Module info struct
 */

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

/**
 * @brief Framebuffer unitialization function
 *
 * @param info Module info struct
 */

void WS2812_uninit_framebuffer(struct WS2812_module_info* info) {
  unregister_framebuffer(info->info);
  if(info->fb_virt)
    vfree(info->fb_virt);
  if(info->work_buffer_input)
    vfree(info->work_buffer_input);
  if(info->spi_buffer)
    vfree(info->spi_buffer);
  framebuffer_release(info->info);

}
EXPORT_SYMBOL_GPL(WS2812_uninit_framebuffer);

/**
 * @brief Removes work queue from memory
 *
 * @param info
 */

void WS2812_uninit_work(struct WS2812_module_info* info) {
  flush_workqueue(info->convert_workqueue);
  destroy_workqueue(info->convert_workqueue);
}
EXPORT_SYMBOL_GPL(WS2812_uninit_work);

void WS2812_uninit_spi(struct WS2812_module_info* info) {
  if(info->WS2812_spi_dev)
    spi_unregister_device(info->WS2812_spi_dev);
}
EXPORT_SYMBOL_GPL(WS2812_uninit_spi);

static int __init common_init_info(void) {
  printk("Common code carrier module init\n");
  return 0;
}

module_init(common_init_info)
MODULE_LICENSE("GPL");