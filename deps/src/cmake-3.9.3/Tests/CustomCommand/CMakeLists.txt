#
# Wrapping
#
cmake_minimum_required (VERSION 2.6)
project (CustomCommand)

add_subdirectory(GeneratedHeader)

#
# Lib and exe path
#
if(NOT DEFINED bin_dir)
  set(bin_dir "bin")
endif()

set (LIBRARY_OUTPUT_PATH
  ${PROJECT_BINARY_DIR}/${bin_dir} CACHE INTERNAL
  "Single output directory for building all libraries.")

set (EXECUTABLE_OUTPUT_PATH
  ${PROJECT_BINARY_DIR}/${bin_dir} CACHE INTERNAL
  "Single output directory for building all executables.")

################################################################
#
#  First test using a compiled generator to create a .c file
#
################################################################
# add the executable that will generate the file
add_executable(generator generator.cxx)

################################################################
#
#  Test using a wrapper to wrap a header file
#
################################################################
# add the executable that will generate the file
add_executable(wrapper wrapper.cxx)

add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/wrapped.c ${PROJECT_BINARY_DIR}/wrapped_help.c
  DEPENDS wrapper
  MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/wrapped.h
  COMMAND ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/wrapper
  ${PROJECT_BINARY_DIR}/wrapped.c ${PROJECT_BINARY_DIR}/wrapped_help.c
  ${CMAKE_CFG_INTDIR} # this argument tests passing of the configuration
  VERBATIM # passing of configuration should work in this mode
  )

################################################################
#
#  Test creating files from a custom target
#
################################################################
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}//doc1.dvi # test 2 slashes
  DEPENDS ${PROJECT_SOURCE_DIR}/doc1.tex
  COMMAND   ${CMAKE_COMMAND}
  ARGS      -E copy ${PROJECT_SOURCE_DIR}/doc1.tex
  ${PROJECT_BINARY_DIR}/doc1.dvi
  )

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/doc1.h
  COMMAND ${CMAKE_COMMAND} -E echo " Copying doc1.dvi to doc1temp.h."
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/doc1.dvi
                                   ${PROJECT_BINARY_DIR}/doc1temp.h
  )
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/doc1.h APPEND
  DEPENDS ${PROJECT_BINARY_DIR}/doc1.dvi
  COMMAND ${CMAKE_COMMAND} -E echo " Copying doc1temp.h to doc1.h."
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/doc1temp.h
                                   ${PROJECT_BINARY_DIR}/doc1.h
  COMMAND ${CMAKE_COMMAND} -E echo " Removing doc1temp.h."
  COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_BINARY_DIR}/doc1temp.h
  )

# Add custom command to generate foo.h.
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/foo.h
  DEPENDS ${PROJECT_SOURCE_DIR}/foo.h.in
  COMMAND ${CMAKE_COMMAND} -E echo " Copying foo.h.in to foo.h."
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/foo.h.in
                                   ${PROJECT_BINARY_DIR}/foo.h
  )

# Add the location of foo.h to the include path.
include_directories(${PROJECT_BINARY_DIR})

# Test generation of a file to the build tree without full path.  As
# of CMake 2.6 custom command outputs specified by relative path go in
# the build tree.
add_custom_command(
  OUTPUT doc1.txt
  COMMAND ${CMAKE_COMMAND} -E echo "Example Document Target" > doc1.txt
  DEPENDS doc1.tex
  VERBATIM
  )

# Add a custom target to drive generation of doc1.h.
add_custom_target(TDocument ALL
  COMMAND ${CMAKE_COMMAND} -E echo " Copying doc1.h to doc2.h."
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/doc1.h
                                   ${PROJECT_BINARY_DIR}/doc2.h
  DEPENDS doc1.txt ${PROJECT_BINARY_DIR}//doc1.h # test 2 slashes
  COMMENT "Running top-level TDocument commands"
  SOURCES doc1.tex
  )

# Setup a pre- and post-build pair that will fail if not run in the
# proper order.
add_custom_command(
  TARGET TDocument PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo " Writing doc1pre.txt."
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/doc1.tex ${PROJECT_BINARY_DIR}/doc1pre.txt
  COMMENT "Running TDocument pre-build commands"
  )
add_custom_command(
  TARGET TDocument POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo " Copying doc1pre.txt to doc2post.txt."
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/doc1pre.txt
                                   ${PROJECT_BINARY_DIR}/doc2post.txt
  BYPRODUCTS ${PROJECT_BINARY_DIR}/doc2post.txt
  COMMENT "Running TDocument post-build commands"
  )

# Setup a custom target that will fail if the POST_BUILD custom command
# isn't run before it.
add_custom_command(
  OUTPUT doc3post.txt
  DEPENDS ${PROJECT_BINARY_DIR}/doc2post.txt
  COMMAND ${CMAKE_COMMAND} -E echo " Copying doc2pre.txt to doc3post.txt."
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/doc2post.txt
                                   ${PROJECT_BINARY_DIR}/doc3post.txt
  COMMENT "Running TDocument post-build dependent custom command"
  )
