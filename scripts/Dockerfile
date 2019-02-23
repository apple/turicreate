# vim: set ft=dockerfile:

# Based on Ubuntu 10.04 for compatibility with older glibc versions
# Builds using GCC 4.8 for compatibility with older libstdc++ versions

FROM ubuntu:10.04

# Prevent apt-get from asking questions and expecting answers
ENV DEBIAN_FRONTEND noninteractive

# Set env variables for tools to pick up
ENV CC="gcc-4.8"
ENV CXX="g++-4.8"
ENV PATH="/usr/local/bin:${PATH}"
ENV CCACHE_DIR=/build/.docker_ccache

# Fix path to new location for older Ubuntu releases
RUN sed -i -e 's/archive\./old-releases\./g' /etc/apt/sources.list

# Update package database
RUN apt-get update

# Install add-apt-repository and set up PPA for GCC 4.8
RUN apt-get -y install python-software-properties
RUN add-apt-repository ppa:ubuntu-toolchain-r/test

# Upgrade all possible packages
RUN apt-get update
RUN apt-get -y upgrade

# Install dependencies
RUN apt-get -y install vim-common gcc-4.8 g++-4.8 libgif-dev libx11-dev ccache make zlib1g-dev libsqlite3-dev tk-dev libgdbm-dev libc6-dev libbz2-dev perl-modules patch rsync

# Install cmake from binary release
ADD https://cmake.org/files/v3.13/cmake-3.13.4-Linux-x86_64.tar.gz /opt/cmake-3.13.4-Linux-x86_64.tar.gz
WORKDIR /opt
RUN tar xvf cmake-3.13.4-Linux-x86_64.tar.gz
ENV PATH="/opt/cmake-3.13.4-Linux-x86_64/bin:${PATH}"

# Install OpenSSL from source
# The built-in SSL is so old it can't talk to anything on the internet anymore
# (so even get-pip.py doesn't work!)
ADD https://www.openssl.org/source/openssl-1.1.0j.tar.gz /src/openssl-1.1.0j.tar.gz
WORKDIR /src
RUN tar xvf openssl-1.1.0j.tar.gz
WORKDIR /src/openssl-1.1.0j
RUN ./config --prefix=/usr/local
RUN make -j4
RUN make install
RUN ldconfig

# Install Python 2.7 from source
ADD http://www.python.org/ftp/python/2.7.15/Python-2.7.15.tgz /src/Python-2.7.15.tgz
WORKDIR /src
RUN tar xvf Python-2.7.15.tgz
WORKDIR /src/Python-2.7.15
RUN ./configure --prefix=/usr/local --enable-unicode=ucs4 --enable-shared --enable-loadable-sqlite-extensions
RUN make -j4
RUN make install
RUN ldconfig

# Install pip and virtualenv
ADD https://bootstrap.pypa.io/get-pip.py /src/get-pip.py
WORKDIR /src
RUN python get-pip.py
RUN pip install virtualenv

# Start at repo root (mounted into Docker)
WORKDIR /build

# Set ccache size to 4GB
CMD mkdir -p $CCACHE_DIR
CMD ccache -s
CMD ccache -M 4G
