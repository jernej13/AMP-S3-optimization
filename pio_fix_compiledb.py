# SPDX-License-Identifier: CC0-1.0

import json
import os
import shlex
import sys

Import("env")


def pre_compiledb_actions(toolchain):
    """
    Restrict compilation database to a single toolchain.
    Prevents mixing include paths from multiple ESP toolchains.
    """

    print("Preparing compile_commands for clangd")

    env.Replace(COMPILATIONDB_INCLUDE_TOOLCHAIN=False)

    toolchain_paths = [
        p for p in env.DumpIntegrationIncludes().get("toolchain", [])
        if toolchain in p
    ]

    env.Replace(CPPPATH=toolchain_paths)

    env.Replace(
        COMPILATIONDB_PATH=os.path.join("$BUILD_DIR", "compile_commands.json")
    )


def fix_compiledb_action(build_dir, main_src):

    compile_commands_path = os.path.join(build_dir, "compile_commands.json")

    if not os.path.isfile(compile_commands_path):
        print("compile_commands.json not found", file=sys.stderr)
        return

    with open(compile_commands_path, "r", encoding="utf-8") as f:
        db = json.load(f)

    match = None
    for entry in db:
        if os.path.normpath(entry.get("file")) == os.path.normpath(main_src):
            match = entry
            break

    if not match:
        print("main.cpp entry not found", file=sys.stderr)
        return

    cmd = match.get("command") or " ".join(match.get("arguments", []))
    tokens = shlex.split(cmd)

    includes = [t for t in tokens if t.startswith("-I")]

    changed = False

    for entry in db:
        path = entry.get("file", "").replace("\\", "/")
        if path.startswith("lib/"):
            cmd = entry.get("command", "")
            for inc in includes:
                if inc not in cmd:
                    cmd += " " + inc
                    changed = True
            entry["command"] = cmd

    if changed:
        with open(compile_commands_path, "w", encoding="utf-8") as f:
            json.dump(db, f, indent=2)

        print("compile_commands fixed for clangd")


if "compiledb" in COMMAND_LINE_TARGETS:

    pre_compiledb_actions(
        "xtensa-esp32s3-elf"
    )


env.AddCustomTarget(
    "fix_compiledb",
    None,
    lambda *args, **kwargs: fix_compiledb_action(
        os.path.join(env.get("PROJECT_BUILD_DIR"), env.get("PIOENV")),
        os.path.join("src", "main.cpp"),
    ),
)