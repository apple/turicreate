math
----

Mathematical expressions.

::

  math(EXPR <output-variable> <math-expression> [OUTPUT_FORMAT <format>])

``EXPR`` evaluates mathematical expression and returns result in the
output variable.  Example mathematical expression is ``5 * (10 + 13)``.
Supported operators are ``+``, ``-``, ``*``, ``/``, ``%``, ``|``, ``&``,
``^``, ``~``, ``<<``, ``>>``, and ``(...)``.  They have the same meaning
as they do in C code.

Numeric constants are evaluated in decimal or hexadecimal representation.

The result is formatted according to the option "OUTPUT_FORMAT" ,
where ``<format>`` is one of:
::

 HEXADECIMAL = Result in output variable will be formatted in C code
 Hexadecimal notation.
 DECIMAL = Result in output variable will be formatted in decimal notation.


For example::

  math(EXPR value "100 * 0xA" DECIMAL)  results in value is set to "1000"
  math(EXPR value "100 * 0xA" HEXADECIMAL)  results in value is set to "0x3e8"
