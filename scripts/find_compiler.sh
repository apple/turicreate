#!/bin/bash

# Find the compiler toolchain. Prints the command to use to stdout.  All other
# progress messages go to stderr.

# Exit immediately on failure of a subcommand
set -e

function unknown_option {
  >&2 echo "Usage: find_compiler.sh (cxx|cc) --ccache=[0|1] --script-dir=<ScriptDir>]"
  >&2 echo "Unknown option $1"
  exit 1
}

command=""
with_ccache=1
script_dir=""

while [ $# -gt 0 ]
  do case $1 in
    cxx)              command=cxx;;
    cc)               command=cc;;
    --ccache=*)       with_ccache=${1##--ccache=};;
    --script-dir=*)   script_dir=${1##--script-dir=} ;;
    *) unknown_option $1 ;;
  esac
  shift
done

if [[ -z $command ]] ; then
  >&2 echo "Pass in cc or cxx as first argument to request compiler type to find."
  unknown_option "<empty>"
fi

# Set some defaults
if [[ $with_ccache -eq 1 ]] ; then
  if [[ ! -f $CCACHE ]] ; then
    CCACHE=`which ccache || echo ""`
  fi

  if [[ ! -f $CCACHE ]] ; then
    >&2 echo "Warning: ccache executable not found on path; disabling ccache support."
    with_ccache=0
  fi

  if [[ ! -d ${script_dir} ]] ; then
    >&2 echo "Warning: --script-dir must be set to a valid directory to enable ccache."
    with_ccache=0
  fi
fi

# First case -- asked for the cxx compiler
if [[ $command == "cxx" ]] ; then

  # On OSX, use clang by default
  if [ -z $CXX ] && [ $OSTYPE == darwin* ]; then
    CXX=clang++
  fi

  if [[ -z $CXX ]]; then
    if [[ `which cxx` ]]; then
      CXX=cxx
    elif [[ `which c++` ]]; then
      CXX=c++
    elif [[ `which g++` ]]; then
      CXX=g++
    elif [[ `which clang++` ]]; then
      CXX=clang++
    else
      >&2 echo "Compiler not in path with cxx/c++/g++/clang++; set manually with CXX=<comp>"
      exit 1
    fi
  fi

  CXXCMD=`which $CXX || echo ""`

  >&2 echo "Using CXX compiler $CXXCMD."

  if [[ $with_ccache -eq 1 ]]; then
    # Write out the script
    CXXSCR="$script_dir/cxx"

    rm -f $CXXSCR

    echo "#!/bin/bash -e
    $CCACHE $CXXCMD \$@
    exit \$?
    " > $CXXSCR && chmod a+x $CXXSCR

    CXXCMD="${CXXSCR}"
  fi

  >&2 echo "Wrote script to $CXXCMD to enable ccache with C++ compiler."

  echo $CXXCMD
  exit 0

# Second case -- CC equals 1
elif [[ $command -eq cc ]] ; then

  if [ -z $CC ] && [ $OSTYPE == darwin* ]; then
    CC=clang
  fi

  if [[ -z $CC ]]; then
    if [[ `which cc` ]]; then
      CC=cc
    elif [[ `which gcc` ]]; then
      CC=gcc
    elif [[ `which clang` ]]; then
      CC=clang
    else
      >&2 echo "Compiler not in path with cc/gcc/clang; set manually with CC=<comp>"
      exit 1
    fi
  fi

  CCCMD=`which $CC || echo ""`

  >&2 echo "Using C compiler $CCCMD."

  if [[ $with_ccache -eq 1 ]]; then

    # Write out the script
    CCSCR="$script_dir/cc"

    rm -f $CCSCR

    echo "#!/bin/bash -e
    $CCACHE $CCCMD \$@
    exit \$?
    " > $CCSCR && chmod a+x $CCSCR

    CCCMD="${CCSCR}"
  fi

  >&2 echo "Wrote script to $CCCMD to enable ccache with C compiler."

  echo $CCCMD
  exit 0
fi
