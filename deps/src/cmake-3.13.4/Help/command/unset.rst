unset
-----

Unset a variable, cache variable, or environment variable.

::

  unset(<variable> [CACHE | PARENT_SCOPE])

Removes a normal variable from the current scope, causing it
to become undefined.  If ``CACHE`` is present, then a cache variable
is removed instead of a normal variable.  Note that when evaluating
:ref:`Variable References` of the form ``${VAR}``, CMake first searches
for a normal variable with that name.  If no such normal variable exists,
CMake will then search for a cache entry with that name.  Because of this
unsetting a normal variable can expose a cache variable that was previously
hidden.  To force a variable reference of the form ``${VAR}`` to return an
empty string, use ``set(<variable> "")``, which clears the normal variable
but leaves it defined.

If ``PARENT_SCOPE`` is present then the variable is removed from the scope
above the current scope.  See the same option in the :command:`set` command
for further details.

``<variable>`` can be an environment variable such as:

::

  unset(ENV{LD_LIBRARY_PATH})

in which case the variable will be removed from the current
environment.
