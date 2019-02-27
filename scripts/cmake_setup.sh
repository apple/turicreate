#!/bin/bash
##=============================================================================
## Support code

cmake_version=3.13.4

set -e

function print_help {
  echo "Usage: ./cmake_setup.sh <cmake_source_directory> <install_prefix>"
  echo
  exit 1
} # end of print help

[[ -z $1 ]] && print_help
[[ -z $2 ]] && print_help

cmake_directory=$1
DEPS_PREFIX=$2
cmake_build_directory=$DEPS_PREFIX/tmp/cmake_build/

echo "Building CMake version $cmake_version."

##=============================================================================
## Main configuration processing

# cd into the extracted directory and install

mkdir -p $cmake_build_directory

cp -r $cmake_directory/ $cmake_build_directory

if [ -f $cmake_build_directory/cmake-$cmake_version/configure ]; then 
  cmake_build_directory=$cmake_build_directory/cmake-$cmake_version/
fi

echo "CMake build directory = $cmake_build_directory"

pushd $cmake_build_directory

if [ $OSTYPE == "msys" ]; then
  ./configure --prefix=`cygpath -w $DEPS_PREFIX` --parallel=4
else
  ./configure --prefix=$DEPS_PREFIX --parallel=4
fi

make -j4
make install
set +e

exit 0

