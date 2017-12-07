#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="vim-cmake-syntax"
readonly ownership="vim-cmake-syntax upstream <kwrobot@kitware.com>"
readonly subtree="Auxiliary/vim"
readonly repo="https://github.com/pboettch/vim-cmake-syntax.git"
readonly tag="master"
readonly shortlog=true
readonly paths="
  indent
  syntax
  cmake.vim.in
  extract-upper-case.pl
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/update-third-party.bash"
