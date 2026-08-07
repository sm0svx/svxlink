#!/usr/bin/env python3
"""Smoke test: every TCL event script loads without error.

Builds the installed events.tcl + events.d/ layout (as symlinks to the
repository source) and sources it under plain `tclsh` with the C++-provided
commands stubbed out, for each logic type. A TCL syntax error or undefined
proc in any sourced script (globals.tcl, Logic.tcl, <Type>LogicType.tcl, ...)
makes tclsh exit non-zero and fails the test.

This catches breakage in the event scripts without launching svxlink.

Run:  python3 tests/test_tcl_loads.py     (exit 0 = pass)
"""

import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
TCL_SRC = os.path.join(REPO, "src", "svxlink", "svxlink")

# Stub the commands that svxlink's C++ side registers in the interpreter, so
# the scripts can be sourced under a plain tclsh.
STUBS = r"""
foreach _c {playFile playSilence playTone playDtmf recordStart recordStop \
            deactivateModule publishStateEvent injectDtmf setConfigValue \
            setAnnounceOnAllLogics} {
  proc $_c {args} {}
}
proc getConfigValue {section tag default} { return $default }
"""

LOGIC_TYPES = [("SimplexLogic", "Simplex"), ("RepeaterLogic", "Repeater")]

failures = 0


def check(cond, msg):
    global failures
    print(("  ok   " if cond else "  FAIL ") + msg)
    if not cond:
        failures += 1


def _build_tree(dest):
    share = os.path.join(dest, "share")
    events_d = os.path.join(share, "events.d")
    os.makedirs(events_d)
    os.symlink(os.path.join(TCL_SRC, "events.tcl"),
               os.path.join(share, "events.tcl"))
    for fn in os.listdir(TCL_SRC):
        if fn.endswith(".tcl") and fn != "events.tcl":
            os.symlink(os.path.join(TCL_SRC, fn), os.path.join(events_d, fn))
    os.makedirs(os.path.join(share, "sounds", "en_US"))
    return os.path.join(share, "events.tcl")


def load_logic(logic_name, logic_type):
    tmp = tempfile.mkdtemp(prefix="tcltest_")
    try:
        events_tcl = _build_tree(tmp)
        wrapper = os.path.join(tmp, "run.tcl")
        with open(wrapper, "w") as f:
            f.write(STUBS)
            f.write(f'set logic_name "{logic_name}"\n')
            f.write(f'set logic_type "{logic_type}"\n')
            # Variables svxlink's C++ side normally sets in the interpreter.
            f.write('set loaded_modules ""\n')
            f.write('set active_module ""\n')
            f.write('set mycall "TEST"\n')
            f.write('set report_ctcss 0.0\n')
            f.write(f'source "{events_tcl}"\n')
            f.write('exit 0\n')
        return subprocess.run(["tclsh", wrapper], capture_output=True,
                              text=True, timeout=30)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    for logic_name, logic_type in LOGIC_TYPES:
        r = load_logic(logic_name, logic_type)
        ok = r.returncode == 0
        check(ok, f"{logic_type} event scripts load cleanly")
        if not ok:
            print("    --- stderr/stdout ---")
            for line in (r.stderr + r.stdout).splitlines()[-15:]:
                print("    " + line)
    print()
    print("PASS" if failures == 0 else f"{failures} check(s) FAILED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
