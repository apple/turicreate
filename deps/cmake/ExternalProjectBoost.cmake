# Build Boost =================================================================
set(PATCHCMD patch -N -p1 -i ${CMAKE_SOURCE_DIR}/deps/patches/boost_date_time_parsers3.patch || true)
# disable fchmodat on mac. not supported for OS X 10.8/10.9
SET(EXTRA_CONFIGURE_COMMANDS "")

if (CLANG)
  SET(ADD_BOOST_BOOTSTRAP --with-toolset=clang)
  SET(cxxflags "-std=c++11 -Wno-everything ${ARCH_FLAG} ${CPP_REAL_COMPILER_FLAGS}")
  if(TC_BUILD_IOS)
    SET(ADD_BOOST_COMPILE_TOOLCHAIN "toolset=clang abi=aapcs address-model=64 architecture=arm binary-format=mach-o threading=multi target-os=iphone cflags=\"-arch arm64\" cxxflags=\"${cxxflags}\"")
  else()
    SET(ADD_BOOST_COMPILE_TOOLCHAIN "toolset=clang cflags=\"-Wno-everything\" cxxflags=\"${cxxflags}\"")
  endif()
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
elseif(UNIX)
  SET(cxxflags "-std=c++11 ${CPP_REAL_COMPILER_FLAGS}")
  SET(ADD_BOOST_BOOTSTRAP --with-toolset=gcc)
  SET(ADD_BOOST_COMPILE_TOOLCHAIN "toolset=gcc cxxflags=\"${cxxflags}\"")
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
  if(TC_BUILD_IOS)
    execute_process(
      COMMAND bash -c "xcrun --sdk iphoneos --show-sdk-path"
      OUTPUT_VARIABLE _ios_sdk_path
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(OPTIONAL_SDKROOT "SDKROOT=${_ios_sdk_path}")
  endif()
  SET(BOOST_BUILD_SHELL_COMMAND "rm -rf <INSTALL_DIR>/include/boost && PATH=$PATH:${CMAKE_SOURCE_DIR}/deps/local/bin C_INCLUDE_PATH=${CMAKE_SOURCE_DIR}/deps/local/include CPLUS_INCLUDE_PATH=${CMAKE_SOURCE_DIR}/deps/local/include LIBRARY_PATH=${CMAKE_SOURCE_DIR}/deps/local/lib ${OPTIONAL_SDKROOT} ./b2 -d0 -q ${ADD_BOOST_COMPILE_TOOLCHAIN} install link=static variant=release threading=multi runtime-link=static ${PLATFORM_DEFINES} cxxflags='-fPIC -std=c++11'")
  STRING(REGEX REPLACE "C:" "/C" BOOST_BUILD_SHELL_COMMAND ${BOOST_BUILD_SHELL_COMMAND})
  SET(BOOST_BUILD_COMMAND sh -c ${BOOST_BUILD_SHELL_COMMAND})
  SET(BOOST_INSTALL_COMMAND "PATH=$PATH:${CMAKE_SOURCE_DIR}/deps/local/bin ./b2 -d0 -q tools/bcp && cp dist/bin/bcp ${CMAKE_SOURCE_DIR}/deps/local/bin")
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
  URL "${CMAKE_SOURCE_DIR}/deps/src/boost_1_68_0/"
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
  BUILD_BYPRODUCTS ${CMAKE_SOURCE_DIR}/deps/local/lib/libboost_chrono.a
                   ${CMAKE_SOURCE_DIR}/deps/local/lib/libboost_context.a
                   ${CMAKE_SOURCE_DIR}/deps/local/lib/libboost_coroutine.a
                   ${CMAKE_SOURCE_DIR}/deps/local/lib/libboost_date_time.a
                   ${CMAKE_SOURCE_DIR}/deps/local/lib/libboost_iostreams.a
                   ${CMAKE_SOURCE_DIR}/deps/local/lib/libboost_filesystem.a
                   ${CMAKE_SOURCE_DIR}/deps/local/lib/libboost_program_options.a
                   ${CMAKE_SOURCE_DIR}/deps/local/lib/libboost_random.a
                   ${CMAKE_SOURCE_DIR}/deps/local/lib/libboost_regex.a
                   ${CMAKE_SOURCE_DIR}/deps/local/lib/libboost_system.a
                   ${CMAKE_SOURCE_DIR}/deps/local/lib/libboost_unit_test_framework.a
                   ${CMAKE_SOURCE_DIR}/deps/local/lib/libboost_thread.a
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


# add an imported library for each boost library
#
set(libnames "")
foreach(blib ${Boost_LIBRARIES})
  string(REGEX REPLACE "\\.a$" ${CMAKE_SHARED_LIBRARY_SUFFIX} bout ${blib})
  set(Boost_SHARED_LIBRARIES ${Boost_SHARED_LIBRARIES} ${bout})
  get_filename_component(FNAME ${blib} NAME)
  add_library(${FNAME} STATIC IMPORTED)
  set_property(TARGET ${FNAME} PROPERTY IMPORTED_LOCATION ${blib})
  list(APPEND libnames ${FNAME})
endforeach()


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

