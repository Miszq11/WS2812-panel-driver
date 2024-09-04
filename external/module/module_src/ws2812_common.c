#include "asm-generic/errno-base.h"
#include "linux/export.h"
#include "linux/init.h"
#include "linux/module.h"
#include "linux/vmalloc.h"
#include "ws2812_common.h"
#include "debug_ctrl.h"

#include "linux/fb.h"  // IWYU pragma: keep

static void WS2812_convert_work_fun(struct work_struct* work_str);
static void WS2712_spi_completion(void* arg);

const uint8_t ws_pixel_gamma_correction[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06,
	0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09,
	0x09, 0x09, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C,
	0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F, 0x10, 0x10, 0x11,
	0x11, 0x12, 0x12, 0x13, 0x13, 0x14, 0x14, 0x15, 0x15, 0x16,
	0x16, 0x17, 0x18, 0x18, 0x19, 0x19, 0x1A, 0x1B, 0x1B, 0x1C,
	0x1D, 0x1D, 0x1E, 0x1F, 0x1F, 0x20, 0x21, 0x22, 0x22, 0x23,
	0x24, 0x25, 0x26, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2A, 0x2B,
	0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
	0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4B,
	0x4C, 0x4D, 0x4E, 0x50, 0x51, 0x52, 0x54, 0x55, 0x56, 0x58,
	0x59, 0x5A, 0x5C, 0x5D, 0x5E, 0x60, 0x61, 0x63, 0x64, 0x66,
	0x67, 0x69, 0x6A, 0x6C, 0x6D, 0x6F, 0x70, 0x72, 0x73, 0x75,
	0x77, 0x78, 0x7A, 0x7C, 0x7D, 0x7F, 0x81, 0x82, 0x84, 0x86,
	0x88, 0x89, 0x8B, 0x8D, 0x8F, 0x91, 0x92, 0x94, 0x96, 0x98,
	0x9A, 0x9C, 0x9E, 0xA0, 0xA2, 0xA4, 0xA6, 0xA8, 0xAA, 0xAC,
	0xAE, 0xB0, 0xB2, 0xB4, 0xB6, 0xB8, 0xBA, 0xBC, 0xBF, 0xC1,
	0xC3, 0xC5, 0xC7, 0xCA, 0xCC, 0xCE, 0xD1, 0xD3, 0xD5, 0xD7,
	0xDA, 0xDC, 0xDF, 0xE1, 0xE3, 0xE6, 0xE8, 0xEB, 0xED, 0xF0,
	0xF2, 0xF5, 0xF7, 0xFA, 0xFC, 0xFF,
};

/**
 * @brief frame buffer initialization function.
 *
 * @param mod_info module info struct
 * @return struct WS2812_module_info* Not NULL Addr on success
 *
 *  This function provides:
 */

struct WS2812_module_info* frame_buffer_init(struct WS2812_module_info* mod_info, struct fb_init_values* fb_init) {
  //out of the tree module starting?
  struct fb_info *info;
  //struct WS2812_module_info *mod_info;
  int ret = -ENOMEM;
  unsigned pixel_buffor_len, real_buffor_len;

  PRINT_LOG("Module Init call with params: \
    \n\tx_panel_len = %d\n\ty_panel_len = %d\
    \n\tx_virtual_length = %d\ty_virtual_length = %d \
    \n\tx_pan_step = %d\ty_pan_step = %d\n\tcolor_bits = %u\
    \n\tg_offset = %u \n\tr_offset = %u \n\tb_offset = %u \n",
    fb_init->x_panel_length, fb_init->y_panel_length, /*colors,*/
    fb_init->x_virtual_length, fb_init->y_virtual_length,
    fb_init->x_pan_step, fb_init->y_pan_step,
    fb_init->color_bits, fb_init->green_offset,
    fb_init->red_offset, fb_init->blue_offset);

