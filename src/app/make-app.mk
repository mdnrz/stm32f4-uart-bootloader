APP_DIR = $(SRC_DIR)/app
FREERTOS_DIR = $(APP_DIR)/freertos

include $(SRC_DIR)/shared/make-shared.mk
include $(APP_DIR)/make-freertos.mk

# Modules
APP_SRC += \
		   $(wildcard $(APP_DIR)/*.c) \
		   $(SHARED_SRC) \
		   $(FREERTOS_SRC)

APPLICATION_INC += $(SHARED_INC) $(FREERTOS_INC)

APP_LINKER_SCRIPT = $(APP_DIR)/linkerscript.ld

