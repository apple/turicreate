This folder is for source drops of external dependencies that have their own
(non-CMake) build systems.  These are difficult to integrate into CMake, so
they are built separately.

For source drops of external dependencies that can be cleanly integrated with
our CMake build system, whether by trivially constructing a `CMakeLists.txt` or
using one that's provided, look in
[src/external](../../src/external/README.md).