  if(fb_init->x_panel_length > fb_init->x_virtual_length ||
      fb_init->y_panel_length > fb_init->y_virtual_length) {
    PRINT_ERR_FA("virtual dimensions should be equal of bigger than panel dimensions");
    module_errno = -EINVAL;
    return NULL;
  }

  /// - framebuffer alocation
  if(!(info = framebuffer_alloc(sizeof(struct WS2812_module_info*), NULL))) {
    PRINT_ERR_FA("frame buffer allocation failed! (private data struc size %u)\n", sizeof(struct WS2812_module_info));
    goto err_framebuffer_alloc;
  }

  PRINT_LOG("framebuffer alloc'ed\n");
  /// - module internal structure address linking
  mod_info->info = info;
  info->par = mod_info;

  /// - allocation of framebuffer pixel bufor
  pixel_buffor_len = fb_init->x_virtual_length*fb_init->y_virtual_length*(fb_init->color_bits);
  pixel_buffor_len = pixel_buffor_len*3/8 + ((pixel_buffor_len%8) ? 1 : 0);

  mod_info->fb_virt = (u8*) vmalloc(pixel_buffor_len); // TODO: fix for panel shifting!
  if(!mod_info->fb_virt)  {
    ret = -ENOMEM;
    PRINT_ERR_FA("fb_virt alloc failed with %u\n", (unsigned)(mod_info->fb_virt));
    goto err_fb_virt_alloc;
  }
  real_buffor_len = fb_init->x_panel_length*fb_init->y_panel_length*(fb_init->color_bits);
  real_buffor_len = real_buffor_len*3/8 + ((real_buffor_len%8) ? 1 : 0);

  mod_info->fb_virt_size = pixel_buffor_len;
  mod_info->fb_real_size = real_buffor_len;

  /// - filling fb_info structure
  info->screen_base = (char __iomem *)mod_info->fb_virt;
  info->fbops = fb_init->prep_fb_ops;
  info->fix = WS_fb_fix;
  info->fix.line_length = fb_init->x_panel_length*(fb_init->color_bits*3/8);
  info->fix.smem_len = pixel_buffor_len;
  info->fix.smem_start = virt_to_phys(mod_info->fb_virt);

  PRINT_LOG("fix.line_length = %d, pixel_buf_len = %d, panel_buff_len = %d\n",
      info->fix.line_length, pixel_buffor_len, real_buffor_len);
  PRINT_LOG("fb_virt at %px, smem_start at: %px\n", mod_info->fb_virt, (void *)(info->fix.smem_start));

  info->var.xres = fb_init->x_panel_length;
  info->var.yres = fb_init->y_panel_length;
  info->var.xres_virtual = fb_init->x_virtual_length;
  info->var.yres_virtual = fb_init->y_virtual_length;
  info->fix.xpanstep = fb_init->x_pan_step;
  info->fix.ypanstep = fb_init->y_pan_step;
  // ywrapping is not supported i suppose?
  info->var.vmode &= ~FB_VMODE_YWRAP;
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

  /// - registration of framebuffer
  ret = register_framebuffer(info);
  if(ret < 0){
    PRINT_ERR_FA("Framebuffer register failed!");
    goto err_fb_register;
  }
  return mod_info;

  err_fb_register:
  framebuffer_release(info);

  err_fb_virt_alloc:
  if(mod_info->fb_virt)
    vfree(mod_info->fb_virt);

  err_framebuffer_alloc:
  PRINT_ERR_FA("WS2812 module did not initialise\n");
  module_errno = ret;
  return NULL;
}
EXPORT_SYMBOL_GPL(frame_buffer_init);

/**
 * @brief Function initializing all neccesary facilities for workqueue to work.
 *
 * @param info Module info struct
 * @return int Returns 0 on success
 *
 * This function provides:
 */
