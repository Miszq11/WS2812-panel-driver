WS2812_MODULE_VERSION = 1.0
WS2812_MODULE_SITE = $(BR2_EXTERNAL_WS2812_panel_PATH)/module_src
WS2812_MODULE_SITE_METHOD = local
WS2812_MODULE_LICENSE = GPL-2.0

$(eval $(kernel-module))
$(eval $(generic-package))