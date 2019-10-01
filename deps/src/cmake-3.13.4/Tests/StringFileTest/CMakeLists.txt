cmake_minimum_required (VERSION 2.6)
project(StringFileTest)
include_directories(${StringFileTest_BINARY_DIR})

# Read file test
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/InputFile.h.in" infile)

# Test reading a binary file into hex representation
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/test.bin" hexContents HEX)

if("${hexContents}" STREQUAL "0001027700")
  message("file(READ HEX) correctly read [${hexContents}]")
else()
  message(SEND_ERROR "file(READ HEX) incorrectly read [${hexContents}], but expected was [0001027700]")
endif()

# file(STRINGS) test
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/InputFile.h.in" infile_strings
  LIMIT_COUNT 1 LIMIT_INPUT 1024 LIMIT_OUTPUT 1024
  LENGTH_MINIMUM 10 LENGTH_MAXIMUM 23 REGEX include NEWLINE_CONSUME)
set(infile_strings_goal "#include \"includefile\"\n")
if("${infile_strings}" STREQUAL "${infile_strings_goal}")
  message("file(STRINGS) correctly read [${infile_strings}]")
else()
  message(SEND_ERROR
    "file(STRINGS) incorrectly read [${infile_strings}]")
endif()

# test reading a file and getting its binary data as hex string
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/main.srec" infilehex LIMIT 4 HEX)
if(NOT "${infilehex}" STREQUAL "53313036")
  message(SEND_ERROR
    "file(READ ... HEX) error, read: \"${infilehex}\", expected \"53313036\"")
endif()


# test that file(STRINGS) also work with Intel hex and Motorola S-record files
# this file has been created with "sdcc main.c"
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/main.ihx" infile_strings REGEX INFO)
set(infile_strings_goal "INFO:compiler\\[SDCC-HEX\\]")
if("${infile_strings}" MATCHES "${infile_strings_goal}")
  message("file(STRINGS) correctly read from hex file [${infile_strings}]")
else()
  message(SEND_ERROR
    "file(STRINGS) incorrectly read from hex file [${infile_strings}]")
endif()

# this file has been created with "sdcc main.c --out-fmt-s19"
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/main.srec" infile_strings REGEX INFO)
set(infile_strings_goal "INFO:compiler\\[SDCC-SREC\\]")
if("${infile_strings}" MATCHES "${infile_strings_goal}")
  message("file(STRINGS) correctly read from srec file [${infile_strings}]")
else()
  message(SEND_ERROR
    "file(STRINGS) incorrectly read from srec file [${infile_strings}]")
endif()

#this file has utf-8 content
file(STRINGS test.utf8 infile_strings ENCODING UTF-8)
list(LENGTH infile_strings content_len)
if(content_len MATCHES "3")
  message("file(STRINGS) correctly read from utf8 file [${infile_strings}]")
else()
  message(SEND_ERROR
    "file(STRINGS) incorrectly read from utf8 file [${infile_strings}]")
endif()

# String test
string(REGEX MATCH "[cC][mM][aA][kK][eE]" rmvar "CMake is great")
string(REGEX MATCHALL "[cC][mM][aA][kK][eE]" rmallvar "CMake is better than cmake or CMake")
string(REGEX REPLACE "[Aa][uU][tT][oO]([cC][oO][nN][fF]|[mM][aA][kK][eE])"
  "CMake" rrepvar "People should use Autoconf and Automake")
string(COMPARE EQUAL "CMake" "Autoconf" nceqvar)
string(COMPARE EQUAL "CMake" "CMake" ceqvar)
string(COMPARE NOTEQUAL "CMake" "Autoconf" cneqvar)
string(COMPARE NOTEQUAL "CMake" "CMake" ncneqvar)
string(COMPARE LESS "before" "after" nclvar)
string(COMPARE LESS "max" "min" clvar)
string(COMPARE GREATER "before" "after" cgvar)
string(COMPARE GREATER "max" "min" ncgvar)
string(ASCII 67 109 97 107 101 savar)
string(TOUPPER "CMake" tuvar)
string(TOLOWER "CMake" tlvar)
string(REPLACE "Autoconf" "CMake" repvar "People should use Autoconf")

if("abc" STREQUAL "xyz")
  message(SEND_ERROR "Problem with the if(STREQUAL), \"abc\" and \"xyz\" considered equal")
endif()

