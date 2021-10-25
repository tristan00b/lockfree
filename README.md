# Lock-Free Queue

I wrote this as a first study in lock-free programming. Feel free to use it, if it suits
you, but I'm not sure how it stacks up against professional implementations. Standard
disclaimers apply!

Any feedback would be welcome, and greatly appreciated.

## Sources that helped me

- Timur Doumler: C++ in the Audio Industry talks (CppCon, Juce)
- Anthony Williams: C++ Concurrency in Action (Manning)

## Compilation

### Lock-Free Queue Tests

To compile the queue requires CMake to be installed. I have used LLVM for compilation and
tested on a Mac with Mojave v10.14.6 installed.

- From your shell, cd into the project root directory
- Execute `mkdir build`, `cd build`, and `cmake ..` to prepare for building
- Then execute `cmake --build .` to build the tests executable
- You can then run the `tests` executable that was output to `build/tests`

### Documentation

If you wish to compile the documentation you will need doxygen installed.

- From your shell, cd into the project root directory
- Execute `mkdir docs`, `cd docs`, and then `doxygen Doxyfile` to build the documentation
- The documentation is output to `docs/html` and for me it works simply to open the
  `index.html` file therein, using the latest version of Firefox Developer Edition. No
  need to spin up a server.

## Authors

- Tristan Bayfield

## Licenses

### Lock-Free Queue

The lock-free queue implementation is available for use under the BSD-3 license. Please
see the included LICENSE file for the complete license.

### Catch 2 Testing Framework

This repository is bundled with the Catch 2 testing framework v2.11.3, whose copyright is
held by Two Blue Cubes Ltd. and is distributed under the Boost Software License,
Version 1.0. Please see the included LICENSE.Catch2 file for the complete license.
