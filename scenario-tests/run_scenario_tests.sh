#!/bin/bash

set -e
set -x

SCENARIO_TESTS_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd $SCENARIO_TESTS_DIR
WORKSPACE=${SCENARIO_TESTS_DIR}/..

# The build image version that will be used for testing
TC_BUILD_IMAGE_1404=$(sh $WORKSPACE/scripts/get_docker_image.sh --ubuntu=14.04)
TC_BUILD_IMAGE_1804=$(sh $WORKSPACE/scripts/get_docker_image.sh --ubuntu=18.04)

function print_help {
  echo "Executes all scenario tests using a given egg location"

  echo "Usage: "
  echo "       ./run_scenario_tests.sh path_to_test_egg [other named options]"
  echo
  echo "Or:"
  echo "       Set environment variable:"
  echo "              TC_WHEEL_PATH"
  echo "       and run"
  echo "              ./run_scenario_tests.sh [other named options]"
  echo
  echo "Also takes the following named parameters"
  echo
  echo "  --exclude=[regex]  Excludes tests which match this regex."
  echo "                     Exclude takes precedence over include."
  echo
  echo "  --include=[regex]  Runs all tests which match this regex"
  echo "                     Exclude takes precedence over include."
  echo
  echo "  --help             Prints this help."
  echo
  echo "  --docker-python2.7       Use docker to test on Python 2.7 in Ubuntu 12.04."
  echo
  echo "  --docker-python3.5       Use docker to test on Python 3.5 in Ubuntu 14.04."
  echo
  echo "  --docker-python3.6       Use docker to test on Python 3.6 in Ubuntu 18.04."
  echo
  exit 0
} # end of print help

# command flag options
TC_WHEEL_UNDER_TEST=""
TEST_EXCLUDE_REGEX=""
TEST_INCLUDE_REGEX=""
# command flag options
# Parse command line configure flags ------------------------------------------
while [ $# -gt 0 ]
  do case $1 in
    --exclude=*)            TEST_EXCLUDE_REGEX=${1##--exclude=} ;;
    --include=*)            TEST_INCLUDE_REGEX=${1##--include=} ;;
    --docker-python2.7)     USE_DOCKER=1;DOCKER_PYTHON=2.7;;
    --docker-python3.5)     USE_DOCKER=1;DOCKER_PYTHON=3.5;;
    --docker-python3.6)     USE_DOCKER=1;DOCKER_PYTHON=3.6;;
    --help)                 print_help ;;
    *) TC_WHEEL_UNDER_TEST=$1 ;;
  esac
  shift
done

if [[ $TC_WHEEL_UNDER_TEST == "" ]]; then
        # No URL/package name specified on the command line
        # use environment variables
        TC_WHEEL_UNDER_TEST=$TC_WHEEL_PATH
fi

if [[ $TC_WHEEL_UNDER_TEST == "" ]]; then
        # Still no egg set -- show help
        print_help
fi

# If we are going to run in Docker,
# send this command into Docker and bail out of here when done.
if [[ -n "${USE_DOCKER}" ]]; then
  # create the build and test images
  # (this should ideally be a no-op if the image exists and is current)
  $WORKSPACE/scripts/create_docker_images.sh

  # Run the tests inside Docker
  if [[ "${DOCKER_PYTHON}" == "2.7" ]] || [[ "${DOCKER_PYTHON}" == "3.5" ]]; then
    docker run --rm -m=4g \
      --mount type=bind,source=$WORKSPACE,target=/build,consistency=delegated \
      -e "VIRTUALENV=virtualenv --python=python${DOCKER_PYTHON}" \
      ${TC_BUILD_IMAGE_1404} \
      /build/scenario-tests/run_scenario_tests.sh $TC_WHEEL_UNDER_TEST
  elif [[ "${DOCKER_PYTHON}" == "3.6" ]]; then
    docker run --rm -m=4g \
      --mount type=bind,source=$WORKSPACE,target=/build,consistency=delegated \
      -e "VIRTUALENV=virtualenv --python=python${DOCKER_PYTHON}" \
      ${TC_BUILD_IMAGE_1804} \
      /build/scenario-tests/run_scenario_tests.sh $TC_WHEEL_UNDER_TEST
  else
    echo "Invalid docker python version detected: ${DOCKER_PYTHON}"
    exit 1
  fi

  exit 0
fi

# Set up virtual environment
if [[ "$VIRTUALENV" == "" ]]; then
  VIRTUALENV=virtualenv
fi
if [[ ! -e venv ]]; then
    $VIRTUALENV venv
fi
source venv/bin/activate
pip install -r ../scripts/requirements.txt
pip install -r additional_requirements.txt
pip install $TC_WHEEL_UNDER_TEST

export TEST_EXCLUDE_REGEX
export TEST_INCLUDE_REGEX
export OPENBLAS_NUM_THREADS=1
echo $TEST_INCLUDE_REGEX
python driver.py
