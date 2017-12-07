#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="curl"
readonly ownership="Curl Upstream <curl-library@cool.haxx.se>"
readonly subtree="Utilities/cmcurl"
readonly repo="https://github.com/curl/curl.git"
readonly tag="curl-7_54_1"
readonly shortlog=false
readonly paths="
  CMake/*
  CMakeLists.txt
  COPYING
  include/curl/*.h
  include/curl/curlbuild.h.cmake
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
    echo "* -whitespace" > .gitattributes
    popd
}

. "${BASH_SOURCE%/*}/update-third-party.bash"
