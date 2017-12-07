#=============================================================================
# Copyright 2015-2016 Kitware, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#=============================================================================

########################################################################
# Script for updating third party packages.
#
# This script should be sourced in a project-specific script which sets
# the following variables:
#
#   name
#       The name of the project.
#   ownership
#       A git author name/email for the commits.
#   subtree
#       The location of the thirdparty package within the main source
#       tree.
#   repo
#       The git repository to use as upstream.
#   tag
#       The tag, branch or commit hash to use for upstream.
#   shortlog
#       Optional.  Set to 'true' to get a shortlog in the commit message.
#
# Additionally, an "extract_source" function must be defined. It will be
# run within the checkout of the project on the requested tag. It should
# should place the desired tree into $extractdir/$name-reduced. This
# directory will be used as the newest commit for the project.
#
# For convenience, the function may use the "git_archive" function which
# does a standard "git archive" extraction using the (optional) "paths"
# variable to only extract a subset of the source tree.
########################################################################

########################################################################
# Utility functions
########################################################################
git_archive () {
    git archive --worktree-attributes --prefix="$name-reduced/" HEAD -- $paths | \
        tar -C "$extractdir" -x
}

disable_custom_gitattributes() {
    pushd "${extractdir}/${name}-reduced"
    # Git does not allow custom attributes in a subdirectory where we
    # are about to merge the `.gitattributes` file, so disable them.
    sed -i '/^\[attr\]/ {s/^/#/}' .gitattributes
    popd
}

die () {
    echo >&2 "$@"
    exit 1
}

warn () {
    echo >&2 "warning: $@"
}

readonly regex_date='20[0-9][0-9]-[0-9][0-9]-[0-9][0-9]'
readonly basehash_regex="$name $regex_date ([0-9a-f]*)"
readonly basehash="$( git rev-list --author="$ownership" --grep="$basehash_regex" -n 1 HEAD )"
readonly upstream_old_short="$( git cat-file commit "$basehash" | sed -n '/'"$basehash_regex"'/ {s/.*(//;s/)//;p}' | egrep '^[0-9a-f]+$' )"

########################################################################
# Sanity checking
########################################################################
[ -n "$name" ] || \
    die "'name' is empty"
[ -n "$ownership" ] || \
    die "'ownership' is empty"
[ -n "$subtree" ] || \
    die "'subtree' is empty"
[ -n "$repo" ] || \
    die "'repo' is empty"
[ -n "$tag" ] || \
    die "'tag' is empty"
[ -n "$basehash" ] || \
    warn "'basehash' is empty; performing initial import"
readonly do_shortlog="${shortlog-false}"

readonly workdir="$PWD/work"
readonly upstreamdir="$workdir/upstream"
readonly extractdir="$workdir/extract"

[ -d "$workdir" ] && \
    die "error: workdir '$workdir' already exists"

trap "rm -rf '$workdir'" EXIT

# Get upstream
git clone "$repo" "$upstreamdir"

if [ -n "$basehash" ]; then
    # Use the existing package's history
    git worktree add "$extractdir" "$basehash"
    # Clear out the working tree
    pushd "$extractdir"
    git ls-files | xargs rm -v
    find . -type d -empty -delete
    popd
else
    # Create a repo to hold this package's history
    mkdir -p "$extractdir"
    git -C "$extractdir" init
fi

# Extract the subset of upstream we care about
pushd "$upstreamdir"
git checkout "$tag"
readonly upstream_hash="$( git rev-parse HEAD )"
readonly upstream_hash_short="$( git rev-parse --short=8 "$upstream_hash" )"
readonly upstream_datetime="$( git rev-list "$upstream_hash" --format='%ci' -n 1 | grep -e "^$regex_date" )"
readonly upstream_date="$( echo "$upstream_datetime" | grep -o -e "$regex_date" )"
if $do_shortlog && [ -n "$basehash" ]; then
    readonly commit_shortlog="

Upstream Shortlog
-----------------

$( git shortlog --no-merges --abbrev=8 --format='%h %s' "$upstream_old_short".."$upstream_hash" )"
else
    readonly commit_shortlog=""
fi
extract_source || \
    die "failed to extract source"
popd

[ -d "$extractdir/$name-reduced" ] || \
    die "expected directory to extract does not exist"
readonly commit_summary="$name $upstream_date ($upstream_hash_short)"

# Commit the subset
pushd "$extractdir"
mv -v "$name-reduced/"* .
rmdir "$name-reduced/"
git add -A .
git commit -n --author="$ownership" --date="$upstream_datetime" -F - <<-EOF
$commit_summary

Code extracted from:

    $repo

at commit $upstream_hash ($tag).$commit_shortlog
EOF
git branch -f "upstream-$name"
popd

# Merge the subset into this repository
if [ -n "$basehash" ]; then
    git merge --log -s recursive "-Xsubtree=$subtree/" --no-commit "upstream-$name"
else
    unrelated_histories_flag=""
    if git merge --help | grep -q -e allow-unrelated-histories; then
        unrelated_histories_flag="--allow-unrelated-histories "
    fi
    readonly unrelated_histories_flag

    git fetch "$extractdir" "upstream-$name:upstream-$name"
    git merge --log -s ours --no-commit $unrelated_histories_flag "upstream-$name"
    git read-tree -u --prefix="$subtree/" "upstream-$name"
fi
git commit --no-edit
git branch -d "upstream-$name"
