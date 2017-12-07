CMake Maintainer Guide
**********************

The following is a guide to CMake maintenance processes.
See documentation on `CMake Development`_ for more information.

.. _`CMake Development`: README.rst

.. contents:: Maintainer Processes:

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