if("CMake is cool" MATCHES "(CMake) (is).+")
  if(NOT "${CMAKE_MATCH_0}" STREQUAL "CMake is cool")
    message(SEND_ERROR "CMAKE_MATCH_0 wrong: \"${CMAKE_MATCH_0}\", expected \"CMake is cool\"")
  endif()
  if(NOT "${CMAKE_MATCH_1}" STREQUAL "CMake")
    message(SEND_ERROR "CMAKE_MATCH_1 wrong: \"${CMAKE_MATCH_1}\", expected \"CMake\"")
  endif()
  if(NOT "${CMAKE_MATCH_2}" STREQUAL "is")
    message(SEND_ERROR "CMAKE_MATCH_2 wrong: \"${CMAKE_MATCH_2}\", expected \"is\"")
  endif()
else()
  message(SEND_ERROR "Problem with the if(MATCHES), no match found")
endif()

string(REGEX MATCH "(People).+CMake" matchResultVar "People should use CMake")
if(NOT "${matchResultVar}" STREQUAL "People should use CMake")
  message(SEND_ERROR "string(REGEX MATCH) problem: \"${matchResultVar}\", expected \"People should use CMake\"")
endif()
if(NOT "${CMAKE_MATCH_0}" STREQUAL "People should use CMake")
  message(SEND_ERROR "CMAKE_MATCH_0 wrong: \"${CMAKE_MATCH_0}\", expected \"People should use CMake\"")
endif()
if(NOT "${CMAKE_MATCH_1}" STREQUAL "People")
  message(SEND_ERROR "CMAKE_MATCH_1 wrong: \"${CMAKE_MATCH_1}\", expected \"People\"")
endif()
if(NOT "${CMAKE_MATCH_2}" STREQUAL "")
  message(SEND_ERROR "CMAKE_MATCH_2 wrong: \"${CMAKE_MATCH_2}\", expected empty string")
endif()


string(STRIP "
  ST1
  " ST1)
string(STRIP "ST2 " ST2)
string(STRIP "  ST3" ST3)

foreach(var ST1 ST2 ST3)
  if("x${var}" STREQUAL "x${${var}}")
    message("[${var}] == [${${var}}]")
  else()
    message(SEND_ERROR "Problem with the STRIP command for ${var}: [${${var}}]")
  endif()
endforeach()

string(SUBSTRING "People should use Autoconf" 7 10 substringres)
set(substringres "Everybody ${substringres} CMake")

string(LENGTH ${substringres} lengthres)

file(RELATIVE_PATH relpath "/usr/local/bin" "/usr/X11R6/bin/xnest")

# Make-style unquoted argument test
set(var $(VAR1)$(VAR2)/$(VAR3))
message("Output: [${var}]")
string(COMPARE EQUAL "${var}" "$(VAR1)$(VAR2)/$(VAR3)" result)
if(NOT result)
  message(SEND_ERROR "Unquoted $(VAR) syntax is broken.")
endif()

# Make directories test
file(MAKE_DIRECTORY
  "${CMAKE_CURRENT_BINARY_DIR}/Includes"
  "${CMAKE_CURRENT_BINARY_DIR}/Directory1"
  "${CMAKE_CURRENT_BINARY_DIR}/Directory2"
  )

# Write results to the file (test write file)
set(file "${CMAKE_CURRENT_BINARY_DIR}/Includes/Values.h")
file(WRITE "${file}" "/* this file is generated */\n")
foreach(var
    rmvar
    rmallvar
    rrepvar
    repvar
    relpath
    substringres
    lengthres
    nceqvar
    ceqvar
    cneqvar
    ncneqvar
    nclvar
    clvar
    cgvar
    ncgvar
    savar
    tuvar
    tlvar)
  file(APPEND "${file}" "#define ${var} \"${${var}}\"\n")
endforeach()

# Verify that the file was created recently.
if(NOT "${file}" IS_NEWER_THAN "${CMAKE_CURRENT_SOURCE_DIR}/InputFile.h.in")
  message(FATAL_ERROR "if(FILE_IS_NEWER) does not seem to work.")
endif()

# Test configuration of the string
set(TEST_DEFINED 123)
set(TEST_NOT_DEFINED)
string(CONFIGURE "${infile}" infile+-/out @ONLY)
set(infile "${infile+-/out}")

# Write include file to a file
string(REGEX REPLACE "includefile" "Includes/Values.h" outfile "${infile}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/OutputFile.h-tmp" "${outfile}")
file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/OutputFile.h-tmp"
            "${CMAKE_CURRENT_BINARY_DIR}/OutputFile.h")

