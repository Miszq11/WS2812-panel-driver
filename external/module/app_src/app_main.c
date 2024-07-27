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

int main(int argc, char* argv[]) {
    signal(SIGINT, int_handler);

   // open "/dev/fb0" by default, use fbg_fbdevSetup("/dev/fb1", 0) if you want to use another framebuffer
   // note : fbg_fbdevInit is the linux framebuffer backend, you can use a different backend easily by including the proper header and compiling with the appropriate backend file found in ../custom_backend/backend_name
    struct _fbg *fbg = fbg_fbdevSetup("/dev/fb0", 0);
    if (fbg == NULL) {
        fprintf(stdout, "couldn't get FB handle\n");
        return 0;
    }
    struct _fbg_fbdev_context *fbdev_context = fbg->user_context;
    unsigned long dummy = 0;
    fprintf(stdout, "APP connected with resolution: %dx%d", fbg->width, fbg->height);
    fprintf(stdout, "Executing now: \"clear\"\n");
    fbg_background(fbg, 255, 255, 255); // can also be replaced by fbg_background(fbg, 0, 0, 0);
    fprintf(stdout, "Drawing now: \"clear\"\n");
    fbg_flip(fbg);
    fbg_draw(fbg);

    ioctl(fbdev_context->fd, WS_IO_PROCESS_AND_SEND, &dummy);

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
    printBuff(fbg);
    ioctl(fbdev_context->fd, WS_IO_PROCESS_AND_SEND, &dummy);
    fprintf(stdout, "Executuing now: \"CLOSE\"\n");
    fbg_close(fbg);

    return 0;
}