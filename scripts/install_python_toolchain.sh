#!/bin/bash
# has to be run from root of the repo
set -x
set -e

if [[ -z $VIRTUALENV ]]; then
  VIRTUALENV=virtualenv
fi

$VIRTUALENV deps/env
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

$PIP install --upgrade "pip>=8.1"

# numpy needs to be installed before everything else so that installing
# numba (a dependency of resampy) doesn't fail on Python 3.5. This can
# be removed once numba publishes a Python 3.5 wheel for their most
# recent version.
$PIP install numpy==1.11.1

$PIP install -r scripts/requirements.txt

mkdir -p deps/local/lib
mkdir -p deps/local/include

pushd deps/local/include
for f in `ls ../../env/include/$PYTHON_FULL_NAME/*`; do  
  ln -Ffs $f
done
popd

mkdir -p deps/local/bin
pushd deps/local/bin
for f in `ls ../../env/bin`; do
  ln -Ffs $f
done
popd

linux_patch_sigfpe_handler