int WS2812_work_init(struct WS2812_module_info* info) {
  int ret = 0;
  /// - Allocation of work_buffer for copying pixel data
  info->work_buffer_input = vmalloc(info->fb_real_size);

  if(!info->work_buffer_input) {
    PRINT_ERR_FA("vmalloc (work_buffer_input) failed");
    ret = -ENOMEM;
    goto vmalloc_err;
  }

  /// - Allocation of work output buffer
  info->spi_buffer_size = (8*info->fb_real_size + WS2812_ZERO_PAADING_SIZE)*sizeof(WS2812_SPI_BUFF_TYPE);
  if(!(info->spi_buffer = vmalloc(info->spi_buffer_size))) {
    PRINT_ERR_FA("vmalloc (spi_buffer) failed");
    ret = -ENOMEM;
    goto vmalloc_err;
  }
  memset(info->spi_buffer, 0, info->spi_buffer_size);

  // idk how to check if this fails
  INIT_WORK(&(info->WS2812_work), WS2812_convert_work_fun);

  /// - creating workqueue
  info->convert_workqueue = create_singlethread_workqueue(WS2812_WORKQUEUE_NAME);
  if(info->convert_workqueue == NULL) {
    PRINT_ERR_FA("Cannot create workqueue named %s\n", WS2812_WORKQUEUE_NAME);
    ret = -EINVAL;
    goto work_queue_error;
  }

  return 0;

work_queue_error:
  vfree(info->work_buffer_input);
vmalloc_err:
  return (module_errno = ret);
}
EXPORT_SYMBOL_GPL(WS2812_work_init);

/**
 * @brief Process map request on owned framebuffer.
 *        Function maps internal pixel bufor into user-space.
 *
 * @param info Frame buffer info structure
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

/**
 * @brief Work function that will:
 *    - copy pixel bufor to work buffor to prevent data races,
 *    - convert every pixel to corresponding codes for spi transfer,
 *    - start spi transfer.
 *
 *
 * \see void WS2812_spi_transfer_begin(struct WS2812_module_info* info) - function starting spi transfer
 *
 * @param  work_str Structure passed on queued work.
 *      Struct WS2812_module_info address may be extracted from that
 */

