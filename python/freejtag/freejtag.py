import usb.core
import usb.util


class FreeJTAG:
    CMD_UNLOCK      = 0x00
    CMD_VERSION     = 0x01
    CMD_ATTACH      = 0x02
    CMD_STATE       = 0x03
    CMD_MEMSET      = 0x04
    CMD_BUFWRITE    = 0x05
    CMD_BUFREAD     = 0x06
    CMD_SELECT      = 0x07
    CMD_SHIFT       = 0x08
    CMD_BULKWRITE8  = 0x09
    CMD_BULKREAD8   = 0x10

    JTAG_RESET       = 0x00
    JTAG_RUNIDLE     = 0x01
    JTAG_DRSELECT    = 0x02
    JTAG_DRCAPTURE   = 0x03
    JTAG_DRSHIFT     = 0x04
    JTAG_DREXIT1     = 0x05
    JTAG_DRPAUSE     = 0x06
    JTAG_DREXIT2     = 0x07
    JTAG_DRUPDATE    = 0x08
    JTAG_IRSELECT    = 0x09
    JTAG_IRCAPTURE   = 0x0A
    JTAG_IRSHIFT     = 0x0B
    JTAG_IREXIT1     = 0x0C
    JTAG_IRPAUSE     = 0x0D
    JTAG_IREXIT2     = 0x0E
    JTAG_IRUPDATE    = 0x0F
    JTAG_DEFAULT     = 0x10

    def __init__(self, vid=0x03EB, pid=0x204B):
        self.dev = usb.core.find(idVendor=vid, idProduct=pid)
        if self.dev is None:
            raise Exception('Device is not found')

        try:
            langids = usb.util.get_langids(self.dev)
            self.langid = langids[0]
        except usb.core.USBError as e:
            raise Exception('Error accessing device, check permissions')

        self.irlens = []
        self.selected = None

    def __enter__(self):
        self._unlock()
        self.attach()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._unlock(False)

    def _unlock(self, unlock=True):
        key = b'FreeJTAG' if unlock else b'LOCKLOCK'
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_OUT,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE)
        self.dev.ctrl_transfer(bmRequestType, self.CMD_UNLOCK, 0, 0, key)

    def version(self):
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_IN,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE)
        data = self.dev.ctrl_transfer(bmRequestType, self.CMD_VERSION, 0, 0, 2)
        value = int.from_bytes(data, 'little')
        major = (value & 0xFF00) >> 8
        minor = (value & 0xF0) >> 4
        patch = (value & 0xF)
        return major, minor, patch

    def attach(self, attach=True):
        wValue = int(attach)
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_OUT,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE)
        self.dev.ctrl_transfer(bmRequestType, self.CMD_ATTACH, wValue)

    def state(self, state, cycles=0):
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_OUT,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE)
        self.dev.ctrl_transfer(bmRequestType, self.CMD_STATE, state, cycles)

    def memset(self, value=0):
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_OUT,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE)
        self.dev.ctrl_transfer(bmRequestType, self.CMD_MEMSET, value)

    def bufwrite(self, data):
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_OUT,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE)
        self.dev.ctrl_transfer(bmRequestType, self.CMD_BUFWRITE, 0, 0, data)

    def bufread(self, length):
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_IN,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE)
        data = self.dev.ctrl_transfer(bmRequestType, self.CMD_BUFREAD, 0, 0,
                                      length)
        return bytes(data)

    def set_irlens(self, *args):
        self.irlens = list(args)

    def select_tap(self, index):
        self.selected = index
        heads = self.irlens[:index - 1]
        tails = self.irlens[index + 1:]
        data = len(heads).to_bytes(1) + len(tails).to_bytes(1) + \
            sum(heads).to_bytes(2, 'little') + sum(tails).to_bytes(2, 'little')
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_OUT,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE)
        self.dev.ctrl_transfer(bmRequestType, self.CMD_SELECT, 0, 0, data)

    def shift(self, data_bits, ir=False, end_state=JTAG_DEFAULT):
        bmRequestType = usb.util.build_request_type(
            usb.util.CTRL_OUT,
            usb.util.CTRL_TYPE_VENDOR,
            usb.util.CTRL_RECIPIENT_DEVICE)
        wIndex = (0x0100 if ir else 0) | end_state
        self.dev.ctrl_transfer(bmRequestType, self.CMD_SHIFT, data_bits, wIndex)

    def shift_ir(self, value=None, read=False, end_state=JTAG_DEFAULT):
        data_bits = self.irlens[self.selected]
        num_bytes = (data_bits + 7) // 8
        if value is not None:
            data = value.to_bytes(num_bytes, 'little')
            self.bufwrite(data)
        self.shift(data_bits, True, end_state)
        if read:
            data = self.bufread(num_bytes)
            return int.from_bytes(data, 'little') #& ((1 << data_bits) - 1)

    def shift_dr(self, data_bits, value=None, read=False, end_state=JTAG_DEFAULT):
        num_bytes = (data_bits + 7) // 8
        if value is not None:
            data = value.to_bytes(num_bytes, 'little')
            self.bufwrite(data)
        self.shift(data_bits, False, end_state)
        if read:
            data = self.bufread(num_bytes)
            return int.from_bytes(data, 'little') #& ((1 << data_bits) - 1)
