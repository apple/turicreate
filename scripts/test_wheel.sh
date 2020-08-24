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
  echo "Run --help for more information."
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
  echo
  echo "  --minimal                test the turicreate+minimal package"
  echo
  exit 1
} # end of print help

USE_MINIMAL=0

# command flag options
# Parse command line configure flags ------------------------------------------
while [ $# -gt 0 ]
  do case $1 in
    --docker-python2.7)     USE_DOCKER=1;DOCKER_PYTHON=2.7;;
    --docker-python3.5)     USE_DOCKER=1;DOCKER_PYTHON=3.5;;
    --docker-python3.6)     USE_DOCKER=1;DOCKER_PYTHON=3.6;;
    --docker-python3.7)     USE_DOCKER=1;DOCKER_PYTHON=3.7;;
    --minimal)              USE_MINIMAL=1;;
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

  docker_extra_args=""
  if [[ "$USE_MINIMAL" -eq 1 ]]; then
    docker_extra_args="$extra_args --minimal"
  fi

  # Run the tests inside Docker
  if [[ "${DOCKER_PYTHON}" == "2.7" ]] || [[ "${DOCKER_PYTHON}" == "3.5" ]]; then
    docker run --rm -m=8g \
      --mount type=bind,source="$WORKSPACE",target=/build,consistency=delegated \
      --env-file "${envlist}" \
      "${TC_BUILD_IMAGE_1404}" \
      /build/scripts/test_wheel.sh ${docker_extra_args}
  elif [[ "${DOCKER_PYTHON}" == "3.6" ]] || [[ "${DOCKER_PYTHON}" == "3.7" ]]; then
    docker run --rm -m=8g \
      --mount type=bind,source="$WORKSPACE",target=/build,consistency=delegated \
      --env-file "${envlist}" \
      "${TC_BUILD_IMAGE_1804}" \
      /build/scripts/test_wheel.sh ${docker_extra_args}
  else
    echo "Invalid docker python version detected: ${DOCKER_PYTHON}"
    exit 1
  fi

  exit 0
fi

test -d deps/env || USE_MINIMAL="$USE_MINIMAL" ./scripts/install_python_toolchain.sh
source deps/env/bin/activate

# make_wheel can build 1 or 2 wheels, namely full or minimal
# we need to find the wheel we need to install and test
bdist_wheels=($(ls target/turicreate-*.whl))

if [[ ${#bdist_wheels[@]} -eq 1 ]]; then
  wheel_to_install=${bdist_wheels[0]}
  if [[ "$USE_MINIMAL" -eq 1 ]]; then
    if [[ ! "$wheel_to_install" =~ .*"+minimal".* ]]; then
      echo "minimal pkg is not found. Only found $wheel_to_install"
      exit 1
    fi
  else
    if [[ "$wheel_to_install" =~ .*"+minimal".* ]]; then
      echo "full pkg is not found. Only found $wheel_to_install"
      exit 1
    fi
  fi

elif [[ ${#bdist_wheels[@]} -eq 2 ]]; then
  wheel_to_install=${bdist_wheels[0]}
  if [[ $USE_MINIMAL -eq 1 ]]; then
    if [[ ! "$wheel_to_install" =~ .*"+minimal".* ]]; then
      wheel_to_install="${bdist_wheels[1]}"
    fi
  else
    if [[ "$wheel_to_install" =~ .*"+minimal".* ]]; then
      wheel_to_install="${bdist_wheels[1]}"
    fi
  fi

else
  echo "Wrong number of wheel: ${#bdist_wheels[@]}. At most 2 wheels are needed."
  exit 1
fi

pip install "$wheel_to_install"

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
if [[ "$USE_MINIMAL" -eq 1 ]]; then
  # remove all toolkits related files
  ${PYTHON} -m pytest -v --durations=20 -m minimal \
    --junit-xml="$WORKSPACE"/pytest-minimal.xml

else
  ${PYTHON} -m pytest --cov -v --durations=100 \
    --junit-xml="$WORKSPACE"/pytest.xml
fi

date
