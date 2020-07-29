### Building

To build coremltools from source, you require you need
[CMake](https://cmake.org) and
[Miniconda](https://docs.conda.io/en/latest/miniconda.html) to configure the
project.

Our makefile & scripts require the **zsh** shell (default shell for macOS
10.16+) installed in `/usr/bin`.

The following targets will handle the development environment for you. If you
need to add packages, edit the reqs/pip files and the auto-environment will
install them automatically.


* `build` | Build coremltools in *debug* mode (include symbols).
* `docs` | Build documentation.
* `clean` | Clean build dir.
* `clean_envs` | Delete all envs created by the scripts.
* `lint` | Linter.
* `proto` | Build coremltools and rebuild MLModel protobuf sources.
* `release` | Setup the package for release, but don’t upload to pypi. Include all wheels from build/dist in the built package.
* `style` | Style checking.
* `test` | Run all tests. Pass TEST_PACKAGES="..." to set which packages to test.
* `test_fast` | Run all fast tests.
* `test_slow` | Run all non-fast tests.
* `wheel` | Build wheels in *release* mode.

By default, we use python 3.7 but you can can pass `python=2.7` (or 3.6, 3.8
etc.)  as a argument to change the env / build / wheel python version.

*Using an unmanaged developer environment*

Use `make env` to create an auto-set-up development environment with the
correct package dependencies.  This env will not be changed by scripts after
creation. However, provided scripts & makefiles do not currently support custom
development environments; rather, they will always auto-activate the managed
environment. Environments are generated and stored at
`envs/coremltools-py<version string>`

