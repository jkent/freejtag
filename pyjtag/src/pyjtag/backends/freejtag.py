#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# FreeJTAG
# Copyright (C) 2026 Jeff Kent <jeff@jkent.net>

from sys import stderr

import usb.core
import usb.util

from ..jtag import JTAG

class Backend:
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

    def __init__(self, **kwargs):
        vid = kwargs.get('vid') or 0x0403
        pid = kwargs.get('pid') or 0x7ba8
        index = kwargs.get('index') or 0

        devices = tuple(usb.core.find(idVendor=vid, idProduct=pid, find_all=True))
        if not devices:
            raise Exception('No devices found')

        try:
            self._dev: usb.core.Device = devices[index]
        except IndexError:
            raise Exception('Invalid FreeJTAG device index')

        self._config: usb.core.Configuration = self._dev.get_active_configuration()
        interfaces = map(lambda i: self._config[(i, 0)], range(self._config.bNumInterfaces))
        interfaces = tuple(filter(self.match_interface_string_re(r'^FreeJTAG Interface$'), interfaces))
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

    def _acquire(self):
        usb.util.claim_interface(self._dev, self._intf)
        self._attach(True)

    def _release(self):
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

    def bulk_write_bytes(self, data: bytes) -> None:
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
