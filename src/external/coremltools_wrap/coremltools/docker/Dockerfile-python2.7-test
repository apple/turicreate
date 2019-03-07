# vim: set ft=dockerfile:

FROM centos:centos7

# Update yum and install some standard dev tools and dependencies
RUN yum -y update
RUN yum -y install git.x86_64 openssl-devel zlib-devel* bzip2-devel vim-common which lapack-devel blas-devel sqlite-devel
RUN yum -y groupinstall 'development tools'

# Install visualization dependencies
RUN yum -y install libX11-devel

# Create a directory for building deps from source code and set up env vars
WORKDIR /
RUN mkdir src
ENV PATH="/usr/local/bin:${PATH}"
ENV LD_LIBRARY_PATH="/usr/local/lib:/usr/local/lib64:${LD_LIBRARY_PATH}"

# Install gcc dependencies
WORKDIR /src
RUN curl -O https://gmplib.org/download/gmp/gmp-6.1.2.tar.bz2
RUN tar xvf gmp-6.1.2.tar.bz2
WORKDIR /src/gmp-6.1.2
RUN ./configure --prefix=/usr/local
RUN make -j16
RUN make install

WORKDIR /src
RUN curl -O http://www.mpfr.org/mpfr-4.0.1/mpfr-4.0.1.tar.gz
RUN tar xvf mpfr-4.0.1.tar.gz
WORKDIR /src/mpfr-4.0.1
RUN ./configure --prefix=/usr/local
RUN make -j16
RUN make install

WORKDIR /src
RUN curl -O https://ftp.gnu.org/gnu/mpc/mpc-1.1.0.tar.gz
RUN tar xvf mpc-1.1.0.tar.gz
WORKDIR /src/mpc-1.1.0
RUN ./configure --prefix=/usr/local
RUN make -j16
RUN make install

# Install gcc 5.5.0 from source
WORKDIR /src
RUN curl -O https://ftp.gnu.org/gnu/gcc/gcc-5.5.0/gcc-5.5.0.tar.gz
RUN tar xvf gcc-5.5.0.tar.gz
WORKDIR /src/gcc-5.5.0
RUN ./configure --prefix=/usr/local --with-system-zlib --disable-multilib
RUN make -j16
RUN make install

# Install Python 2.7 from source
WORKDIR /src
RUN curl -O https://www.python.org/ftp/python/2.7.13/Python-2.7.13.tgz
RUN tar xvf Python-2.7.13.tgz
WORKDIR /src/Python-2.7.13
RUN ./configure --prefix=/usr/local --enable-unicode=ucs4 --enable-shared --enable-loadable-sqlite-extensions
RUN make -j16
RUN make install

# Install cmake from binary release
WORKDIR /opt
RUN curl -O https://cmake.org/files/v3.10/cmake-3.10.2-Linux-x86_64.tar.gz
RUN tar xvf cmake-3.10.2-Linux-x86_64.tar.gz
ENV PATH="/opt/cmake-3.10.2-Linux-x86_64/bin:${PATH}"

# Set compiler binary paths for CMake to pick up
ENV CC="/usr/local/bin/gcc"
ENV CXX="/usr/local/bin/g++"

# Install pip and virtualenv
WORKDIR /src
RUN curl -O https://bootstrap.pypa.io/get-pip.py
RUN python get-pip.py
RUN pip install virtualenv

# Install node.js from binary
WORKDIR /opt
RUN curl -O https://nodejs.org/dist/v8.9.4/node-v8.9.4-linux-x64.tar.xz
RUN tar xvf node-v8.9.4-linux-x64.tar.xz
ENV PATH="/opt/node-v8.9.4-linux-x64/bin:${PATH}"

# Start at home directory
WORKDIR /root
