#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# FreeJTAG
# Copyright (C) 2026 Jeff Kent <jeff@jkent.net>

from sys import stderr

import usb.core
import usb.util

AVR_IR_EXTEST           = 0
AVR_IR_IDCODE           = 1
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

class FreeJTAG:
    REQ_VERSION             = 0x00
    REQ_RESET               = 0x01
    REQ_EXECUTE             = 0x02
    REQ_READBUF             = 0x03
    REQ_BULKBYTE            = 0x04
    REQ_READOCDR            = 0x80

    CMD_NOP                 = 0x00
    CMD_ATTACH              = 0x01
    CMD_SET_TDI             = 0x02
    CMD_SET_TMS             = 0x03
    CMD_SET_STATE           = 0x04
    CMD_CLOCK               = 0x05
    CMD_SHIFT               = 0x06
    CMD_SHIFT_EXIT          = 0x07
    CMD_SHIFT_OUT           = 0x40
    CMD_SHIFT_OUT_EXIT      = 0x41
    CMD_SHIFT_IN            = 0x80
    CMD_SHIFT_IN_EXIT       = 0x81
    CMD_SHIFT_OUTIN         = 0xC0
    CMD_SHIFT_OUTIN_EXIT    = 0xC1

    STATE_RESET             = 0x00
    STATE_RUNIDLE           = 0x01
    STATE_DRSELECT          = 0x02
    STATE_DRCAPTURE         = 0x03
    STATE_DRSHIFT           = 0x04
    STATE_DREXIT1           = 0x05
    STATE_DRPAUSE           = 0x06
    STATE_DREXIT2           = 0x07
    STATE_DRUPDATE          = 0x08
    STATE_IRSELECT          = 0x09
    STATE_IRCAPTURE         = 0x0A
    STATE_IRSHIFT           = 0x0B
    STATE_IREXIT1           = 0x0C
    STATE_IRPAUSE           = 0x0D
    STATE_IREXIT2           = 0x0E
    STATE_IRUPDATE          = 0x0F
    STATE_UNKNOWN           = 0x10

    def __init__(self, vid=0x0403, pid=0x7ab8, index=0):
        devices = tuple(usb.core.find(idVendor=vid, idProduct=pid, find_all=True))
        if not devices:
            raise Exception('No devices found')

        try:
            self._dev: usb.core.Device = devices[index]
        except IndexError:
            raise Exception('Invalid FreeJTAG device index')

        self._config: usb.core.Configuration = self._dev.get_active_configuration()
        interfaces = map(lambda i: self._config[(i, 0)], range(self._config.bNumInterfaces))
        interfaces = tuple(filter(dev.match_interface_string_re(r'^FreeJTAG Interface$'), interfaces))
        try:
            self._intf: usb.core.Interface = interfaces[0]
        except:
            raise Exception('FreeJTAG interface was not found')

    @staticmethod
    def get_default_language_id(device: usb.core.Device):
        try:
            lang_id = usb.util.get_langids(device)[0]
        except usb.core.USBError:
            raise Exception('Error accessing device, check permissions')
        return lang_id

    @classmethod
    def match_product_string_re(cls, pattern):
        import re
        def match(device: usb.core.Device):
            if not device.iProduct:
                return False
            lang_id = cls.get_default_language_id(device)
            string = usb.util.get_string(device, device.iProduct, lang_id)
            m = re.search(pattern, string)
            return bool(m)
        return match

    @classmethod
    def match_interface_string_re(cls, pattern):
        import re
        def match(intf: usb.core.Interface):
            if not intf.iInterface:
                return False
            lang_id = cls.get_default_language_id(intf.device)
            string = usb.util.get_string(intf.device, intf.iInterface, lang_id)
            m = re.search(pattern, string)
            return bool(m)
        return match

    def __enter__(self):
        usb.util.claim_interface(self._dev, self._intf)
        self._attach(True)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._attach(False)
        usb.util.release_interface(self._dev, self._intf)

    def version(self):
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_IN,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_INTERFACE)
        data = self._dev.ctrl_transfer(bmRequestType, self.REQ_VERSION, 0,
                self._intf.bInterfaceNumber, 2)
        value = int.from_bytes(data, 'little')
        major = (value & 0xFF00) >> 8
        minor = (value & 0xF0) >> 4
        patch = (value & 0xF)
        return major, minor, patch

    def _execute(self, cmd, arg, data=None):
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_OUT,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_INTERFACE)
        wValue = (arg << 8) | (cmd & 0xff)
        self._dev.ctrl_transfer(bmRequestType, self.REQ_EXECUTE, wValue,
                self._intf.bInterfaceNumber, data)

    def _readbuf(self, wLength):
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_IN,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_INTERFACE)
        return self._dev.ctrl_transfer(bmRequestType, self.REQ_READBUF, 0,
                self._intf.bInterfaceNumber, wLength)

    def _attach(self, attach=True):
        self._execute(self.CMD_ATTACH, attach)

    def set_tdi(self, value=True):
        self._execute(self.CMD_SET_TDI, value)

    def set_tms(self, value=True):
        self._execute(self.CMD_SET_MS, value)

    def set_state(self, state):
        self._execute(self.CMD_SET_STATE, state)
        self._state = state

    def clock(self, cycles):
        self._execute(self.CMD_CLOCK, cycles - 1)

    def shift(self, bits, exit=True):
        cmd = self.CMD_SHIFT_EXIT if exit else self.CMD_SHIFT
        self._execute(cmd, bits - 1)

    def shift_out(self, bits, value: int, exit=True):
        cmd = self.CMD_SHIFT_OUT_EXIT if exit else self.CMD_SHIFT_OUT
        n_bytes = (bits + 7) // 8
        data = value.to_bytes(n_bytes, 'little')
        self._execute(cmd, bits - 1, data)

    def shift_in(self, bits, exit=True):
        cmd = self.CMD_SHIFT_IN_EXIT if exit else self.CMD_SHIFT_IN
        n_bytes = (bits + 7) // 8
        self._execute(cmd, bits - 1)
        data = self._readbuf(n_bytes)
        return int.from_bytes(data, 'little') & ((1 << bits) - 1)

    def shift_outin(self, bits, value, exit=True):
        cmd = self.CMD_SHIFT_OUTIN_EXIT if exit else self.CMD_SHIFT_OUTIN
        n_bytes = (bits + 7) // 8
        data = value.to_bytes(n_bytes, 'little')
        self._execute(cmd, bits - 1, data)
        data = self._readbuf(n_bytes)
        return int.from_bytes(data, 'little') & ((1 << bits) - 1)

    def shift_ir(self, total_bits, value=None, read=False) -> None | int:
        result = None
        self.set_state(self.STATE_IRSHIFT)
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

    def write_bytes(self, data: bytes) -> None:
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_OUT,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_INTERFACE)
        while True:
            chunk, data = data[:32], data[32:]
            if not chunk:
                break
            self._dev.ctrl_transfer(bmRequestType, self.REQ_BULKBYTE, 0,
                    self._intf.bInterfaceNumber, chunk)

    def bulk_read_bytes(self, count: int) -> bytes:
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_IN,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_INTERFACE)
        data = b''
        while count > 0:
            chunk = min(32, count)
            data += self._dev.ctrl_transfer(bmRequestType, self.REQ_BULKBYTE, 0,
                    self._intf.bInterfaceNumber, chunk)
            count -= chunk
        return data

    def avr_read_ocdr(self):
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_IN,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_INTERFACE)
        data = self._dev.ctrl_transfer(bmRequestType, self.REQ_READOCDR, 0,
                self._intf.bInterfaceNumber, 2)
        ch = int.from_bytes(data, 'little', signed=True)
        if ch < 0:
            return None
        return bytes((ch,))

    def get_signature(self):
        jtag.shift_ir(4, AVR_IR_PROG_COMMANDS)
        def read_byte(addr):
            jtag.shift_dr(15, 0b0100011_00001000)
            jtag.shift_dr(15, 0b0000011_00000000 | addr)
            jtag.shift_dr(15, 0b0110010_00000000)
            return jtag.shift_dr(15, 0b0110011_00000000, read=True) & 0xFF

        return bytes(read_byte(addr) for addr in range(3))

if __name__ == '__main__':
    device = FreeJTAG()

    with device as jtag:
        print(jtag.version())

        jtag.shift_ir(4, AVR_IR_IDCODE)
        idcode = jtag.shift_dr(32, read=True)
        print(f'IDCODE = 0x{idcode:08X}')

        jtag.shift_ir(4, AVR_IR_RESET)
        jtag.shift_dr(1, 1)

        jtag.shift_ir(4, AVR_IR_PROG_ENABLE)
        jtag.shift_dr(16, 0xa370)

        signature = jtag.get_signature(jtag)
        print('Signature: ' + ' '.join(f'{byte:02X}' for byte in signature))

        jtag.shift_ir(4, AVR_IR_PROG_ENABLE)
        jtag.shift_dr(16, 0)

        jtag.shift_ir(4, AVR_IR_RESET)
        jtag.shift_dr(1, 0)

    with device as jtag:
        while True:
            ch = jtag.avr_read_ocdr()
            if ch is not None:
                stderr.buffer.write(ch)
                stderr.buffer.flush()
