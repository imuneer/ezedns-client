include $(TOPDIR)/rules.mk

PKG_NAME:=ezedns-client
PKG_VERSION:=1
PKG_RELEASE:=1

PKG_BUILD_DIR=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/ezedns-client
	SECTION:=ezelink
	CATEGORY:=eZeLink
	DEPENDS:= +libcurl #+libc +USE_EGLIBC:librt +USE_EGLIBC:libpthread 
	TITLE:=eZeDNSClient -- dynamic dns client
endef

define Package/ezedns-client/install
        $(INSTALL_DIR) $(1)/usr/bin
	mkdir -p $(1)/etc
	$(CP) $(PKG_BUILD_DIR)/*.conf $(1)/etc/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ezedns-client $(1)/usr/bin/
endef

$(eval $(call BuildPackage,ezedns-client))
