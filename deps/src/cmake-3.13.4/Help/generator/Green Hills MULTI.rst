Green Hills MULTI
-----------------

Generates Green Hills MULTI project files (experimental, work-in-progress).

Customizations that are used to pick toolset and target system:

The ``-A <arch>`` can be supplied for setting the target architecture.
``<arch>`` usually is one of "arm", "ppc", "86", etcetera.  If the target architecture
is not specified then the default architecture of "arm" will be used.

The ``-T <toolset>`` can be supplied for setting the toolset to be used.
All toolsets are expected to be located at ``GHS_TOOLSET_ROOT``.
If the toolset is not specified then the latest toolset will be used.

* ``GHS_TARGET_PLATFORM``

Default to ``integrity``.
Usual values are ``integrity``, ``threadx``, ``uvelosity``,
``velosity``, ``vxworks``, ``standalone``.

* ``GHS_PRIMARY_TARGET``

Sets ``primaryTarget`` field in project file.
Defaults to ``<arch>_<GHS_TARGET_PLATFORM>.tgt``.

* ``GHS_TOOLSET_ROOT``

Default to ``C:/ghs``.  Root path for ``toolset``.

* ``GHS_OS_ROOT``

Default to ``C:/ghs``.  Root path for RTOS searches.

* ``GHS_OS_DIR``

Default to latest platform OS installation at ``GHS_OS_ROOT``.  Set this value if
a specific RTOS is to be used.

* ``GHS_BSP_NAME``

Defaults to ``sim<arch>`` if not set by user.

Customizations are available through the following cache variables:

* ``GHS_CUSTOMIZATION``
* ``GHS_GPJ_MACROS``

.. note::
  This generator is deemed experimental as of CMake |release|
  and is still a work in progress.  Future versions of CMake
  may make breaking changes as the generator matures.
