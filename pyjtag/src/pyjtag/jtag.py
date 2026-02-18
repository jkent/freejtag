#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# Copyright (C) 2026 Jeff Kent <jeff@jkent.net>

import click
import importlib
from .util import HexParamType

IR_EXTEST               = 0
IR_IDCODE               = 1
AVR_IR_SAMPLE_PAYLOAD   = 2
AVR_IR_PROG_ENABLE      = 4
AVR_IR_PROG_COMMANDS    = 5
AVR_IR_PROG_PAGELOAD    = 6
AVR_IR_PROG_PAGEREAD    = 7
AVR_IR_PRIVATE0         = 8
AVR_IR_PRIVATE1         = 9
AVR_IR_PRIVATE2         = 10
AVR_IR_PRIVATE3         = 11
AVR_IR_RESET            = 12
AVR_IR_BYPASS           = 15

class JTAG:
    STATE_RESET         = 0x00
    STATE_RUNIDLE       = 0x01
    STATE_DRSELECT      = 0x02
    STATE_DRCAPTURE     = 0x03
    STATE_DRSHIFT       = 0x04
    STATE_DREXIT1       = 0x05
    STATE_DRPAUSE       = 0x06
    STATE_DREXIT2       = 0x07
    STATE_DRUPDATE      = 0x08
    STATE_IRSELECT      = 0x09
    STATE_IRCAPTURE     = 0x0A
    STATE_IRSHIFT       = 0x0B
    STATE_IREXIT1       = 0x0C
    STATE_IRPAUSE       = 0x0D
    STATE_IREXIT2       = 0x0E
    STATE_IRUPDATE      = 0x0F
    STATE_UNKNOWN       = 0x10

    def __init__(self, backend='freejtag', *args, **kwargs):
        mod = importlib.import_module(f'.{backend}', 'pyjtag.backends')
        self.backend = mod.Backend(*args, **kwargs)

    def __enter__(self):
        self.backend._acquire()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.backend._release()

    def set_state(self, state):
        self.backend.set_state(state)

    def shift(self, bits, exit=True):
        self.backend.shift(bits, exit)

    def shift_out(self, bits, value, exit=True):
        self.backend.shift_out(bits, value, exit=True)

    def shift_in(self, bits, exit=True):
        return self.backend.shift_in(bits, exit)

    def shift_outin(self, bits, value, exit=True):
        return self.backend.shift_outin(bits, value, exit)

    def shift_ir(self, total_bits, value=None, read=False) -> None | int:
        result = None
        self.set_state(JTAG.STATE_IRSHIFT)
        if value is None and not read:
           self.shift(total_bits)
        elif value is not None and not read:
           self.shift_out(total_bits, value)
        elif value is None and read:
           result = self.shift_in(total_bits)
        elif value is not None and read:
           result = self.shift_outin(total_bits, value)
        self.set_state(self.STATE_RUNIDLE)
        return result

    def shift_dr(self, total_bits, value=None, read=False) -> None | int:
        result = None
        self.set_state(self.STATE_DRSHIFT)
        if value is None and not read:
            self.shift(total_bits)
        elif value is not None and not read:
            self.shift_out(total_bits, value)
        elif value is None and read:
            result = self.shift_in(total_bits)
        elif value is not None and read:
            result = self.shift_outin(total_bits, value)
        self.set_state(self.STATE_RUNIDLE)
        return result

    def avr_reset(self, state=True):
        self.shift_ir(4, AVR_IR_RESET)
        self.shift_dr(1, state)

    def avr_prog_enable(self, state=True):
        self.shift_ir(4, AVR_IR_PROG_ENABLE)
        self.shift_dr(16, 0xa370 if state else 0)

    def avr_signature(self):
        self.shift_ir(4, AVR_IR_PROG_COMMANDS)
        def read_byte(addr):
            self.shift_dr(15, 0b0100011_00001000)
            self.shift_dr(15, 0b0000011_00000000 | (addr & 0xff))
            self.shift_dr(15, 0b0110010_00000000)
            return self.shift_dr(15, 0b0110011_00000000, read=True) & 0xff
        return bytes(read_byte(addr) for addr in range(3))

    def avr_prog_write(self, addr: int, value: int) -> None:
        addr &= 0xF
        value &= 0xFFFF

        self.shift_ir(4, AVR_IR_PRIVATE3)
        self.shift_dr(5, addr)
        self.shift_dr(21, (1 << 20) | (addr << 16) | value)

    def avr_prog_read(self, addr: int) -> int:
        addr &= 0xF

        self.shift_ir(4, AVR_IR_PRIVATE3)
        self.shift_dr(5, addr)
        return self.shift_dr(16, read=True)

    def avr_read_ocdr(self):
        if hasattr(self.backend, 'avr_read_ocdr'):
           return self.backend.avr_read_ocdr()
        if self.avr_prog_read(0xD) & 0x10:
            return bytes((self.avr_prog_read(0xC) >> 8,))

@click.command
@click.option('--backend', default='freejtag')
@click.option('--vid', type=HexParamType('vid'))
@click.option('--pid', type=HexParamType('pid'))
@click.option('--index', type=click.INT)
def main(backend, **kwargs):
    with JTAG(backend, **kwargs) as jtag:
        jtag.shift_ir(4, IR_IDCODE)
        idcode = jtag.shift_dr(32, read=True)
        print(f'IDCODE = 0x{idcode:08X}')

        jtag.avr_reset(True)
        jtag.avr_prog_enable(True)

        signature = jtag.avr_signature()
        print('Signature: ' + ' '.join(f'{byte:02X}' for byte in signature))

        jtag.avr_prog_enable(False)
        jtag.avr_reset(False)

if __name__ == '__main__':
    main()
