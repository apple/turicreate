set_target_properties
---------------------

Targets can have properties that affect how they are built.

::

  set_target_properties(target1 target2 ...
                        PROPERTIES prop1 value1
                        prop2 value2 ...)

Set properties on a target.  The syntax for the command is to list all
the files you want to change, and then provide the values you want to
set next.  You can use any prop value pair you want and extract it
later with the :command:`get_property` or :command:`get_target_property`
command.

See :ref:`Target Properties` for the list of properties known to CMake.
