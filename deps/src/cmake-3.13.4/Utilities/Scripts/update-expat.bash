#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="expat"
readonly ownership="Expat Upstream <kwrobot@kitware.com>"
readonly subtree="Utilities/cmexpat"
readonly repo="https://github.com/libexpat/libexpat.git"
readonly tag="R_2_2_3"
readonly shortlog=false
readonly paths="
  expat/lib/asciitab.h
  expat/lib/expat.h
  expat/lib/xmltok.h
  expat/lib/internal.h
  expat/lib/xmlrole.h
  expat/lib/iasciitab.h
  expat/lib/latin1tab.h
  expat/lib/loadlibrary.c
  expat/lib/xmlrole.c
  expat/lib/utf8tab.h
  expat/lib/nametab.h
  expat/lib/ascii.h
  expat/lib/siphash.h
  expat/lib/xmltok_impl.h
  expat/lib/xmltok_ns.c
  expat/lib/winconfig.h
  expat/lib/expat_external.h
  expat/lib/xmltok.c
  expat/lib/xmlparse.c
  expat/lib/xmltok_impl.c
  expat/README.md
  expat/ConfigureChecks.cmake
  expat/expat_config.h.cmake
  expat/COPYING
"

extract_source () {
    git_archive
    pushd "${extractdir}/${name}-reduced"
    fromdos expat/ConfigureChecks.cmake expat/CMakeLists.txt expat/expat_config.h.cmake
    chmod a-x expat/ConfigureChecks.cmake expat/CMakeLists.txt expat/expat_config.h.cmake
    mv expat/* .
    rmdir expat
    popd
}

. "${BASH_SOURCE%/*}/update-third-party.bash"
