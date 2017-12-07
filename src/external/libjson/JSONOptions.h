#ifndef JSON_OPTIONS_H
#define JSON_OPTIONS_H

/**
 *  This file holds all of the compiling options for easy access and so
 *  that you don't have to remember them, or look them up all the time
 */


/*
 *  JSON_LIBRARY must be declared if libjson is compiled as a static or dynamic 
 *  library.  This exposes a C-style interface, but none of the inner workings of libjson
 */
// #define JSON_LIBRARY


/*
 *  JSON_STRICT removes all of libjson's extensions.  Meaning no comments, no special numbers
 */
//#define JSON_STRICT


/*
 *  JSON_DEBUG is used to perform extra error checking.  Because libjson usually 
 *  does on the fly parsing, validation is impossible, so this option will allow
 *  you to register an error callback so that you can record what is going wrong 
 *  before the library crashes.  This option does not protect from these errors,
 *  it simply tells you about them, which is nice for debugging, but not preferable
 *  for release candidates
 */
//#define JSON_DEBUG


/*
 *  JSON_ISO_STRICT turns off all code that uses non-standard C++.  This removes all
 *  references to long long and long double as well as a few others
 */
//#define JSON_ISO_STRICT


/*
 *  JSON_SAFE performs similarly to JSON_DEBUG, except this option does protect
 *  from the errors that it encounters.  This option is recommended for those who
 *  feel it's possible for their program to encounter invalid json.
 */
#define JSON_SAFE


/*
 *  JSON_STDERROR routes error messages to cerr instead of a callback, this 
 *  option hides the callback registering function.  This will usually display
 *  messages in the console
 */
//#define JSON_STDERROR


/*
 *  JSON_PREPARSE causes all parsing to be done immediately.  By default, libjson
 *  parses nodes on the fly as they are needed, this makes parsing much faster if
 *  your program gets a lot of information that it doesn't need.  An example of
 *  this would be a client application communicating with a server if the server
 *  returns things like last modified date and other things that you don't use.
 */
//#define JSON_PREPARSE


/*
 *  JSON_LESS_MEMORY will force libjson to let go of memory as quickly as it can
 *  this is recommended for software that has to run on less than optimal machines.
 *  It will cut libjson's memory usage by about 20%, but also run slightly slower.
 *  It's recommended that you also compile using the -Os option, as this will also
 *  reduce the size of the library
 */
//#define JSON_LESS_MEMORY


/*
 *  JSON_UNICODE tells libjson to use wstrings instead of regular strings, this
 *  means that libjson supports the full array of unicode characters, but also takes
 *  much more memory and processing power.
 */
//#define JSON_UNICODE


/*
 *  JSON_REF_COUNT causes libjson to reference count JSONNodes, which makes copying
 *  and passing them around much faster.  It is recommended that this stay on for
 *  most uses
 */
#define JSON_REF_COUNT


/*
 *  JSON_BINARY is used to support binary, which is base64 encoded and decoded by libjson,
 *  if this option is not turned off, no base64 support is included
 */
// #define JSON_BINARY


/*
 *  JSON_EXPOSE_BASE64 is used to turn on the functionality of libjson's base64 encoding
 *  and decoding.  This may be useful if you want to obfuscate your json, or send binary data over
 *  a network
 */
// #define JSON_EXPOSE_BASE64


/*
 *  JSON_ITERATORS turns on all of libjson's iterating functionality.  This would usually
 *  only be turned off while compiling for use with C
 */
#define JSON_ITERATORS


/*
 *  JSON_STREAM turns on libjson's streaming functionality.  This allows you to give parts of 
 *  your json into a stream, which will automatically hit a callback when full nodes are
 *  completed
 */
#define JSON_STREAM


/*
 *  JSON_MEMORY_CALLBACKS exposes functions to register callbacks for allocating, resizing,
 *  and freeing memory.  Because libjson is designed for customizability, it is feasible
 *  that some users would like to further add speed by having the library utilize a memory
 *  pool.  With this option turned on, the default behavior is still done internally unless
 *  a callback is registered.  So you can have this option on and not use it.
 */
