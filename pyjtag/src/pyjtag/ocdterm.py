#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
#
# Copyright (C) 2026 Jeff Kent <jeff@jkent.net>

from sys import stderr

import click
from .jtag import JTAG
from .util import HexParamType


@click.command
@click.option('--backend', default='freejtag')
@click.option('--vid', type=HexParamType('vid'))
@click.option('--pid', type=HexParamType('pid'))
@click.option('--index', type=click.INT)
def main(backend, vid, pid, index):
    with JTAG(backend) as jtag:
        jtag.avr_reset(True)
        jtag.avr_reset(False)

        try:
            while True:
                ch = jtag.avr_read_ocdr()
                if ch is not None:
                    stderr.buffer.write(ch)
                    stderr.buffer.flush()
        except KeyboardInterrupt:
            print()

if __name__ == '__main__':
    main()
