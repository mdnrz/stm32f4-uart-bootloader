
# source directory
FREERTOS_SRC_DIR     = $(FREERTOS_DIR)
FREERTOS_INC_DIR     = $(FREERTOS_DIR)/include
FREERTOS_ARM_CM4_DIR = $(FREERTOS_DIR)/portable/GCC/ARM_CM4F
FREERTOS_MemMang_DIR = $(FREERTOS_DIR)/portable/MemMang

# add freertos source
FREERTOS_SRC  += $(wildcard $(FREERTOS_SRC_DIR)/*.c)
FREERTOS_SRC  += $(FREERTOS_ARM_CM4_DIR)/port.c
FREERTOS_SRC  += $(FREERTOS_MemMang_DIR)/heap_4.c

# include directories
FREERTOS_INC = -I$(FREERTOS_INC_DIR) \
			   -I$(FREERTOS_ARM_CM4_DIR)
