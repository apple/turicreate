#!/bin/bash -e


function print_help {
  echo "Configures and builds the C API as a standalone library."
  echo
  echo "Usage: ./build.sh [build options] [configure options]"
  echo
  echo "  --target-dir, -t                 The target directory to install artifacts to."
  echo "                                   default: `pwd`/target."
  echo
  echo "  --release,-r                     Build in release mode."
  echo "  --debug,-d                       Build in debug mode (default)."
  echo
  echo "  --jobs,-j                        The number of parallel jobs to run."
  echo
  echo "  --cleanup,-c                     Clean up everything before building."
  echo
  echo "  --skip-configure,-s              Skip running ./configure."
  echo
  echo "  -f,--framework                   Build the C-API as a macOS/iOS framework."
  echo "  --framework-path                 The location coded into the capi framework (default @rpath)."
  echo "  --iOS,-i                         Build for iOS (default macOS when on macOS)."
  echo
  echo "  All additional options passed through to ./configure."
  exit 1
} # end of print help

function unknown_target {
  echo "Unrecognized target: $1"
  echo "To get help, run ./configure --help"
  exit 1
} 

function unknown_option {
  echo "Unrecognized option: $1"
  echo "To get help, run ./configure --help"
  exit 1
}

if [[ ${OSTYPE} == darwin* ]] ; then
  apple=1
else
  apple=0
fi


# command flag options
target="capi"
cross_compile="none" # Defaults to the OS running the build
cleanup=0
skip_configure=0
jobs=4
configure_options=""
cmake_options=""
build_mode="release"
target_dir=`pwd`/target
framework_path='@rpath'

###############################################################################
#
# Parse command line configure flags ------------------------------------------
#
while [ $# -gt 0 ]
  do case $1 in
    --framework|-f)         target="capi-framework";;

    --iOS|-i)               cross_compile="iOS";;

    --cleanup|-c)           cleanup=1;;

    --skip-configure|-s)    skip_configure=1;;

    --release|-r)           build_mode="release";;
    --debug|-d)             build_mode="debug";;

    --target-dir=*)         target_dir="${1##--target-dir=}" ;;
    --target-dir|-t)        target_dir="$2"; shift ;;

    --jobs=*)               jobs=${1##--jobs=} ;;
    --jobs|-j)              jobs=$2; shift ;;

    --framework-path)       framework_path="$2"; shift ;;

    --help)                 print_help; exit 0;;

    -D)                     configure_options="${configure_options} -D $2"; shift ;;

    *)                      configure_options="${configure_options} $1";; 
  esac
  shift
done

build_dir=`pwd`/${build_mode}
src_dir=`pwd`


echo "Setting up build:"

echo "build_mode     = ${build_mode}"
echo "target_dir     = ${target_dir}"
echo "target         = ${target}"
echo "cross_compile  = ${cross_compile}"
echo "build_dir      = ${build_dir}"

function run_cleanup {
  ./configure --cleanup --yes || exit 1
}

function run_configure {
  if [[ ${skip_configure} -eq 0 ]] ; then
   ./configure ${configure_options} $@ || exit 1
 else
   echo "Skipping configure script as requested."
 fi
}

if [[ ${cleanup} -eq 1 ]]; then
  run_cleanup
fi

if [[ "$cross_compile" == "iOS" ]]; then
  ios_flag=--target-ios
fi

function build_capi {
  echo
  echo "Building C-API"
  echo
  run_configure ${ios_flag} --with-capi --no-python --no-visualization --release-opt-for-size || exit 1
  install_dir=${target_dir}/capi
  rm -rf ${target_dir}/capi
  mkdir -p ${target_dir}/capi

  cd ${build_dir}/src/capi && make -j ${jobs} || exit 1
  echo "Installing C API header and shared library to ${install_dir}."
  cp lib*.* ${install_dir} || exit 1
  cp ${src_dir}/src/capi/TuriCreate.h ${install_dir}/TuriCreate.h || exit 1

  if [[ $build_mode == release ]] ; then
    echo "Stripping local and debug symbols."
    strip -S -x ${install_dir}/lib*.* || echo "Non-fatal error stripping symbols."
  fi
}

function build_capi_framework {
  echo
  echo "Building C-API as macOS/iOS Framework"
  echo

  run_configure --with-capi-framework ${ios_flag} --no-python --no-visualization -D TC_CAPI_FRAMEWORK_PATH=\"${framework_path}\" --release-opt-for-size || exit 1
  mkdir -p ${target_dir}
  cd ${build_dir}/src/capi || exit 1
  make -j ${jobs} || exit 1

  echo "Installing C API Framework to ${target_dir}."

  rsync -a --delete ${build_dir}/src/capi/TuriCreate.framework/ ${target_dir}/TuriCreate.framework/ || exit 1

  if [[ $build_mode == release ]] ; then
    echo "Stripping local and debug symbols."
    strip -S -x ${target_dir}/TuriCreate.framework/Versions/Current/TuriCreate || echo "Non-fatal error stripping symbols."
  fi

}




case $target in
  capi)                   build_capi;;
  capi-framework)         build_capi_framework;;
  *)                      echo "NOT IMPLEMENTED" && exit 1;;
esac

