# module needs to drop include files before app compiles
include $(BR2_EXTERNAL_WS2812_panel_PATH)/package/WS2812_module/WS2812_module.mk
include $(BR2_EXTERNAL_WS2812_panel_PATH)/package/WS2812_app/WS2812_app.mk
#include $(sort $(wildcard $(BR2_EXTERNAL_WS2812_panel_PATH)/package/*/*.mk))