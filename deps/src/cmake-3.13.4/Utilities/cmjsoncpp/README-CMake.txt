The Utilities/cmjsoncpp directory contains a reduced distribution
of the jsoncpp source tree with only the library source code and
CMake build system.  It is not a submodule; the actual content is part
of our source tree and changes can be made and committed directly.

We update from upstream using Git's "subtree" merge strategy.  A
special branch contains commits of upstream jsoncpp snapshots and
nothing else.  No Git ref points explicitly to the head of this
branch, but it is merged into our history.

Update jsoncpp from upstream as follows.  Create a local branch to
explicitly reference the upstream snapshot branch head:

 git branch jsoncpp-upstream 53f6ccb0

Use a temporary directory to checkout the branch:

 mkdir jsoncpp-tmp
 cd jsoncpp-tmp
 git init
 git pull .. jsoncpp-upstream
 rm -rf *

Now place the (reduced) jsoncpp content in this directory.  See
instructions shown by

 git log 53f6ccb0

for help extracting the content from the upstream svn repo.  Then run
the following commands to commit the new version.  Substitute the
appropriate date and version number:

 git add --all

 GIT_AUTHOR_NAME='JsonCpp Upstream' \
 GIT_AUTHOR_EMAIL='kwrobot@kitware.com' \
 GIT_AUTHOR_DATE='Thu Nov 20 08:45:58 2014 -0600' \
 git commit -m 'JsonCpp 1.0.0 (reduced)' &&
 git commit --amend

Edit the commit message to describe the procedure used to obtain the
content.  Then push the changes back up to the main local repository:

 git push .. HEAD:jsoncpp-upstream
 cd ..
 rm -rf jsoncpp-tmp

Create a topic in the main repository on which to perform the update:

 git checkout -b update-jsoncpp master

Merge the jsoncpp-upstream branch as a subtree:

 git merge -s recursive -X subtree=Utilities/cmjsoncpp \
           jsoncpp-upstream

If there are conflicts, resolve them and commit.  Build and test the
tree.  Commit any additional changes needed to succeed.

Finally, run

 git rev-parse --short=8 jsoncpp-upstream

to get the commit from which the jsoncpp-upstream branch must be started
on the next update.  Edit the "git branch jsoncpp-upstream" line above to
record it, and commit this file.
