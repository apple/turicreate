string
------

.. only:: html

   .. contents::

String operations.

Search and Replace
^^^^^^^^^^^^^^^^^^

FIND
""""

::

  string(FIND <string> <substring> <output variable> [REVERSE])

Return the position where the given substring was found in
the supplied string.  If the ``REVERSE`` flag was used, the command will
search for the position of the last occurrence of the specified
substring.  If the substring is not found, a position of -1 is returned.

REPLACE
"""""""

::

  string(REPLACE <match_string>
         <replace_string> <output variable>
         <input> [<input>...])

Replace all occurrences of ``match_string`` in the input
with ``replace_string`` and store the result in the output.

Regular Expressions
^^^^^^^^^^^^^^^^^^^

REGEX MATCH
"""""""""""

::

  string(REGEX MATCH <regular_expression>
         <output variable> <input> [<input>...])

Match the regular expression once and store the match in the output variable.
All ``<input>`` arguments are concatenated before matching.

REGEX MATCHALL
""""""""""""""

::

  string(REGEX MATCHALL <regular_expression>
         <output variable> <input> [<input>...])

Match the regular expression as many times as possible and store the matches
in the output variable as a list.
All ``<input>`` arguments are concatenated before matching.

REGEX REPLACE
"""""""""""""

::

  string(REGEX REPLACE <regular_expression>
         <replace_expression> <output variable>
         <input> [<input>...])

Match the regular expression as many times as possible and substitute the
replacement expression for the match in the output.
All ``<input>`` arguments are concatenated before matching.

The replace expression may refer to paren-delimited subexpressions of the
match using ``\1``, ``\2``, ..., ``\9``.  Note that two backslashes (``\\1``)
are required in CMake code to get a backslash through argument parsing.

.. _`Regex Specification`:

Regex Specification
"""""""""""""""""""

The following characters have special meaning in regular expressions:

``^``
  Matches at beginning of input
``$``
  Matches at end of input
``.``
  Matches any single character
``[ ]``
  Matches any character(s) inside the brackets
``[^ ]``
  Matches any character(s) not inside the brackets
``-``
  Inside brackets, specifies an inclusive range between
  characters on either side e.g. ``[a-f]`` is ``[abcdef]``
  To match a literal ``-`` using brackets, make it the first
  or the last character e.g. ``[+*/-]`` matches basic
  mathematical operators.
``*``
  Matches preceding pattern zero or more times
``+``
  Matches preceding pattern one or more times
``?``
  Matches preceding pattern zero or once only
``|``
  Matches a pattern on either side of the ``|``
``()``
  Saves a matched subexpression, which can be referenced
  in the ``REGEX REPLACE`` operation. Additionally it is saved
  by all regular expression-related commands, including
  e.g. :command:`if(MATCHES)`, in the variables
  :variable:`CMAKE_MATCH_<n>` for ``<n>`` 0..9.

``*``, ``+`` and ``?`` have higher precedence than concatenation.  ``|``
has lower precedence than concatenation.  This means that the regular
expression ``^ab+d$`` matches ``abbd`` but not ``ababd``, and the regular
expression ``^(ab|cd)$`` matches ``ab`` but not ``abd``.

Manipulation
^^^^^^^^^^^^

APPEND
""""""

::

  string(APPEND <string variable> [<input>...])

Append all the input arguments to the string.

CONCAT
""""""

::

  string(CONCAT <output variable> [<input>...])

Concatenate all the input arguments together and store
the result in the named output variable.

TOLOWER
"""""""

::

  string(TOLOWER <string1> <output variable>)

Convert string to lower characters.

TOUPPER
"""""""

::

  string(TOUPPER <string1> <output variable>)

Convert string to upper characters.

LENGTH
""""""

::

  string(LENGTH <string> <output variable>)

Store in an output variable a given string's length.

SUBSTRING
"""""""""

::

  string(SUBSTRING <string> <begin> <length> <output variable>)

Store in an output variable a substring of a given string.  If length is
``-1`` the remainder of the string starting at begin will be returned.
If string is shorter than length then end of string is used instead.

.. note::
  CMake 3.1 and below reported an error if length pointed past
  the end of string.

STRIP
"""""

::

  string(STRIP <string> <output variable>)

Store in an output variable a substring of a given string with leading and
trailing spaces removed.

