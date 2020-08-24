#!/bin/bash


##=============================================================================
# Exit immediately on failure of a subcommand
set -e

## Main configuration processing
COREMLTOOLS_HOME=$( cd "$( dirname "$0" )/.." && pwd )
COREMLTOOLS_NAME=$(basename $COREMLTOOLS_HOME)
ENV_DIR="${COREMLTOOLS_HOME}/envs"

# command flag options
cleanup_option=0
include_build_deps=1
include_test_deps=1
include_docs_deps=0
DEV=0
PYTHON="3.7"
force=0

function print_help {
  echo "Configures the build with the specified toolchain. "
  echo
  echo "If env_create has already been run before, running env_create "
  echo "will simply reconfigure the build with no changes. "
  echo
  echo "Usage: zsh -i env_create <options>"
  echo
  echo "  --dev                Setup the environment for development."
  echo "  --exclude-build-deps Exclude packages needed for building"
  echo "  --exclude-test-deps  Exclude packages needed for testing"
  echo "  --force              Rebuild the environment if it exists already."
  echo "  --include-docs-deps  Include packages needed for making docs"
  echo "  --python=*           Python to use for configuration."
  echo
  echo "Example: zsh -i env_create --python==3.7"
  echo
  exit 1
} # end of print help

function unknown_option {
  echo "Unrecognized option: $1"
  echo "To get help, run zsh -i env_create --help"
  exit 1
} # end of unknown option

###############################################################################
#
# Parse command line configure flags ------------------------------------------
#
while [ $# -gt 0 ]
  do case $1 in
    --python=*)            PYTHON=${1##--python=} ;;
    --dev)                 DEV=1;;
    --exclude-build-deps)  include_build_deps=0;;
    --exclude-test-deps)   include_test_deps=0;;
    --include-docs-deps)   include_docs_deps=1;;
    --force)               force=1;;
    --help)                print_help ;;

    *) unknown_option $1 ;;
  esac
  shift
done

if [[ $DEV == 1 ]] then
    ENV_DIR="${ENV_DIR}/${COREMLTOOLS_NAME}-dev-py${PYTHON}"
else
    ENV_DIR="${ENV_DIR}/${COREMLTOOLS_NAME}-py${PYTHON}"
fi

echo "Using python version string $PYTHON"

# Setup a new conda env using the existing python
if conda activate $ENV_DIR && [ ${force} -eq 0 ]
then
  echo "Build environment already exists in $ENV_DIR."
else
  echo "Creating a new conda environment in $ENV_DIR"
  conda create --prefix "$ENV_DIR" python="$PYTHON"
  conda activate $ENV_DIR
fi

# Activate and install packages in the environment
echo "Installing basic build requirements."
if [[ $include_build_deps == 1 ]]; then
    python -m pip install -r $COREMLTOOLS_HOME/reqs/build.pip
fi

# Install test requirements (upgrades packages if required)
if [[ $include_test_deps == 1 ]]; then
  echo "Installing additional test requirements."
  python -m pip install -r $COREMLTOOLS_HOME/reqs/test.pip --upgrade
fi

# Install doc requirements (upgrades packages if required)
if [[ $include_docs_deps == 1 ]]; then
  echo "Installing additional document requirements."
  python -m pip install -r $COREMLTOOLS_HOME/reqs/docs.pip --upgrade
fi

# Install doc requirements (upgrades packages if required)
if [[ $include_docs_deps == 1 ]]; then
  echo "Installing additional document requirements."
  python -m pip install -r $COREMLTOOLS_HOME/reqs/docs.pip --upgrade
fi

if [[ $DEV == 1 ]]; then
  echo "Setting up environment for development."
  python -m pip install -e "$COREMLTOOLS_HOME/../coremltools" --upgrade
fi

conda deactivate

echo
echo
echo
echo "Python env created for coremltools development."
echo
echo "Run the following command to to activate it."
echo
if [[ $DEV == 1 ]]; then
    echo "      source ./scripts/env_activate.sh --python=${PYTHON} --dev"
else
    echo "      source ./scripts/env_activate.sh --python=${PYTHON}"
fi
echo
