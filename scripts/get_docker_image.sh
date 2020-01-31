#!/bin/bash

set -e

unknown_option() {
  echo "Unknown option $1. Exiting."
  exit 1
}

print_help() {
  echo "Gets the name and version number of the Docker image for building/testing Turi Create."
  echo
  echo "Usage: ./get_docker_image.sh --ubuntu=[version]"
  echo
  echo "  --ubuntu=[version]       The base Ubuntu version, OR:"
  echo "  --centos=[version]       The base CentOS version (defaults to 6)"
  echo
  echo "  --help                   Prints this help text."
  echo
  exit 1
} # end of print help

# command flag options
# Parse command line configure flags ------------------------------------------
while [ $# -gt 0 ]
  do case $1 in
    --ubuntu=*)             UBUNTU_VERSION=${1##--ubuntu=} ;;
    --centos=*)             CENTOS_VERSION=${1##--centos=} ;;
    --help)                 print_help ;;
    *) unknown_option $1 ;;
  esac
  shift
done

if [[ -z "${UBUNTU_VERSION}" && -z "${CENTOS_VERSION}" ]]; then
  CENTOS_VERSION="6"
fi

# The build image version that will be used for building
TC_BUILD_IMAGE_VERSION="1.0.6"

if [[ -z "${CENTOS_VERSION}" ]]; then
  # The base image name - using Gitlab CI registry
  TC_BUILD_IMAGE="registry.gitlab.com/zach_nation/turicreate/build-image-${UBUNTU_VERSION}:${TC_BUILD_IMAGE_VERSION}"
else
  TC_BUILD_IMAGE="registry.gitlab.com/zach_nation/turicreate/build-image-centos-${CENTOS_VERSION}:${TC_BUILD_IMAGE_VERSION}"
fi

echo ${TC_BUILD_IMAGE}
