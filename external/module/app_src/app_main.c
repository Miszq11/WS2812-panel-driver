#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>

#define WS_IO_DUMMY _IO('x', 1)
#define WS_IO_PROCESS_AND_SEND _IO('x', 2)

#include "fbgraphics.h"
#include "fbg_fbdev.h" // insert any backends from ../custom_backend/backend_name folder

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

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

void printHelp() {
    printf("Missing program argument! \
    \n\tThis app should be called with path to framebuffer\
    \n\tallocated by ws2812_mod\
    \n\t\
    \n\tRun module with: modprobe ws2812_mod\
    \n\tRun app (example): WS2812_app /dev/fb1\
    \n\tlist all framebuffers with: ls /dev/fb*\n");
}

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

int main(int argc, char* argv[]) {
    if(argc < 2) {
        printHelp();
        return -1;
    }

    signal(SIGINT, int_handler);

   // open "/dev/fb0" by default, use fbg_fbdevSetup("/dev/fb1", 0) if you want to use another framebuffer
   // note : fbg_fbdevInit is the linux framebuffer backend, you can use a different backend easily by including the proper header and compiling with the appropriate backend file found in ../custom_backend/backend_name
    struct _fbg *fbg = fbg_fbdevSetup(argv[1], 0);
    if (fbg == NULL) {
        fprintf(stdout, "couldn't get FB handle\n");
        return 0;
    }
    struct _fbg_fbdev_context *fbdev_context = fbg->user_context;
    unsigned long dummy = 0;

    //draw whole white red!
    fprintf(stdout, "APP connected with resolution: %dx%d", fbg->width, fbg->height);
    fprintf(stdout, "Executing now: \"fbg_background\"\n");
    fbg_background(fbg, 255, 0, 0); // can also be replaced by fbg_background(fbg, 0, 0, 0);
    fprintf(stdout, "Drawing now: \"fbg_background\"\n");
    fbg_flip(fbg);
    fbg_draw(fbg);

    // force buffer to be send to spi
    ioctl(fbdev_context->fd, WS_IO_PROCESS_AND_SEND, &dummy);

    printf("sleeping for 2 seconds?\n");
    sleep(2);

    // draw some stuff
    fprintf(stdout, "Executing now: \"rect\"\n");
    fbg_rect(fbg, fbg->width / 2 - 2, fbg->height / 2 - 2, 4, 4, 0, 255, 0);
    printBuff(fbg);
    fprintf(stdout, "Executing now: \"pixel\"\n");
    fbg_pixel(fbg, fbg->width / 2, fbg->height / 2, 255, 0, 0);
    fbg_pixel(fbg, 0, 0, 1, 2, 3);
    fprintf(stdout, "Executing now: \"flip\"??\n");
    fbg_flip(fbg);
    fprintf(stdout, "Drawing now: \"Compound\"\n");
    fbg_draw(fbg);
    //printBuff(fbg);

    // force buffer to be send to spi
    ioctl(fbdev_context->fd, WS_IO_PROCESS_AND_SEND, &dummy);

    fprintf(stdout, "Executing: \"animation\"\n");
    animate_pixel(fbg, fbdev_context, 191, 0, 191);
    fprintf(stdout, "Executuing now: \"CLOSE\"\n");
    fbg_close(fbg);

    return 0;
}