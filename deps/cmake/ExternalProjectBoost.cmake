# Build Boost =================================================================
set(PATCHCMD patch -N -p1 -i ${CMAKE_SOURCE_DIR}/deps/patches/boost_date_time_parsers3.patch || true)
# disable fchmodat on mac. not supported for OS X 10.8/10.9
SET(EXTRA_CONFIGURE_COMMANDS "")

if (CLANG)
  SET(ADD_BOOST_BOOTSTRAP --with-toolset=clang)
  SET(cxxflags "-std=c++11 -stdlib=libc++")
  SET(tmp2 "-stdlib=libc++")
  SET(ADD_BOOST_COMPILE_TOOLCHAIN "toolset=clang cxxflags=${cxxflags} linkflags=${tmp2}")
  UNSET(tmp)
  UNSET(tmp2)
elseif(APPLE)
  SET(ADD_BOOST_COMPILE_TOOLCHAIN toolset=darwin)
elseif(WIN32 AND ${MINGW_MAKEFILES})
  SET(ADD_BOOST_BOOTSTRAP mingw)
  SET(ADD_BOOST_COMPILE_TOOLCHAIN toolset=gcc)
  SET(PATCHCMD ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/deps/patches/user-config.jam.mingw ./tools/build/src/user-config.jam)
elseif(WIN32 AND ${MSYS_MAKEFILES})
  SET(ADD_BOOST_BOOTSTRAP --with-toolset=mingw)
  SET(ADD_BOOST_COMPILE_TOOLCHAIN toolset=gcc)
  SET(EXTRA_CONFIGURE_COMMANDS && perl -pi -e "s/mingw/gcc/g" ./project-config.jam)
else()
  SET(ADD_BOOST_BOOTSTRAP "")
  SET(ADD_BOOST_COMPILE_TOOLCHAIN "")
endif()

if (WIN32 AND ${MINGW_MAKEFILES})
  SET(SCRIPT_EXTENSION ".bat")
  SET(BOOST_BUILD_COMMAND ${CMAKE_COMMAND} -E remove_directory <INSTALL_DIR>/include/boost &&
    SET C_INCLUDE_PATH=${CMAKE_SOURCE_DIR}\\deps\\local\\include &&
    SET CPLUS_INCLUDE_PATH=${CMAKE_SOURCE_DIR}\\deps\\local\\include &&
    SET LIBRARY_PATH=${CMAKE_SOURCE_DIR}\\deps\\local\\lib &&
    SET PATH=$PATH;${CMAKE_SOURCE_DIR}\\deps\\local\\bin &&
    .\\b2 ${ADD_BOOST_COMPILE_TOOLCHAIN} install link=static variant=release threading=multi runtime-link=static cxxflags=-fPIC --prefix=<INSTALL_DIR>)
else()
  SET(SCRIPT_EXTENSION ".sh")
  if(WIN32 AND MSYS_MAKEFILES)
    SET(PLATFORM_DEFINES define=MS_WIN64)
  endif()
  SET(BOOST_BUILD_SHELL_COMMAND "rm -rf <INSTALL_DIR>/include/boost && PATH=$PATH:${CMAKE_SOURCE_DIR}/deps/local/bin C_INCLUDE_PATH=${CMAKE_SOURCE_DIR}/deps/local/include CPLUS_INCLUDE_PATH=${CMAKE_SOURCE_DIR}/deps/local/include LIBRARY_PATH=${CMAKE_SOURCE_DIR}/deps/local/lib ./b2 ${ADD_BOOST_COMPILE_TOOLCHAIN} install link=static variant=release threading=multi runtime-link=static ${PLATFORM_DEFINES} cxxflags=-fPIC")
  STRING(REGEX REPLACE "C:" "/C" BOOST_BUILD_SHELL_COMMAND ${BOOST_BUILD_SHELL_COMMAND})
  SET(BOOST_BUILD_COMMAND sh -c ${BOOST_BUILD_SHELL_COMMAND})
  SET(BOOST_INSTALL_COMMAND "PATH=$PATH:${CMAKE_SOURCE_DIR}/deps/local/bin ./b2 tools/bcp && cp dist/bin/bcp ${CMAKE_SOURCE_DIR}/deps/local/bin")
  SET(BOOST_INSTALL_COMMAND sh -c ${BOOST_INSTALL_COMMAND})
endif()

set(BOOST_ROOT ${CMAKE_SOURCE_DIR}/deps/local )
set(BOOST_LIBS_DIR ${CMAKE_SOURCE_DIR}/deps/local/lib)
FILE(GLOB CURRENT_BOOST_LIBS ${BOOST_LIBS_DIR} libboost*.a)
if(WIN32)
  SET(BOOST_INSTALL_COMMAND ./b2 tools/bcp &&
    ${CMAKE_COMMAND} -E copy ./dist/bin/bcp.exe ${CMAKE_SOURCE_DIR}/deps/local/bin &&
    ${CMAKE_COMMAND} -E copy_directory <INSTALL_DIR>/include/boost-1_56/boost <INSTALL_DIR>/include/boost &&
    sh -c "${CMAKE_SOURCE_DIR}/scripts/boostlib_rename.sh ${BOOST_LIBS_DIR}/libboost*")
