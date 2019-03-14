#!/bin/bash
set -x
set -e

# The build image version that will be used for testing
TC_BUILD_IMAGE_VERSION=1.0.2

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=${SCRIPT_DIR}/..
cd ${WORKSPACE}

date
(test -d deps/env) || ./scripts/install_python_toolchain.sh
date
source deps/env/bin/activate
date
pip install target/turicreate-*.whl
date

PYTHON="$PWD/deps/env/bin/python"
PYTHON_MAJOR_VERSION=$(${PYTHON} -c 'import sys; print(sys.version_info.major)')
PYTHON_MINOR_VERSION=$(${PYTHON} -c 'import sys; print(sys.version_info.minor)')
PYTHON_VERSION="python${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}"
cp -a src/unity/python/turicreate/test deps/env/lib/${PYTHON_VERSION}/site-packages/turicreate/
cd deps/env/lib/${PYTHON_VERSION}/site-packages/turicreate/test

# run tests
${PYTHON} -m pytest -v --junit-xml=../../../../../../../pytest.xml

date
