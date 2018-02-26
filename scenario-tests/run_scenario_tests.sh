#!/bin/bash

set -e
set -x

function print_help {
  echo "Executes all scenario tests using a given egg location"

  echo "Usage: "
  echo "       ./run_scenario_tests.sh path_to_test_egg [other named options]"
  echo
  echo "Or:"
  echo "       Set environment variables"
  echo "              GLC_LINUX_EGG_URL"
  echo "              GLC_MAC_EGG_URL"
  echo "              GLC_WINDOWS_EGG_URL"
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
  exit 0
} # end of print help

# command flag options
GLC_EGG_UNDER_TEST=""
TEST_EXCLUDE_REGEX=""
TEST_INCLUDE_REGEX=""
# command flag options
# Parse command line configure flags ------------------------------------------
while [ $# -gt 0 ]
  do case $1 in
    --exclude=*)            TEST_EXCLUDE_REGEX=${1##--exclude=} ;;
    --include=*)            TEST_INCLUDE_REGEX=${1##--include=} ;;
    --help)                 print_help ;;
    *) GLC_EGG_UNDER_TEST=$1 ;;
  esac
  shift
done

if [[ $GLC_EGG_UNDER_TEST == "" ]]; then
        # No URL/package name specified on the command line
        # use environment variables
        OS="`uname -s`"
        if [[ $OS == 'Linux' ]]; then
                GLC_EGG_UNDER_TEST=$GLC_LINUX_EGG_URL
        elif [[ $OS == 'Darwin' ]]; then
                GLC_EGG_UNDER_TEST=$GLC_MAC_EGG_URL
        elif [[ `uname -o` == 'Msys' ]]; then
                GLC_EGG_UNDER_TEST=$GLC_WINDOWS_EGG_URL
        fi
fi

if [[ $GLC_EGG_UNDER_TEST == "" ]]; then
        # Still no egg set -- show help
        print_help
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
pip install $GLC_EGG_UNDER_TEST

export TEST_EXCLUDE_REGEX
export TEST_INCLUDE_REGEX
export OPENBLAS_NUM_THREADS=1
echo $TEST_INCLUDE_REGEX
python driver.py
