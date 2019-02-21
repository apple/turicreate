#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="curl"
readonly ownership="Curl Upstream <curl-library@cool.haxx.se>"
readonly subtree="Utilities/cmcurl"
readonly repo="https://github.com/curl/curl.git"
readonly tag="curl-7_62_0"
readonly shortlog=false
readonly paths="
  CMake/*
  CMakeLists.txt
  COPYING
  include/curl/*.h
  lib/*.c
  lib/*.h
  lib/CMakeLists.txt
  lib/Makefile.inc
  lib/curl_config.h.cmake
  lib/libcurl.rc
  lib/vauth/*.c
  lib/vauth/*.h
  lib/vtls/*.c
  lib/vtls/*.h
"

extract_source () {
    git_archive
    pushd "${extractdir}/${name}-reduced"
    rm lib/config-*.h
    chmod a-x lib/connect.c
    for f in \
      lib/cookie.c \
      lib/krb5.c \
      lib/security.c \
      ; do
        iconv -f LATIN1 -t UTF8 $f -o $f.utf-8
        mv $f.utf-8 $f
    done
    echo "* -whitespace" > .gitattributes
    popd
}

. "${BASH_SOURCE%/*}/update-third-party.bash"
