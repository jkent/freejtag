#!/usr/bin/env python3
#!/usr/bin/env python3
# SPDX-License-Identifier: MIT */
#
# FreeJTAG
# Copyright (C) 2026 Jeff Kent <jeff@jkent.net>

from random import randbytes
from time import time

from serial import Serial

if __name__ == '__main__':
    with Serial('/dev/ttyACM0') as serial:
        iterations = 64
        n_bytes = 256

        print(f'{iterations} iterations of {n_bytes} bytes')

        start = time()
        for i in range(iterations):
            data_out = randbytes(n_bytes)
            serial.write(data_out)
            data_in = serial.read(n_bytes)
            if data_in != data_out:
                print(f'{len(data_out): 4} << {data_out}')
                print(f'{len(data_in): 4} >> {data_in}')
                print('match error')
            assert(data_in == data_out)
        end = time()

        time = end - start
        print(f'{round(time, 3)}s')
        kbs = round((i / 1024) / time * n_bytes, 3)
        print(f'{kbs} KiB/s')
        baud = round(kbs * 10240)
        print(f'{baud} baud')
