################################################################################
#
# WS2812-simple-app
#
################################################################################

WS2812_APP_VERSION = 1.1
WS2812_APP_SITE = $(BR2_EXTERNAL_WS2812_panel_PATH)/app_src
WS2812_APP_SITE_METHOD = local
WS2812_APP_INSTALL_STAGING = NO
WS2812_APP_INSTALL_TARGET = YES
WS2812_APP_LICENSE = GPL-2.0

define WS2812_APP_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/WS2812_app $(TARGET_DIR)/usr/bin
endef

$(eval $(cmake-package))