//#define JSON_MEMORY_CALLBACKS


/*
 *  JSON_MEMORY_MANAGE is used to create functionality to automatically track and clean
 *  up memory that has been allocated by the user.  This includes strings, binary data, and
 *  nodes.  It also exposes bulk delete functions.
 */
//#define JSON_MEMORY_MANAGE


/*
 *	JSON_MEMORY_POOL Turns on libjson's iteraction with mempool++.  It is more efficient that simply
 *	connecting mempool++ to the callbacks because it integrates things internally and uses a number
 *	of memory pools.  This value tells libjson how large of a memory pool to start out with.  500KB 
 *	should suffice for most cases.  libjson will distribute that within the pool for the best
 *	performance depending on other settings.
 */
//#define JSON_MEMORY_POOL 524288


/*
 *  JSON_MUTEX_CALLBACKS exposes functions to register callbacks to lock and unlock
 *  mutexs and functions to lock and unlock JSONNodes and all of it's children.  This 
 *  does not prevent other threads from accessing the node, but will prevent them from
 *  locking it. It is much easier for the end programmer to allow libjson to manage
 *  your mutexs because of reference counting and manipulating trees, libjson automatically
 *  tracks mutex controls for you, so you only ever lock what you need to
 */
//#define JSON_MUTEX_CALLBACKS


/*
 *  JSON_MUTEX_MANAGE lets you set mutexes and forget them, libjson will not only keep
 *  track of the mutex, but also keep a count of how many nodes are using it, and delete
 *  it when there are no more references
 */
//#define JSON_MUTEX_MANAGE


/*
 *  JSON_NO_C_CONSTS removes consts from the C interface.  It still acts the same way, but
 *  this may be useful for using the header with languages or variants that don't have const
 */
//#define JSON_NO_C_CONSTS


/*
 *  JSON_OCTAL allows libjson to use octal values in numbers.
 */
//#define JSON_OCTAL


/*
 *  JSON_WRITE_PRIORITY turns on libjson's writing capabilties.  Without this libjson can only
 *  read and parse json, this allows it to write back out.  Changing the value of the writer
 *  changes how libjson compiles, and how fast it will go when writing
 */
#define JSON_WRITE_PRIORITY MED


/*
 *  JSON_READ_PRIORITY turns on libjson's reading capabilties.  Changing the value of the reader
 *  changes how libjson compiles, and how fast it will go when writing
 */
#define JSON_READ_PRIORITY HIGH


/*
 *  JSON_NEWLINE affects how libjson writes.  If this option is turned on, libjson
 *  will use whatever it's defined as for the newline signifier, otherwise, it will use 
 *  standard unix \n.
 */
//#define JSON_NEWLINE "\r\n"  //\r\n is standard for most windows and dos programs


/*
 *  JSON_INDENT affects how libjson writes.  If this option is turned on, libjson
 *  will use \t to indent formatted json, otherwise it will use the number of characters
 *  that you specify.  If this is not turned on, then it will use the tab (\t) character
 */
//#define JSON_INDENT "    "


/*
 *  JSON_ESCAPE_WRITES tells the libjson engine to escape special characters when it writes
 *  out.  If this option is turned off, the json it outputs may not adhere to JSON standards
 */
#define JSON_ESCAPE_WRITES


/*
 *  JSON_COMMENTS tells libjson to store and write comments.  libjson always supports
 *  parsing json that has comments in it as it simply ignores them, but with this option
 *  it keeps the comments and allows you to insert further comments
 */
#define JSON_COMMENTS


/*
 *  JSON_WRITE_BASH_COMMENTS will cause libjson to write all comments in bash (#) style
 *  if this option is not turned on, then it will use C-style comments.  Bash comments are
 *  all single line
 */
//#define JSON_WRITE_BASH_COMMENTS


/*
 *  JSON_WRITE_SINGLE_LINE_COMMENTS will cause libjson to write all comments in using //
 *  notation, or (#) if that option is on.  Some parsers do not support multiline C comments
 *  although, this option is not needed for bash comments, as they are all single line anyway
 */
//#define JSON_WRITE_SINGLE_LINE_COMMENTS


