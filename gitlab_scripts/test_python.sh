#!/bin/bash
set -x
set -e

export TURICREATE_USERNAME=''

# command line arguments
if [[ -z $1 ]]; then
  echo "build type must be specified. "
  echo "Usage: $0 [debug | release]"
  exit 1
fi

# platform specific logic
if [[ $OSTYPE == darwin* ]]; then
  ARCHIVE_FILE_EXT=whl
else
  ARCHIVE_FILE_EXT=tar.gz
fi

BUILD_TYPE=$1
date
git lfs pull
date
(test -d $BUILD_TYPE) || ./configure
date
source deps/env/bin/activate
date
pip install target/turicreate-*.$ARCHIVE_FILE_EXT
date

# create a variable for project root dir
# it will be used inside unit test
export LFS_ROOT=$PWD/lfs

cd $BUILD_TYPE/src/unity/python
make python_source
cd ../../../..
cp -a $BUILD_TYPE/src/unity/python/turicreate/test deps/env/lib/python2.7/site-packages/turicreate/
cd deps/env/lib/python2.7/site-packages/turicreate/test

# run tests
pytest -v --junit-xml=../../../../../../../pytest.xml

date
