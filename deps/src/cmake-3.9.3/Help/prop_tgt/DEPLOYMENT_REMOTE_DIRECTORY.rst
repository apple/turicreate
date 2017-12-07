DEPLOYMENT_REMOTE_DIRECTORY
---------------------------

Set the WinCE project ``RemoteDirectory`` in ``DeploymentTool`` and
``RemoteExecutable`` in ``DebuggerTool`` in ``.vcproj`` files generated
by the :generator:`Visual Studio 9 2008` and :generator:`Visual Studio 8 2005`
generators.  This is useful when you want to debug on remote WinCE device.
For example:

.. code-block:: cmake

  set_property(TARGET ${TARGET} PROPERTY
    DEPLOYMENT_REMOTE_DIRECTORY "\\FlashStorage")

produces::

  <DeploymentTool RemoteDirectory="\FlashStorage" ... />
  <DebuggerTool RemoteExecutable="\FlashStorage\target_file" ... />
