#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="KWIML"
readonly ownership="KWIML Upstream <kwrobot@kitware.com>"
readonly subtree="Utilities/KWIML"
readonly repo="https://gitlab.kitware.com/utils/kwiml.git"
readonly tag="master"
readonly shortlog=true
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/update-third-party.bash"
