# vim: set ft=dockerfile:

# This image is intended for building Turi Create for maximum compatibility
# with various Linux distributions.
# Based on CentOS 6 for compatibility with older glibc versions.
# Builds with LLVM 8.0.0 for modern compiler features and bug fixes.
# Builds against libstdc++ from GCC 4.8 for compatibility with older libstdc++
# runtime versions.

FROM quay.io/pypa/manylinux2010_x86_64@sha256:191e8bc35089ce4ceda48224289b7475322ee6853faa4bf7d4b8e0d9c3b0358e

# Set env variables for tools to pick up
ENV CC="gcc"
ENV CXX="g++"
ENV PATH="/opt/rh/devtoolset-8/root/usr/bin:/usr/local/bin:${PATH}"
ENV CCACHE_DIR=/build/.docker_ccache
ENV CCACHE_COMPILERCHECK=content
RUN ldconfig

# Install dependencies
RUN yum -y update && \
    yum -y install \
               vim-common \
               libX11-devel \
               sqlite-devel \
               gcc \
               gcc-multilib \
               glibc-devel \
               ccache \
               tk-devel \
               patch \
               rsync \
               git \
               file \
               xz

RUN yum install -y wget tar make flex gcc gcc-c++ gcc-devel.s390 binutils-devel bzip2
ENV PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
RUN mkdir /src
WORKDIR /src
RUN wget ftp://gcc.gnu.org/pub/gcc/releases/gcc-4.8.5/gcc-4.8.5.tar.gz
RUN tar xzf gcc-4.8.5.tar.gz
WORKDIR gcc-4.8.5
RUN ./contrib/download_prerequisites
WORKDIR ..
RUN mkdir build
WORKDIR build
RUN ../gcc-4.8.5/configure --enable-languages=c,c++ --disable-checking --disable-multilib --disable-bootstrap
RUN make -j16
RUN make install

# Install OpenSSL
# The built-in SSL is so old it can't talk to anything on the internet anymore
# (so even get-pip.py doesn't work!)
WORKDIR /src
RUN curl -O https://www.openssl.org/source/openssl-1.1.0j.tar.gz
RUN tar xf openssl-1.1.0j.tar.gz
RUN mkdir -p /etc/ssl/certs/
WORKDIR /src/openssl-1.1.0j
RUN ./config --prefix=/usr/local --openssldir=/usr/local && \
    make -j4 --quiet && \
    make install && \
    cp libssl.a /usr/local/lib/libssl.a && \
    cp libcrypto.a /usr/local/lib/libcrypto.a && \
    cp libcrypto.so.1.1 /usr/local/lib/libcrypto.so.1.1 && \
    cp libcrypto.so /usr/local/lib/libcrypto.so && \
    cp libssl.so.1.1 /usr/local/lib/libssl.so.1.1 && \
    cp libssl.so /usr/local/lib/libssl.so && \
    ldconfig && \
    rm -rf /src/openssl*

# Install cmake from binary release
RUN mkdir -p /opt
WORKDIR /opt
RUN curl -O https://cmake.org/files/v3.13/cmake-3.13.4-Linux-x86_64.tar.gz
RUN tar xf cmake-3.13.4-Linux-x86_64.tar.gz && \
    rm -rf /opt/cmake-3.13.4-Linux-x86_64.tar.gz
ENV PATH="/opt/cmake-3.13.4-Linux-x86_64/bin:${PATH}"

# Install libffi from source
RUN mkdir -p /src
WORKDIR /src
RUN curl -O ftp://sourceware.org/pub/libffi/libffi-3.2.1.tar.gz
RUN tar xf libffi-3.2.1.tar.gz
WORKDIR /src/libffi-3.2.1
RUN ./configure --prefix=/usr/local && \
    make -j4 --quiet && \
    make install && \
    ldconfig && \
    rm -rf /src/libffi-3.2.1*

# Install Python 2.7 from source
WORKDIR /src
RUN curl -O https://www.python.org/ftp/python/2.7.15/Python-2.7.15.tgz
RUN tar xf Python-2.7.15.tgz
WORKDIR /src/Python-2.7.15
RUN ./configure --prefix=/usr/local --enable-unicode=ucs4 --enable-shared --enable-loadable-sqlite-extensions && \
    make -j4 --quiet && \
    make install && \
    ldconfig && \
    rm -rf /src/Python-2.7.15*

