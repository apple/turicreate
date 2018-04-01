#!/bin/bash

####  Usage ####
# source python_env.sh [debug | release]

# Check build type
if [[ -z $1 ]]; then
        echo "build type must be specified. "
        echo "Usage: $0 [debug | release ]"
        return
fi
if [[ $PYTHON_ENV_SET == 1 ]]; then
        echo "Previous values set. Unsetting some variables..."
        if [ ! -z "$OLD_PYTHON_ENV_PATH" ]; then
                export PATH=$OLD_PYTHON_ENV_PATH
                PATH=$OLD_PYTHON_ENV_PATH
        else
                PATH=""
        fi
        if [ ! -z "$OLD_PYTHON_ENV_PYTHONPATH" ]; then
                export PYTHONPATH=$OLD_PYTHON_ENV_PYTHONPATH
                PYTHONPATH=$OLD_PYTHON_ENV_PYTHONPATH
        else
                PYTHONPATH=""
        fi
fi
BUILD_TYPE=$1
BUILD_TYPE=`echo $BUILD_TYPE | tr "[A-Z]" "[a-z]"` # convert to lower case
echo BUILD_TYPE=$BUILD_TYPE

if [[ -z $BASH_SOURCE ]]; then
        SCRIPT_DIR=$( cd "$( dirname $0)" && pwd )
else
        SCRIPT_DIR=$( cd "$( dirname $BASH_SOURCE )" && pwd )
fi
# Turi project root
ROOT_DIR=$SCRIPT_DIR/..

#export LD_LIBRARY_PATH=${ROOT_DIR}/deps/local/lib:${ROOT_DIR}/deps/local/lib64:$LD_LIBRARY_PATH
PYTHON_SCRIPTS=deps/env/bin
if [[ $OSTYPE == msys ]]; then
  PYTHON_SCRIPTS=deps/conda/bin/Scripts
fi

# python executable
export PYTHON_EXECUTABLE=$ROOT_DIR/deps/env/bin/python
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
export PYTHONHOME=$ROOT_DIR/deps/env
if [[ $OSTYPE == msys ]]; then
        export PYTHONHOME="$PYTHONHOME/bin"
fi

if [[ $OSTYPE == msys ]]; then
  export PATH=$ROOT_DIR/deps/conda/bin:$ROOT_DIR/deps/conda/bin/Scripts:$ROOT_DIR/deps/local/bin:$PATH
else
  export PATH=$ROOT_DIR/deps/env/bin:$ROOT_DIR/deps/local/bin:$PATH
fi
echo TURI_ROOT=$TURI_ROOT
echo TURI_BUILD_ROOT=$TURI_BUILD_ROOT
echo PYTHONPATH=$PYTHONPATH
echo PYTHONHOME=$PYTHONHOME
echo PYTHON_EXECUTABLE=$PYTHON_EXECUTABLE
echo PYTEST_EXECUTABLE=$PYTEST_EXECUTABLE
echo PIP_EXECUTABLE=$PIP_EXECUTABLE
