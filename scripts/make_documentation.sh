#!/bin/bash

set -e
set -x

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=${SCRIPT_DIR}/..
TARGET_DIR=${WORKSPACE}/target

cd ${WORKSPACE}
pip install target/turicreate-*.whl

OUTPUT_DIR=${TARGET_DIR}/docs
mkdir -p ${OUTPUT_DIR}

echo -e "\n\n\n================= Generating API Docs ================\n\n\n"
pip install sphinx==1.6.5 sphinx-bootstrap-theme numpydoc
rm -rf pydocs
mkdir -p pydocs
cd pydocs
cp -R ${WORKSPACE}/src/python/doc/* .
make clean
make html
mv build/html ${OUTPUT_DIR}/api

echo -e "\n\n\n================= Generating User Guide ================\n\n\n"
cd ${WORKSPACE}/userguide
npm install
npm run build
mv _book/ ${OUTPUT_DIR}/userguide

echo -e "\n\n\n================= Generating C++ Docs ================\n\n\n"
cd ${WORKSPACE}
doxygen
mv doc/html/ ${OUTPUT_DIR}/cpp

cd ${TARGET_DIR}
zip -r docs.zip docs
