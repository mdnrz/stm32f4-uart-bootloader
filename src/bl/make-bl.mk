BL_DIR = $(SRC_DIR)/bl

include $(SRC_DIR)/shared/make-shared.mk

BL_SRC += $(SHARED_SRC) \
		  $(wildcard $(BL_DIR)/packet/*.c) \
		  $(wildcard $(BL_DIR)/stimer/*.c) \
		  $(wildcard $(BL_DIR)/update/*.c) \
		  $(BL_DIR)/bootloader.c

BL_INC += $(SHARED_INC)

BL_LINKER_SCRIPT = $(BL_DIR)/linkerscript.ld

