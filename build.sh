#!/bin/bash -e


function print_help {
  echo "Configures and builds a specified target."
  echo
  echo "Usage: ./build.sh <target> [options] "
  echo
  echo "Values for <target>: "
  echo
  echo    python                           Build python, inplace.
  echo    python-egg                       Build python egg.
  echo    capi                             Build C-API.
  echo    capi-framework                   Build C-API as OSX Framework.
  echo    capi-framework-tests             Build C-API Framework tests
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

  echo "capi-framework options"
  echo "------------------------------------------------------------------------"
  echo "  --install-sysroot,-i             Also install headers/libraries to this directory. "
  echo "                                   If target is a framework, it will be installed to "
  echo "                                   <install-sysroot>/<name>.framework. If given multiple times, "
  echo "                                   copies will be installed in each."
  echo
  echo "  --system-framework               Prepares the framework to be put in "
  echo "                                   /System/Library/PrivateFrameworks/."
  echo
  echo "  --no-sudo                        Disable use of sudo when installing."
  echo
  echo "  --copy-links                     Do not use links in installing XCode framework (workaround for XCode bug)."
  echo
  echo
  echo "Example: ./build.sh capi-framework && cp -R targets/TuriCore.framework."
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
target=""
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

    capi)                   target="capi";;

    capi-framework)         target="capi-framework";;

    capi-framework-tests)   target="capi-framework-tests";;

    --cleanup|-c)           cleanup=1;;

    --skip-configure|-s)    skip_configure=1;;

    --copy-links)           copy_links=1;;

    --install-sysroot=*)    install_sysroot="${install_sysroot}
                                             ${1##--install-sysroot=}" ;;

    --install-sysroot|-i)   install_sysroot="${install_sysroot}
                                             $2"; shift;;

    --system-framework)     configure_options="${configure_options} -D TC_CAPI_AS_SYSTEM_FRAMEWORK=1"; install_sysroot="${install_sysroot} /System/Library/PrivateFrameworks/";;

    --release|-r)           build_mode="release";;
    --debug|-d)             build_mode="debug";;

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
  run_configure --with-python --no-capi

  install_dir=${target_dir}/python
  rm -rf ${target_dir}/python
  mkdir -p ${target_dir}/python

  bash scripts/make_egg.sh --skip_test --skip_cpp_test --build_number="$build_number" --num_procs=${jobs} --${build_mode} --target-dir="${install_dir}"
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

function build_capi {
  echo
  echo "Building C-API"
  echo
  run_configure --with-capi --no-python --no-visualization || exit 1
  install_dir=${target_dir}/capi
  rm -rf ${target_dir}/capi
  mkdir -p ${target_dir}/capi

  cd ${build_dir}/src/capi && make -j ${jobs} || exit 1
  echo "Installing C API header and shared library to ${install_dir}."
  cp libturi.* ${install_dir} || exit 1
  cp ${src_dir}/src/capi/TuriCore.h ${install_dir}/turi.h || exit 1


  if [[ ! $install_sysroot -eq "" ]] ; then

    for inst_dir in ${install_sysroot} ; do

      sudo_cmd=`prepare_install_directory ${inst_dir}`

      if [[ $sudo_cmd == "sudo" ]] ; then
        echo "Copying headers/libraries into $inst_dir/ using sudo."
      else
        echo "Copying headers/libraries into $inst_dir/."
      fi

      ${sudocmd} bash -e -c "
      mkdir -p $inst_dir/include/
      cp ${install_dir}/turi.h  $inst_dir/include/
      mkdir -p $inst_dir/lib/
      cp ${install_dir}/libturi.\*  $inst_dir/lib/"

    done
  fi
}

function build_capi_framework {
  echo
  echo "Building C-API as OSX Framework"
  echo

  run_configure --with-capi-framework --no-python --no-visualization || exit 1
  mkdir -p ${target_dir}
  cd ${build_dir}/src/capi || exit 1
  make -j ${jobs} || exit 1

  echo "Installing C API Framework to ${target_dir}."

  rsync -a --delete ${build_dir}/src/capi/TuriCore.framework/ ${target_dir}/TuriCore.framework/ || exit 1

  for inst_dir in ${install_sysroot} ; do

    sudo_cmd=`prepare_install_directory ${inst_dir}`

    if [[ $sudo_cmd == "sudo" ]] ; then
      echo "Copying framework into $inst_dir/TuriCore.framework using sudo."
    else
      echo "Copying framework into $inst_dir/TuriCore.framework."
    fi

    rsync_error=0
    $sudo_cmd bash -c "
      rsync -rlptDv --delete ${target_dir}/TuriCore.framework/ ${inst_dir}/TuriCore.framework/"

    if [[ $? -ne 0 ]] ; then
      rsync_error=1
    fi

    if [[ $copy_links == 1 ]] ; then
      echo "Copying over links as absolute directories."
      # HAve to copy first over so the symlinks are valid.
      $sudo_cmd bash -c "
        rsync -rLptDv --delete ${target_dir}/TuriCore.framework/ ${inst_dir}/TuriCore.framework/"

      if [[ $? -ne 0 ]] ; then
        rsync_error=1
      fi
    fi

    if [[ $rsync_error == 1 ]] ; then
      echo "WARNING: Possible error installing framework into ${inst_dir}."
    fi

  done
}

function build_capi_tests {
  echo
  echo "Building C-API Tests"
  echo

  run_configure --with-capi-framework --no-python --no-visualization || exit 1
  mkdir -p ${target_dir}
  cd ${build_dir}/test/capi || exit 1
  make -j ${jobs} || exit 1

  echo "Gathering ctest, cxxtest binaries and creating tarball"
  rsync -rLptDv --delete-excluded --include '*.cxxtest' --exclude '*'  ${build_dir}/test/capi/ ${target_dir}/Tests/ || exit 1
  rsync -rLptDv ${build_dir}/../deps/local/bin/ctest ${target_dir}/Tests/
  pushd ${target_dir}/Tests/
  tar -czvf turi_capi_tests.tar.gz ./*
  rm ./*.cxxtest ./ctest
  popd

  for inst_dir in ${install_sysroot} ; do

    sudo_cmd=`prepare_install_directory ${inst_dir}`

    if [[ $sudo_cmd == "sudo" ]] ; then
      echo "Copying tests into $inst_dir using sudo."
    else
      echo "Copying tests into $inst_dir."
    fi

    rsync_error=0
    $sudo_cmd bash -c "
      rsync -rlptDv --delete ${target_dir}/Tests/ ${inst_dir}"

    if [[ $? -ne 0 ]] ; then
      rsync_error=1
    fi

    if [[ $copy_links == 1 ]] ; then
      echo "Copying over links as absolute directories."
      # HAve to copy first over so the symlinks are valid.
      $sudo_cmd bash -c "
        rsync -rLptDv --delete ${target_dir}/Tests/ ${inst_dir}"

      if [[ $? -ne 0 ]] ; then
        rsync_error=1
      fi
    fi

    if [[ $rsync_error == 1 ]] ; then
      echo "WARNING: Possible error installing framework into ${inst_dir}."
    fi

  done


}





case $target in
  python-egg)             build_python_egg;;
  capi)                   build_capi;;
  capi-framework)         build_capi_framework;;
  capi-framework-tests)   build_capi_tests;;
  *)                      echo "NOT IMPLEMENTED" && exit 1;;
esac
