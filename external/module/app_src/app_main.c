#include <linux/fb.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <WS2812_panel.h>

//#define WS_IO_DUMMY _IO('x', 1)
//#define WS_IO_PROCESS_AND_SEND _IO('x', 2)

#include "fbgraphics.h"
#include "fbg_fbdev.h" // insert any backends from ../custom_backend/backend_name folder
#include "linux/fb.h"
#include "pixel_art.h"

int keep_running = 1;

void int_handler(int dummy) {
  keep_running = 0;
}

void fb_fbgdraw_fix(struct _fbg *fbg) {
    struct _fbg_fbdev_context *fbdev_context = fbg->user_context;
    for(size_t y = 0; y < fbg->height; y++)
      memcpy(
        (fbdev_context->buffer + y*fbg->components*fbdev_context->vinfo.xres_virtual),
        (fbg->disp_buffer + y*fbg->components*fbg->width),
        fbg->line_length);
}

/**
 *
 * @brief Function for printing buffered values
 *
 * @param[in] fbg frame buffer structure, defined in external library
 */

void printBuff(struct _fbg *fbg) {
  for(int y = 0; y < fbg->height; y++) {
    for(int x = 0; x < fbg->width; x++) {
      printf("(%u, %u, %u) ",
      fbg->disp_buffer[3*(x + y*fbg->height)],
      fbg->disp_buffer[3*(x + y*fbg->height)+1],
      fbg->disp_buffer[3*(x + y*fbg->height)+2]);
    }
    printf("\n");
  }
}

/**
 * @brief Function prints out app manual
 *
 */

void printHelp() {
  printf("Missing program argument! \
  \n\tThis app should be called with path to framebuffer\
  \n\tallocated by ws2812_mod\
  \n\t\
  \n\tRun module with: modprobe ws2812_mod\
  \n\tRun app (example): WS2812_app /dev/fb1\
  \n\tlist all framebuffers with: ls /dev/fb*\n");
}

/**
 * @brief Displays simple animation. In this version the animation is a pixel moving through the LED panel.
 * The r, g and b parameters are inverted to GRB
 *
 * @param fbg frame buffer structure, defined in external library
 * @param fbdev_context fbdev wrapper data structure
 * @param r green colour value for RGB space
 * @param g red colour value for RGB space
 * @param b blue colour value for RGB space
 */

void animate_pixel(struct _fbg *fbg, struct _fbg_fbdev_context *fbdev_context, unsigned char r, unsigned char g, unsigned char b) {
  unsigned long dummy;

  for(int y = 0; y< fbg->height; y++)
    for(int x = 0; x < fbg->width; x++) {
      fbg_draw(fbg);
      ioctl(fbdev_context->fd, WS_IO_PROCESS_AND_SEND, &dummy);

      fbg_clear(fbg, 0);
      fbg_pixel(fbg, x, y, r, g,b);
      fbg_flip(fbg);
      usleep(500000);
  }
}

int print_image_and_pan_display(struct _fbg *fbg, struct _fbg_fbdev_context *fbdev_context, uint32_t useconds) {
  unsigned long dummy = 0;
  #define PIX_COL(x,y,color) pixel_buffor[3*(y + x*pixel_buffor_y) + color]

  fbg_clear(fbg, 0);

  // we have to draw directly, because the fbg lib is operating only on visible
  // part of the display...
  memcpy(fbdev_context->buffer, pixel_buffor, pixel_buffor_len);

  //printBuff(fbg);
  ioctl(fbdev_context->fd, WS_IO_PROCESS_AND_SEND, dummy);
  fprintf(stdout, "performing display pan\n");

  const size_t max_x = fbdev_context->vinfo.xres_virtual - fbdev_context->vinfo.xres;

  struct fb_var_screeninfo var_pan_test;
  if(ioctl(fbdev_context->fd, FBIOGET_VSCREENINFO, &var_pan_test)) {
    fprintf(stdout, "couldn't get vscreeninfo! \n");
  }

  for(int xoffset = 1; xoffset <= (max_x + 1); xoffset++) {
    var_pan_test.vmode |= FB_VMODE_CONUPDATE;
    var_pan_test.xoffset = xoffset;
    if(ioctl(fbdev_context->fd, FBIOPAN_DISPLAY, &var_pan_test)) {
      if(xoffset > max_x) {
        fprintf(stdout,
            "xoffset (%d) grater than max_x (%d) [thats good it failed]\n",
            xoffset, max_x);
      } else {
        fprintf(stderr, "ioctl pan display failed!\n");
        return -1;
      }
    }
    ioctl(fbdev_context->fd, WS_IO_PROCESS_AND_SEND, dummy);
    usleep(useconds);
  }

  var_pan_test.vmode &= ~FB_VMODE_YWRAP;
  var_pan_test.xoffset = 0;
  if(ioctl(fbdev_context->fd, FBIOPAN_DISPLAY, &var_pan_test)) {
    fprintf(stderr, "Couldnt bring back 0 offset!\n");
    return -1;
  }
  return 0;
}

/**
 * @brief The main function. Tests the application and displays set of colours and animation on WS2812 LED panel.
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return int
 */

int main(int argc, char* argv[]) {
  if(argc < 2) {
    printHelp();
    return -1;
  }

  signal(SIGINT, int_handler);

  // open "/dev/fb0" by default, use fbg_fbdevSetup("/dev/fb1", 0) if you
  // want to use another framebuffer

  // note : fbg_fbdevInit is the linux framebuffer backend, you can use a
  // different backend easily by including the proper header and compiling
  // with the appropriate backend file found in ../custom_backend/backend_name
  struct _fbg *fbg = fbg_fbdevSetup(argv[1], 0);
  if (fbg == NULL) {
    fprintf(stdout, "couldn't get FB handle\n");
    return 0;
  }
  fbg->user_draw = fb_fbgdraw_fix;
  struct _fbg_fbdev_context *fbdev_context = fbg->user_context;
  unsigned long dummy = 0;


  //draw whole white red!
  fprintf(stdout, "APP connected with resolution: %dx%d", fbg->width, fbg->height);

  fprintf(stdout, "Drawing: \"display pan test\"\n");
  if(print_image_and_pan_display(fbg, fbdev_context, 1000000)) {
    fprintf(stderr, "exit\n");
    exit(-1);
  }

  fbg_background(fbg, 255, 0, 0); // can also be replaced by fbg_background(fbg, 0, 0, 0);
  fprintf(stdout, "Drawing now: \"fbg_background\"\n");
  fbg_flip(fbg);
  fbg_draw(fbg);

  // force buffer to be send to spi
  ioctl(fbdev_context->fd, WS_IO_PROCESS_AND_SEND, &dummy);
  sleep(1);

  // draw some stuff
  fprintf(stdout, "Drawing: \"rect\"\n");
  fbg_rect(fbg, fbg->width / 2 - 2, fbg->height / 2 - 2, 4, 4, 0, 255, 0);
  fbg_pixel(fbg, fbg->width / 2, fbg->height / 2, 255, 0, 0);
  fbg_pixel(fbg, 0, 0, 1, 2, 3);
  fbg_flip(fbg);
  fbg_draw(fbg);
  printBuff(fbg);

  // force buffer to be send to spi
  ioctl(fbdev_context->fd, WS_IO_PROCESS_AND_SEND, &dummy);
  printf("sleeping for 2 seconds?\n");
  sleep(1);



  fprintf(stdout, "Drawing: \"animation\"\n");
  animate_pixel(fbg, fbdev_context, 0, 191, 191);
  fprintf(stdout, "Executuing now: \"CLOSE\"\n");
  fbg_close(fbg);

  return 0;
}