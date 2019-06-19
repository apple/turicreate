#!/bin/bash

set -e

PYTHON_SCRIPTS=deps/conda/bin
if [[ $OSTYPE == msys ]]; then
  PYTHON_SCRIPTS=deps/conda/bin/Scripts
fi

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=${SCRIPT_DIR}/..


print_help() {
  echo "Modifies the code to set a version number in all places that need the version number"
  echo
  echo "Usage: $0 --version=[version]"
  echo
  echo "  --version=[version]      The version number. For instance 1.3.1"

  echo "Example: $0 --version=1.3.1"
  exit 1
} # end of print help


# command flag options
# Parse command line configure flags ------------------------------------------
while [ $# -gt 0 ]
  do case $1 in
    --version=*)            VERSION_NUMBER=${1##--version=} ;;
    --help)                 print_help ;;
    *) print_help ;;
  esac
  shift
done


if [[ -z "${VERSION_NUMBER}" ]]; then
  echo "--version must be provided"
  exit 1
fi


set_version() {
  # set the engine version string
  cd ${WORKSPACE}/src/model_server/lib
  # C++, replace "1.8"//{{VERSION_STRING}} with "new_version_string"//{{VERSION_STRING}}"
  sed -i '' -e "s/\".*\"\/\/#{{VERSION_STRING}}/\"${VERSION_NUMBER}\"\/\/#{{VERSION_STRING}}/g" version_number.hpp
  # Python, replace '1.8'#{{VERSION_STRING}}' with 'new_version_string'#{{VERSION_STRING}}
  cd ${WORKSPACE}/src/python/
  sed -i '' -e "s/'.*'#{{VERSION_STRING}}/'${VERSION_NUMBER}'#{{VERSION_STRING}}/g" turicreate/version_info.py setup.py
}

set_version
