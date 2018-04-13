#!/bin/bash
# has to be run from root of the repo
set -x
set -e

if [[ -z $VIRTUALENV ]]; then
  VIRTUALENV=virtualenv
fi

PIP=pip3
# TODO - not sure why 'm' is necessary here (and not in 2.7)
# note that PYTHON_VERSION includes the word "python", like "python2.7" or "python3.6"
PYTHON_FULL_NAME=${PYTHON_VERSION}m
if [[ "${PYTHON_VERSION}" == "python2.7" ]]; then
  PIP=pip
  PYTHON_FULL_NAME=python2.7
fi

$VIRTUALENV ${PWD}/deps/env
source ${PWD}/deps/env/bin/activate

python_scripts=deps/env/bin

function make_windows_exec_link {
  targetname=$1/`basename $2 .exe`
  echo "#!/bin/sh" > $targetname
  echo "$2 \$@" >> $targetname
}

function linux_patch_sigfpe_handler {
if [[ $OSTYPE == linux* ]]; then
        targfile=deps/local/include/pyfpe.h
        echo "#undef WANT_SIGFPE_HANDLER" | cat - $targfile > tmp
        mv -f tmp $targfile
fi
}


function download_file {
  # detect wget
  echo "Downloading $2 from $1 ..."
  if [ -z `which wget` ] ; then
    if [ -z `which curl` ] ; then
      echo "Unable to find either curl or wget! Cannot proceed with
            automatic install."
      exit 1
    fi
    curl $1 -o $2
  else
    wget $1 -O $2
  fi
} # end of download file

$python_scripts/$PIP install --upgrade "pip>=8.1"
$python_scripts/$PIP install -r scripts/requirements.txt

mkdir -p deps/local/lib
mkdir -p deps/local/include

pushd deps/local/include
for f in `ls ../../env/include/$PYTHON_FULL_NAME/*`; do  
  ln -Ffs $f
done
popd

mkdir -p deps/local/bin
pushd deps/local/bin
for f in `ls ../../env/bin`; do
  ln -Ffs $f
done
popd

linux_patch_sigfpe_handler

