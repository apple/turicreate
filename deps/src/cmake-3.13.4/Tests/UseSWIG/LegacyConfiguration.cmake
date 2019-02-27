
# Prevent deprecated warnings from new UseSWIG module
set (CMAKE_WARN_DEPRECATED FALSE)

find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

# Path separator
if (WIN32)
  set (PS "$<SEMICOLON>")
else()
  set (PS ":")
endif()

unset(SWIG_LANG_TYPE)
if(${language} MATCHES python)
  find_package(PythonInterp REQUIRED)
  find_package(PythonLibs REQUIRED)
  include_directories(${PYTHON_INCLUDE_PATH})
  set(SWIG_LANG_LIBRARIES ${PYTHON_LIBRARIES})
endif()
if(${language} MATCHES perl)
  find_package(Perl REQUIRED)
  find_package(PerlLibs REQUIRED)
  include_directories(${PERL_INCLUDE_PATH})
  separate_arguments(c_flags UNIX_COMMAND "${PERL_EXTRA_C_FLAGS}")
  add_compile_options(${c_flags})
  set(SWIG_LANG_LIBRARIES ${PERL_LIBRARY})
endif()
if(${language} MATCHES tcl)
  find_package(TCL REQUIRED)
  include_directories(${TCL_INCLUDE_PATH})
  set(SWIG_LANG_LIBRARIES ${TCL_LIBRARY})
endif()
if(${language} MATCHES ruby)
  find_package(Ruby REQUIRED)
  include_directories(${RUBY_INCLUDE_PATH})
  set(SWIG_LANG_LIBRARIES ${RUBY_LIBRARY})
endif()
if(${language} MATCHES php4)
  find_package(PHP4 REQUIRED)
  include_directories(${PHP4_INCLUDE_PATH})
  set(SWIG_LANG_LIBRARIES ${PHP4_LIBRARY})
endif()
if(${language} MATCHES pike)
  find_package(Pike REQUIRED)
  include_directories(${PIKE_INCLUDE_PATH})
  set(SWIG_LANG_LIBRARIES ${PIKE_LIBRARY})
endif()
if(${language} MATCHES lua)
  find_package(Lua REQUIRED)
  include_directories(${LUA_INCLUDE_DIR})
  set(SWIG_LANG_TYPE TYPE SHARED)
  set(SWIG_LANG_LIBRARIES ${LUA_LIBRARIES})
endif()

unset(CMAKE_SWIG_FLAGS)

include_directories(${CMAKE_CURRENT_LIST_DIR})

set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/example.i" PROPERTIES CPLUSPLUS ON)
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/example.i" PROPERTIES SWIG_FLAGS "-includeall")
SWIG_ADD_LIBRARY(example
                 LANGUAGE "${language}"
                 ${SWIG_LANG_TYPE}
                 SOURCES "${CMAKE_CURRENT_LIST_DIR}/example.i"
                         "${CMAKE_CURRENT_LIST_DIR}/example.cxx")
SWIG_LINK_LIBRARIES(example ${SWIG_LANG_LIBRARIES})
