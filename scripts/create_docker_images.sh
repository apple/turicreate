#!/bin/bash

set -e
set -x

#
# TC_BUILD_IMAGE_VERSION: the version of the build images.
#
# Bump the version number to force a rebuild. This is probably necessary
# whenever the contents of the image change.
#
TC_BUILD_IMAGE_VERSION=1.0.5

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=${SCRIPT_DIR}/..
cd ${WORKSPACE}

print_help() {
  echo "Creates Docker images for building and testing Turi Create."
  echo
  echo "Usage: ./create_docker_images.sh [--persist_to_repo_dir]"
  echo
  echo "  --persist_to_repo_dir    Write the Docker images to $WORKSPACE/.docker_images"
  exit 1
} # end of print help

# command flag options
# Parse command line configure flags ------------------------------------------
while [ $# -gt 0 ]
  do case $1 in
    --persist_to_repo_dir)  PERSIST_TO_REPO_DIR=1;;
    --help)                 print_help ;;
    *) unknown_option $1 ;;
  esac
  shift
done

# Load images from repo directory if possible
if [[ -n $PERSIST_TO_REPO_DIR ]]; then
  mkdir -p .docker_images
  (test -f .docker_images/image-10.04.gz && docker load -i .docker_images/image-10.04.gz) || true
  (test -f .docker_images/image-12.04.gz && docker load -i .docker_images/image-12.04.gz) || true
  (test -f .docker_images/image-14.04.gz && docker load -i .docker_images/image-14.04.gz) || true
  (test -f .docker_images/image-18.04.gz && docker load -i .docker_images/image-18.04.gz) || true
fi

(docker image ls turicreate/build-image-10.04:${TC_BUILD_IMAGE_VERSION} | grep turicreate/build-image) || \
cat scripts/Dockerfile-Ubuntu-10.04 | docker build -t turicreate/build-image-10.04:${TC_BUILD_IMAGE_VERSION} -

(docker image ls turicreate/build-image-12.04:${TC_BUILD_IMAGE_VERSION} | grep turicreate/build-image) || \
cat scripts/Dockerfile-Ubuntu-12.04 | docker build -t turicreate/build-image-12.04:${TC_BUILD_IMAGE_VERSION} -

(docker image ls turicreate/build-image-14.04:${TC_BUILD_IMAGE_VERSION} | grep turicreate/build-image) || \
cat scripts/Dockerfile-Ubuntu-14.04 | docker build -t turicreate/build-image-14.04:${TC_BUILD_IMAGE_VERSION} -

(docker image ls turicreate/build-image-18.04:${TC_BUILD_IMAGE_VERSION} | grep turicreate/build-image) || \
cat scripts/Dockerfile-Ubuntu-18.04 | docker build -t turicreate/build-image-18.04:${TC_BUILD_IMAGE_VERSION} -

# Save images back to repo directory so they can be re-used the next time this script is run.
# (This assumes the build may run on another machine, but with the same directory contents. This setup is
#  primarily intended for CI systems that create a new VM for each run, but can cache the filesystem.)
if [[ -n $PERSIST_TO_REPO_DIR ]]; then
  docker save turicreate/build-image-10.04:${TC_BUILD_IMAGE_VERSION} | gzip -c > .docker_images/image-10.04.gz
  docker save turicreate/build-image-12.04:${TC_BUILD_IMAGE_VERSION} | gzip -c > .docker_images/image-12.04.gz
  docker save turicreate/build-image-14.04:${TC_BUILD_IMAGE_VERSION} | gzip -c > .docker_images/image-14.04.gz
  docker save turicreate/build-image-18.04:${TC_BUILD_IMAGE_VERSION} | gzip -c > .docker_images/image-18.04.gz
fi

