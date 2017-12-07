#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="liblzma"
readonly ownership="liblzma upstream <xz-devel@tukaani.org>"
readonly subtree="Utilities/cmliblzma"
readonly repo="http://git.tukaani.org/xz.git"
readonly tag="v5.0.8"
readonly shortlog=false
readonly paths="
  COPYING
  src/common/common_w32res.rc
  src/common/sysdefs.h
  src/common/tuklib_integer.h
  src/liblzma/
"

extract_source () {
    git_archive
    pushd "${extractdir}/${name}-reduced"
    mv src/common .
    mv src/liblzma .
    rmdir src
    popd
}

. "${BASH_SOURCE%/*}/update-third-party.bash"
