#!/usr/bin/env python3
"""Verify that newlib global constructors cannot run before XR806 XIP setup.

This verifies linked ARM instructions, not merely successful compilation.
The _start/newlib call must be redirected to the no-op ESPHome wrapper;
the real constructor dispatcher must run after platform_init and before
ESPHome, unless the SDK's own cpp_init handles that path.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

FUNCTION = re.compile(r"(?m)^[0-9a-fA-F]+\s+<([^>]+)>:\s*$")
REFERENCE = re.compile(r"<([^>]+)>")


def functions_in(disassembly: str) -> dict[str, str]:
    matches = list(FUNCTION.finditer(disassembly))
    return {
        match.group(1): disassembly[
            match.end() : (matches[i + 1].start() if i + 1 < len(matches) else len(disassembly))
        ]
        for i, match in enumerate(matches)
    }


def references(body: str) -> list[str]:
    return [ref.split("+", 1)[0] for ref in REFERENCE.findall(body)]


def cplusplus_is_enabled(elf: Path) -> bool:
    header = elf.parent / "sdk-work/include/generated/autoconf.h"
    if not header.is_file():
        raise AssertionError(f"{elf}: SDK autoconf.h missing: {header}")
    return re.search(r"(?m)^#define\s+CONFIG_CPLUSPLUS\s+1\b", header.read_text()) is not None


def verify(elf: Path) -> None:
    disasm = subprocess.check_output(["arm-none-eabi-objdump", "-d", str(elf)], text=True)
    functions = functions_in(disasm)
    # ARM newlib aliases _start to _mainCRTStartup. objdump may choose
    # the latter as its single disassembly label for the shared address.
    start_symbol = "_start" if "_start" in functions else "_mainCRTStartup"
    for symbol in (start_symbol, "main", "__wrap___libc_init_array"):
        if symbol not in functions:
            raise AssertionError(f"{elf}: missing linked function {symbol}")

    # None of these entry-path functions may itself execute from XIP:
    # they run before HAL_Xip_Init() has mapped flash at 0x00400000.
    for symbol in (start_symbol, "main", "platform_init", "__wrap___libc_init_array"):
        match = re.search(rf"(?m)^([0-9a-fA-F]+) <{re.escape(symbol)}>:", disasm)
        if match is None:
            raise AssertionError(f"{elf}: no linked address for early function {symbol}")
        if int(match.group(1), 16) >= 0x00400000:
            raise AssertionError(f"{elf}: early function {symbol} is placed in XIP flash")

    startup = references(functions[start_symbol])
    if "__wrap___libc_init_array" not in startup:
        raise AssertionError(f"{elf}: CRT _start does not call the constructor wrapper: {startup}")
    if "__libc_init_array" in startup:
        raise AssertionError(f"{elf}: CRT _start still calls constructors before XIP setup")

    wrapper = references(functions["__wrap___libc_init_array"])
    if "__libc_init_array" in wrapper:
        raise AssertionError(f"{elf}: ESPHome wrapper is not an early no-op")

    main = references(functions["main"])
    if "platform_init" not in main or "xr806_esphome_run" not in main:
        raise AssertionError(f"{elf}: platform_init / ESPHome entry missing: {main}")
    p = main.index("platform_init")
    run = main.index("xr806_esphome_run")
    if not p < run:
        raise AssertionError(f"{elf}: ESPHome entry precedes platform_init")

    late_ctor = [i for i, symbol in enumerate(main) if symbol == "__libc_init_array"]
    if cplusplus_is_enabled(elf):
        if late_ctor:
            raise AssertionError(f"{elf}: CONFIG_CPLUSPLUS may run constructors twice")
        platform = references(functions.get("platform_init_level2", ""))
        if "cpp_init" not in platform:
            raise AssertionError(f"{elf}: SDK cpp_init() is missing")
    elif len(late_ctor) != 1 or not (p < late_ctor[0] < run):
        raise AssertionError(f"{elf}: expected exactly one post-XIP constructor run: {main}")

    print(f"PASS {elf}: CRT no-op -> platform_init/XIP -> one ctor pass -> ESPHome")


def main() -> None:
    elfs = [Path(p) for p in sys.argv[1:]]
    if not elfs:
        raise SystemExit("Provide one or more app.elf paths")
    for elf in elfs:
        verify(elf)


if __name__ == "__main__":
    main()
