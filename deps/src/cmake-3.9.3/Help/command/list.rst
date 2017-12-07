list
----

List operations.

::

  list(LENGTH <list> <output variable>)
  list(GET <list> <element index> [<element index> ...]
       <output variable>)
  list(APPEND <list> [<element> ...])
  list(FILTER <list> <INCLUDE|EXCLUDE> REGEX <regular_expression>)
  list(FIND <list> <value> <output variable>)
  list(INSERT <list> <element_index> <element> [<element> ...])
  list(REMOVE_ITEM <list> <value> [<value> ...])
  list(REMOVE_AT <list> <index> [<index> ...])
  list(REMOVE_DUPLICATES <list>)
  list(REVERSE <list>)
  list(SORT <list>)

``LENGTH`` will return a given list's length.

``GET`` will return list of elements specified by indices from the list.

``APPEND`` will append elements to the list.

``FILTER`` will include or remove items from the list that match the
mode's pattern.
In ``REGEX`` mode, items will be matched against the given regular expression.
For more information on regular expressions see also the :command:`string`
command.

``FIND`` will return the index of the element specified in the list or -1
if it wasn't found.

``INSERT`` will insert elements to the list to the specified location.

``REMOVE_AT`` and ``REMOVE_ITEM`` will remove items from the list.  The
difference is that ``REMOVE_ITEM`` will remove the given items, while
``REMOVE_AT`` will remove the items at the given indices.

``REMOVE_DUPLICATES`` will remove duplicated items in the list.

``REVERSE`` reverses the contents of the list in-place.

``SORT`` sorts the list in-place alphabetically.

The list subcommands ``APPEND``, ``INSERT``, ``FILTER``, ``REMOVE_AT``,
``REMOVE_ITEM``, ``REMOVE_DUPLICATES``, ``REVERSE`` and ``SORT`` may create new
values for the list within the current CMake variable scope.  Similar to the
:command:`set` command, the LIST command creates new variable values in the
current scope, even if the list itself is actually defined in a parent
scope.  To propagate the results of these operations upwards, use
:command:`set` with ``PARENT_SCOPE``, :command:`set` with
``CACHE INTERNAL``, or some other means of value propagation.

NOTES: A list in cmake is a ``;`` separated group of strings.  To create a
list the set command can be used.  For example, ``set(var a b c d e)``
creates a list with ``a;b;c;d;e``, and ``set(var "a b c d e")`` creates a
string or a list with one item in it.   (Note macro arguments are not
variables, and therefore cannot be used in LIST commands.)

When specifying index values, if ``<element index>`` is 0 or greater, it
is indexed from the beginning of the list, with 0 representing the
first list element.  If ``<element index>`` is -1 or lesser, it is indexed
from the end of the list, with -1 representing the last list element.
Be careful when counting with negative indices: they do not start from
0.  -0 is equivalent to 0, the first list element.
