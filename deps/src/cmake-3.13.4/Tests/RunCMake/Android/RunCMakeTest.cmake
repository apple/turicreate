cmake_minimum_required(VERSION 3.6)

include(RunCMake)
foreach(v TEST_ANDROID_NDK TEST_ANDROID_STANDALONE_TOOLCHAIN)
  string(REPLACE "|" ";" ${v} "${${v}}")
endforeach()

function(run_Android case)
  set(RunCMake_TEST_OPTIONS
    -DCMAKE_SYSTEM_NAME=Android
    ${RunCMake_TEST_OPTIONS}
    ${ARGN}
    )

  # Use a single build tree for a few tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${case}-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(${case})
  run_cmake_command(${case}-build ${CMAKE_COMMAND} --build .)
endfunction()

set(RunCMake_TEST_OPTIONS
  -DCMAKE_SYSTEM_NAME=Android
  -DCMAKE_SYSROOT=${CMAKE_CURRENT_SOURCE_DIR}
  )
run_cmake(BadSYSROOT)
unset(RunCMake_TEST_OPTIONS)

foreach(ndk IN LISTS TEST_ANDROID_NDK)
  # Load available toolchain versions and abis.
  file(GLOB _config_mks
    "${ndk}/build/core/toolchains/*/config.mk"
    "${ndk}/toolchains/*/config.mk"
    )
  set(_versions "")
  set(_latest_gcc 0)
  set(_latest_clang "")
  set(_latest_clang_vers 0)
  foreach(config_mk IN LISTS _config_mks)
    file(STRINGS "${config_mk}" _abis REGEX "^TOOLCHAIN_ABIS +:= +[^ ].*( |$)")
    if(_abis AND "${config_mk}" MATCHES [[-((clang)?([0-9]\.[0-9]|))/config\.mk$]])
      set(_version "${CMAKE_MATCH_1}")
      set(_is_clang "${CMAKE_MATCH_2}")
      set(_cur_vers "${CMAKE_MATCH_3}")
      if(_is_clang)
        if(_latest_clang_vers STREQUAL "")
          # already the latest possible
        elseif(_cur_vers STREQUAL "" OR _cur_vers VERSION_GREATER _latest_clang_vers)
          set(_latest_clang_vers "${_cur_vers}")
          set(_latest_clang "${_version}")
        endif()
      else()
        if(_version VERSION_GREATER _latest_gcc)
          set(_latest_gcc ${_version})
        endif()
      endif()
      list(APPEND _versions "${_version}")
      string(REGEX MATCHALL "[a-z][a-z0-9_-]+" _abis "${_abis}")
      list(APPEND _abis_${_version} ${_abis})
    endif()
  endforeach()
  set(_abis_clang ${_abis_${_latest_clang}})
  if(_latest_gcc)
    set(_abis_ ${_abis_${_latest_gcc}})
  else()
    set(_abis_ ${_abis_clang})
  endif()
  if(_versions MATCHES "clang")
    set(_versions "clang" ${_versions})
  endif()
  list(REMOVE_DUPLICATES _versions)
  list(SORT _versions)
  set(_versions ";${_versions}")
  foreach(vers IN LISTS _versions)
    list(REMOVE_DUPLICATES _abis_${vers})
  endforeach()

  # Test failure cases.
  message(STATUS "ndk='${ndk}'")
  set(RunCMake_TEST_OPTIONS
    -DCMAKE_SYSTEM_NAME=Android
    -DCMAKE_ANDROID_NDK=${ndk}
    -DCMAKE_ANDROID_ARCH_ABI=badabi
    )
  run_cmake(ndk-badabi)
  set(RunCMake_TEST_OPTIONS
    -DCMAKE_SYSTEM_NAME=Android
    -DCMAKE_ANDROID_NDK=${ndk}
    -DCMAKE_ANDROID_ARCH_ABI=x86
    -DCMAKE_ANDROID_ARM_MODE=0
    )
  run_cmake(ndk-badarm)
  if("armeabi" IN_LIST _abis_)
    set(RunCMake_TEST_OPTIONS
      -DCMAKE_SYSTEM_NAME=Android
      -DCMAKE_ANDROID_NDK=${ndk}
      -DCMAKE_ANDROID_ARM_NEON=0
      )
    run_cmake(ndk-badneon)
  endif()
  set(RunCMake_TEST_OPTIONS
    -DCMAKE_SYSTEM_NAME=Android
    -DCMAKE_ANDROID_NDK=${ndk}
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=badver
    )
  run_cmake(ndk-badver)
  set(RunCMake_TEST_OPTIONS
    -DCMAKE_SYSTEM_NAME=Android
    -DCMAKE_ANDROID_NDK=${ndk}
    -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=1.0
    )
  run_cmake(ndk-badvernum)
  set(RunCMake_TEST_OPTIONS
    -DCMAKE_SYSTEM_NAME=Android
    -DCMAKE_ANDROID_NDK=${ndk}
    -DCMAKE_ANDROID_STL_TYPE=badstl
    )
  run_cmake(ndk-badstl)
  unset(RunCMake_TEST_OPTIONS)

  # Find a sysroot to test.
  file(GLOB _sysroots "${ndk}/platforms/android-[0-9][0-9]/arch-arm")
  if(_sysroots)
    list(GET _sysroots 0 _sysroot)
    set(RunCMake_TEST_OPTIONS
      -DCMAKE_SYSTEM_NAME=Android
      -DCMAKE_SYSROOT=${_sysroot}
      )
    run_cmake(ndk-sysroot-armeabi)
    unset(RunCMake_TEST_OPTIONS)
  endif()

  # Find available STLs.
  set(stl_types
    none
    system
    )

  if(IS_DIRECTORY "${ndk}/sources/cxx-stl/gnu-libstdc++")
    list(APPEND stl_types gnustl_static gnustl_shared)
  endif()
  if(IS_DIRECTORY "${ndk}/sources/cxx-stl/gabi++/libs")
    list(APPEND stl_types gabi++_static gabi++_shared)
  endif()
  if(IS_DIRECTORY "${ndk}/sources/cxx-stl/stlport/libs")
    list(APPEND stl_types stlport_static stlport_shared)
  endif()
  if(IS_DIRECTORY "${ndk}/sources/cxx-stl/llvm-libc++/libs")
    list(APPEND stl_types c++_static c++_shared)
  endif()

  # List possible ABIs.
  set(abi_names
    armeabi
    armeabi-v6
    armeabi-v7a
    arm64-v8a
    mips
    mips64
    x86
    x86_64
    )

  # Test all combinations.
  foreach(vers IN LISTS _versions)
    foreach(stl IN LISTS stl_types)
      foreach(config Release Debug)
        # Test this combination for all available abis.
        message(STATUS "ndk='${ndk}' vers='${vers}' stl='${stl}' config='${config}'")
        set(RunCMake_TEST_OPTIONS
          -DCMAKE_ANDROID_NDK=${ndk}
          -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=${vers}
          -DCMAKE_ANDROID_STL_TYPE=${stl}
          -DCMAKE_BUILD_TYPE=${config}
          )
        foreach(abi IN LISTS abi_names)
          # Skip ABIs not supported by this compiler.
          if(NOT ";${_abis_${vers}};" MATCHES ";${abi};")
            continue()
          endif()

          # Run the tests for this combination.
          if("${abi}" STREQUAL "armeabi")
            run_Android(ndk-armeabi-thumb) # default: -DCMAKE_ANDROID_ARCH_ABI=armeabi -DCMAKE_ANDROID_ARM_MODE=0
            run_Android(ndk-armeabi-arm -DCMAKE_ANDROID_ARM_MODE=1) # default: -DCMAKE_ANDROID_ARCH_ABI=armeabi
          else()
            run_Android(ndk-${abi} -DCMAKE_ANDROID_ARCH_ABI=${abi})
            if("${abi}" STREQUAL "armeabi-v7a")
              run_Android(ndk-${abi}-neon -DCMAKE_ANDROID_ARCH_ABI=${abi} -DCMAKE_ANDROID_ARM_NEON=1)
            endif()
          endif()
        endforeach()
        unset(RunCMake_TEST_OPTIONS)
      endforeach()
    endforeach()
  endforeach()
endforeach()

foreach(toolchain IN LISTS TEST_ANDROID_STANDALONE_TOOLCHAIN)
  message(STATUS "toolchain='${toolchain}'")

  set(RunCMake_TEST_OPTIONS
    -DCMAKE_SYSTEM_NAME=Android
    -DCMAKE_SYSROOT=${toolchain}/sysroot
    )
  run_cmake(standalone-sysroot)
  unset(RunCMake_TEST_OPTIONS)

  foreach(config Release Debug)
    message(STATUS "toolchain='${toolchain}' config='${config}'")
    set(RunCMake_TEST_OPTIONS
      -DCMAKE_ANDROID_STANDALONE_TOOLCHAIN=${toolchain}
      -DCMAKE_BUILD_TYPE=${config}
      )
    run_Android(standalone)
    unset(RunCMake_TEST_OPTIONS)
  endforeach()
endforeach()
