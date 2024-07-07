#ifndef WS2812_CONFIG_H
#define WS2812_CONFIG_H

#include "linux/fb.h"
#include <linux/types.h>

typedef struct WS2812_regs_t {
  uint32_t ddd;
} WS2812_regs;

struct WS2812_module_fb_info {
  u8 *fb_virt;
  dma_addr_t fb_phys;
  struct device *dev;
};

static const struct fb_fix_screeninfo WS_fb_fix = {
  .id         = "WS2812 Panel fb",
  .type       = FB_TYPE_PACKED_PIXELS, // ??
  .visual     = FB_VISUAL_TRUECOLOR, // ??
  .xpanstep   = 0, // ??
  .ypanstep   = 0, // ??
  .ywrapstep  = 0, // ??
  .accel      = FB_ACCEL_NONE,
};


#endif //WS2812_CONFIG_H
