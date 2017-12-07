AUTOMOC_DEPEND_FILTERS
----------------------

Filter definitions used by :prop_tgt:`AUTOMOC` to extract file names from
source code as additional dependencies for the ``moc`` file.

This property is only used if the :prop_tgt:`AUTOMOC` property is ``ON``
for this target.

Filters are defined as ``KEYWORD;REGULAR_EXPRESSION`` pairs. First the file
content is searched for ``KEYWORD``. If it is found at least once, then file
names are extracted by successively searching for ``REGULAR_EXPRESSION`` and
taking the first match group.

Consider a filter extracts the file name ``DEP`` from the content of a file
``FOO``. If ``DEP`` changes, then the ``moc`` file for ``FOO`` gets rebuilt.
The file ``DEP`` is searched for first in the vicinity
of ``FOO`` and afterwards in the target's :prop_tgt:`INCLUDE_DIRECTORIES`.

By default :prop_tgt:`AUTOMOC_DEPEND_FILTERS` is initialized from
:variable:`CMAKE_AUTOMOC_DEPEND_FILTERS`, which is empty by default.

See the :manual:`cmake-qt(7)` manual for more information on using CMake
with Qt.


Example
-------

Consider a file ``FOO.hpp`` holds a custom macro ``OBJ_JSON_FILE`` and we
want the ``moc`` file to depend on the macro`s file name argument::

  class My_Class : public QObject
  {
    Q_OBJECT
    OBJ_JSON_FILE ( "DEP.json" )
  ...
  };

Then we might use :variable:`CMAKE_AUTOMOC_DEPEND_FILTERS` to
define a filter like this::

  set(CMAKE_AUTOMOC_DEPEND_FILTERS
    "OBJ_JSON_FILE" "[\n][ \t]*OBJ_JSON_FILE[ \t]*\\([ \t]*\"([^\"]+)\""
  )
