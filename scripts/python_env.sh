#!/bin/bash

####  Usage ####
# source python_env.sh [debug | release]

# Check build type parameter
if [[ -z $1 ]]; then
        echo "build type must be specified. "
        echo "Usage: $0 [debug | release ]"
        return 1
fi
BUILD_TYPE=$1
BUILD_TYPE=`echo $BUILD_TYPE | tr "[A-Z]" "[a-z]"` # convert to lower case
if [ "debug" != $BUILD_TYPE ] && [ "release" != $BUILD_TYPE ]; then
    echo "build type must be either \"debug\" or \"release\". "
    return 1
fi
echo BUILD_TYPE=$BUILD_TYPE

if [[ -z $BASH_SOURCE ]]; then
        SCRIPT_DIR=$( cd "$( dirname $0)" && pwd )
else
        SCRIPT_DIR=$( cd "$( dirname $BASH_SOURCE )" && pwd )
fi
# Turi project root
ROOT_DIR=$SCRIPT_DIR/..

PYTHON_SCRIPTS=deps/env/bin

# python executable
export PYTHON_EXECUTABLE=$ROOT_DIR/$PYTHON_SCRIPTS/python
export PYTEST_EXECUTABLE=$ROOT_DIR/$PYTHON_SCRIPTS/pytest
export PIP_EXECUTABLE=$ROOT_DIR/$PYTHON_SCRIPTS/pip

# environment varialbes for running the test
export TURI_ROOT=$ROOT_DIR
export TURI_BUILD_ROOT=$ROOT_DIR/$BUILD_TYPE
if [ -z "$PYTHONPATH" ]; then
        export PYTHONPATH="$TURI_BUILD_ROOT/src/unity/python"
else
        export PYTHONPATH="$TURI_BUILD_ROOT/src/unity/python:$PYTHONPATH"
        export OLD_PYTHON_ENV_PYTHONPATH=$PYTHONPATH
fi

echo TURI_ROOT=$TURI_ROOT
echo TURI_BUILD_ROOT=$TURI_BUILD_ROOT
echo PYTHON_EXECUTABLE=$PYTHON_EXECUTABLE
echo PYTEST_EXECUTABLE=$PYTEST_EXECUTABLE
echo PIP_EXECUTABLE=$PIP_EXECUTABLE

echo "Activating virtualenv in deps/env"
source $ROOT_DIR/deps/env/bin/activate
