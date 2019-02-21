Contributing to KWSys
*********************

Patches
=======

KWSys is kept in its own Git repository and shared by several projects
via copies in their source trees.  Changes to KWSys should not be made
directly in a host project, except perhaps in maintenance branches.

KWSys uses `Kitware's GitLab Instance`_ to manage development and code review.
To contribute patches:

#. Fork the upstream `KWSys Repository`_ into a personal account.
#. Base all new work on the upstream ``master`` branch.
#. Run ``./SetupForDevelopment.sh`` in new local work trees.
#. Create commits making incremental, distinct, logically complete changes.
#. Push a topic branch to a personal repository fork on GitLab.
#. Create a GitLab Merge Request targeting the upstream ``master`` branch.

Once changes are reviewed, tested, and integrated to KWSys upstream then
copies of KWSys within dependent projects can be updated to get the changes.

.. _`Kitware's GitLab Instance`: https://gitlab.kitware.com
.. _`KWSys Repository`: https://gitlab.kitware.com/utils/kwsys

Code Style
==========

We use `clang-format`_ version **6.0** to define our style for C++ code in
the KWSys source tree.  See the `.clang-format`_ configuration file for
our style settings.  Use the `clang-format.bash`_ script to format source
code.  It automatically runs ``clang-format`` on the set of source files
for which we enforce style.  The script also has options to format only
a subset of files, such as those that are locally modified.

.. _`clang-format`: http://clang.llvm.org/docs/ClangFormat.html
.. _`.clang-format`: .clang-format
.. _`clang-format.bash`: clang-format.bash

License
=======

We do not require any formal copyright assignment or contributor license
agreement.  Any contributions intentionally sent upstream are presumed
to be offered under terms of the OSI-approved BSD 3-clause License.
See `Copyright.txt`_ for details.

.. _`Copyright.txt`: Copyright.txt
