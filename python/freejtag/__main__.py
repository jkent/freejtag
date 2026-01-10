from freejtag import FreeJTAG


IDCODE = 0x1
PROG_ENABLE = 0x4
PROG_COMMANDS = 0x5
PROG_PAGELOAD = 0x6
PROG_PAGEREAD = 0x7
AVR_RESET = 0xC

def avr_cmd(jtag, cmd, read=False):
    jtag.shift_ir(PROG_COMMANDS)
    return jtag.shift_dr(15, cmd, read)

def read_sig(jtag, addr):
    avr_cmd(jtag, 0b0100011_00001000)
    avr_cmd(jtag, 0b0000011_00000000 | (addr & 0xff))
    avr_cmd(jtag, 0b0110000_00000000)
    return avr_cmd(jtag, 0b0110011_00000000, read=True) & 0xff

if __name__ == '__main__':
    with FreeJTAG() as jtag:
        jtag.set_irlens(4)
        jtag.select_tap(0)

        jtag.shift_ir(IDCODE)
        value = jtag.shift_dr(32, read=True)
        print(f'IDCODE = 0x{value:08X}')

        jtag.shift_ir(AVR_RESET)
        jtag.shift_dr(1, 1)

        jtag.shift_ir(PROG_ENABLE)
        jtag.shift_dr(16, 0xA370)

        sig = read_sig(jtag, 0)
        sig |= read_sig(jtag, 1) << 8
        sig |= read_sig(jtag, 2) << 16
        print(f'0x{sig:06X}')

        jtag.shift_ir(PROG_ENABLE)
        jtag.shift_dr(16, 0x0000)

        jtag.shift_ir(AVR_RESET)
        jtag.shift_dr(1, 0)