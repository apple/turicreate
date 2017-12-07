.. index::
   single: directive ignored

title_text
----------

.. comment ignored
..
   comment ignored

Command :cmake:command:`some_cmd` explicit cmake domain.
Command :command:`some_cmd` without target.
Command :command:`some_cmd <some_cmd>` with target.
Command :command:`some_cmd_<cmd>` placeholder without target.
Command :command:`some_cmd_<cmd> <some_cmd>` placholder with target.
Command :command:`some_cmd()` with parens.
Command :command:`some_cmd(SUB)` with subcommand.
Command :command:`some_cmd(SUB) <some_cmd>` with subcommand and target.
Command :command:`some_cmd (SUB) <some_cmd>` with space and subcommand and target.
Command :command:`some command <some_cmd>` with space and target.
Variable :variable:`some variable <some_var>` space and target.
Variable :variable:`<PLACEHOLDER>_VARIABLE` with leading placeholder.
Variable :variable:`VARIABLE_<PLACEHOLDER>` with trailing placeholder.
Variable :variable:`<PLACEHOLDER>_VARIABLE <target>` with leading placeholder and target.
Variable :variable:`VARIABLE_<PLACEHOLDER> <target>` with trailing placeholder and target.
Generator :generator:`Some Generator` with space.

.. |not replaced| replace:: not replaced through toctree
.. |not replaced in literal| replace:: replaced in parsed literal

.. toctree::
   :maxdepth: 2

   testRSTtoc1
   /testRSTtoc2

.. cmake-module:: testRSTmod.cmake

.. cmake:command:: some_cmd

   Command some_cmd description.

.. command:: other_cmd

   Command other_cmd description.

.. cmake:variable:: some_var

   Variable some_var description.

.. variable:: other_var

   Variable other_var description.

.. parsed-literal::

    Parsed-literal included without directive.
   Common Indentation Removed
   # |not replaced in literal|

.. code-block:: cmake

   # Sample CMake code block
   if(condition)
     message(indented)
   endif()
   # |not replaced in literal|

A literal block starts after a line consisting of two colons

::

    Literal block.
   Common Indentation Removed
   # |not replaced in literal|

or after a paragraph ending in two colons::

    Literal block.
   Common Indentation Removed
   # |not replaced in literal|

but not after a line ending in two colons::
in the middle of a paragraph.

.. productionlist::
 grammar: `production`
 production: "content rendered"

.. note::
 Notes are called out.

.. |substitution| replace::
   |nested substitution|
   with multiple lines becomes one line
.. |nested substitution| replace:: substituted text

.. include:: testRSTinclude1.rst
.. include:: /testRSTinclude2.rst
