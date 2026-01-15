from sys import stderr, stdout

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

class CDC:
    def __init__(self, device):
        interfaces = map(lambda i: device.config[(i, 0)], range(device.config.bNumInterfaces))
        interfaces = filter(device.match_interface_string_re(r'^FreeJTAG CDC Interface$'), interfaces)
        try:
            self.intf: usb.core.Interface = tuple(interfaces)[0]
        except:
            print('FreeJTAG CDC interface was not found', file=stderr)
            exit(1)

    def __enter__(self):
        self._reattach = self.intf.device.is_kernel_driver_active(self.intf.bInterfaceNumber)
        if self._reattach:
            self.intf.device.detach_kernel_driver(self.intf.bInterfaceNumber)
        usb.util.claim_interface(self.intf.device, self.intf.bInterfaceNumber)
        self.ep_out = usb.util.find_descriptor(self.intf, custom_match = \
                lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == \
                usb.util.ENDPOINT_OUT)
        self.ep_in = usb.util.find_descriptor(self.intf, custom_match = \
                lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == \
                usb.util.ENDPOINT_IN)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.ep_out = None
        self.ep_in = None
        usb.util.release_interface(self.intf.device, self.intf.bInterfaceNumber)
        if self._reattach:
            self.intf.device.attach_kernel_driver(self.intf.bInterfaceNumber-1)

    def write(self, data):
        self.ep_out.write(data)

    def read(self, len):
        return bytes(self.ep_in.read(len))

class Tap:
    CMD_NOP             = 0
    CMD_VERSION         = 1
    CMD_ATTACH          = 2
    CMD_SET_STATE       = 3
    CMD_CLOCK           = 4
    CMD_CLOCK_OUT       = 5
    CMD_CLOCK_IN        = 6
    CMD_CLOCK_OUTIN     = 7
    CMD_BULK_LOAD_BYTES = 8
    CMD_BULK_READ_BYTES = 9
    CMD_AVR_READ_OCDR   = 128
    CMD_RESET           = 255

    STATE_UNNKOWN       = 0
    STATE_RESET         = 1
    STATE_RUNIDLE       = 2
    STATE_DRSELECT      = 3
    STATE_DRCAPTURE     = 4
    STATE_DRSHIFT       = 5
    STATE_DREXIT1       = 6
    STATE_DRPAUSE       = 7
    STATE_DREXIT2       = 8
    STATE_DRUPDATE      = 9
    STATE_IRSELECT      = 10
    STATE_IRCAPTURE     = 11
    STATE_IRSHIFT       = 12
    STATE_IREXIT1       = 13
    STATE_IRPAUSE       = 14
    STATE_IREXIT2       = 15
    STATE_IRUPDATE      = 16

    def __init__(self, device):
        interfaces = map(lambda i: device.config[(i, 0)], range(device.config.bNumInterfaces))
        interfaces = tuple(filter(device.match_interface_string_re(r'^FreeJTAG TAP Interface$'), interfaces))
        try:
            self.intf: usb.core.Interface = interfaces[0]
        except:
            print('FreeJTAG TAP interface was not found', file=stderr)
            exit(1)

    def __enter__(self):
        usb.util.claim_interface(self.intf.device, self.intf.bInterfaceNumber)
        self.ep_out = usb.util.find_descriptor(self.intf, custom_match = \
                lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == \
                usb.util.ENDPOINT_OUT)
        self.ep_in = usb.util.find_descriptor(self.intf, custom_match = \
                lambda e: usb.util.endpoint_direction(e.bEndpointAddress) == \
                usb.util.ENDPOINT_IN)

        # flush endpoints
        for i in range(33):
           try:
               self.ep_in.read(8, timeout=1)
           except:
               pass
           try:
               self.ep_out.write(b'\xff', timeout=1)
           except:
               pass

        self.attach(True)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.attach(False)
        self.ep_out = None
        self.ep_in = None
        usb.util.release_interface(self.intf.device, self.intf.bInterfaceNumber)

    def version(self):
        self.ep_out.write(bytes((self.CMD_VERSION,)))
        data = self.ep_in.read(2)
        value = int.from_bytes(data, 'little')
        major = (value & 0xFF00) >> 8
        minor = (value & 0xF0) >> 4
        patch = (value & 0xF)
        return major, minor, patch

    def attach(self, value=True):
        value = bool(value)
        self.ep_out.write(bytes((self.CMD_ATTACH, value)))
        if value:
            self.state = self.STATE_RESET

    def set_state(self, new_state):
        self.ep_out.write(bytes((self.CMD_SET_STATE, new_state)))
        self.state = self.ep_in.read(1)[0]
        assert(self.state == new_state)
        return self.state

    def shift(self, total_bits) -> None:
        for i in range(0, total_bits, 32):
            bits = min(total_bits - i, 32)
            exit = True if i + 32 >= total_bits else False
            self.ep_out.write(bytes((self.CMD_CLOCK, bits, exit)))

    def shift_out(self, total_bits, value: int) -> None:
        for i in range(0, total_bits, 32):
            bits = min(total_bits - i, 32)
            nbytes = (bits + 7) // 8
            exit = True if i + 32 >= total_bits else False
            chunk = (value >> i) & ((1 << bits) - 1)
            self.ep_out.write(bytes((self.CMD_CLOCK_OUT, bits, exit)) + \
                    chunk.to_bytes(nbytes, 'little'))

    def shift_in(self, total_bits) -> int:
        data = b''
        for i in range(0, total_bits, 32):
            bits = min(total_bits - i, 32)
            nbytes = (bits + 7) // 8
            exit = True if i + 32 >= total_bits else False
            self.ep_out.write(bytes((self.CMD_CLOCK_IN, bits, exit)))
            data = bytes(self.ep_in.read(nbytes)) + data
        return int.from_bytes(data, 'little') & (1 << total_bits) - 1

    def shift_outin(self, total_bits, value: int) -> int:
        data = b''
        for i in range(0, total_bits, 32):
            bits = min(total_bits - i, 32)
            nbytes = (bits + 7) // 8
            exit = True if i + 32 >= total_bits else False
            chunk = (value >> i) & ((1 << bits) - 1)
            self.ep_out.write(bytes((self.CMD_CLOCK_OUTIN, bits, exit)) + \
                    chunk.to_bytes(nbytes, 'little'))
            data = bytes(self.ep_in.read(nbytes)) + data
        return int.from_bytes(data, 'little') & (1 << total_bits) - 1

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

    def bulk_load_bytes(self, count: int, data: bytes) -> None:
        assert(len(data) == count)
        self.ep_out.write(bytes((self.CMD_BULK_LOAD_BYTES,)) + \
                count.to_bytes(2, 'little'))
        self.ep_out.write(data)

    def bulk_read_bytes(self, count: int) -> bytes:
        self.ep_out.write(bytes((self.CMD_BULK_READ_BYTES,)) + \
                count.to_bytes(2, 'little'))
        data = bytes(self.ep_in.read(count))
        return data

    def avr_read_ocdr(self):
        self.ep_out.write(bytes((self.CMD_AVR_READ_OCDR,)))
        data = bytes(self.ep_in.read(2))
        ch = int.from_bytes(data, 'little', signed=True)
        if ch < 0:
            return None
        return bytes((ch,))

