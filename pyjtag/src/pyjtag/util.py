# SPDX-License-Identifier: MIT
#
# Copyright (C) 2026 Jeff Kent <jeff@jkent.net>

import click


class HexParamType(click.ParamType):
    def __init__(self, name, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.name = name

    def convert(self, value, param, ctx):
        if isinstance(value, int):
            return value
        try:
            if value[:2].lower() == "0x":
                return int(value[2:], 16)
            return int(value, 16)
        except ValueError:
            self.fail(f"{value!r} is not a valid {self.name}", param, ctx)
