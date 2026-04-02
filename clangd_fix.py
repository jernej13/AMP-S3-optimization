import json
from pathlib import Path
import re

# ---------------------------
# Paths
# ---------------------------
ROOT = Path(".").resolve()                  # Project root
CDB_PATH = ROOT / "compile_commands.json"  # Path to compile_commands.json
CLANGD_PATH = ROOT / ".clangd"             # Path to .clangd
FIX_DIR = ROOT / "clangd_fix"              # Directory for fake headers
MACHINE_DIR = FIX_DIR / "machine"          # machine/ subdir for endian.h

if not CDB_PATH.exists():
    exit("No compile_commands.json found. Please run 'pio run -t compile_commands' first.")

# ---------------------------
# Helper to split command string into flags
# ---------------------------
def split_flags(cmd: str):
    # Handles quoted paths correctly
    return re.findall(r'(?:[^\s"]+|"[^"]*")+', cmd)

# ---------------------------
# Load compile_commands.json
# ---------------------------
with open(CDB_PATH) as f:
    cdb = json.load(f)

includes = set()  # Will collect unique include paths
compiler = None   # Will auto-detect compiler from first entry

# ---------------------------
# Collect includes and detect compiler
# ---------------------------
for entry in cdb:
    cmd = entry.get("command", "")
    parts = split_flags(cmd)

    if parts and compiler is None:
        compiler = Path(parts[0]).name

    for p in parts:
        if p.startswith("-I"):
            path = p[2:].strip('"')
            includes.add(f"-isystem{path}")
        elif p.startswith("-isystem"):
            path = p[8:].strip('"')
            includes.add(f"-isystem{path}")

# ---------------------------
# Add PlatformIO toolchain includes automatically
# ---------------------------
PIO_PACKAGES = Path.home() / ".platformio" / "packages"
if PIO_PACKAGES.exists():
    for toolchain in PIO_PACKAGES.glob("toolchain-*"):
        # Standard include folder
        inc = toolchain / "include"
        if inc.exists():
            includes.add(f"-isystem{inc}")

        # All subfolders matching *-elf/include (xtensa/riscv)
        for sub in toolchain.glob("**/*-elf/include"):
            includes.add(f"-isystem{sub}")

# ---------------------------
# Create fake machine/endian.h header
# ---------------------------
MACHINE_DIR.mkdir(parents=True, exist_ok=True)
(MACHINE_DIR / "endian.h").write_text(
r'''#pragma once

#ifndef _MACHINE_ENDIAN_H_
#define _MACHINE_ENDIAN_H_

#ifndef _LITTLE_ENDIAN
#define _LITTLE_ENDIAN 1234
#endif

#ifndef _BIG_ENDIAN
#define _BIG_ENDIAN 4321
#endif

#ifndef _BYTE_ORDER
#define _BYTE_ORDER _LITTLE_ENDIAN
#endif

#endif
'''
)

# ---------------------------
# Prepend fake header include path to all compile commands
# This ensures clangd will find machine/endian.h
# ---------------------------
for entry in cdb:
    entry["command"] = f"-I{FIX_DIR} " + entry["command"]

# Overwrite the root compile_commands.json
with open(CDB_PATH, "w") as f:
    json.dump(cdb, f, indent=2)

# ---------------------------
# Flags clangd should remove (each with "- -" prefix)
# ---------------------------
REMOVE_FLAGS = [
    "-mlongcalls",
    "-fno-shrink-wrap",
    "-fstrict-volatile-bitfields",
    "-fno-tree-switch-conversion",
]

# ---------------------------
# Write .clangd config
# ---------------------------
with open(CLANGD_PATH, "w") as f:
    f.write("CompileFlags:\n")
    if compiler:
        f.write(f"  Compiler: {compiler}\n")
    f.write("  BuiltinHeaders: QueryDriver\n")
    f.write("  Add:\n")
    # Always add the fake headers first
    f.write(f"    -I{FIX_DIR}\n")
    for inc in sorted(includes):
        f.write(f"    {inc}\n")

    f.write("  Remove:\n")
    for r in REMOVE_FLAGS:
        # double dash for clangd syntax
        f.write(f"    - {r}\n")

    # Suppress diagnostics we don't care about
    f.write("\nDiagnostics:\n")
    f.write("  Suppress:\n")
    f.write("    - missing-include\n")
    f.write("    - unused-includes\n")
    f.write("    - unused-include-directive\n")
    f.write("    - unused-header\n")

print(".clangd generated successfully")
print(f"{FIX_DIR}/machine/endian.h created")
print(f"Root compile_commands.json updated to include fake headers")
