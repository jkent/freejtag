#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# Copyright (C) 2026 Jeff Kent <jeff@jkent.net>

import os
import sys

from freejtag import AVR_IR_RESET, FreeJTAG


with FreeJTAG() as jtag:
    def reset():
        jtag.shift_ir(4, AVR_IR_RESET)
        jtag.shift_dr(1, True)
        jtag.shift_dr(1, False)

    reset()
    try:
        while True:
            ch = jtag.avr_read_ocdr()
            if ch is not None:
                sys.stderr.buffer.write(ch)
                sys.stderr.buffer.flush()
    except KeyboardInterrupt:
        pass

