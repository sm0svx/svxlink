# SvxLink tests

Unit/component tests for SvxLink and the bundled Async framework. They are
registered with CTest and require no special hardware, network, or sound card.

## Running

```sh
cmake -S src -B build
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

The TCL smoke test additionally needs `tclsh` and `python3`. It is left out
when either is missing.

## What is covered

Run `ctest --test-dir build -N` to list the tests. The C++ unit tests live
next to the code they test, named `<Name>Test.cpp`. The TCL smoke test
(`tcl_event_scripts_load`) sources the event scripts for the Simplex,
Repeater and Reflector logic types under `tclsh`.

## Adding a test

Drop a new `<Name>Test.cpp` file into a directory that calls
`svxlink_add_tests()` in its `CMakeLists.txt`. No other file needs to be
edited. If the test needs extra sources or libraries, declare them in a
`<Name>Test.deps.cmake` file next to it (see
`src/cmake/Modules/SvxLinkAddTests.cmake`).

## Conventions

Each C++ test is a small standalone program that prints `ok`/`FAIL` lines and
exits 0 on success, 1 on failure — no test framework dependency. Audio-pipe
tests drive samples through a component and measure the captured output (RMS,
or per-tone level via a Goertzel filter) to assert frequency/gain behaviour.