add_custom_target(doc3Post ALL DEPENDS doc3post.txt)
add_dependencies(doc3Post TDocument)

################################################################
#
#  Test using a multistep generated file
#
################################################################
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/foo.pre
  DEPENDS ${PROJECT_SOURCE_DIR}/foo.in
          TDocument # Ensure doc1.h generates before this target
  COMMAND   ${CMAKE_COMMAND}
  ARGS      -E copy ${PROJECT_SOURCE_DIR}/foo.in
  ${PROJECT_BINARY_DIR}/foo.pre
  )

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/foo.c
  DEPENDS   ${PROJECT_BINARY_DIR}/foo.pre
  COMMAND   ${CMAKE_COMMAND}
  ARGS      -E copy ${PROJECT_BINARY_DIR}/foo.pre
  ${PROJECT_BINARY_DIR}/foo.c
  )

# Test using OBJECT_DEPENDS to bring in a custom command.
# Use a path that can be simplified to make sure paths
# are consistently normalized.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/subdir/../subdir/subdir.h
  COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/subdir.h.in
            ${CMAKE_CURRENT_BINARY_DIR}/subdir/subdir.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/subdir.h.in
  )
set_property(SOURCE ${PROJECT_BINARY_DIR}/foo.c PROPERTY
  OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/subdir/../subdir/subdir.h)

# Add custom command to generate not_included.h, which is a header
# file that is not included by any source in this project.  This will
# test whether all custom command outputs explicitly listed as sources
# get generated even if they are not needed by an object file.
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/not_included.h
  DEPENDS ${PROJECT_SOURCE_DIR}/foo.h.in
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/foo.h.in
                                   ${PROJECT_BINARY_DIR}/not_included.h
  )

# add the executable
add_executable(CustomCommand
  ${PROJECT_BINARY_DIR}/foo.h
  ${PROJECT_BINARY_DIR}/foo.c
  ${PROJECT_BINARY_DIR}/wrapped.c
  ${PROJECT_BINARY_DIR}/wrapped_help.c
  ${PROJECT_BINARY_DIR}/generated.c
  ${PROJECT_BINARY_DIR}/not_included.h
  gen_redirect.c # default location for custom commands is in build tree
  )

# Add the rule to create generated.c at build time.  This is placed
# here to test adding the generation rule after referencing the
# generated source in a target.
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/generated.c
  DEPENDS $<1:generator> $<0:does_not_exist>
  COMMAND generator
  ARGS ${PROJECT_BINARY_DIR}/generated.c
  )

target_link_libraries(CustomCommand GeneratedHeader)

##############################################################################
# Test for using just the target name as executable in the COMMAND
# section. Has to be recognized and replaced by CMake with the output
# actual location of the executable.
# Additionally the generator is created in an extra subdir after the
# add_custom_command() is used.
#
# Test the same for add_custom_target()

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated_extern.cxx
  COMMAND generator_extern ${CMAKE_CURRENT_BINARY_DIR}/generated_extern.cxx
  )

add_executable(CustomCommandUsingTargetTest main.cxx ${CMAKE_CURRENT_BINARY_DIR}/generated_extern.cxx )

add_custom_target(RunTarget
  COMMAND generator_extern ${CMAKE_CURRENT_BINARY_DIR}/run_target.cxx
  )

add_custom_command(TARGET CustomCommandUsingTargetTest POST_BUILD
                   COMMAND dummy_generator ${CMAKE_CURRENT_BINARY_DIR}/generated_dummy.cxx)

add_subdirectory(GeneratorInExtraDir)

##############################################################################
# Test shell operators in custom commands.

add_executable(tcat tcat.cxx)

# Test that list expansion from a generator expression works.
set_property(TARGET tcat PROPERTY DEPSLIST tcat gen_redirect_in.c)

add_custom_command(OUTPUT gen_redirect.c
  DEPENDS $<TARGET_PROPERTY:tcat,DEPSLIST>
  COMMAND tcat < ${CMAKE_CURRENT_SOURCE_DIR}/gen_redirect_in.c > gen_redirect.c
  COMMAND ${CMAKE_COMMAND} -E echo "#endif" >> gen_redirect.c
  VERBATIM
  )

##############################################################################
# Test non-trivial command line arguments in custom commands.
set(EXPECTED_ARGUMENTS)
set(CHECK_ARGS)
if(NOT MSVC71)
  set(CHECK_ARGS -DPATH=c:/posix/path)
