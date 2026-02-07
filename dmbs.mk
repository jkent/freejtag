# SPDX-License-Identifier: MIT
#
# FreeJTAG
# Copyright (C) 2026 Jeff Kent <jeff@jkent.net>

# Include Guard
ifeq ($(filter FREEJTAG, $(DMBS_BUILD_MODULES)),)

# Sanity check user supplied DMBS path
ifndef DMBS_PATH
$(error Makefile DMBS_PATH option cannot be blank)
endif

# Location of the current module
FREEJTAG_MODULE_PATH := $(dir $(lastword $(MAKEFILE_LIST)))

# Import the CORE module of DMBS
include $(DMBS_PATH)/core.mk

# This module needs to be included before gcc.mk
ifneq ($(findstring GCC, $(DMBS_BUILD_MODULES)),)
$(error Include this module before gcc.mk)
endif

# Help settings
DMBS_BUILD_MODULES         += FREEJTAG
DMBS_BUILD_TARGETS         +=
DMBS_BUILD_MANDATORY_VARS  += DMBS_PATH
DMBS_BUILD_OPTIONAL_VARS   +=
DMBS_BUILD_PROVIDED_VARS   += FREEJTAG_SRC
DMBS_BUILD_PROVIDED_MACROS +=

# Sanity check user supplied values
$(foreach MANDATORY_VAR, $(DMBS_BUILD_MANDATORY_VARS), $(call ERROR_IF_UNSET, $(MANDATORY_VAR)))

# Common freejtag source
FREEJTAG_SRC :=     \
    freejtag.c      \

# Compiler flags and sources
SRC			       += $(patsubst %,$(FREEJTAG_MODULE_PATH)/%,src/$(FREEJTAG_SRC))

CC_FLAGS 	       += -I$(FREEJTAG_MODULE_PATH)/include

# Phony build targets for this module
.PHONY: $(DMBS_BUILD_TARGETS)

endif
