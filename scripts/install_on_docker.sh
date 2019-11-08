# vim: set ft=dockerfile:

# This image is intended for building Turi Create for maximum compatibility
# with various Linux distributions.
# Based on Centos 6 for compatibility with older glibc versions.
# Builds with LLVM 8.0.0 for modern compiler features and bug fixes.
# Builds against libstdc++ from GCC 4.8 for compatibility with older libstdc++
# runtime versions.

FROM quay.io/pypa/manylinux2010_x86_64@sha256:191e8bc35089ce4ceda48224289b7475322ee6853faa4bf7d4b8e0d9c3b0358e

# Set env variables for tools to pick up
export CC="gcc"
export CXX="g++"
export PATH="/opt/rh/devtoolset-8/root/usr/bin:/usr/local/bin:${PATH}"
export CCACHE_DIR=/build/.docker_ccache
export CCACHE_COMPILERCHECK=content
ldconfig

# Install dependencies
yum -y update && \
    yum -y install \
               vim-common \
               libX11-devel \
               bzip2-devel \
               sqlite-devel \
               zlib-devel \
               gcc \
               gcc-multilib \
               glibc-devel \
               ccache \
               make \
               wget \
               tk-devel \
               patch \
               rsync \
               git \
               file \
               xz

yum install -y wget tar make flex gcc gcc-c++ gcc-devel.s390 binutils-devel bzip2
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
mkdir src/
cd src/
wget ftp://gcc.gnu.org/pub/gcc/releases/gcc-4.8.5/gcc-4.8.5.tar.gz
tar xzf gcc-4.8.5.tar.gz
cd gcc-4.8.5
./contrib/download_prerequisites
cd ..
mkdir build
cd build
../gcc-4.8.5/configure --enable-languages=c,c++ --disable-checking --disable-multilib --disable-bootstrap # --program-prefix=dt-
make -j16
make install

# Install OpenSSL
# The built-in SSL is so old it can't talk to anything on the internet anymore
# (so even get-pip.py doesn't work!)
cd /src
curl -O https://www.openssl.org/source/openssl-1.1.0j.tar.gz
tar xf openssl-1.1.0j.tar.gz
cd /src/openssl-1.1.0j
./config --prefix=/usr/local --openssldir=/usr/local && \
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
mkdir -p /opt
cd /opt
curl -O https://cmake.org/files/v3.13/cmake-3.13.4-Linux-x86_64.tar.gz
tar xf cmake-3.13.4-Linux-x86_64.tar.gz && \
    rm -rf /opt/cmake-3.13.4-Linux-x86_64.tar.gz
export PATH="/opt/cmake-3.13.4-Linux-x86_64/bin:${PATH}"

# Install libffi from source
mkdir -p /src
cd /src
curl -O ftp://sourceware.org/pub/libffi/libffi-3.2.1.tar.gz
tar xf libffi-3.2.1.tar.gz
cd /src/libffi-3.2.1
./configure --prefix=/usr/local && \
    make -j4 --quiet && \
    make install && \
    ldconfig && \
    rm -rf /src/libffi-3.2.1*

# Install Python 2.7 from source
cd /src
curl -O https://www.python.org/ftp/python/2.7.15/Python-2.7.15.tgz
tar xf Python-2.7.15.tgz
cd /src/Python-2.7.15
./configure --prefix=/usr/local --enable-unicode=ucs4 --enable-shared --enable-loadable-sqlite-extensions && \
    make -j4 --quiet && \
    make install && \
    ldconfig && \
    rm -rf /src/Python-2.7.15*

# Install Python 3.5 from source
# WORKDIR /src
# RUN curl -O https://www.python.org/ftp/python/3.5.6/Python-3.5.6.tgz
# RUN tar xf Python-3.5.6.tgz
# WORKDIR /src/Python-3.5.6
# RUN ./configure --prefix=/usr/local --enable-unicode=ucs4 --enable-shared --enable-loadable-sqlite-extensions && \
#     make -j4 --quiet && \
#     make install && \
#     ldconfig && \
#     rm -rf /src/Python-3.5.6*

