#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="librhash"
readonly ownership="librhash upstream <kwrobot@kitware.com>"
readonly subtree="Utilities/cmlibrhash"
readonly repo="https://github.com/rhash/rhash.git"
readonly tag="master"
readonly shortlog=false
readonly paths="
  COPYING
  README
  librhash/algorithms.c
  librhash/algorithms.h
  librhash/byte_order.c
  librhash/byte_order.h
  librhash/hex.c
  librhash/hex.h
  librhash/md5.c
  librhash/md5.h
  librhash/rhash.c
  librhash/rhash.h
  librhash/sha1.c
  librhash/sha1.h
  librhash/sha256.c
  librhash/sha256.h
  librhash/sha3.c
  librhash/sha3.h
  librhash/sha512.c
  librhash/sha512.h
  librhash/ustd.h
  librhash/util.h
"

extract_source () {
    git_archive
    pushd "${extractdir}/${name}-reduced"
    echo "* -whitespace" > .gitattributes
    popd
}

. "${BASH_SOURCE%/*}/update-third-party.bash"
