Contributing to CMake
*********************

The following summarizes the process for contributing changes.
See documentation on `CMake Development`_ for more information.

.. _`CMake Development`: Help/dev/README.rst

Community
=========

CMake is maintained and supported by `Kitware`_ and developed in
collaboration with a productive community of contributors.
Please subscribe and post to the `CMake Developers List`_ to raise
discussion of development topics.

.. _`Kitware`: http://www.kitware.com/cmake
.. _`CMake Developers List`: https://cmake.org/mailman/listinfo/cmake-developers

Patches
=======

CMake uses `Kitware's GitLab Instance`_ to manage development and code review.
To contribute patches:

#. Fork the upstream `CMake Repository`_ into a personal account.
#. Run `Utilities/SetupForDevelopment.sh`_ for local configuration.
#. See the `CMake Source Code Guide`_ for coding guidelines.
#. Base all new work on the upstream ``master`` branch.
   Base work on the upstream ``release`` branch only if it fixes a
   regression or bug in a feature new to that release.
   If in doubt, prefer ``master``.  Reviewers may simply ask for
   a rebase if deemed appropriate in particular cases.
#. Create commits making incremental, distinct, logically complete changes
   with appropriate `commit messages`_.
#. Push a topic branch to a personal repository fork on GitLab.
#. Create a GitLab Merge Request targeting the upstream ``master`` branch
   (even if the change is intended for merge to the ``release`` branch).
   Check the box labelled "Allow commits from members who can merge to the
   target branch".  This will allow maintainers to make minor edits on your
   behalf.

The merge request will enter the `CMake Review Process`_ for consideration.

.. _`Kitware's GitLab Instance`: https://gitlab.kitware.com
.. _`CMake Repository`: https://gitlab.kitware.com/cmake/cmake
.. _`Utilities/SetupForDevelopment.sh`: Utilities/SetupForDevelopment.sh
.. _`CMake Source Code Guide`: Help/dev/source.rst
.. _`commit messages`: Help/dev/review.rst#commit-messages
.. _`CMake Review Process`: Help/dev/review.rst

CMake Dashboard Client
======================

The *integration testing* step of the `CMake Review Process`_ uses a set of
testing machines that follow an integration branch on their own schedule to
drive testing and submit results to the `CMake CDash Page`_.  Anyone is
welcome to provide testing machines in order to help keep support for their
platforms working.

See documentation on `CMake Testing Process`_ for more information.

.. _`CMake CDash Page`: https://open.cdash.org/index.php?project=CMake
.. _`CMake Testing Process`: Help/dev/testing.rst

License
=======

We do not require any formal copyright assignment or contributor license
agreement.  Any contributions intentionally sent upstream are presumed
to be offered under terms of the OSI-approved BSD 3-clause License.
See `Copyright.txt`_ for details.

.. _`Copyright.txt`: Copyright.txt
