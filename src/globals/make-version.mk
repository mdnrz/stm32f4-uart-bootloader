GLOBAL_DIR = $(SRC_DIR)/globals
###############################################################################
# Versioning
VERSION = $(shell git describe --always --abbrev=4 --long)
VERSION_MAJOR = $(shell git describe --always --long | sed "s/^v\([0-9]\+\).*/\1/")
VERSION_MINOR = $(shell git describe --always --long | sed "s/^v[0-9]\+\.\([0-9]\+\).*/\1/")
VERSION_PATCH = $(shell git describe --always --long | sed "s/^v[0-9]\+\.[0-9]\+-\([0-9]\+\).*/\1/")
VERSION_HASH  = $(shell git describe --always --abbrev=4 --long | sed "s/^v[0-9]\+\.[0-9]\+-[0-9]\+-\([a-Z0-9]\{5\}\).*/\1/")

VERSION_FILE = $(GLOBAL_DIR)/version.h

version: | $(VERSION_FILE)
	$(Q)printf "// fw version: $(VERSION)\n \
	#include <stdint.h>\n \
	const uint8_t gGIT_VERSION_MAJOR = $(VERSION_MAJOR);\n \
	const uint8_t gGIT_VERSION_MINOR = $(VERSION_MINOR);\n \
	const uint8_t gGIT_VERSION_PATCH = $(VERSION_PATCH);\n \
	#define gGIT_VERSION_HASH \"$(VERSION_HASH)\"\n" > $(VERSION_FILE)

$(VERSION_FILE):
	$(shell touch $(VERSION_FILE)) \
