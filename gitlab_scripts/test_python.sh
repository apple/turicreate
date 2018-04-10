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
(test -d $BUILD_TYPE) || ./configure --$2
date
source deps/env/bin/activate
date
pip install target/turicreate-*.whl
date

cd $BUILD_TYPE/src/unity/python
make python_source
cd ../../../..
cp -a $BUILD_TYPE/src/unity/python/turicreate/test deps/env/lib/$2/site-packages/turicreate/
cd deps/env/lib/$2/site-packages/turicreate/test

# run tests
pytest -v --junit-xml=../../../../../../../pytest.xml

date
