# Common Installation Issues

### `I get an error "ImportError /lib64/libstdc++.so.6: version GLIBCXX_3.4.21 not found"`

You need an updated libstdc++.so. See [LINUX\_INSTALL.md](LINUX_INSTALL.md) for details.

### `I get an error "ImportError ..  version GLIBCXX\_3.4.21 not defined in libstdc++.so.6 with link time reference"`

You need an updated libstdc++.so. See [LINUX\_INSTALL.md](LINUX_INSTALL.md) for details.
(Strictly, speaking you have an updated libstdc++.so, but it was not compiled with the cxx11 abi)

### `I get an error "ImportError: libblas.so.3: Cannot open shared object file: No such file or directory"`

You need to install Blas and Lapack. See [LINUX\_INSTALL.md](LINUX_INSTALL.md) for details.

### `I get an error "ImportError: liblapack.so.3: Cannot open shared object file: No such file or directory"`

You need to install Blas and Lapack. See [LINUX\_INSTALL.md](LINUX_INSTALL.md) for details.
