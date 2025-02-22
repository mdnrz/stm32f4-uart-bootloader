# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q		:= @
NULL		:= 2>/dev/null
endif

.DEFAULT_GOAL := all

# toolchain
TOOLCHAIN    = arm-none-eabi-
CC           = $(TOOLCHAIN)gcc
DB           = $(TOOLCHAIN)gdb
CP           = $(TOOLCHAIN)objcopy
AS           = $(TOOLCHAIN)gcc -x assembler-with-cpp
HEX          = $(CP) -O ihex
BIN          = $(CP) -O binary -S --gap-fill 0xFF

# define mcu, specify the target processor
MCU          = cortex-m4

ROOT_DIR = .
SRC_DIR = $(ROOT_DIR)/src
include $(SRC_DIR)/globals/make-version.mk
include $(SRC_DIR)/app/make-app.mk
include $(SRC_DIR)/bl/make-bl.mk

# all the files will be generated with this name (main.elf, main.bin, main.hex, etc)
PROJECT_NAME = 8ch
APP_NAME = $(PROJECT_NAME)-$(VERSION)
BL_NAME = $(PROJECT_NAME)-bl

APP_INC += -I$(SRC_DIR) $(APPLICATION_INC)

# CPU defs
DEFS += -DSTM32F4
FP_FLAGS ?= -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS = -mthumb -mcpu=$(MCU) $(FP_FLAGS)

# Compile flags
OPT := -Os
DEBUG := -ggdb3
CSTD ?= -std=c99

BUILD_DIR = $(ROOT_DIR)/build

APP_OBJ = $(addprefix $(BUILD_DIR)/,$(notdir $(APP_SRC:.c=.o)))
vpath %.c $(sort $(dir $(APP_SRC)))

BL_OBJ = $(addprefix $(BUILD_DIR)/,$(notdir $(BL_SRC:.c=.o)))
vpath %.c $(sort $(dir $(BL_SRC)))



###############################################################################
# C flags
TGT_CFLAGS	+= $(OPT) $(CSTD) $(DEBUG)
TGT_CFLAGS	+= $(ARCH_FLAGS)
TGT_CFLAGS	+= -Wall -Wextra -Wpedantic -Wshadow -Wdouble-promotion -Wformat=2 \
			   -Wformat-truncation -Wundef -fno-common -MD $(DEFS) -Wimplicit-function-declaration \

TGT_CFLAGS	+= -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes
TGT_CFLAGS	+= -ffunction-sections -fdata-sections

###############################################################################
# Linker flags
TGT_LDFLAGS		+= --static -nostartfiles
TGT_LDFLAGS		+= $(ARCH_FLAGS) $(DEBUG)
TGT_LDFLAGS		+= -Wl,--cref
TGT_LDFLAGS		+= -Wl,--gc-sections
TGT_LDFLAGS		+= -Wl,--print-memory-usage
TGT_LDFLAGS		+= --specs=rdimon.specs -u _printf_float
ifeq ($(V),99)
TGT_LDFLAGS		+= -Wl,--print-gc-sections
endif

###############################################################################
# Used libraries
LDLIBS += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group $(SHARED_LIB_PATH) $(SHARED_LIB) 

APP_LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(APP_NAME).map $(TGT_LDFLAGS) -T$(APP_LINKER_SCRIPT) $(LDLIBS) -lm -DBOOTLOADER=0

BL_LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(BL_NAME).map $(TGT_LDFLAGS) -T$(BL_LINKER_SCRIPT) $(LDLIBS) -DBOOTLOADER=1 


all: update_bin #tags

update_bin: version bl app 
	$(eval APP_ELF = $(notdir $(shell ls -t $(BUILD_DIR)/$(APP_NAME).elf)))
	$(eval BL_ELF = $(notdir $(shell ls -t $(BUILD_DIR)/$(BL_NAME).elf)))
	@(cp $(BUILD_DIR)/$(APP_ELF) $(BUILD_DIR)/fw.elf)

app: $(BUILD_DIR)/$(APP_NAME).elf $(BUILD_DIR)/$(APP_NAME).bin $(BUILD_DIR)/$(APP_NAME).hex
	$(Q)#$(TOOLCHAIN)size $(BUILD_DIR)/$(APP_NAME).elf

bl: $(BUILD_DIR)/$(BL_NAME).elf $(BUILD_DIR)/$(BL_NAME).bin $(BUILD_DIR)/$(BL_NAME).hex
	$(Q)#$(TOOLCHAIN)size $(BUILD_DIR)/$(BL_NAME).elf

OPENOCD_INTERFACE = /usr/share/openocd/scripts/interface/stlink.cfg
OPENOCD_TARGET = /usr/share/openocd/scripts/target/stm32f4x.cfg

flash-app: update_bin
	$(Q)openocd -d0 -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) -c "program $(BUILD_DIR)/"$(APP_ELF)" verify reset exit"
flash-bl: update_bin
	$(Q)openocd -d0 -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) -c "program $(BUILD_DIR)/$(BL_ELF) verify reset exit"
flash: update_bin
	$(Q)openocd -d0 -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) -c "program $(BUILD_DIR)/$(BL_ELF) verify" -c "program $(BUILD_DIR)/"$(APP_ELF)" verify reset exit"

fw-debug: update_bin
	$(DB) $(BUILD_DIR)/$(APP_ELF)

bl-debug: update_bin
	$(DB) $(BUILD_DIR)/$(BL_ELF)

# Force to re-build version file 
$(BUILD_DIR)/firmware_info.o: .FORCE

.PHONY: .FORCE

.FORCE:

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@printf "  CC      $(<F)\n"
	$(Q)$(CC) -c $(TGT_CFLAGS) $(APP_INC) $< -o $@

$(BUILD_DIR)/$(APP_NAME).elf: $(APP_OBJ)
	$(Q)$(CC) $(APP_OBJ) $(APP_LDFLAGS) -o $@

$(BUILD_DIR)/$(BL_NAME).elf: $(BL_OBJ)
	$(Q)$(CC) $(BL_OBJ) $(BL_LDFLAGS) -o $@
	$(Q)printf "\n"

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(Q)$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(Q)$(BIN)  $< $@

$(BUILD_DIR):
	$(Q)mkdir -p $(BUILD_DIR)

clean:
	$(Q)rm -rf $(BUILD_DIR)

tags: $(APP_OBJ) $(BL_OBJ)
	$(Q)@printf "Generating tags ...\n"
	$(Q)ctags -R .
