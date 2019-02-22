FROM ubuntu:10.04

ENV DEBIAN_FRONTEND noninteractive

# Fix path to new location for older Ubuntu releases
RUN sed -i -e 's/archive\./old-releases\./g' /etc/apt/sources.list

# Update package database
RUN apt-get update

# Upgrade all possible packages
RUN apt-get -y upgrade

# Install dependencies
