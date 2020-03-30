#!/bin/bash

set -e
set -x

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=${SCRIPT_DIR}/..

# The build image version that will be used for testing
TC_BUILD_IMAGE_1404=$(bash "$WORKSPACE"/scripts/get_docker_image.sh --ubuntu=14.04)
TC_BUILD_IMAGE_1804=$(bash "$WORKSPACE"/scripts/get_docker_image.sh --ubuntu=18.04)

unknown_option() {
  echo "Unknown option $1. Exiting."
  exit 1
}

print_help() {
  echo "Tests the wheel in the targets directory."
  echo "Currently quite fragile: assumes there is exactly one wheel in the targets directory."
  echo
  echo "Usage: ./test_wheel.sh [options]"
  echo
  echo "  --docker-python2.7       Use docker to test on Python 2.7 in Ubuntu 12.04."
  echo
  echo "  --docker-python3.5       Use docker to test on Python 3.5 in Ubuntu 14.04."
  echo
  echo "  --docker-python3.6       Use docker to test on Python 3.6 in Ubuntu 18.04."
  echo
  echo "  --docker-python3.7       Use docker to test on Python 3.7 in Ubuntu 18.04."
  exit 1
} # end of print help

# command flag options
# Parse command line configure flags ------------------------------------------
while [ $# -gt 0 ]
  do case $1 in
    --docker-python2.7)     USE_DOCKER=1;DOCKER_PYTHON=2.7;;
    --docker-python3.5)     USE_DOCKER=1;DOCKER_PYTHON=3.5;;
    --docker-python3.6)     USE_DOCKER=1;DOCKER_PYTHON=3.6;;
    --docker-python3.7)     USE_DOCKER=1;DOCKER_PYTHON=3.7;;
    --help)                 print_help ;;
    *) unknown_option "$1" ;;
  esac
  shift
done

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=${SCRIPT_DIR}/..

# PWD is WORKSPACE

cd "${WORKSPACE}"

# If we are going to run in Docker,
# send this command into Docker and bail out of here when done.
if [[ -n "${USE_DOCKER}" ]]; then
  # create the build and test images
  # (this should ideally be a no-op if the image exists and is current)
  "$WORKSPACE"/scripts/create_docker_images.sh

  # define env-files to forward env vars to inner docker session
  envlist="${WORKSPACE}/${0%.sh}-env.list"
  echo "VIRTUALENV=virtualenv --python=python${DOCKER_PYTHON}" >> "${envlist}"
  echo "TC_ENABLE_S3_TESTS=${TC_ENABLE_S3_TESTS:-0}" >> "${envlist}"
  if [[ "${TC_ENABLE_S3_TESTS:-0}" -gt 0 ]]; then
    echo TC_ENABLE_S3_TESTS  >> "${envlist}"
    echo TURI_S3_ENDPOINT >> "${envlist}"
    echo TURI_S3_REGION >> "${envlist}"
    echo AWS_SECRET_ACCESS_KEY >> "${envlist}"
    echo AWS_ACCESS_KEY_ID >> "${envlist}"
  fi

  # Run the tests inside Docker
  if [[ "${DOCKER_PYTHON}" == "2.7" ]] || [[ "${DOCKER_PYTHON}" == "3.5" ]]; then
    docker run --rm -m=8g \
      --mount type=bind,source="$WORKSPACE",target=/build,consistency=delegated \
      --env-file "${envlist}" \
      "${TC_BUILD_IMAGE_1404}" \
      /build/scripts/test_wheel.sh
  elif [[ "${DOCKER_PYTHON}" == "3.6" ]] || [[ "${DOCKER_PYTHON}" == "3.7" ]]; then
    docker run --rm -m=8g \
      --mount type=bind,source="$WORKSPACE",target=/build,consistency=delegated \
      --env-file "${envlist}" \
      "${TC_BUILD_IMAGE_1804}" \
      /build/scripts/test_wheel.sh
  else
    echo "Invalid docker python version detected: ${DOCKER_PYTHON}"
    exit 1
  fi

  exit 0
fi

test -d deps/env || ./scripts/install_python_toolchain.sh
source deps/env/bin/activate
pip install target/turicreate-*.whl

PYTHON="$WORKSPACE/deps/env/bin/python"
PYTHON_MAJOR_VERSION=$(${PYTHON} -c 'import sys; print(sys.version_info.major)')
PYTHON_MINOR_VERSION=$(${PYTHON} -c 'import sys; print(sys.version_info.minor)')
PYTHON_VERSION="python${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}"

cp -a src/python/turicreate/test deps/env/lib/"${PYTHON_VERSION}"/site-packages/turicreate/

TEST_DIR="$WORKSPACE"/deps/env/lib/"${PYTHON_VERSION}"/site-packages/turicreate/test
SOURCE_DIR=$(dirname "$TEST_DIR")

$PYTHON -c "import sys; lines=sys.stdin.read(); \
  print(lines.replace('{{ source }}', \"$SOURCE_DIR\"))" \
  < scripts/.coveragerc-template \
  > "${TEST_DIR}"/.coveragerc

cd "$TEST_DIR"

# run tests
${PYTHON} -m pytest --cov -v --durations=100 \
  --junit-xml="$WORKSPACE"/pytest.xml

date
