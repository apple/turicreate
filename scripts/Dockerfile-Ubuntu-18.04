# vim: set ft=dockerfile:

# This image is intended for testing Turi Create on Ubuntu 18.04.

FROM ubuntu:18.04

# Prevent apt-get from asking questions and expecting answers
ENV DEBIAN_FRONTEND noninteractive

# Update package database
RUN apt-get update

# Upgrade all possible packages
RUN apt-get -y upgrade

# Install Python 3.6 with apt, as well as
# turicreate and upstream dependencies
RUN apt-get -y install python3.6 python3.6-distutils libgomp1 lsb-release

# Install build-essential (needed by numpy==1.11.1, which tries to
# build itself from source on 3.6)
RUN apt-get -y install build-essential libpython3.6-dev
RUN ln -s /usr/include/locale.h /usr/include/xlocale.h

# Install pip and virtualenv
ADD https://bootstrap.pypa.io/get-pip.py /src/get-pip.py
WORKDIR /src
RUN python3.6 get-pip.py
RUN pip3.6 install virtualenv
