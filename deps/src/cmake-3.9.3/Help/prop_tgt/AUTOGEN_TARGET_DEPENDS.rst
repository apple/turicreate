AUTOGEN_TARGET_DEPENDS
----------------------

Target dependencies of the corresponding ``_autogen`` target.

Targets which have their :prop_tgt:`AUTOMOC` target ``ON`` have a
corresponding ``_autogen`` target which is used to autogenerate generate moc
files.  As this ``_autogen`` target is created at generate-time, it is not
possible to define dependencies of it, such as to create inputs for the ``moc``
executable.

The ``AUTOGEN_TARGET_DEPENDS`` target property can be set instead to a list of
dependencies for the ``_autogen`` target.  The buildsystem will be generated to
depend on its contents.

See the :manual:`cmake-qt(7)` manual for more information on using CMake
with Qt.
