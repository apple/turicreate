#!/bin/bash
# has to be run from root of the repo
set -exo pipefail

if [[ -z $VIRTUALENV ]]; then
  VIRTUALENV=virtualenv
fi

$VIRTUALENV "$(pwd)"/deps/env
source deps/env/bin/activate

PYTHON="${PWD}/deps/env/bin/python"
PIP="${PYTHON} -m pip"

PYTHON_MAJOR_VERSION=$(${PYTHON} -c 'import sys; print(sys.version_info.major)')
PYTHON_MINOR_VERSION=$(${PYTHON} -c 'import sys; print(sys.version_info.minor)')
PYTHON_VERSION="python${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}"

# TODO - not sure why 'm' is necessary here (and not in 2.7)
# note that PYTHON_VERSION includes the word "python", like "python2.7" or "python3.6"
PYTHON_FULL_NAME=${PYTHON_VERSION}m
if [[ "${PYTHON_VERSION}" == "python2.7" ]]; then
  PYTHON_FULL_NAME=python2.7
fi


function linux_patch_sigfpe_handler {
  if [[ $OSTYPE == linux* ]]; then
    targfile=deps/local/include/pyfpe.h
    if [[ -f $targfile ]]; then
      echo "#undef WANT_SIGFPE_HANDLER" | cat - $targfile > tmp
      mv -f tmp $targfile
    fi
  fi
}

$PIP install --upgrade "pip"
$PIP install -r scripts/requirements.txt

# install pre-commit hooks for git
with_pre_commit=${with_pre_commit:-0}
if [[ $with_pre_commit -eq 1 ]]; then
  # install under root
  $PYTHON -m pre_commit install
  # TODO: pre-commit-hooks for clang-format
fi

mkdir -p deps/local/lib
mkdir -p deps/local/include

pushd deps/local/include
for f in ../../env/include/"$PYTHON_FULL_NAME"/*; do
  ln -Ffs "$f" "$(basename "$f")"
done
popd

mkdir -p deps/local/bin
pushd deps/local/bin
for f in ../../env/bin/*; do
  ln -Ffs "$f" "$(basename "$f")"
done
popd

linux_patch_sigfpe_handler
