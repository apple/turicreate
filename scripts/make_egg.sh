#!/bin/bash

set -e

# Force LD_LIBRARY_PATH to look up from deps
# Otherwise, binaries run during compilation will prefer system libraries,
# which might not use the correct glibc version.
# This seems to repro on Ubuntu 11.10 (Oneiric).

PYTHON_SCRIPTS=deps/env/bin
if [[ $OSTYPE == msys ]]; then
  PYTHON_SCRIPTS=deps/conda/bin/Scripts
fi

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=${SCRIPT_DIR}/..
ABS_WORKSPACE=`dirname $SCRIPT_DIR`
build_type="release"
export TURICREATE_USERNAME=''
#export LD_LIBRARY_PATH=${ROOT_DIR}/deps/local/lib:${ROOT_DIR}/deps/local/lib64:$LD_LIBRARY_PATH

print_help() {
  echo "Builds the release branch and produce an egg to the targets directory "
  echo
  echo "Usage: ./make_egg.sh --version=[version] [remaining options]"
  echo
  echo "  --build_number=[version] The build number of the egg, e.g. 123. Or 123.gpu"
  echo
  echo "  --skip_test              Skip unit test and doc generation."
  echo
  echo "  --skip_cpp_test          Skip C++ unit tests. C++ tests are default skipped on Windows."
  echo
  echo "  --skip_build             Skip the build process"
  echo
  echo "  --skip_doc               Skip the doc generation"
  echo
  echo "  --debug                  Use debug build instead of release"
  echo
  echo "  --num_procs=n            Specify the number of proceses to run in parallel"
  echo
  echo "  --target-dir=[dir]       The directory where the egg and associated files are put."
  echo
  echo "Produce an local egg and skip test and doc generation"
  echo "Example: ./make_egg.sh --skip_test"
  exit 1
} # end of print help

# command flag options
# Parse command line configure flags ------------------------------------------
while [ $# -gt 0 ]
  do case $1 in
    --build_number=*)       BUILD_NUMBER=${1##--build_number=} ;;
    --toolchain=*)          toolchain=${1##--toolchain=} ;;
    --target-dir=*)         TARGET_DIR=${1##--target-dir=} ;;
    --num_procs=*)          NUM_PROCS=${1##--num_procs=} ;;
    --skip_test)            SKIP_TEST=1;;
    --skip_cpp_test)        SKIP_CPP_TEST=1;;
    --skip_build)           SKIP_BUILD=1;;
    --skip_doc)             SKIP_DOC=1;;
    --release)              build_type="release";;
    --debug)                build_type="debug";;
    --help)                 print_help ;;
    *) unknown_option $1 ;;
  esac
  shift
done


set -x

if [[ -z "${BUILD_NUMBER}" ]]; then
  BUILD_NUMBER="0"
fi

if [[ -z "${NUM_PROCS}" ]]; then
  NUM_PROCS=4
fi

if [[ -z "${TARGET_DIR}" ]]; then
  TARGET_DIR=${WORKSPACE}/target
fi

mkdir -p ${TARGET_DIR}

# Windows specific
archive_file_ext="tar.gz"
if [[ $OSTYPE == msys ]]; then
  # C++ tests are default skipped on windows due to annoying issues around
  # unable to find libstdc++.dll which are not so easily fixable
  SKIP_CPP_TEST=1
  archive_file_ext="zip"
fi

push_ld_library_path() {
  OLD_LIBRARY_PATH=$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH=${WORKSPACE}/deps/local/lib:${WORKSPACE}/deps/local/lib64:$LD_LIBRARY_PATH
}

pop_ld_library_path() {
  export LD_LIBRARY_PATH=$OLD_LIBRARY_PATH
}

### Build the source with version number ###
build_source() {
  echo -e "\n\n\n================= Build Release: VERSION ${BUILD_NUMBER} ================\n\n\n"
  # cleanup build
  rm -rf ${WORKSPACE}/${build_type}/src/unity/python

  # Configure
  cd ${WORKSPACE}

  # ./configure --cleanup --yes

  PY_MAJOR_VERSION=`python -V 2>&1 | perl -ne 'print m/^Python (\d\.\d)/'`
  if [[ $PY_MAJOR_VERSION == 3.4 ]]; then
      ./configure --python3
  elif [[ $PY_MAJOR_VERSION == 3.5 ]]; then
      ./configure --python3.5
  else
      ./configure
  fi

  push_ld_library_path

  # Make clean
  cd ${WORKSPACE}/${build_type}/src/unity/python
  make clean_python
  # Make unity
  cd ${WORKSPACE}/${build_type}/src/unity
  make -j${NUM_PROCS}

  # make test
  if [[ -z $SKIP_CPP_TEST ]]; then
    cd ${WORKSPACE}/test
    touch $(find . -name "*.cxx")
    cd ${WORKSPACE}/${build_type}/test
    make -j${NUM_PROCS}
  fi
  pop_ld_library_path

  echo -e "\n\n================= Done Build Source ================\n\n"
}