GENEX_STRIP
"""""""""""

::

  string(GENEX_STRIP <input string> <output variable>)

Strip any :manual:`generator expressions <cmake-generator-expressions(7)>`
from the ``input string`` and store the result in the ``output variable``.

Comparison
^^^^^^^^^^

::

  string(COMPARE LESS <string1> <string2> <output variable>)
  string(COMPARE GREATER <string1> <string2> <output variable>)
  string(COMPARE EQUAL <string1> <string2> <output variable>)
  string(COMPARE NOTEQUAL <string1> <string2> <output variable>)
  string(COMPARE LESS_EQUAL <string1> <string2> <output variable>)
  string(COMPARE GREATER_EQUAL <string1> <string2> <output variable>)

Compare the strings and store true or false in the output variable.

.. _`Supported Hash Algorithms`:

Hashing
^^^^^^^

::

  string(<HASH> <output variable> <input>)

Compute a cryptographic hash of the input string.
The supported ``<HASH>`` algorithm names are:

``MD5``
  Message-Digest Algorithm 5, RFC 1321.
``SHA1``
  US Secure Hash Algorithm 1, RFC 3174.
``SHA224``
  US Secure Hash Algorithms, RFC 4634.
``SHA256``
  US Secure Hash Algorithms, RFC 4634.
``SHA384``
  US Secure Hash Algorithms, RFC 4634.
``SHA512``
  US Secure Hash Algorithms, RFC 4634.
``SHA3_224``
  Keccak SHA-3.
``SHA3_256``
  Keccak SHA-3.
``SHA3_384``
  Keccak SHA-3.
``SHA3_512``
  Keccak SHA-3.

Generation
^^^^^^^^^^

ASCII
"""""

::

  string(ASCII <number> [<number> ...] <output variable>)

Convert all numbers into corresponding ASCII characters.

CONFIGURE
"""""""""

::

  string(CONFIGURE <string1> <output variable>
         [@ONLY] [ESCAPE_QUOTES])

Transform a string like :command:`configure_file` transforms a file.

RANDOM
""""""

::

  string(RANDOM [LENGTH <length>] [ALPHABET <alphabet>]
         [RANDOM_SEED <seed>] <output variable>)

Return a random string of given length consisting of
characters from the given alphabet.  Default length is 5 characters
and default alphabet is all numbers and upper and lower case letters.
If an integer ``RANDOM_SEED`` is given, its value will be used to seed the
random number generator.

TIMESTAMP
"""""""""

::

  string(TIMESTAMP <output variable> [<format string>] [UTC])

Write a string representation of the current date
and/or time to the output variable.

Should the command be unable to obtain a timestamp the output variable
will be set to the empty string "".

The optional ``UTC`` flag requests the current date/time representation to
be in Coordinated Universal Time (UTC) rather than local time.

The optional ``<format string>`` may contain the following format
specifiers:

::

   %%        A literal percent sign (%).
   %d        The day of the current month (01-31).
   %H        The hour on a 24-hour clock (00-23).
   %I        The hour on a 12-hour clock (01-12).
   %j        The day of the current year (001-366).
   %m        The month of the current year (01-12).
   %b        Abbreviated month name (e.g. Oct).
   %M        The minute of the current hour (00-59).
   %s        Seconds since midnight (UTC) 1-Jan-1970 (UNIX time).
   %S        The second of the current minute.
             60 represents a leap second. (00-60)
   %U        The week number of the current year (00-53).
   %w        The day of the current week. 0 is Sunday. (0-6)
   %a        Abbreviated weekday name (e.g. Fri).
   %y        The last two digits of the current year (00-99)
   %Y        The current year.

Unknown format specifiers will be ignored and copied to the output
as-is.

If no explicit ``<format string>`` is given it will default to:

::

   %Y-%m-%dT%H:%M:%S    for local time.
   %Y-%m-%dT%H:%M:%SZ   for UTC.


::

  string(MAKE_C_IDENTIFIER <input string> <output variable>)

Write a string which can be used as an identifier in C.

.. note::

  If the ``SOURCE_DATE_EPOCH`` environment variable is set,
  its value will be used instead of the current time.
  See https://reproducible-builds.org/specs/source-date-epoch/ for details.

UUID
""""

::

  string(UUID <output variable> NAMESPACE <namespace> NAME <name>
         TYPE <MD5|SHA1> [UPPER])

Create a univerally unique identifier (aka GUID) as per RFC4122
based on the hash of the combined values of ``<namespace>``
(which itself has to be a valid UUID) and ``<name>``.
The hash algorithm can be either ``MD5`` (Version 3 UUID) or
``SHA1`` (Version 5 UUID).
A UUID has the format ``xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx``
where each `x` represents a lower case hexadecimal character.
Where required an uppercase representation can be requested
with the optional ``UPPER`` flag.
