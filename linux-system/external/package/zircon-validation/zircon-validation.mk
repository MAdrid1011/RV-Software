################################################################################
#
# zircon-validation
#
################################################################################

ZIRCON_VALIDATION_SITE = $(BR2_EXTERNAL_ZIRCON_PATH)/package/zircon-validation/src
ZIRCON_VALIDATION_SITE_METHOD = local

define ZIRCON_VALIDATION_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) -O2 -static -ffixed-f8 \
		-march=rv32imaf_zicsr_zifencei -mabi=ilp32f \
		-Wall -Wextra -Werror \
		-o $(@D)/zircon-validation $(@D)/zircon-validation.c
endef

define ZIRCON_VALIDATION_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/zircon-validation $(TARGET_DIR)/usr/bin/zircon-validation
endef

$(eval $(generic-package))