# Install Python 3.6 from source
# WORKDIR /src
# RUN curl -O https://www.python.org/ftp/python/3.6.8/Python-3.6.8.tgz
# RUN tar xf Python-3.6.8.tgz
# WORKDIR /src/Python-3.6.8
# RUN ./configure --prefix=/usr/local --enable-unicode=ucs4 --enable-shared --enable-loadable-sqlite-extensions && \
#     make -j4 --quiet && \
#     make install && \
#     ldconfig && \
#     rm -rf /src/Python-3.6.8*

# Install Python 3.7 from source
# WORKDIR /src
# RUN curl -O https://www.python.org/ftp/python/3.7.3/Python-3.7.3.tgz
# RUN tar xf Python-3.7.3.tgz
# WORKDIR /src/Python-3.7.3
# RUN ./configure --prefix=/usr/local --enable-unicode=ucs4 --enable-shared --enable-loadable-sqlite-extensions && \
#     make -j4 --quiet && \
#     make install && \
#     ldconfig && \
#     rm -rf /src/Python-3.7.3*

export CC="/usr/local/bin/gcc"
export CXX="/usr/local/bin/g++"

# Install pip and virtualenv
cd /src
curl -O https://bootstrap.pypa.io/get-pip.py
python2.7 get-pip.py
pip2.7 install virtualenv
# RUN python3.5 get-pip.py
# RUN pip3.5 install virtualenv
# RUN python3.6 get-pip.py
# RUN pip3.6 install virtualenv
# RUN python3.7 get-pip.py
# RUN pip3.7 install virtualenv
# RUN rm -rf /src/get-pip.py

export LD="/usr/local/lib64" # ???

# Start here! 

# Install llvm 8.0.0 from source with clang (no libc++)
cd /src
curl -O https://releases.llvm.org/8.0.0/llvm-8.0.0.src.tar.xz
curl -O https://releases.llvm.org/8.0.0/cfe-8.0.0.src.tar.xz
tar xf llvm-8.0.0.src.tar.xz && \
    tar xf cfe-8.0.0.src.tar.xz && \
    mv cfe-8.0.0.src llvm-8.0.0.src/tools/clang && \
    mkdir llvm-8.0.0.build
cd /src/llvm-8.0.0.build
cmake ../llvm-8.0.0.src/ -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX -DLLVM_TEMPORARILY_ALLOW_OLD_TOOLCHAIN=ON && \
    make -j24 || make -j4 && \
    make --quiet install && \
    ldconfig && \
    rm -rf /src/llvm-8.0.0*

# Make clang the default compiler
# To work around https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60367
# (By not using gcc 4.8)
export CC="clang"
export CXX="clang++"

# Start at repo root (mounted into Docker)
cd /build

# Set ccache size to 4GB
mkdir -p $CCACHE_DIR
ccache -M 4G

cd /usr/libexec/gcc/x86_64-redhat-linux/
mkdir 4.8.5
cd /usr/include/c++/
mkdir 4.8.5
cd /usr/lib/gcc/x86_64-redhat-linux/
mkdir 4.8.5

cp -a /usr/local/libexec/gcc/x86_64-unknown-linux-gnu/4.8.5/* /usr/libexec/gcc/x86_64-redhat-linux/4.8.5/
cp -a /usr/local/include/c++/4.8.5/* /usr/include/c++/4.8.5/
cp -a /usr/local/lib/gcc/x86_64-unknown-linux-gnu/4.8.5/* /usr/lib/gcc/x86_64-redhat-linux/4.8.5/
cp -a /usr/local/lib64/* /usr/lib/gcc/x86_64-redhat-linux/4.8.5/
# Clean up now-unnecessary paths in image
rm -rf /src
