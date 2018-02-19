#!/bin/bash

set -e

PYTHON_SCRIPTS=deps/conda/bin
if [[ $OSTYPE == msys ]]; then
  PYTHON_SCRIPTS=deps/conda/bin/Scripts
fi

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=${SCRIPT_DIR}/..
build_type="release"

print_help() {
  echo "Builds the release branch and produce an egg to the targets directory "
  echo
  echo "Usage: ./make_docs.sh"
  echo
  echo "  --debug      Use the debug build instead of the release build" 
  echo
  echo "Produce an sphinx docs at pydocs/" 
  echo "Example: ./make_pydocs.sh"
  exit 1
} # end of print help

while [ $# -gt 0 ]
  do case $1 in
    --debug)                build_type="debug";;
  esac
  shift
done

set -x

TARGET_DIR=${WORKSPACE}/target
if [[ ! -d "${TARGET_DIR}" ]]; then
  mkdir ${TARGET_DIR}
fi

# Setup the build environment,
# Set PYTHONPATH, PYTHONHOME, PYTHON_EXECUTABLE
source ${WORKSPACE}/scripts/python_env.sh $build_type


# Generate docs
generate_docs() {
  echo -e "\n\n\n================= Generating Docs ================\n\n\n"

  $PIP_EXECUTABLE install sphinx==1.6.5
  $PIP_EXECUTABLE install sphinx-bootstrap-theme
  $PIP_EXECUTABLE install numpydoc
  SPHINXBUILD=${WORKSPACE}/$PYTHON_SCRIPTS/sphinx-build
  cd ${WORKSPACE}
#  rm -rf pydocs
  mkdir -p pydocs
  cd pydocs
  cp -R ${WORKSPACE}/src/unity/python/doc/* .
  make clean SPHINXBUILD=${SPHINXBUILD}
  make html SPHINXBUILD=${SPHINXBUILD}
}

generate_docs

