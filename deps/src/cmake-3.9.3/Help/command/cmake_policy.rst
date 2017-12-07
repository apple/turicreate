cmake_policy
------------

Manage CMake Policy settings.  See the :manual:`cmake-policies(7)`
manual for defined policies.

As CMake evolves it is sometimes necessary to change existing behavior
in order to fix bugs or improve implementations of existing features.
The CMake Policy mechanism is designed to help keep existing projects
building as new versions of CMake introduce changes in behavior.  Each
new policy (behavioral change) is given an identifier of the form
``CMP<NNNN>`` where ``<NNNN>`` is an integer index.  Documentation
associated with each policy describes the ``OLD`` and ``NEW`` behavior
and the reason the policy was introduced.  Projects may set each policy
to select the desired behavior.  When CMake needs to know which behavior
to use it checks for a setting specified by the project.  If no
setting is available the ``OLD`` behavior is assumed and a warning is
produced requesting that the policy be set.

Setting Policies by CMake Version
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``cmake_policy`` command is used to set policies to ``OLD`` or ``NEW``
behavior.  While setting policies individually is supported, we
encourage projects to set policies based on CMake versions::

  cmake_policy(VERSION major.minor[.patch[.tweak]])

Specify that the current CMake code is written for the given
version of CMake.  All policies introduced in the specified version or
earlier will be set to use ``NEW`` behavior.  All policies introduced
after the specified version will be unset (unless the
:variable:`CMAKE_POLICY_DEFAULT_CMP<NNNN>` variable sets a default).
This effectively requests behavior preferred as of a given CMake
version and tells newer CMake versions to warn about their new policies.
The policy version specified must be at least 2.4 or the command will
report an error.

Note that the :command:`cmake_minimum_required(VERSION)`
command implicitly calls ``cmake_policy(VERSION)`` too.

Setting Policies Explicitly
^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

  cmake_policy(SET CMP<NNNN> NEW)
  cmake_policy(SET CMP<NNNN> OLD)

Tell CMake to use the ``OLD`` or ``NEW`` behavior for a given policy.
Projects depending on the old behavior of a given policy may silence a
policy warning by setting the policy state to ``OLD``.  Alternatively
one may fix the project to work with the new behavior and set the
policy state to ``NEW``.

.. include:: ../policy/DEPRECATED.txt

Checking Policy Settings
^^^^^^^^^^^^^^^^^^^^^^^^

::

  cmake_policy(GET CMP<NNNN> <variable>)

Check whether a given policy is set to ``OLD`` or ``NEW`` behavior.
The output ``<variable>`` value will be ``OLD`` or ``NEW`` if the
policy is set, and empty otherwise.

CMake Policy Stack
^^^^^^^^^^^^^^^^^^

CMake keeps policy settings on a stack, so changes made by the
cmake_policy command affect only the top of the stack.  A new entry on
the policy stack is managed automatically for each subdirectory to
protect its parents and siblings.  CMake also manages a new entry for
scripts loaded by :command:`include` and :command:`find_package` commands
except when invoked with the ``NO_POLICY_SCOPE`` option
(see also policy :policy:`CMP0011`).
The ``cmake_policy`` command provides an interface to manage custom
entries on the policy stack::

  cmake_policy(PUSH)
  cmake_policy(POP)

Each ``PUSH`` must have a matching ``POP`` to erase any changes.
This is useful to make temporary changes to policy settings.
Calls to the :command:`cmake_minimum_required(VERSION)`,
``cmake_policy(VERSION)``, or ``cmake_policy(SET)`` commands
influence only the current top of the policy stack.

Commands created by the :command:`function` and :command:`macro`
commands record policy settings when they are created and
use the pre-record policies when they are invoked.  If the function or
macro implementation sets policies, the changes automatically
propagate up through callers until they reach the closest nested
policy stack entry.
