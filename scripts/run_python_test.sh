#!/bin/bash

####  Usage ####
# ./run_python_test.sh [debug | release]

set -e
set -x

# Check build type
if [[ -z $1 ]]; then
        echo "build type must be specified. "
        echo "Usage: $0 [debug | release ]"
        exit 1
fi
BUILD_TYPE=$1
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=${SCRIPT_DIR}/..

unset PYTHONPATH

cd $WORKSPACE
scripts/install_python_toolchain.sh
source scripts/python_env.sh $BUILD_TYPE

# Make python unit tests print unity server log on exception
export TURI_VERBOSE=1

push_ld_library_path() {
  OLD_LIBRARY_PATH=$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH=${WORKSPACE}/deps/local/lib:${WORKSPACE}/deps/local/lib64:$LD_LIBRARY_PATH
}

pop_ld_library_path() {
  export LD_LIBRARY_PATH=$OLD_LIBRARY_PATH
}
push_ld_library_path
cd $TURI_BUILD_ROOT/src/unity/python
make -j4
pop_ld_library_path

find . -name "*.xml" -delete
if ! type "parallel" 2> /dev/null; then
        cmd=""
        if [[ $OSTYPE != msys ]]; then
          cmd=${PYTHON_EXECUTABLE}
        fi
        cmd="${cmd} ${PYTEST_EXECUTABLE} -v $PYTHONPATH/turicreate/test --junit-xml=alltests.pytest.xml"
        echo $cmd
        $cmd
else
        cmd=""
        if [[ $OSTYPE != msys ]]; then
          cmd=${PYTHON_EXECUTABLE}
        fi
        cmd="${cmd} ${PYTEST_EXECUTABLE} -v -s --junit-xml={}.pytest.xml {}"
        echo "Tests are running in parallel. Output is buffered until job is done..."
        find turicreate/test -name "*.py" | parallel --group -P 4 $cmd
fi
