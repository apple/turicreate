CMake Testing Process
*********************

The following documents the process for running integration testing builds.
See documentation on `CMake Development`_ for more information.

.. _`CMake Development`: README.rst

CMake Dashboard Scripts
=======================

The *integration testing* step of the `CMake Review Process`_ uses a set of
testing machines that follow an integration branch on their own schedule to
drive testing and submit results to the `CMake CDash Page`_.  Anyone is
welcome to provide testing machines in order to help keep support for their
platforms working.

The `CMake Dashboard Scripts Repository`_ provides CTest scripts to drive
nightly, continuous, and experimental testing of CMake.  Use the following
commands to set up a new integration testing client:

.. code-block:: console

  $ mkdir -p ~/Dashboards
  $ cd ~/Dashboards
  $ git clone https://gitlab.kitware.com/cmake/dashboard-scripts.git CMakeScripts
  $ cd CMakeScripts

The `cmake_common.cmake`_ script contains comments at the top with
instructions to set up a testing client.  As it instructs, create a
CTest script with local settings and include ``cmake_common.cmake``.

.. _`CMake Review Process`: review.rst
.. _`CMake CDash Page`: https://open.cdash.org/index.php?project=CMake
.. _`CMake Dashboard Scripts Repository`: https://gitlab.kitware.com/cmake/dashboard-scripts
.. _`cmake_common.cmake`: https://gitlab.kitware.com/cmake/dashboard-scripts/blob/master/cmake_common.cmake

Nightly Start Time
------------------

The ``cmake_common.cmake`` script expects its includer to be run from a
nightly scheduled task (cron job).  Schedule such tasks for sometime after
``1:00am UTC``, the time at which our nightly testing branches fast-forward.
