#!/bin/bash

set -e
set -x


unknown_option() {
  echo "Unknown option $1. Exiting."
  exit 1
}

print_help() {
  echo "Builds all the documentaiton: Python API docs, user guide, and C++ doxygen docs"
  echo
  echo "Usage: ./make_documentation.sh [options]"
  echo
  echo "  --docker       Use docker to generate the docs"
  exit 0
} # end of print help


# command flag options
# Parse command line configure flags ------------------------------------------
while [ $# -gt 0 ]
  do case $1 in
    --docker)     USE_DOCKER=1;;
    --help)       print_help ;;
    *) unknown_option $1 ;;
  esac
  shift
done


SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=${SCRIPT_DIR}/..
cd ${WORKSPACE}


# If we are going to run in Docker,
# send this command into Docker and bail out of here when done.
if [[ -n "${USE_DOCKER}" ]]; then
  # create the build and test images
  # (this should ideally be a no-op if the image exists and is current)
  $WORKSPACE/scripts/create_docker_images.sh

  TC_BUILD_IMAGE_1804=$(bash $WORKSPACE/scripts/get_docker_image.sh --ubuntu=18.04)

  docker run --rm -m=8g \
         --mount type=bind,source=$WORKSPACE,target=/build,consistency=delegated \
         ${TC_BUILD_IMAGE_1804} \
         /build/scripts/make_documentation.sh

  exit 0
fi


TARGET_DIR=${WORKSPACE}/target

virtualenv ./deps/env
source deps/env/bin/activate

# ignore minimal build
pip install "$(ls -1 target/turicreate-*.whl | grep -v minimal)"

OUTPUT_DIR=${TARGET_DIR}/docs
mkdir -p ${OUTPUT_DIR}

echo -e "\n\n\n================= Generating API Docs ================\n\n\n"
pip install sphinx==1.6.5 sphinx-bootstrap-theme numpydoc
rm -rf pydocs
mkdir -p pydocs
cd pydocs
cp -R ${WORKSPACE}/src/python/doc/* .
make clean
make html
mv build/html ${OUTPUT_DIR}/api

echo -e "\n\n\n================= Generating User Guide ================\n\n\n"
cd ${WORKSPACE}/userguide
npm install
npm run build
mv _book/ ${OUTPUT_DIR}/userguide

echo -e "\n\n\n================= Generating C++ Docs ================\n\n\n"
cd ${WORKSPACE}
doxygen
mv doc/html/ ${OUTPUT_DIR}/cpp

cd ${TARGET_DIR}
zip -r docs.zip docs
