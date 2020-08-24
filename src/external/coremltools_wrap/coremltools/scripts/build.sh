#!/bin/bash

set -e
set -x

##=============================================================================
## Main configuration processing
COREMLTOOLS_HOME=$( cd "$( dirname "$0" )/.." && pwd )
BUILD_DIR="${COREMLTOOLS_HOME}/build"

# command flag options
BUILD_MODE="Release"
NUM_PROCS=1
BUILD_PROTO=0
BUILD_DIST=0
PYTHON="3.7"
CHECK_ENV=1

unknown_option() {
  echo "Unknown option $1. Exiting."
  exit 1
}

print_help() {
  echo "Builds coremltools and dependent libraries."
  echo
  echo "Usage: zsh -i build.sh"
  echo
  echo "  --num_procs=n (default 1)       Specify the number of proceses to run in parallel."
  echo "  --python=*                      Python to use for configuration."
  echo "  --protobuf                      Rebuild & overwrite the protocol buffers in MLModel."
  echo "  --debug                         Build without optimizations and stripping symbols."
  echo "  --dist                          Build the distribution (wheel)."
  echo "  --no-check-env                  Don't check the environment to verify it's up to date."
  echo
  exit 1
} # end of print help

# command flag options
# Parse command line configure flags ------------------------------------------
while [ $# -gt 0 ]
  do case $1 in
    --python=*)          PYTHON=${1##--python=} ;;
    --num-procs=*)       NUM_PROCS=${1##--num-procs=} ;;
    --protobuf)          BUILD_PROTO=1 ;;
    --debug)             BUILD_MODE="Debug" ;;
    --dist)              BUILD_DIST=1 ;;
    --no-check-env)      CHECK_ENV=0 ;;
    --help)              print_help ;;
    *) unknown_option $1 ;;
  esac
  shift
done

# First configure
echo
echo "Configuring using python from $PYTHON"
echo
echo ${COREMLTOOLS_HOME}
cd ${COREMLTOOLS_HOME}
if [[ $CHECK_ENV == 1 ]]; then
    zsh -i -e scripts/env_create.sh --python=$PYTHON --exclude-test-deps
fi

# Setup the right python
source scripts/env_activate.sh --python=$PYTHON
echo
echo "Using python from $(which python)"
echo

# Uninstall any existing coremltools inside the build environment
pip uninstall -y coremltools

# Create a directory for building the artifacts
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# Configure CMake correctly
ADDITIONAL_CMAKE_OPTIONS=""
if [[ "$OSTYPE" == "darwin"* ]]; then
    NUM_PROCS=$(sysctl -n hw.ncpu)
    ADDITIONAL_CMAKE_OPTIONS=" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13"
else
    NUM_PROCS=$(nproc)
fi

# Call CMake
cmake $ADDITIONAL_CMAKE_OPTIONS \
  -DCMAKE_BUILD_TYPE=$BUILD_MODE \
  -DPYTHON_EXECUTABLE:FILEPATH=$PYTHON_EXECUTABLE \
  -DPYTHON_INCLUDE_DIR=$PYTHON_INCLUDE_DIR \
  -DPYTHON_LIBRARY=$PYTHON_LIBRARY \
  -DOVERWRITE_PB_SOURCE=$BUILD_PROTO \
  ${COREMLTOOLS_HOME}

# Make the python wheel
make -j${NUM_PROCS}

if [ $BUILD_DIST -eq 1 ]
then
  make dist
fi
