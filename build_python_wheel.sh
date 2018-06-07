#!/bin/bash -e


function print_help {
  echo "Configures and builds a specified target."
  echo
  echo "Usage: ./build.sh [build options] [configure options]"
  echo
  echo "Common Options:"
  echo "  --target-dir, -t                 The target directory to install artifact to."
  echo "                                   default: `pwd`/targets."
  echo
  echo "  --release                        Build in release mode."
  echo "  --debug                          Build in debug mode (default)."
  echo
  echo "  --jobs,-j                        The number of parallel jobs to run."
  echo
  echo "  --cleanup,-c                     Clean up everything before building."
  echo
  echo "  --skip-configure,-s              Skip running ./configure."
  echo
  echo "  --build-number                   Set build number.  "
  echo "                                   Defaults to part of git commit hash. "
  echo
  echo "  All additional options passed through to ./configure."
  echo
  exit 1
} # end of print help

function unknown_option {
  echo "Unrecognized option: $1"
  echo "To get help, run ./configure --help"
  exit 1
} # end of unknown option

if [[ ${OSTYPE} == darwin* ]] ; then
  apple=1
else
  apple=0
fi


# command flag options
cleanup=0
skip_configure=0
jobs=4
configure_options=""
build_mode="release"
target_dir=`pwd`/targets
install_sysroot=""
no_sudo=0
copy_links=0
build_number=`git rev-parse --short HEAD || echo "NULL"`

###############################################################################
#
# Parse command line configure flags ------------------------------------------
#
while [ $# -gt 0 ]
  do case $1 in

    --cleanup|-c)           cleanup=1;;

    --skip-configure|-s)    skip_configure=1;;

    --copy-links)           copy_links=1;;

    --build-number=*)       build_number=${1##--build-number=} ;;
    --build-number)         build_number="$2"; shift;;

    --target-dir=*)         target_dir="${1##--target-dir=}" ;;
    --target-dir|-t)        target_dir="$2"; shift ;;

    --jobs=*)               jobs=${1##--jobs=} ;;
    --jobs|-j)              jobs=$2; shift ;;

    --help)                 print_help; exit 0;;

    -D)                     configure_options="${configure_options} -D $2"; shift ;;

    *)                      configure_options="${configure_options} $1";; 
  esac
  shift
done

build_dir=`pwd`/${build_mode}
src_dir=`pwd`


echo "Setting up build:"
echo "build_mode = ${build_mode}"
echo "target_dir = ${target_dir}"
echo "target     = ${target}"
echo "build_dir  = ${build_dir}"


if [[ ${cleanup} -eq 1 ]]; then
  ./configure --cleanup --yes || exit 1
fi

if [[ ${skip_configure} -eq 0 ]] ; then
  ./configure ${configure_options} --with-python || exit 1
else
  echo "skipping configure script as requested."
fi


install_dir=${target_dir}/python
rm -rf ${target_dir}/python
mkdir -p ${target_dir}/python

bash scripts/make_wheel.sh --skip_test --skip_cpp_test --build_number="$build_number" --num_procs=${jobs} --${build_mode} --target-dir="${install_dir}"