class FreeJTAG:
    def get_default_language_id(self, device: usb.core.Device):
        try:
            lang_id = usb.util.get_langids(device)[0]
        except usb.core.USBError:
            print('Error accessing device, check permissions', file=stderr)
            exit(1)
        return lang_id

    def match_serial_string_prefix(self, prefix):
        def match(device: usb.core.Device):
            if not device.iSerialNumber:
                return False
            lang_id = self.get_default_language_id(device)
            string = usb.util.get_string(device, device.iSerialNumber, lang_id)
            return string.startswith(prefix + ':')
        return match

    def match_product_string_re(self, pattern):
        import re
        def match(device: usb.core.Device):
            if not device.iProduct:
                return False
            lang_id = self.get_default_language_id(device)
            string = usb.util.get_string(device, device.iProduct, lang_id)
            m = re.search(pattern, string)
            return bool(m)
        return match

    def match_interface_string_re(self, pattern):
        import re
        def match(intf: usb.core.Interface):
            if not intf.iInterface:
                return False
            lang_id = self.get_default_language_id(intf.device)
            string = usb.util.get_string(intf.device, intf.iInterface, lang_id)
            m = re.search(pattern, string)
            return bool(m)
        return match

    def __init__(self, vid=0x16c0, pid=0x27dd, index=0):
        devices = tuple(usb.core.find(idVendor=vid, idProduct=pid, find_all=True))
        if not tuple(devices):
            print('No FreeJTAG devices found', file=stderr)
            exit(1)
        devices = filter(self.match_serial_string_prefix('jkent.net'), devices)
        devices = filter(self.match_product_string_re(r'\bFreeJTAG\b'), devices)
        devices = tuple(devices)
        if not devices:
            print('No FreeJTAG devices found', file=stderr)
            exit(1)

        try:
            self.dev: usb.core.Device = devices[index]
        except IndexError:
            print('Invalid FreeJTAG device index', file=stderr)
            exit(1)

        self.config: usb.core.Configuration = self.dev.get_active_configuration()

    @property
    def cdc(self):
        if hasattr(self, '_cdc'):
            return self._cdc
        self._cdc = CDC(self)
        return self._cdc

    @property
    def tap(self):
        if hasattr(self, '_tap'):
            return self._tap
        self._tap = Tap(self)
        return self._tap

def read_signature(jtag: Tap):
    jtag.shift_ir(4, AVR_IR_PROG_COMMANDS)
    def read_byte(addr):
        jtag.shift_dr(15, 0b0100011_00001000)
        jtag.shift_dr(15, 0b0000011_00000000 | addr)
        jtag.shift_dr(15, 0b0110010_00000000)
        return jtag.shift_dr(15, 0b0110011_00000000, read=True) & 0xFF

    signature = 0
    for addr in range(3):
        signature |= read_byte(addr) << (addr * 8)
    return signature

if __name__ == '__main__':
    freejtag = FreeJTAG()

    with freejtag.tap as tap:
        print(tap.version())

        tap.shift_ir(4, AVR_IR_IDCODE)
        idcode = tap.shift_dr(32, read=True)
        print(f'IDCODE = 0x{idcode:08X}')

        tap.shift_ir(4, AVR_IR_RESET)
        tap.shift_dr(1, 1)

        tap.shift_ir(4, AVR_IR_PROG_ENABLE)
        tap.shift_dr(16, 0xa370)

        sig = read_signature(tap).to_bytes(3, 'little')
        print('Signature: ' + ' '.join(f'{byte:02X}' for byte in sig))

        tap.shift_ir(4, AVR_IR_PROG_ENABLE)
        tap.shift_dr(16, 0)

        tap.shift_ir(4, AVR_IR_RESET)
        tap.shift_dr(1, 0)

    with freejtag.cdc as cdc:
        cdc.write('test')
        print(cdc.read(4))

    with freejtag.tap as tap:
        while True:
            ch = tap.avr_read_ocdr()
            if ch is not None:
                stdout.buffer.write(ch)
                stdout.buffer.flush()