/*
 *  JSON_ARRAY_SIZE_ON_ON_LINE allows you to put small arrays of primitives all on one line
 *  in a write_formatted.  This is common for tuples, like coordinates.  If must be defined 
 *  as an integer
 */
//#define JSON_ARRAY_SIZE_ON_ONE_LINE 2


/*
 *  JSON_VALIDATE turns on validation features of libjson.
 */
#define JSON_VALIDATE


/*
 *  JSON_CASE_INSENSITIVE_FUNCTIONS turns on funtions for finding child nodes in a case-
 *  insenititve way
 */
#define JSON_CASE_INSENSITIVE_FUNCTIONS


/*
 *  JSON_INDEX_TYPE allows you th change the size type for the children functions. If this 
 *  option is not used then unsigned int is used.  This option is useful for cutting down
 *  on memory, or using huge numbers of child nodes (over 4 billion)
 */
//#define JSON_INDEX_TYPE unsigned int


/*
 *  JSON_BOOL_TYPE lets you change the bool type for the C interface.  Because before C99 there
 *  was no bool, and even then it's just a typedef, you may want to use something else.  If this 
 *  is not defined, it will revert to int
 */
//#define JSON_BOOL_TYPE char


/*
 *  JSON_INT_TYPE lets you change the int type for as_int.  If you ommit this option, the default
 *  long will be used
 */
//#define JSON_INT_TYPE long


/*
 *  JSON_NUMBER_TYPE lets you change the number type for as_float as well as the internal storage for the
 *	number.  If you omit this option, the default double will be used for most cases and float for JSON_LESS_MEMORY
 */
//#define JSON_NUMBER_TYPE double


/*
 *  JSON_STRING_HEADER allows you to change the type of string that libjson uses both for the
 *  interface and internally.  It must implement most of the STL string interface, but not all
 *  of it.  Things like wxString or QString should wourk without much trouble
 */
//#define JSON_STRING_HEADER "../TestSuite/StringTest.h"


/*
 *  JSON_UNIT_TEST is used to maintain and debug the libjson.  It makes all private
 *  members and functions public so that tests can do checks of the inner workings
 *  of libjson.  This should not be turned on by end users.
 */
//#define JSON_UNIT_TEST


/*
 *  JSON_NO_EXCEPTIONS turns off any exception throwing by the library.  It may still use exceptions
 *  internally, but the interface will never throw anything.
 */
//#define JSON_NO_EXCEPTIONS


/*
 *  JSON_DEPRECATED_FUNCTIONS turns on functions that have been deprecated, this is for backwards
 *  compatibility between major releases.  It is highly recommended that you move your functions
 *  over to the new equivalents
 */
#define JSON_DEPRECATED_FUNCTIONS


/*
 *  JSON_CASTABLE allows you to call as_bool on a number and have it do the 0 or not 0 check,
 *  it also allows you to ask for a string from a number, or boolean, and have it return the right thing.
 *  Without this option, those types of requests are undefined.  It also exposes the as_array, as_node, and cast 
 *  functions
 */
#define JSON_CASTABLE


/*
 *  JSON_SECURITY_MAX_NEST_LEVEL is a security measure added to make prevent against DoS attacks
 *  This only affects validation, as if you are worried about security attacks, then you are
 *  most certainly validating json before sending it to be parsed.  This option allows you to limitl how many
 *  levels deep a JSON Node can go.  128 is a good depth to start with
 */
#define JSON_SECURITY_MAX_NEST_LEVEL 128


/*
 *  JSON_SECURITY_MAX_STRING_LENGTH is another security measure, preventing DoS attacks with very long
 *  strings of JSON.  32MB is the default value for this, this allows large images to be embedded
 */
#define JSON_SECURITY_MAX_STRING_LENGTH 33554432


/*
 *	JSON_SECURITY_MAX_STREAM_OBJECTS is a security measure for streams.  It prevents DoS attacks with
 *	large number of objects hitting the stream all at once.  128 is a lot of objects, but not out of
 *	the question for high speed systems.
 */
#define JSON_SECURITY_MAX_STREAM_OBJECTS 128

#endif