endif()
set(CHECK_ARGS
  ${CHECK_ARGS}
  c:/posix/path
  c:\\windows\\path
  'single-quotes'
  single'quote
  \"double-quotes\"
  "\\;semi-colons\\;"
  "semi\\;colon"
  `back-ticks`
  back`tick
  "(parens)"
  "(lparen"
  "rparen)"
  {curly}
  {lcurly}
  rcurly}
  <angle>
  <langle
  rangle>
  [square]
  [lsquare # these have funny behavior due to special cases for
  rsquare] # windows registry value names in list expansion
  $dollar-signs$
  dollar$sign
  &ampersands&x # Borland make does not like trailing ampersand
  one&ampersand
  @two-ats@
  one@at
  ~two-tilda~
  one~tilda
  ^two-carrots^
  one^carrot
  %two-percents%
  one%percent
  !two-exclamations!
  one!exclamation
  ?two-questions?
  one?question
  *two-stars*
  one*star
  =two+equals=
  one=equals
  _two-underscores_
  one_underscore
  ,two-commas,
  one,comma
  .two-periods.
  one.period
  |two-pipes|
  one|pipe
  |nopipe
  "#two-pounds#"
  "one#pound"
  "#nocomment"
  "c:/posix/path/with space"
  "c:\\windows\\path\\with space"
  "'single quotes with space'"
  "single'quote with space"
  "\"double-quotes with space\""
  "\\;semi-colons w s\\;"
  "semi\\;colon w s"
  "`back-ticks` w s"
  "back`tick w s"
  "(parens) w s"
  "(lparen w s"
  "rparen) w s"
  "{curly} w s"
  "{lcurly w s"
  "rcurly} w s"
  "<angle> w s"
  "<langle w s"
  "rangle> w s"
  "[square] w s"
  "[lsquare w s" # these have funny behavior due to special cases for
  "rsquare] w s" # windows registry value names in list expansion
  "$dollar-signs$ w s"
  "dollar$sign w s"
  "&ampersands& w s"
  "one&ampersand w s"
  "@two-ats@ w s"
  "one@at w s"
  "~two-tilda~ w s"
  "one~tilda w s"
  "^two-carrots^ w s"
  "one^carrot w s"
  "%two-percents% w s"
  "one%percent w s"
  "!two-exclamations! w s"
  "one!exclamation w s"
  "*two-stars* w s"
  "one*star w s"
  "=two+equals= w s"
  "one=equals w s"
  "_two-underscores_ w s"
  "one_underscore w s"
  "?two-questions? w s"
  "one?question w s"
  ",two-commas, w s"
  "one,comma w s"
  ".two-periods. w s"
  "one.period w s"
  "|two-pipes| w s"
  "one|pipe w s"
  "#two-pounds# w s"
  "one#pound w s"
  ~ ` ! @ \# $ % ^ & _ - + = : "\;" \" ' , . ? "(" ")" { } []
  )
if(NOT MINGW)
  # *  # MinGW programs on windows always expands the wildcard!
  # /  # MSys make converts a leading slash to the mingw home directory
  list(APPEND CHECK_ARGS * /)
endif()

# The windows command shell does not support a double quote by itself:
#   double\"quote
# without messing up quoting of arguments following it.

# Make tools need help with escaping a single backslash
#   \
# at the end of a command because they think it is a continuation
# character.

# We now have special cases for shell operators:
#   | < > << >> &> 2>&1 1>&2
# to allow custom commands to perform redirection.

foreach(arg ${CHECK_ARGS} "")
  set(ARG "${arg}")
  string(REPLACE "\\" "\\\\" ARG "${ARG}")
  string(REPLACE "\"" "\\\"" ARG "${ARG}")
  string(APPEND EXPECTED_ARGUMENTS
    "  \"${ARG}\",
")
endforeach()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/check_command_line.c.in
               ${CMAKE_CURRENT_BINARY_DIR}/check_command_line.c
               @ONLY)
add_executable(check_command_line
  ${CMAKE_CURRENT_BINARY_DIR}/check_command_line.c)
set(output_name "check_command_line")
set_property(TARGET check_command_line
  PROPERTY OUTPUT_NAME ${output_name})
# set_target_properties(check_command_line PROPERTIES
#   COMPILE_FLAGS -DCHECK_COMMAND_LINE_VERBOSE)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/command_line_check
  COMMAND ${CMAKE_COMMAND} -DMARK_FILE=${CMAKE_CURRENT_BINARY_DIR}/check_mark.txt
  -P ${CMAKE_CURRENT_SOURCE_DIR}/check_mark.cmake
  COMMAND ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/${output_name}
  ${CHECK_ARGS} ""
  VERBATIM
  COMMENT "Checking custom command line escapes (single'quote)"
  )
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/command_line_check
  PROPERTIES SYMBOLIC 1)
add_custom_target(do_check_command_line ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/command_line_check
  COMMAND ${CMAKE_COMMAND} -E echo "Checking custom target command escapes"
  COMMAND ${EXECUTABLE_OUTPUT_PATH}/${CMAKE_CFG_INTDIR}/${output_name}
  ${CHECK_ARGS} ""
  VERBATIM
  COMMENT "Checking custom target command line escapes ($dollar-signs$)"
  )
add_dependencies(do_check_command_line check_command_line)

add_custom_target(pre_check_command_line
  COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_BINARY_DIR}/check_mark.txt
  )
add_dependencies(do_check_command_line pre_check_command_line)

# <SameNameTest>
#
# Add a custom target called "SameName" -- then add a custom command in a
# different target whose output is a full-path file called "SameName" -- then
# add a second custom target that depends on the full-path file ".../SameName"
#
# At first, this reproduces a bug reported by a customer. After fixing it,
# having this test here makes sure it stays fixed moving forward.
#
add_custom_command(
  OUTPUT SameName1.txt
  COMMAND ${CMAKE_COMMAND} -E touch SameName1.txt
  )
add_custom_target(SameName ALL
  DEPENDS SameName1.txt
  )

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/subdir/SameName
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/subdir
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/subdir/SameName
  )
add_custom_target(DifferentName ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/subdir/SameName
  )
#
# </SameNameTest>

# Per-config target name and generator expressions.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../PerConfig PerConfig)
add_custom_command(
  OUTPUT perconfig.out
  COMMAND ${PerConfig_COMMAND}
  DEPENDS ${PerConfig_DEPENDS}
  VERBATIM
  )
set_property(SOURCE perconfig.out PROPERTY SYMBOLIC 1)
add_custom_target(perconfig_target ALL
  COMMAND ${CMAKE_COMMAND} -E echo "perconfig=$<TARGET_FILE:perconfig>" "config=$<CONFIGURATION>"
  DEPENDS perconfig.out)

# Test SOURCES in add_custom_target() with COMPILE_DEFINITIONS
# which previously caused a crash in the makefile generators.
add_custom_target(source_in_custom_target SOURCES source_in_custom_target.cpp)
set_property(SOURCE source_in_custom_target
  PROPERTY COMPILE_DEFINITIONS "TEST"
)

set(gen_path "${CMAKE_CURRENT_BINARY_DIR}//./foo")
set(gen_file "${gen_path}/foo.cxx")

add_custom_command(
  OUTPUT "${gen_file}"
  # Make sure the output directory exists before trying to write to it.
  COMMAND ${CMAKE_COMMAND} -E make_directory "${gen_path}"
  COMMAND ${CMAKE_COMMAND} -E touch "${gen_file}"
)

add_library(NormOutput "${gen_file}")

string(APPEND gen_path "/bar")
set(gen_file "${gen_path}/bar.cxx")

add_custom_command(
  OUTPUT "${gen_path}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${gen_path}"
)

add_custom_command(
  OUTPUT "${gen_file}"
  DEPENDS "${gen_path}"
  COMMAND ${CMAKE_COMMAND} -E touch "${gen_file}")

add_library(NormDepends "${gen_file}")

# Test that USES_TERMINAL is parsed correctly.
# It seems much more difficult to test that USES_TERMINAL actually gives
# the subprocess console access, as test output is piped through CTest,
# and CTest itself might not be connected to the console.

set(gen_file "${gen_path}/bar2.cxx")

add_custom_command(
  OUTPUT "${gen_file}"
  DEPENDS "${gen_path}"
  COMMAND ${CMAKE_COMMAND} -E touch "${gen_file}"
  VERBATIM
  USES_TERMINAL
)

add_library(UseConsole "${gen_file}")

add_custom_target(UseConsoleTarget ALL
  COMMAND ${CMAKE_COMMAND} -E echo "Custom console target."
  VERBATIM
  USES_TERMINAL
)

# Test COMMAND_EXPAND_LISTS
set(cmp_args "1ARG=COMMAND_EXPAND_LISTS" "2ARG=test" "3ARG=outfile"
  "4ARG=content")
set(AARGS "")
foreach(arg IN LISTS cmp_args)
  list(APPEND AARGS "-DA${arg}")
endforeach()

set(gen_file "expand_custom_command.phony")
add_custom_command(
  OUTPUT "${gen_file}"
  COMMAND ${CMAKE_COMMAND} ${AARGS}
    "-DB$<JOIN:$<TARGET_PROPERTY:command_expand_lists,CMPARGS>,;-DB>"
    "-P" "${CMAKE_CURRENT_SOURCE_DIR}/compare_options.cmake"
    COMMAND_EXPAND_LISTS
  VERBATIM
)
set_property(SOURCE "${gen_file}" PROPERTY SYMBOLIC ON)
add_custom_target(command_expand_lists ALL DEPENDS "${gen_file}")
set_property(TARGET command_expand_lists PROPERTY CMPARGS "${cmp_args}")
