#!/usr/bin/env python3
"""Verify the structure and embedded metadata of a Windows VST3 bundle.

Checks (stdlib only, runs on any OS):
  - bundle layout: Contents/x86_64-win/<name>.dll
  - the payload is a PE32+ (x86-64) image
  - the payload exports the VST3 module entry points
  - expected metadata strings (plugin name, manufacturer) are embedded

This is a fast structural smoke check for CI. Full VST3 conformance is
covered separately by Steinberg's official `validator` (see
docs/VALIDATION.md). A green run here does NOT mean the plugin is
FL Studio-tested.

Usage: python3 tools/verify_vst3_bundle.py <path-to-GNARL.vst3>
       [--name GNARL] [--manufacturer zwiebo]
"""

import argparse
import os
import struct
import sys


def fail(msg):
    print(f"FAIL: {msg}")
    sys.exit(1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("bundle")
    ap.add_argument("--name", default="GNARL")
    ap.add_argument("--manufacturer", default="zwiebo")
    args = ap.parse_args()

    bundle = args.bundle
    if not os.path.isdir(bundle):
        fail(f"bundle directory not found: {bundle}")

    contents = os.path.join(bundle, "Contents")
    arch = os.path.join(contents, "x86_64-win")
    if not os.path.isdir(arch):
        fail(f"missing Contents/x86_64-win/ in {bundle}")

    dlls = [f for f in os.listdir(arch) if f.lower().endswith(".dll")]
    if len(dlls) != 1:
        fail(f"expected exactly one .dll in Contents/x86_64-win/, found {dlls}")
    dll_path = os.path.join(arch, dlls[0])
    print(f"payload: {dll_path} ({os.path.getsize(dll_path)} bytes)")

    with open(dll_path, "rb") as fh:
        data = fh.read()

    # PE32+ / x86-64 checks.
    if data[:2] != b"MZ":
        fail("payload is not a PE image (missing MZ signature)")
    pe_off = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_off:pe_off + 4] != b"PE\x00\x00":
        fail("missing PE signature")
    machine = struct.unpack_from("<H", data, pe_off + 4)[0]
    if machine != 0x8664:
        fail(f"payload is not x86-64 (machine=0x{machine:04x})")
    magic = struct.unpack_from("<H", data, pe_off + 24)[0]
    if magic != 0x20B:
        fail(f"payload is not PE32+ (optional header magic=0x{magic:04x})")
    print("PE32+ x86-64 image: OK")

    # VST3 module entry points are exported by name; export names are stored
    # as plain ASCII in the export directory, so a byte search is sufficient.
    for export in (b"GetPluginFactory", b"InitModule", b"ExitModule"):
        if export not in data:
            fail(f"missing VST3 export {export.decode()} in payload")
    print("VST3 exports (GetPluginFactory/InitModule/ExitModule): OK")

    # Embedded class-info metadata strings (UTF-8 in the binary).
    for needle, label in ((args.name.encode(), "plugin name"),
                          (args.manufacturer.encode(), "manufacturer")):
        if needle not in data:
            fail(f"{label} string {needle.decode()!r} not embedded in payload")
    print(f"embedded metadata strings ({args.name}, {args.manufacturer}): OK")

    print("VST3 BUNDLE STRUCTURE OK")


if __name__ == "__main__":
    main()