# Test file copy with relative paths
file(COPY .
  DESTINATION src
  FILE_PERMISSIONS OWNER_READ # test no OWNER_WRITE
  DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  FILES_MATCHING PATTERN *.cxx # Only copy the main source file
  REGEX /src$ EXCLUDE # Block recursion for in-source build
  )

# Test file glob
file(GLOB_RECURSE src_files "${CMAKE_CURRENT_SOURCE_DIR}/*")
message("Files in ${CMAKE_CURRENT_SOURCE_DIR} are ${src_files}")
set(expr "${CMAKE_CURRENT_BINARY_DIR}/src/[sS][!a-su-zA-Z0-9][^a-qs-zA-Z0-9]ing?ile*.cxx")
message("Glob expression is [${expr}].")
file(GLOB src_files "${expr}")
message("Globbed files [${src_files}].")
add_executable(StringFileTest ${src_files})

set(expr "${CMAKE_CURRENT_SOURCE_DIR}/../*.cxx")
file(GLOB_RECURSE rel_src_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/.." "${expr}")
message("Globbed files [${rel_src_files}].")

# Test FOREACH range
message("Cheack if FOREACH with RANGE works")
macro(TEST_RANGE ARGS CHECK)
  set(r)
  foreach(a RANGE ${ARGS})
    set(r ${r} ${a})
  endforeach()
  message("FOREACH with RANGE ${ARGS} produces ${r}")
  if("x${r}x" MATCHES "^x${CHECK}x$")
  else()
    message(SEND_ERROR "The range resulted in: ${r} should be ${CHECK}")
  endif()
endmacro()
TEST_RANGE("5" "0;1;2;3;4;5")
TEST_RANGE("3;5" "3;4;5")
TEST_RANGE("5;3" "5;4;3")
TEST_RANGE("3;10;2" "3;5;7;9")
TEST_RANGE("10;0;-3" "10;7;4;1")

# Test FOREACH IN signature
set(list1 "" a "")
set(list2 a "" b)
set(var_)
set(var_a)
set(var_b)
foreach(item IN LISTS list1 list2 ITEMS "" a "")
  set(var_${item} "${var_${item}}x")
endforeach()
if(NOT "${var_}" STREQUAL "xxxxx")
  message(FATAL_ERROR "count incorrect for \"\": [${var_}]")
endif()
if(NOT "${var_a}" STREQUAL "xxx")
  message(FATAL_ERROR "count incorrect for \"a\": [${var_a}]")
endif()
if(NOT "${var_b}" STREQUAL "x")
  message(FATAL_ERROR "count incorrect \"b\": [${var_b}]")
endif()

# Test SUBSTRING command
set(ST_INPUTSTRING "0123456789")
string(SUBSTRING ${ST_INPUTSTRING} 3 0 ST_EMPTY)
string(SUBSTRING ${ST_INPUTSTRING} 1 1 ST_ONE)
string(SUBSTRING ${ST_INPUTSTRING} 0 10 ST_ALL)
string(SUBSTRING ${ST_INPUTSTRING} 0 -1 ST_ALL_MINUS)
string(SUBSTRING ${ST_INPUTSTRING} 9 -1 ST_NINE)

if(ST_EMPTY)
  message(SEND_ERROR "SUBSTRING with length 0 does not return an empty string")
endif()
if(NOT ST_ONE STREQUAL "1")
  message(SEND_ERROR "SUBSTING command does not cut the correct selected character, was \"" ${ST_ONE} "\", should be \"1\"")
endif()
if(NOT ST_INPUTSTRING STREQUAL ST_ALL)
  message(SEND_ERROR "SUBSTRING does not return the whole string when selected with length")
endif()
if(NOT ST_INPUTSTRING STREQUAL ST_ALL_MINUS)
  message(SEND_ERROR "SUBSTRING does not return the whole string when selected with -1")
endif()
if(NOT ST_NINE STREQUAL "9")
  message(SEND_ERROR "SUBSTRING does not return the tail when selected with -1")
endif()

string(MAKE_C_IDENTIFIER "1one-two$" MCI_1)

if(NOT MCI_1 STREQUAL _1one_two_)
  message(SEND_ERROR "MAKE_C_IDENTIFIER did not create expected result.")
endif()

string(GENEX_STRIP "one;$<1:two;three>;four;$<TARGET_OBJECTS:some_target>" strip_result)

if (NOT strip_result STREQUAL "one;four")
  message(SEND_ERROR "GENEX_STRIP did not create expected result: ${strip_result}")
endif()
