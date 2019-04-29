#!/bin/bash

set -e
set -x

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=${SCRIPT_DIR}/..
cd ${WORKSPACE}

# The build image version that will be used for testing
TC_BUILD_IMAGE_1004=$(sh $WORKSPACE/scripts/get_docker_image.sh --ubuntu=10.04)
TC_BUILD_IMAGE_1204=$(sh $WORKSPACE/scripts/get_docker_image.sh --ubuntu=12.04)
TC_BUILD_IMAGE_1404=$(sh $WORKSPACE/scripts/get_docker_image.sh --ubuntu=14.04)
TC_BUILD_IMAGE_1804=$(sh $WORKSPACE/scripts/get_docker_image.sh --ubuntu=18.04)

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

# Load images from registry if possible
(docker pull ${TC_BUILD_IMAGE_1004}) || true
(docker pull ${TC_BUILD_IMAGE_1204}) || true
(docker pull ${TC_BUILD_IMAGE_1404}) || true
(docker pull ${TC_BUILD_IMAGE_1804}) || true

(docker image ls ${TC_BUILD_IMAGE_1004} | grep turicreate/build-image) || \
cat scripts/Dockerfile-Ubuntu-10.04 | docker build -t ${TC_BUILD_IMAGE_1004} -

(docker image ls ${TC_BUILD_IMAGE_1204} | grep turicreate/build-image) || \
cat scripts/Dockerfile-Ubuntu-12.04 | docker build -t ${TC_BUILD_IMAGE_1204} -

(docker image ls ${TC_BUILD_IMAGE_1404} | grep turicreate/build-image) || \
cat scripts/Dockerfile-Ubuntu-14.04 | docker build -t ${TC_BUILD_IMAGE_1404} -

(docker image ls ${TC_BUILD_IMAGE_1804} | grep turicreate/build-image) || \
cat scripts/Dockerfile-Ubuntu-18.04 | docker build -t ${TC_BUILD_IMAGE_1804} -

# Save images back to repo directory so they can be re-used the next time this script is run.
# (This assumes the build may run on another machine, but with the same directory contents. This setup is
#  primarily intended for CI systems that create a new VM for each run, but can cache the filesystem.)
if [[ -n $PERSIST_TO_REPO_DIR ]]; then
  docker save ${TC_BUILD_IMAGE_1004} | gzip -c > .docker_images/image-10.04.gz
  docker save ${TC_BUILD_IMAGE_1204} | gzip -c > .docker_images/image-12.04.gz
  docker save ${TC_BUILD_IMAGE_1404} | gzip -c > .docker_images/image-14.04.gz
  docker save ${TC_BUILD_IMAGE_1804} | gzip -c > .docker_images/image-18.04.gz
fi