static void WS2812_convert_work_fun(struct work_struct* work_str) {
  struct WS2812_module_info *priv = container_of(work_str, struct WS2812_module_info, WS2812_work);
  size_t x, y, color;
  size_t x_offset = 0, y_offset = 0;

  if(priv->spi_transfer_in_progress) {
    PRINT_LOG("previous message still in progress! ABORTING\n");
    return;
  }

  #ifdef NO_DEVICE_TESTING
  unsigned long flags;
  #endif

  lock_fb_info(priv->info);
    x_offset = priv->info->var.xoffset;
    y_offset = priv->info->var.yoffset;
  unlock_fb_info(priv->info);

  for(y = 0; y < priv->info->var.yres; y++) {
    memcpy(
        (priv->work_buffer_input + y*3*(priv->info->var.xres)),
        (priv->fb_virt + 3*(x_offset + (y + y_offset)*(priv->info->var.xres_virtual))),
        priv->info->fix.line_length);
  }

  //if(!memcpy(priv->work_buffer_input, priv->fb_virt, priv->fb_virt_size)) {
  //  PRINT_ERR_FA("Work failed due to memcpy failure!\n");
  //  return;
  //}

  // DEBUG PRINT (REMOVE later)
  #ifdef NO_DEVICE_TESTING
  local_irq_save(flags);
  printk("x_offset = %d, y_offset = %d\n", x_offset, y_offset);
  printk("Frame:\n");
  for(y = 0; y < priv->info->var.yres; y++) {
    for(x = 0; x < priv->info->var.xres; x++) {
      printk(KERN_CONT "(%u, %u, %u) ",
          priv->work_buffer_input[3*(x + y*priv->info->var.xres) + 0],
          priv->work_buffer_input[3*(x + y*priv->info->var.xres) + 1],
          priv->work_buffer_input[3*(x + y*priv->info->var.xres) + 2]);
    }
    printk("");
  }
  local_irq_restore(flags);
  #endif

  for(y = 0; y < priv->info->var.yres; y++) {
    for(x = 0; x < priv->info->var.xres; x++) {
      // I had overwhelming desire to do something stupid
      // ...
      // here I go :)
      #define WS_SPI_IDX(x, y, color) \
          24*(x + y*priv->info->var.xres) + 8*color + WS2812_ZERO_PAADING_SIZE
      #define WS_WORK_IDX(x, y, color) \
          3*(x + y*priv->info->var.xres) + color

      #define WS_CONVERT_PIXEL(x, y, color, bit) \
          priv->spi_buffer[ WS_SPI_IDX(x, y, color) + bit] = \
          ((ws_pixel_gamma_correction[priv->work_buffer_input[WS_WORK_IDX(x, y, color)]] & BIT(7-bit))? WS2812_SPI_TRUE : WS2812_SPI_FALSE)

      for(color = 0; color <3 /*<3*/; color++) {
        WS_CONVERT_PIXEL(x, y, color, 0);
        WS_CONVERT_PIXEL(x, y, color, 1);
        WS_CONVERT_PIXEL(x, y, color, 2);
        WS_CONVERT_PIXEL(x, y, color, 3);
        WS_CONVERT_PIXEL(x, y, color, 4);
        WS_CONVERT_PIXEL(x, y, color, 5);
        WS_CONVERT_PIXEL(x, y, color, 6);
        WS_CONVERT_PIXEL(x, y, color, 7);
      }
    }
  }

  #ifndef NO_DEVICE_TESTING
    WS2812_spi_transfer_begin(priv);
  #endif

  // This will deffinitelly fail... change that to delayed work
  if(run_continously && !priv->spi_transfer_in_progress)
    queue_work(priv->convert_workqueue, &priv->WS2812_work);
}

/**
 * @brief IOCTL code handler.
 *
 * \see WS_IO_DUMMY - test command
 * \see WS_IO_PROCESS_AND_SEND - flush to panel command
 *
 * @param info Frame buffer info structure
 * @param cmd IOCTL Command
 * @param arg IOCTL Argument value
 * @return status of IOCTL completion
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
      PRINT_LOG("Unknown IOCTL? cmd: 0x%px arg: 0x%px\n", (void *)cmd, (void *)arg);
      break;
  }
  return 0;
}
EXPORT_SYMBOL_GPL(WS_fb_ioctl);

/**
 * @brief Starts the asynchronous SPI message transfer, prints out errors if they occur.
 *        Before starting new transfer it checks if the old one has finished.
 *
 * @param info calling module data struct
 */

void WS2812_spi_transfer_begin(struct WS2812_module_info* info) {
  if(info->spi_transfer_in_progress) {
    PRINT_LOG("previous message still in progress! ABORTING\n");
    return;
  }

  info->spi_transfer_in_progress = true;
  if(spi_async(info->WS2812_spi_dev, &info->WS2812_message)) {
    PRINT_ERR_FA("could not send spi message msg_ptr: 0x%px", &info->WS2812_message);
    info->spi_transfer_in_progress = false;
  }
}

/**
 * @brief Framebuffer unitialization function.
 *        Releases pixel memory, and framebuffer
 *        structures.
 *
 * @param info calling module data struct
 */

void WS2812_uninit_framebuffer(struct WS2812_module_info* info) {
  unregister_framebuffer(info->info);
  if(info->fb_virt)
    vfree(info->fb_virt);
  framebuffer_release(info->info);

}
EXPORT_SYMBOL_GPL(WS2812_uninit_framebuffer);

/**
 * @brief Work and workqueue uninitialisation function.
 *        Frees work buffer.
 * @param info calling module data struct
 */

