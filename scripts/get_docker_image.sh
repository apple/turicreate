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
  echo "  --ubuntu=[version]       The base Ubuntu version (defaults to 10.04)"
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
    --help)                 print_help ;;
    *) unknown_option $1 ;;
  esac
  shift
done

if [[ -z "${UBUNTU_VERSION}" ]]; then
  UBUNTU_VERSION="10.04"
fi

# The build image version that will be used for building
TC_BUILD_IMAGE_VERSION="1.0.0"

# The base image name - using Gitlab CI registry
TC_BUILD_IMAGE="registry.gitlab.com/zach_nation/turicreate/build-image-${UBUNTU_VERSION}:${TC_BUILD_IMAGE_VERSION}"

echo ${TC_BUILD_IMAGE}