# Run all unit test
cpp_test() {
  echo -e "\n\n\n================= Running Unit Test ================\n\n\n"
  cd ${WORKSPACE}/${build_type}
  push_ld_library_path
  ${WORKSPACE}/scripts/run_cpp_tests.py -j 1
  pop_ld_library_path
}

# Run all unit test
unit_test() {
  echo -e "\n\n\n================= Running Unit Test ================\n\n\n"

  cd ${WORKSPACE}
  scripts/run_python_test.sh ${build_type}
  echo -e "\n\n================= Done Unit Test ================\n\n"
}

mac_patch_rpath() {
  echo -e "\n\n\n================= Patching Mac RPath ================\n\n\n"
  # on mac we need to do a little work to fix up RPaths
  cd ${WORKSPACE}/${build_type}/src/unity/python
  # - look for all files
  # - run 'file' on it
  # - look for binary files (shared libraries, executables)
  # - output is in the form [file]: type, so cut on ":", we just want the file
  flist=`find . -type f -not -path "*/CMakeFiles/*" -not -path "./dist/*" | xargs -L 1 file | grep x86_64 | cut -f 1 -d :`

  # change libpython2.7 link to @rpath/libpython2.7
  # Empirically, it could be one of either
  for f in $flist; do
    install_name_tool -change libpython2.7.dylib @rpath/libpython2.7.dylib $f || true
    install_name_tool -change /System/Library/Frameworks/Python.framework/Versions/2.7/Python @rpath/libpython2.7.dylib $f || true
  done
  # We are generally going to be installed in
  # a place of the form
  # PREFIX/lib/python2.7/site-packages/[module]
  # libpython2.7 will probably in PREFIX/lib
  # So for instance if I am in
  # PREFIX/lib/python2.7/site-packages/turicreate/unity_server
  # I need to add
  # ../../../ to the rpath (to make it to lib/)
  # But if I am in
  #
  # PREFIX/lib/python2.7/site-packages/turicreate/something/somewhere
  # I need to add
  # ../../../../ to the rpath
  for f in $flist; do
    # Lets explain the regexes
    # - The first regex removes "./"
    # - The second regex replaces the string "[something]/" with "../"
    #   (making sure something does not contain '/' characters)
    # - The 3rd regex removes the last "filename" bit
    #
    # Example:
    # Input: ./turicreate/cython/cy_ipc.so
    # After 1st regex: turicreate/cython/cy_ipc.so
    # After 2nd regex: ../../cy_ipc.so
    # After 3rd regex: ../../
    relative=`echo $f | sed 's:\./::g' | sed 's:[^/]*/:../:g' | sed 's:[^/]*$::'`
    # Now we need ../../ to get to PREFIX/lib
    rpath="@loader_path/../../$relative"
    install_name_tool -add_rpath $rpath $f || true
  done
}


