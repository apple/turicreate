#!/bin/bash -e


function print_help {
  echo "Configures and builds a specified target."
  echo
  echo "Usage: ./build.sh <target> [options] "
  echo
  echo "Values for <target>: "
  echo
  echo "   python-egg                      Build python egg (default)."
  echo
  echo "Common Options:"
  echo "  --target-dir, -t                 The target directory to install artifacts to."
  echo "                                   default: `pwd`/targets."
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
  echo "  -C <option>                      Option passed to the configure script."
  echo
  echo "  -D <cmake option>                Option passed through to the CMake build."
  echo
  echo "python-egg options"
  echo "------------------------------------------------------------------------"
  echo
  echo "  --build-number                   Set build number.  "
  echo "                                   Defaults to part of git commit hash. "
  echo
  echo "Example: ./build.sh python-framework && cp -R targets/TuriCore.framework."
  echo
  exit 1
} # end of print help

function unknown_target {
  echo "Unrecognized target: $1"
  echo "To get help, run ./configure --help"
  exit 1
} # end of unknown option

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
target="python-egg"
cleanup=0
skip_configure=0
jobs=4
configure_options=""
cmake_options=""
build_mode="debug"
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
    python)                 target="python";;

    python-egg)             target="python-egg";;

    --cleanup|-c)           cleanup=1;;

    --skip-configure|-s)    skip_configure=1;;

    --copy-links)           copy_links=1;;

    --no-sudo)              no_sudo=1 ;;

    --build-number=*)       build_number=${1##--build-number=} ;;
    --build-number)         build_number="$2"; shift;;

    --target-dir=*)         target_dir="${1##--target-dir=}" ;;
    --target-dir|-t)        target_dir="$2"; shift ;;

    --jobs=*)               jobs=${1##--jobs=} ;;
    --jobs|-j)              jobs=$2; shift ;;

    --help)                 print_help; exit 0;;

    -C)                     configure_options="${configure_options} $2"; shift ;;
    -D)                     configure_options="${configure_options} -D $2"; shift ;;

    *) unknown_option $1 ;;
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
  exit 0
fi

function build_python_egg {
  run_configure --with-python

  install_dir=${target_dir}/python
  rm -rf ${target_dir}/python
  mkdir -p ${target_dir}/python

  bash scripts/make_wheel.sh --skip_test --skip_cpp_test --build_number="$build_number" --num_procs=${jobs} --${build_mode} --target-dir="${install_dir}"
  pushd ${build_mode}/src

  if [[ $apple -eq 1 ]]; then
    find . -type f -name '*.dylib' -o -name '*.so' | xargs strip -x -
  else
    find . -type f -name '*.so' | xargs strip -s
  fi

  find . -type f -name '*.dylib' -o -name '*.so' | xargs tar cvzf ${install_dir}/shared_objects.tar.gz
}


function prepare_install_directory {

  inst_dir=$1

  if [[ ! -d $inst_dir ]] ; then
    >&2 echo  "Install sysroot \"${inst_dir}\" not a valid destination; creating."
    mkdir -p ${inst_dir}
    if [[ $? -ne 0 ]] ; then
      echo "Failure creating destination ${inst_dir}."
      exit 1
    fi
    echo ""
    return
  fi

  if [[ $no_sudo -eq 1 ]] ; then
    echo ""
    return
  fi

  _dst_owner=`ls -ld ${inst_dir} | awk '{ print $3 }' || echo ""`

  if [[ $_dst_owner == `whoami` ]] ; then
    echo ""
  else
    echo "sudo"
  fi
}


case $target in
  python-egg)             build_python_egg;;
  *)                      echo "NOT IMPLEMENTED" && exit 1;;
esac
