#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="GitSetup"
readonly ownership="GitSetup Upstream <kwrobot@kitware.com>"
readonly subtree="Utilities/GitSetup"
readonly repo="https://gitlab.kitware.com/utils/gitsetup.git"
readonly tag="setup"
readonly shortlog=false
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/update-third-party.bash"
