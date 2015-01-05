include $(TOPDIR)/rules.mk

# Name and release number of this package
PKG_NAME:=ezedns-client
PKG_RELEASE:=1
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
include $(INCLUDE_DIR)/package.mk
# The variables defined here should be self explanatory.
define Package/ezedns-client
	SECTION:=utils
	CATEGORY:=Utilities
	DEPENDS:= +libcurl #+libc +USE_EGLIBC:librt +USE_EGLIBC:libpthread 
	TITLE:=eZeDNSClient -- prints a snarky message
endef
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef
define Package/ezedns-client/install	
	$(INSTALL_DIR) $(1)/bin
	mkdir -p $(1)/etc
	$(CP) $(PKG_BUILD_DIR)/*.conf $(1)/etc/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ezedns-client $(1)/bin/	
endef
$(eval $(call BuildPackage,ezedns-client))
