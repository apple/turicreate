CMake Maintainer Guide
**********************

The following is a guide to CMake maintenance processes.
See documentation on `CMake Development`_ for more information.

.. _`CMake Development`: README.rst

.. contents:: Maintainer Processes:

Review a Merge Request
======================

The `CMake Review Process`_ requires a maintainer to issue the ``Do: merge``
command to integrate a merge request.  Please check at least the following:

* If the MR source branch is not named well for the change it makes
  (e.g. it is just ``master`` or the patch changed during review),
  add a ``Topic-rename: <topic>`` trailing line to the MR description
  to provide a better topic name.

* If the MR introduces a new feature or a user-facing behavior change,
  such as a policy, ensure that a ``Help/release/dev/$topic.rst`` file
  is added with a release note.

* If a commit changes a specific area, such as a module, its commit
  message should have an ``area:`` prefix on its first line.

* If a commit fixes a tracked issue, its commit message should have
  a trailing line such as ``Fixes: #00000``.

* Ensure that the MR adds sufficient documentation and test cases.

* Ensure that the MR has been tested sufficiently.  Typically it should
  be staged for nightly testing with ``Do: stage``.  Then manually
  review the `CMake CDash Page`_ to verify that no regressions were
  introduced.  (Learn to tolerate spurious failures due to idiosyncrasies
  of various nightly builders.)

* Ensure that the MR targets the ``master`` branch.  A MR intended for
  the ``release`` branch should be based on ``release`` but still merged
  to ``master`` first (via ``Do: merge``).  A maintainer may then merge
  the MR topic to ``release`` manually.

Maintain Current Release
========================

The ``release`` branch is used to maintain the current release or release
candidate.  The branch is published with no version number but maintained
using a local branch named ``release-$ver``, where ``$ver`` is the version
number of the current release in the form ``$major.$minor``.  It is always
merged into ``master`` before publishing.

Before merging a ``$topic`` branch into ``release``, verify that the
``$topic`` branch has already been merged to ``master`` via the usual
``Do: merge`` process.  Then, to merge the ``$topic`` branch into
``release``, start by creating the local branch:

.. code-block:: shell

  git fetch origin
  git checkout -b release-$ver origin/release

Merge the ``$topic`` branch into the local ``release-$ver`` branch, making
sure to include a ``Merge-request: !xxxx`` footer in the commit message:

.. code-block:: shell

  git merge --no-ff $topic

Merge the ``release-$ver`` branch to ``master``:

.. code-block:: shell

  git checkout master
  git pull
  git merge --no-ff release-$ver

Review new ancestry to ensure nothing unexpected was merged to either branch:

.. code-block:: shell

  git log --graph --boundary origin/master..master
  git log --graph --boundary origin/release..release-$ver

Publish both ``master`` and ``release`` simultaneously:

.. code-block:: shell

  git push --atomic origin master release-$ver:release

.. _`CMake Review Process`: review.rst
.. _`CMake CDash Page`: https://open.cdash.org/index.php?project=CMake

Branch a New Release
====================

This section covers how to start a new ``release`` branch for a major or
minor version bump (patch releases remain on their existing branch).

In the following we use the placeholder ``$ver`` to represent the
version number of the new release with the form ``$major.$minor``,
and ``$prev`` to represent the version number of the prior release.

Review Prior Release
--------------------

Review the history around the prior release branch:

.. code-block:: shell

  git log --graph --boundary \
   ^$(git rev-list --grep="Merge topic 'doc-.*-relnotes'" -n 1 master)~1 \
   $(git rev-list --grep="Begin post-.* development" -n 1 master) \
   $(git tag --list *-rc1| tail -1)

Consolidate Release Notes
-------------------------

Starting from a clean work tree on ``master``, create a topic branch to
use for consolidating the release notes:

.. code-block:: shell

  git checkout -b doc-$ver-relnotes

Run the `consolidate-relnotes.bash`_ script:

.. code-block:: shell

  Utilities/Release/consolidate-relnotes.bash $ver $prev

.. _`consolidate-relnotes.bash`: ../../Utilities/Release/consolidate-relnotes.bash

This moves notes from the ``Help/release/dev/*.rst`` files into a versioned
``Help/release/$ver.rst`` file and updates ``Help/release/index.rst`` to
link to the new document.  Commit the changes with a message such as::

  Help: Consolidate $ver release notes

  Run the `Utilities/Release/consolidate-relnotes.bash` script to move
  notes from `Help/release/dev/*` into `Help/release/$ver.rst`.

Manually edit ``Help/release/$ver.rst`` to add section headers, organize
the notes, and revise wording.  Then commit with a message such as::

  Help: Organize and revise $ver release notes

  Add section headers similar to the $prev release notes and move each
  individual bullet into an appropriate section.  Revise a few bullets.

Open a merge request with the ``doc-$ver-relnotes`` branch for review
and integration.  Further steps may proceed after this has been merged
to ``master``.

Update 'release' Branch
-----------------------

Starting from a clean work tree on ``master``, create a new ``release-$ver``
branch locally:

.. code-block:: shell

  git checkout -b release-$ver origin/master

Remove the development branch release note infrastructure:

.. code-block:: shell

  git rm Help/release/dev/0-sample-topic.rst
  sed -i '/^\.\. include:: dev.txt/ {N;d}' Help/release/index.rst

Commit with a message such as::

  Help: Drop development topic notes to prepare release

  Release versions do not have the development topic section of
  the CMake Release Notes index page.

Update ``Source/CMakeVersion.cmake`` to set the version to
``$major.$minor.0-rc1``:

.. code-block:: cmake

  # CMake version number components.
  set(CMake_VERSION_MAJOR $major)
  set(CMake_VERSION_MINOR $minor)
  set(CMake_VERSION_PATCH 0)
  set(CMake_VERSION_RC 1)

Update ``Utilities/Release/upload_release.cmake``:

.. code-block:: cmake

  set(VERSION $ver)

Update uses of ``DEVEL_CMAKE_VERSION`` in the source tree to mention the
actual version number:

.. code-block:: shell

  $EDITOR $(git grep -l DEVEL_CMAKE_VERSION)

Commit with a message such as::

  CMake $major.$minor.0-rc1 version update

Merge the ``release-$ver`` branch to ``master``:

.. code-block:: shell

  git checkout master
  git pull
  git merge --no-ff release-$ver

Begin post-release development by restoring the development branch release
note infrastructure and the version date from ``origin/master``:

.. code-block:: shell

  git checkout origin/master -- \
    Source/CMakeVersion.cmake Help/release/dev/0-sample-topic.rst
  sed -i $'/^Releases/ i\\\n.. include:: dev.txt\\\n' Help/release/index.rst

Update ``Source/CMakeVersion.cmake`` to set the version to
``$major.$minor.$date``:

.. code-block:: cmake

  # CMake version number components.
  set(CMake_VERSION_MAJOR $major)
  set(CMake_VERSION_MINOR $minor)
  set(CMake_VERSION_PATCH $date)
  #set(CMake_VERSION_RC 1)

Commit with a message such as::

  Begin post-$ver development

Push the update to the ``master`` and ``release`` branches:

.. code-block:: shell

  git push --atomic origin master release-$ver:release

Announce 'release' Branch
-------------------------

Send email to the ``cmake-developers@cmake.org`` mailing list (perhaps
in reply to a release preparation thread) announcing that post-release
development is open::

  I've branched 'release' for $ver.  The repository is now open for
  post-$ver development.  Please rebase open merge requests on 'master'
  before staging or merging.
