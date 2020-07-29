#!/bin/bash

set -e

# Defaults
COREMLTOOLS_HOME=$( cd "$( dirname "$0" )/.." && pwd )
COREMLTOOLS_NAME=$(basename $COREMLTOOLS_HOME)

WHEEL_DIR="$COREMLTOOLS_HOME/build/dist"
PYPI="pypi"
PYENV_PY_VERSION="3.7"
CHECK_ENV=1

unknown_option() {
  echo "Unknown option $1. Exiting."
  exit 1
}

print_help() {
  echo "Release the Python CoreMLTools Wheel."
  echo
  echo "Usage: ./release_wheel.sh --wheel-dir=${WHEEL_DIR}"
  echo
  echo "  --wheel-dir               (Optional) Directory in which the wheels sit."
  echo "  --pypi                    (Optional) Name of PyPi repository to release to."
  echo "  --no-check-env            Don't check the environment to verify it's up to date."
  echo
  exit 1
} # end of print help

# command flag options
# Parse command line configure flags ------------------------------------------
while [ $# -gt 0 ]
  do case $1 in
    --wheel-dir=*)       WHEEL_DIR=${1##--wheel-dir=} ;;
    --pypi=*)            PYPI=${1##--pypi=} ;;
    --no-check-env)      CHECK_ENV=0 ;;
    --help)              print_help ;;
    *) unknown_option $1 ;;
  esac
  shift
done

echo "------------------------------------------------------------------------------"
echo "Releasing CoreML Tools"
echo "------------------------------------------------------------------------------"


cd $COREMLTOOLS_HOME

# Setup the python env
if [[ $CHECK_ENV == 1 ]]; then
  zsh -i scripts/env_create.sh --python=$PYENV_PY_VERSION --exclude-test-deps
fi

source scripts/env_activate.sh --python=$PYENV_PY_VERSION
pip install twine

# Setup the wheel
rm -rf dist
python setup.py sdist
cp $WHEEL_DIR/*.whl dist/.
twine check dist/*

# Disabled. For now, we treat "release" as a collection job.
# Upload the wheel
# twine upload --config-file ~/.pypirc --repository $PYPI $1/dist/*
