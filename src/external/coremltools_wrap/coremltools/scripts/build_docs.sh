#!/bin/bash

set -e

##=============================================================================
## Main configuration processing
COREMLTOOLS_HOME=$( cd "$( dirname "$0" )/.." && pwd )

# command flag options
PYTHON="3.7"
SOURCE_VERSION=""
RELEASE=0
UPLOAD=0
MAIN_VERSION=0
AUTH_TOKEN=""
CHECK_ENV=1
WHEEL_PATH=""

unknown_option() {
  echo "Unknown option $1. Exiting."
  exit 1
}

print_help() {
  echo "Builds the docs associated with the code"
  echo
  echo "Usage: zsh -i make_docs.sh"
  echo
  echo "  --wheel-path=*          Specify which wheel to use to make docs."
  echo "  --python=*              Python to use for configuration."
  echo "  --upload                Upload these docs with the current coremltools version."
  echo "  --release               Release the uploaded docs with the current coremltools version."
  echo "  --from-source-version=* If a version must be created, use this as the base to copy from.\
                                  Default is the most recent version."
  echo "  --auth-token=*          Auth token for accessing documentation API."
  echo "  --set-main-version      Set the uploaded doc version as the main ('stable release') version."
  echo "  --no-check-env          Don't check the environment to verify it's up to date."
  echo
  exit 1
} # end of print help

# command flag options
# Parse command line configure flags ------------------------------------------
while [ $# -gt 0 ]
  do case $1 in
    --python=*)              PYTHON=${1##--python=} ;;
    --wheel-path=*)          WHEEL_PATH=${1##--wheel-path=} ;;
    --from-source-version=*) SOURCE_VERSION=${1##--from-source-version=} ;;
    --auth-token=*)          AUTH_TOKEN=${1##--auth-token=} ;;
    --upload)                UPLOAD=1 ;;
    --release)               RELEASE_VERSION=1 ;;
    --no-check-env)          CHECK_ENV=0 ;;
    --set-main-version)      MAIN_VERSION=1 ;;
    --help)              print_help ;;
    *) unknown_option $1 ;;
  esac
  shift
done

cd ${COREMLTOOLS_HOME}

if [[ $PYTHON != "None" ]]; then
  # Setup the right python
  if [[ $CHECK_ENV == 1 ]]; then
    zsh -i scripts/env_create.sh --python=$PYTHON --include-docs-deps
  fi
  source scripts/env_activate.sh --python=$PYTHON
fi

echo
echo "Using python from $(which python)"
echo

if [[ $WHEEL_PATH != "" ]]; then
    $PIP_EXECUTABLE install ${WHEEL_PATH} --upgrade
else
    cd ..
    $PIP_EXECUTABLE install -e coremltools --upgrade
    cd ${COREMLTOOLS_HOME}
fi

cd docs
make html
cd ..

if [[ $UPLOAD == 1 ]]; then
  if [[ $AUTH_TOKEN == "" ]]; then
    echo "No auth token provided. Skipping upload."
    pip uninstall coremltools
    exit
  fi

  # Set up base API call
  DOC_COMMAND=(python docs/upload_docs.py --auth_token $AUTH_TOKEN)

  if [[ $SOURCE_VERSION != "" ]]; then
    DOC_COMMAND+=(--from_source_version $SOURCE_VERSION)
  fi
  if [[ $RELEASE == 1 ]]; then
    DOC_COMMAND+=(--release_version)
  fi
  if [[ $MAIN_VERSION == 1 ]]; then
    DOC_COMMAND+=(--set_version_stable)
  fi
  ${DOC_COMMAND[@]}
else
  if [[ $MAIN_VERSION == 1 ]]; then
    echo "You must set release-version to use set-main-version."
  fi
fi

pip uninstall -y coremltools # We're using the build env for this script, so uninstall the wheel when we're done.
