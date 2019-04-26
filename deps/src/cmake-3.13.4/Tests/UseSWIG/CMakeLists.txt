add_test(NAME UseSWIG.LegacyPython COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/UseSWIG/LegacyPython"
  "${CMake_BINARY_DIR}/Tests/UseSWIG/LegacyPython"
  ${build_generator_args}
  --build-project TestLegacyPython
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
add_test(NAME UseSWIG.LegacyPerl COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/UseSWIG/LegacyPerl"
  "${CMake_BINARY_DIR}/Tests/UseSWIG/LegacyPerl"
  ${build_generator_args}
  --build-project TestLegacyPerl
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )

include(CheckLanguage)
check_language(CSharp)
if (CMAKE_CSharp_COMPILER)
  add_test(NAME UseSWIG.BasicCsharp COMMAND
    ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
    --build-and-test
    "${CMake_SOURCE_DIR}/Tests/UseSWIG/BasicCsharp"
    "${CMake_BINARY_DIR}/Tests/UseSWIG/BasicCsharp"
    ${build_generator_args}
    --build-project TestBasicCsharp
    --build-options ${build_options}
    --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
    )
endif()

add_test(NAME UseSWIG.BasicPython COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/UseSWIG/BasicPython"
  "${CMake_BINARY_DIR}/Tests/UseSWIG/BasicPython"
  ${build_generator_args}
  --build-project TestBasicPython
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
add_test(NAME UseSWIG.BasicPerl COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/UseSWIG/BasicPerl"
  "${CMake_BINARY_DIR}/Tests/UseSWIG/BasicPerl"
  ${build_generator_args}
  --build-project TestBasicPerl
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )


add_test(NAME UseSWIG.MultipleModules COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/UseSWIG/MultipleModules"
  "${CMake_BINARY_DIR}/Tests/UseSWIG/MultipleModules"
  ${build_generator_args}
  --build-project TestMultipleModules
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
add_test(NAME UseSWIG.MultiplePython COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/UseSWIG/MultiplePython"
  "${CMake_BINARY_DIR}/Tests/UseSWIG/MultiplePython"
  ${build_generator_args}
  --build-project TestMultiplePython
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )


add_test(NAME UseSWIG.ModuleVersion2 COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/UseSWIG/ModuleVersion2"
  "${CMake_BINARY_DIR}/Tests/UseSWIG/ModuleVersion2"
  ${build_generator_args}
  --build-project TestModuleVersion2
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )


add_test(NAME UseSWIG.UseTargetINCLUDE_DIRECTORIES COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/UseSWIG/UseTargetINCLUDE_DIRECTORIES"
  "${CMake_BINARY_DIR}/Tests/UseSWIG/UseTargetINCLUDE_DIRECTORIES"
  ${build_generator_args}
  --build-project TestModuleVersion2
  --build-options ${build_options}
  )
