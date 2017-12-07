get_test_property
-----------------

Get a property of the test.

::

  get_test_property(test property VAR)

Get a property from the test.  The value of the property is stored in
the variable ``VAR``.  If the test or property is not found, ``VAR`` will
be set to "NOTFOUND".  For a list of standard properties you can type
``cmake --help-property-list``.

See also the more general :command:`get_property` command.