endif()

ExternalProject_Add(ex_boost
  PREFIX ${CMAKE_SOURCE_DIR}/deps/build/boost
  URL "${CMAKE_SOURCE_DIR}/deps/src/boost_1_65_1/"
  BUILD_IN_SOURCE 1
  INSTALL_DIR ${CMAKE_SOURCE_DIR}/deps/local
  PATCH_COMMAND ${PATCHCMD}
  CONFIGURE_COMMAND
  ./bootstrap${SCRIPT_EXTENSION}
  ${ADD_BOOST_BOOTSTRAP}
  --with-libraries=filesystem
  --with-libraries=program_options
  --with-libraries=system
  --with-libraries=iostreams
  --with-libraries=date_time
  --with-libraries=random
  --with-libraries=context
  --with-libraries=thread
  --with-libraries=chrono
  --with-libraries=coroutine
  --with-libraries=regex
  --with-libraries=test
  --prefix=<INSTALL_DIR>
  ${EXTRA_CONFIGURE_COMMANDS}
  BUILD_COMMAND ${BOOST_BUILD_COMMAND}
  INSTALL_COMMAND ${BOOST_INSTALL_COMMAND}
  )

set(Boost_LIBRARIES
  ${BOOST_LIBS_DIR}/libboost_coroutine.a
  ${BOOST_LIBS_DIR}/libboost_filesystem.a
  ${BOOST_LIBS_DIR}/libboost_program_options.a
  ${BOOST_LIBS_DIR}/libboost_system.a
  ${BOOST_LIBS_DIR}/libboost_iostreams.a
  ${BOOST_LIBS_DIR}/libboost_context.a
  ${BOOST_LIBS_DIR}/libboost_thread.a
  ${BOOST_LIBS_DIR}/libboost_chrono.a
  ${BOOST_LIBS_DIR}/libboost_date_time.a
  ${BOOST_LIBS_DIR}/libboost_regex.a)


message(STATUS "Boost libs: " ${Boost_LIBRARIES})

# add an imported library for each boost library
#
set(libnames "")
foreach(blib ${Boost_LIBRARIES})
  message(STATUS "Boost libs: " ${blib})
  string(REGEX REPLACE "\\.a$" ${CMAKE_SHARED_LIBRARY_SUFFIX} bout ${blib})
  message(STATUS "Boost dyn libs: " ${bout})
  set(Boost_SHARED_LIBRARIES ${Boost_SHARED_LIBRARIES} ${bout})
  get_filename_component(FNAME ${blib} NAME)
  add_library(${FNAME} STATIC IMPORTED)
  set_property(TARGET ${FNAME} PROPERTY IMPORTED_LOCATION ${blib})
  list(APPEND libnames ${FNAME})
endforeach()


message(STATUS "Boost Shared libs: " ${Boost_SHARED_LIBRARIES})

add_dependencies(ex_boost ex_libbz2 ex_libz)
# add_definitions(-DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG)
# add_definitions(-DBOOST_ALL_DYN_LINK)
# set(Boost_SHARED_LIBRARIES "")
#

# Create Boost interface library
add_library(boost INTERFACE )
add_dependencies(boost ex_boost)

target_link_libraries(boost INTERFACE ${libnames})
if (APPLE OR WIN32)
  target_link_libraries(boost INTERFACE z)
else()
  target_link_libraries(boost INTERFACE z rt)
endif()
target_compile_definitions(boost INTERFACE HAS_BOOST)


set(HAS_BOOST TRUE CACHE BOOL "")



# Repeat for boost test libraries
##################################
set(Boost_Test_LIBRARIES
  ${BOOST_LIBS_DIR}/libboost_unit_test_framework.a)

set(libnames "")
foreach(blib ${Boost_Test_LIBRARIES})
  message(STATUS "Boost libs: " ${blib})
  string(REGEX REPLACE "\\.a$" ${CMAKE_SHARED_LIBRARY_SUFFIX} bout ${blib})
  get_filename_component(FNAME ${blib} NAME)
  add_library(${FNAME} STATIC IMPORTED)
  set_property(TARGET ${FNAME} PROPERTY IMPORTED_LOCATION ${blib})
  list(APPEND libnames ${FNAME})
endforeach()

# Create Boost Test interface library
add_library(boost_test INTERFACE )
add_dependencies(boost_test ex_boost)

target_link_libraries(boost_test INTERFACE ${libnames} boost)

set(HAS_BOOST_TEST TRUE CACHE BOOL "")