# Install Python 3.5 from source
WORKDIR /src
RUN curl -O https://www.python.org/ftp/python/3.5.6/Python-3.5.6.tgz
RUN tar xf Python-3.5.6.tgz
WORKDIR /src/Python-3.5.6
RUN ./configure --prefix=/usr/local --enable-unicode=ucs4 --enable-shared --enable-loadable-sqlite-extensions && \
    make -j4 --quiet && \
    make install && \
    ldconfig && \
    rm -rf /src/Python-3.5.6*

# Install Python 3.6 from source
WORKDIR /src
RUN curl -O https://www.python.org/ftp/python/3.6.8/Python-3.6.8.tgz
RUN tar xf Python-3.6.8.tgz
WORKDIR /src/Python-3.6.8
RUN ./configure --prefix=/usr/local --enable-unicode=ucs4 --enable-shared --enable-loadable-sqlite-extensions && \
    make -j4 --quiet && \
    make install && \
    ldconfig && \
    rm -rf /src/Python-3.6.8*

# Install Python 3.7 from source
WORKDIR /src
RUN curl -O https://www.python.org/ftp/python/3.7.3/Python-3.7.3.tgz
RUN tar xf Python-3.7.3.tgz
WORKDIR /src/Python-3.7.3
RUN ./configure --prefix=/usr/local --enable-unicode=ucs4 --enable-shared --enable-loadable-sqlite-extensions && \
    make -j4 --quiet && \
    make install && \
    ldconfig && \
    rm -rf /src/Python-3.7.3*

ENV CC="/usr/local/bin/gcc"
ENV CXX="/usr/local/bin/g++"

# Install pip and virtualenv
WORKDIR /src
RUN curl -O https://bootstrap.pypa.io/get-pip.py
RUN python2.7 get-pip.py
RUN pip2.7 install virtualenv
RUN python3.5 get-pip.py
RUN pip3.5 install virtualenv
RUN python3.6 get-pip.py
RUN pip3.6 install virtualenv
RUN python3.7 get-pip.py
RUN pip3.7 install virtualenv
RUN rm -rf /src/get-pip.py

ENV LD="/usr/local/lib64"

# So this is where the real magic happens. Clang has a flag to specify the 
# version of libstdc++ `--gcc-toolchain`. In turicreate we don't necessarily
# have a flag to take care of this so we can re-create the expected directory
# structure using the script below. This is effectively creating the structure
# you'd expect when calling yum install gcc-4.*
# 
# Using `clang -v` you can see the version of gcc that clang picks up
# automatically from `/usr`. If all goes well, this should be the `libstdc++`
# from gcc-4.8.5 and turicreate can build.
RUN mkdir -p /usr/libexec/gcc/x86_64-redhat-linux/4.8.5
RUN mkdir -p /usr/include/c++/4.8.5
RUN mkdir -p /usr/lib/gcc/x86_64-redhat-linux/4.8.5

RUN cp -a /usr/local/libexec/gcc/x86_64-unknown-linux-gnu/4.8.5/* /usr/libexec/gcc/x86_64-redhat-linux/4.8.5/
RUN cp -a /usr/local/include/c++/4.8.5/* /usr/include/c++/4.8.5/
RUN cp -a /usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.8.5/* /usr/lib/gcc/x86_64-redhat-linux/4.8.5/
RUN cp -a /usr/local/lib64/* /usr/lib/gcc/x86_64-redhat-linux/4.8.5/

# Install llvm 8.0.0 from source with clang (no libc++)
WORKDIR /src
RUN curl -O https://releases.llvm.org/8.0.0/llvm-8.0.0.src.tar.xz
RUN curl -O https://releases.llvm.org/8.0.0/cfe-8.0.0.src.tar.xz
RUN tar xf llvm-8.0.0.src.tar.xz && \
    tar xf cfe-8.0.0.src.tar.xz && \
    mv cfe-8.0.0.src llvm-8.0.0.src/tools/clang && \
    mkdir llvm-8.0.0.build
WORKDIR /src/llvm-8.0.0.build
RUN cmake ../llvm-8.0.0.src/ -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX -DLLVM_TEMPORARILY_ALLOW_OLD_TOOLCHAIN=ON && \
    make -j24 || make -j4 && \
    make --quiet install && \
    ldconfig && \
    rm -rf /src/llvm-8.0.0*

# Make clang the default compiler
# To work around https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60367
# (By not using gcc 4.8)
ENV CC="clang"
ENV CXX="clang++"

# Start at repo root (mounted into Docker)
WORKDIR /build

# Set ccache size to 4GB
RUN mkdir -p $CCACHE_DIR
RUN ccache -M 4G

# Clean up now-unnecessary paths in image
RUN rm -rf /src

WORKDIR /build/
