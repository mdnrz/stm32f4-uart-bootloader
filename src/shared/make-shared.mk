SHARED_DIR = $(SRC_DIR)/shared
OPENCM3_DIR = $(SHARED_DIR)/opencm3
OPENCM3_LIB = opencm3_stm32f4
OPENCM3_INC = $(OPENCM3_DIR)/include

# Modules
DRIVERS_SRC = $(wildcard $(SHARED_DIR)/drivers/*.c)

UTILS_SRC = $(wildcard $(SHARED_DIR)/comms/protocol/*c) \
			$(wildcard $(SHARED_DIR)/comms/ring-buffer/*.c) \
			$(wildcard $(SHARED_DIR)/crc/*.c) \
			$(wildcard $(SHARED_DIR)/memory/*.c) \
			$(wildcard $(SHARED_DIR)/parameters/*.c)

SHARED_SRC = $(DRIVERS_SRC) $(UTILS_SRC)
SHARED_INC = -I$(OPENCM3_INC)
SHARED_LIB = -l$(OPENCM3_LIB)
SHARED_LIB_PATH = -L$(OPENCM3_DIR)/lib