void WS2812_uninit_work(struct WS2812_module_info* info) {
  flush_workqueue(info->convert_workqueue);
  if(info->work_buffer_input)
    vfree(info->work_buffer_input);
  destroy_workqueue(info->convert_workqueue);
}
EXPORT_SYMBOL_GPL(WS2812_uninit_work);

/**
 * @brief Spi handle deinitialisation function.
 *        Frees spi_buffer.
 * @param info calling module data struct
 */

void WS2812_uninit_spi(struct WS2812_module_info* info) {
  if(info->WS2812_spi_dev)
    spi_unregister_device(info->WS2812_spi_dev);
  if(info->spi_buffer)
    vfree(info->spi_buffer);
}
EXPORT_SYMBOL_GPL(WS2812_uninit_spi);

/**
 * @brief Initializes SPI message and sets all it fields.
 *        Xfer structure is also initialized here to
 *        match required info.
 *
 *        SPI message is used as a main method to transfer
 *        data contained in associated fb buffer
 *
 *        \see int WS2812_work_init(struct WS2812_module_info* info) for Buffer initialization
 *        \see int WS2812_spi_init(struct WS2812_module_info* info) for SPI setup
 *
 *
 * @param info calling module data struct
 */

void WS2812_spi_setup_message(struct WS2812_module_info* info) {
  spi_message_init(&(info->WS2812_message));
  info->WS2812_message.complete = WS2712_spi_completion;
  info->WS2812_message.context = info;
  info->WS2812_xfer.tx_buf = info->spi_buffer;
  info->WS2812_xfer.rx_buf = NULL;
  info->WS2812_xfer.speed_hz = WS2812_SPI_TARGET_HZ;
  #ifndef NO_DEVICE_TESTING
  info->WS2812_xfer.bits_per_word = info->WS2812_spi_dev->bits_per_word;
  #endif
  //info->WS2812_xfer.bits_per_word = BITS_PER_WORD;
  info->WS2812_xfer.len = info->spi_buffer_size;

  spi_message_add_tail(&info->WS2812_xfer, &info->WS2812_message);
}
EXPORT_SYMBOL_GPL(WS2812_spi_setup_message);

/**
 * @brief fbops pan display function.
 *
 *
 * @param arg Address of WS2812_module_info structure (calling module data struct).
 */


int WS2812_fb_pan_display(struct fb_var_screeninfo* var, struct fb_info* info) {
  // ypanstep, xpanstep should be the size of y/x_virt_size?
  //
  // IOCTL will fail if the pan will force the display to
  // go over the buffer
  //
  // else the wrapping should be done manually with
  // user declared IOCTL
  //

  PRINT_GOOD("Display panned\n");
  PRINT_GOOD("xoffset = %u, yoffset = %u\n", var->xoffset, var->yoffset);
  // dont have to do anything, internal fb_pan will do most checks for us
  return 0;
}
EXPORT_SYMBOL_GPL(WS2812_fb_pan_display);

/**
 * @brief Callback on SPI transfer completion.
 *        It resets internal structure flag (spi_transfer_in_progress)
 *        indicating that new transfer may begin.
 *        \see struct WS2812_module_info
 *
 * @param arg Address of WS2812_module_info structure (calling module data struct).
 */

static void WS2712_spi_completion(void* arg) {
  struct WS2812_module_info *info = arg; // idk mby might be needed
  PRINT_GOOD("SPI Transfer completed and was %s with status %d (xfered: %dBytes/ %d All Bytes)!\n",
      (info->WS2812_message.status)?"\033[1;31mUNSUCCESFULL:\033[0m":"\033[1;32mSUCCESFULL:\033[0m",
      info->WS2812_message.status, info->WS2812_message.frame_length, info->WS2812_message.actual_length);
  info->spi_transfer_in_progress = false;
}

/// @private
static int __init common_init_info(void) {
  printk("Common code carrier module init\n");
  return 0;
}

module_init(common_init_info)

MODULE_DESCRIPTION("WS2812 stip driver common code");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Smolinski");