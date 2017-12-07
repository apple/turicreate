get_target_property
-------------------

Get a property from a target.

::

  get_target_property(VAR target property)

Get a property from a target.  The value of the property is stored in
the variable ``VAR``.  If the property is not found, ``VAR`` will be set to
"NOTFOUND".  Use :command:`set_target_properties` to set property values.
Properties are usually used to control how a target is built, but some
query the target instead.  This command can get properties for any
target so far created.  The targets do not need to be in the current
``CMakeLists.txt`` file.

See also the more general :command:`get_property` command.
