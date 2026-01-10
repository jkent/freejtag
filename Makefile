Q            = @
MCU          = atmega16u2
ARCH         = AVR8
F_CPU        = 8000000
F_USB        = $(F_CPU)
OPTIMIZATION = s
TARGET       = bin/firmware
CC_FLAGS     = -Iinclude -DUSE_LUFA_CONFIG_HEADER
LD_FLAGS     =
SRC          =              \
	src/cdc.c               \
	src/descriptors.c       \
	src/jtag.c              \
	src/main.c              \
	$(LUFA_SRC_USB)         \
	$(LUFA_SRC_USBCLASS)    \

AVRDUDE_PROGRAMMER = stk500v2
AVRDUDE_PORT = /dev/ttyUSB0
AVRDUDE_BUAD = 115200

# https://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega16u2&LOW=EF&HIGH=D1&EXTENDED=FC&LOCKBIT=FF
# LFUSE: Ext Osc 8MHz 16 CK + 4.1 ms
AVRDUDE_LFUSE = 0xEF
# HFUSE: SPIEN EESAVE BOOTSZ=4KiB
AVRDUDE_HFUSE = 0xD1
# EFUSE: BODLEVEL=3.0v
AVRDUDE_EFUSE = 0xFC
# LOCK: None
AVRDUDE_LOCK = 0xFF

# Default target
all:

program: all avrdude
program-all: all avrdude avrdude-ee avrdude-fuses

# Setup paths
LUFA_PATH           ?= third-party/lufa/LUFA
DMBS_LUFA_PATH      ?= $(LUFA_PATH)/Build/LUFA
DMBS_PATH           ?= $(LUFA_PATH)/Build/DMBS/DMBS

# Include LUFA-specific DMBS extension modules
include $(DMBS_LUFA_PATH)/lufa-sources.mk
include $(DMBS_LUFA_PATH)/lufa-gcc.mk

# Include common DMBS build system modules
include $(DMBS_PATH)/core.mk
include $(DMBS_PATH)/cppcheck.mk
include $(DMBS_PATH)/gcc.mk
include $(DMBS_PATH)/avrdude.mk

build_begin: build_mkdir

build_mkdir:
	$(Q)mkdir -p $(shell dirname $(TARGET))
