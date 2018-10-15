#!/bin/bash
set -x
set -e

# command line arguments
if [[ -z $1 ]]; then
  echo "build type must be specified. "
  echo "Usage: $0 [debug | release]"
  exit 1
fi

BUILD_TYPE=$1
date
(test -d $BUILD_TYPE) || ./configure
date
source deps/env/bin/activate
date
pip install target/turicreate-*.whl
date

cd $BUILD_TYPE/src/unity/python
make python_source
cd ../../../..

PYTHON="$PWD/deps/env/bin/python"
PYTHON_MAJOR_VERSION=$(${PYTHON} -c 'import sys; print(sys.version_info.major)')
PYTHON_MINOR_VERSION=$(${PYTHON} -c 'import sys; print(sys.version_info.minor)')
PYTHON_VERSION="python${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}"
cp -a $BUILD_TYPE/src/unity/python/turicreate/test deps/env/lib/${PYTHON_VERSION}/site-packages/turicreate/
cd deps/env/lib/${PYTHON_VERSION}/site-packages/turicreate/test

# run tests
${PYTHON} -m pytest -v --junit-xml=../../../../../../../pytest.xml

date
