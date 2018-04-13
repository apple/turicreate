#!/bin/bash

set -e

# Force LD_LIBRARY_PATH to look up from deps
# Otherwise, binaries run during compilation will prefer system libraries,
# which might not use the correct glibc version.

PYTHON_SCRIPTS=deps/env/bin
if [[ $OSTYPE == msys ]]; then
  PYTHON_SCRIPTS=deps/conda/bin/Scripts
fi

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=${SCRIPT_DIR}/..
build_type="release"
#export LD_LIBRARY_PATH=${ROOT_DIR}/deps/local/lib:${ROOT_DIR}/deps/local/lib64:$LD_LIBRARY_PATH

print_help() {
  echo "Builds the release branch and produce a wheel to the targets directory "
  echo
  echo "Usage: ./make_wheel.sh --version=[version] [remaining options]"
  echo
  echo "  --build_number=[version] The build number of the wheel, e.g. 123. Or 123.gpu"
  echo
  echo "  --skip_test              Skip unit tests (Python & C++)."
  echo
  echo "  --skip_cpp_test          Skip C++ unit tests. C++ tests are default skipped on Windows."
  echo
  echo "  --skip_build             Skip the build process."
  echo
  echo "  --skip_doc               Skip the generation of documentation."
  echo
  echo "  --debug                  Use debug build instead of release."
  echo
  echo "  --num_procs=n            Specify the number of proceses to run in parallel."
  echo
  echo "  --target-dir=[dir]       The directory where the wheel and associated files are put."
  echo
  echo "  --python2.7              Use Python 2.7 (default)."
  echo
  echo "  --python3.5              Use Python 3.5, default is Python 2.7."
  echo
  echo "  --python3.6              Use Python 3.6, default is Python 2.7."
  echo
  echo "Produce a local wheel and skip test and doc generation"
  echo "Example: ./make_wheel.sh --skip_test"
  exit 1
} # end of print help

python27=0
python35=0
python36=0

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
    --python2.7)            python27=1;;
    --python3.5)            python35=1;;
    --python3.6)            python36=1;;
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
archive_file_ext="whl"
if [[ $OSTYPE == msys ]]; then
  # C++ tests are default skipped on windows due to annoying issues around
  # unable to find libstdc++.dll which are not so easily fixable
  SKIP_CPP_TEST=1
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

  if [[ $(($python27 + $python35 + $python36)) -gt 1 ]]; then
    echo "Two or more versions of Python specified. Pick one."
    exit 1
  fi

  if [[ "$python35" == "1" ]]; then
      ./configure --python3.5
  elif [[ "$python36" == "1" ]]; then
      ./configure --python3.6
  else
      ./configure --python2.7
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
  echo -e "\n\n\n================= Running C++ Unit Tests ================\n\n\n"
  cd ${WORKSPACE}/${build_type}/test
  push_ld_library_path
  ${WORKSPACE}/scripts/run_cpp_tests.py -j${NUM_PROCS}
  pop_ld_library_path
  echo -e "\n\n\n================= Done C++ Unit Tests ================\n\n\n"
}

# Run all unit test
unit_test() {
  echo -e "\n\n\n================= Running Python Unit Tests ================\n\n\n"

  cd ${WORKSPACE}
  scripts/run_python_test.sh ${build_type}
  echo -e "\n\n================= Done Python Unit Tests ================\n\n"
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


### Package the release folder into a wheel, and strip the binaries ###
package_wheel() {
  if [[ $OSTYPE == darwin* ]]; then
    mac_patch_rpath
  fi
  echo -e "\n\n\n================= Packaging Wheel  ================\n\n\n"
  cd ${WORKSPACE}/${build_type}/src/unity/python
  # cleanup old builds
  rm -rf dist

  # strip binaries
  if [[ ! $OSTYPE == darwin* ]]; then
    cd ${WORKSPACE}/${build_type}/src/unity/python/turicreate
    BINARY_LIST=`find . -type f -exec file {} \; | grep x86 | cut -d: -f 1`
    echo "Stripping binaries: $BINARY_LIST"

    # make newline the separator for items in for loop - default is whitespace
    OLD_IFS=${IFS}
    IFS=$'\n'

    for f in $BINARY_LIST; do
      if [ $OSTYPE == "msys" ] && [ $f == "./pylambda_worker.exe" ]; then
        echo "Skipping pylambda_worker"
      else
        echo "Stripping $f"
        strip -s $f;
      fi
    done

    # set IFS back to default
    IFS=${OLD_IFS}
    cd ..
  fi

  cd ${WORKSPACE}/${build_type}/src/unity/python
  rm -rf build
  dist_type="bdist_wheel"

  VERSION_NUMBER=`${PYTHON_EXECUTABLE} -c "import turicreate; print(turicreate.version_info.version)"`
  ${PYTHON_EXECUTABLE} setup.py ${dist_type} # This produced an wheel starting with turicreate-${VERSION_NUMBER} under dist/

  rm -f turicreate/util/turicreate_env.py

  cd ${WORKSPACE}

  WHEEL_PATH=`ls ${WORKSPACE}/${build_type}/src/unity/python/dist/turicreate-${VERSION_NUMBER}*.whl`
  if [[ $OSTYPE == darwin* ]]; then
    # Change the platform tag embedded in the file name
    temp=`echo $WHEEL_PATH | perl -ne 'print m/(^.*-).*$/'`
    temp=${temp/-cpdarwin-/-cp35m-}
    platform_tag="macosx_10_12_intel.macosx_10_12_x86_64"
    NEW_WHEEL_PATH=${temp}${platform_tag}".whl"
    mv ${WHEEL_PATH} ${NEW_WHEEL_PATH}
    WHEEL_PATH=${NEW_WHEEL_PATH}
  fi

  # Set Python Language Version Number
  NEW_WHEEL_PATH=${WHEEL_PATH}
  if [[ "$python35" == "1" ]]; then
      NEW_WHEEL_PATH=${WHEEL_PATH/-py3-/-cp35-}
  elif [[ "$python36" == "1" ]]; then
      NEW_WHEEL_PATH=${WHEEL_PATH/-py3-/-cp36-}
  else
      NEW_WHEEL_PATH=${WHEEL_PATH/-py2-/-cp27-}
  fi
  if [[ ! ${WHEEL_PATH} == ${NEW_WHEEL_PATH} ]]; then
      mv ${WHEEL_PATH} ${NEW_WHEEL_PATH}
      WHEEL_PATH=${NEW_WHEEL_PATH}
  fi

  # Install the wheel and do a sanity check
  $PIP_EXECUTABLE install --force-reinstall --ignore-installed ${WHEEL_PATH}
  unset PYTHONPATH
  $PYTHON_EXECUTABLE -c "import turicreate; turicreate.SArray(range(100)).apply(lambda x: x)"

  # Done copy to the target directory
  cp $WHEEL_PATH ${TARGET_DIR}/
  echo -e "\n\n================= Done Packaging Wheel  ================\n\n"
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
  tar -czf ${TARGET_DIR}/turicreate_python_sphinx_docs.tar.gz *
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

package_wheel

if [[ -z $SKIP_DOC ]]; then
  generate_docs
fi