### Package the release folder into an egg tarball, and strip the binaries ###
package_egg() {
  if [[ $OSTYPE == darwin* ]]; then
    mac_patch_rpath
  fi
  echo -e "\n\n\n================= Packaging Egg  ================\n\n\n"
  cd ${WORKSPACE}/${build_type}/src/unity/python
  # cleanup old builds
  rm -rf *.egg-info
  rm -rf dist

  # strip binaries
  if [[ ! $OSTYPE == darwin* ]]; then
    cd ${WORKSPACE}/${build_type}/src/unity/python/turicreate
    BINARY_LIST=`find . -type f -exec file {} \; | grep x86 | cut -d: -f 1`
    echo "Stripping binaries: $BINARY_LIST"
    for f in $BINARY_LIST; do
      if [ $OSTYPE == "msys" ] && [ $f == "./pylambda_worker.exe" ]; then
        echo "Skipping pylambda_worker"
      else
        strip -s $f;
      fi
    done
    cd ..
  fi

  cd ${WORKSPACE}/${build_type}/src/unity/python
  rm -rf build
  dist_type="sdist"
  if [[ $OSTYPE == darwin* ]] || [[ $OSTYPE == msys ]]; then
    dist_type="bdist_wheel"
  fi

  VERSION_NUMBER=`python -c "import turicreate; print(turicreate.version_info.version)"`
  ${PYTHON_EXECUTABLE} setup.py ${dist_type} # This produced an egg/wheel starting with turicreate-${VERSION_NUMBER} under dist/

  rm -f turicreate/util/turicreate_env.py

  cd ${WORKSPACE}

  EGG_PATH=${WORKSPACE}/${build_type}/src/unity/python/dist/turicreate-${VERSION_NUMBER}.${archive_file_ext}
  if [[ $OSTYPE == darwin* ]]; then
    # Change the platform tag embedded in the file name
    EGG_PATH=`ls ${WORKSPACE}/${build_type}/src/unity/python/dist/turicreate-${VERSION_NUMBER}*.whl`
    temp=`echo $EGG_PATH | perl -ne 'print m/(^.*-).*$/'`
    temp=${temp/-cpdarwin-/-cp35m-}
    platform_tag="macosx_10_5_x86_64.macosx_10_6_intel.macosx_10_9_intel.macosx_10_9_x86_64.macosx_10_10_intel.macosx_10_10_x86_64.macosx_10_11_intel.macosx_10_11_x86_64"
    NEW_EGG_PATH=${temp}${platform_tag}".whl"
    mv ${EGG_PATH} ${NEW_EGG_PATH}
    EGG_PATH=${NEW_EGG_PATH}
  elif [[ $OSTYPE == msys ]]; then
    EGG_PATH=`ls ${WORKSPACE}/${build_type}/src/unity/python/dist/turicreate-${VERSION_NUMBER}-*-none-*.whl`

    NEW_EGG_PATH=${EGG_PATH/-any./-win_amd64.}
    if [[ ! ${EGG_PATH} == ${NEW_EGG_PATH} ]]; then
        mv ${EGG_PATH} ${NEW_EGG_PATH}
        EGG_PATH=${NEW_EGG_PATH}
    fi
  elif [[ $OSTYPE == linux-* ]]; then
    PYTHON_VERSION=`$PYTHON_EXECUTABLE -V 2>&1 | perl -ne 'print m/^Python (\d\.\d)/'`
    NEW_EGG_PATH=${WORKSPACE}/${build_type}/src/unity/python/dist/turicreate-${VERSION_NUMBER}-py${PYTHON_VERSION}.${archive_file_ext}
    mv ${EGG_PATH} ${NEW_EGG_PATH}
    EGG_PATH=${NEW_EGG_PATH}
  fi

  # Set Python Language Version Number
  NEW_EGG_PATH=${EGG_PATH}
  if [[ $PY_MAJOR_VERSION == 3.4 ]]; then
      NEW_EGG_PATH=${EGG_PATH/-py3-/-cp34-}
  elif [[ $PY_MAJOR_VERSION == 3.5 ]]; then
      NEW_EGG_PATH=${EGG_PATH/-py3-/-cp35-}
  elif [[ $PY_MAJOR_VERSION == 2.7 ]]; then
      NEW_EGG_PATH=${EGG_PATH/-py2-/-cp27-}
  fi
  if [[ ! ${EGG_PATH} == ${NEW_EGG_PATH} ]]; then
      mv ${EGG_PATH} ${NEW_EGG_PATH}
      EGG_PATH=${NEW_EGG_PATH}
  fi

  # Install the egg and do a sanity check
  $PIP_EXECUTABLE install --force-reinstall --ignore-installed ${EGG_PATH}
  unset PYTHONPATH
  $PYTHON_EXECUTABLE -c "import turicreate; turicreate.SArray(range(100)).apply(lambda x: x)"

  # Done copy to the target directory
  cp $EGG_PATH ${TARGET_DIR}/
  echo -e "\n\n================= Done Packaging Egg  ================\n\n"
}


# Generate docs
generate_docs() {
  echo -e "\n\n\n================= Generating Docs ================\n\n\n"

  $PIP_EXECUTABLE install sphinx==1.3.0b1
  $PIP_EXECUTABLE install sphinx-bootstrap-theme
  $PIP_EXECUTABLE install numpydoc
  SPHINXBUILD=${WORKSPACE}/$PYTHON_SCRIPTS/sphinx-build
  cd ${WORKSPACE}
  rm -rf pydocs
  mkdir -p pydocs
  cd pydocs
  cp -R ${WORKSPACE}/src/unity/python/doc/* .
  make clean SPHINXBUILD=${SPHINXBUILD}
  make html SPHINXBUILD=${SPHINXBUILD} || true
  tar -czf ${TARGET_DIR}/turicreate_python_sphinx_docs.${archive_file_ext} *
}

set_build_number() {
  # set the build number
  cd ${WORKSPACE}/${build_type}/src/unity/python/
  sed -i -e "s/'.*'#{{BUILD_NUMBER}}/'${BUILD_NUMBER}'#{{BUILD_NUMBER}}/g" turicreate/version_info.py
}

set_git_SHA() {
  # set the git SHA1 revision
  cd ${WORKSPACE}
  GIT_SHA=$(git rev-parse HEAD)
  if [ $? -ne 0 ]; then
    GIT_SHA = "NA"
  fi
  
  cd ${WORKSPACE}/${build_type}/src/unity/python/
  sed -i -e "s/'.*'#{{GIT_SHA}}/'${GIT_SHA}'#{{GIT_SHA}}/g" turicreate/version_info.py
}

# Here is the main function()
if [[ -z $SKIP_BUILD ]]; then
  build_source
fi

if [[ -z $SKIP_TEST ]]; then
  if [[ -z $SKIP_CPP_TEST ]]; then
    cpp_test
  fi
  unit_test
fi

set_build_number
set_git_SHA

source ${WORKSPACE}/scripts/python_env.sh $build_type

package_egg

if [[ -z $SKIP_DOC ]]; then
  generate_docs
fi
