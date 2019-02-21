cmake_policy(SET CMP0057 NEW)
include(RunCMake)

run_cmake(NotAFeature)
run_cmake(NotAFeatureGenex)
run_cmake(NotAFeatureTransitive)
run_cmake(NotAFeature_OriginDebug)
run_cmake(NotAFeature_OriginDebugGenex)
run_cmake(NotAFeature_OriginDebugTransitive)
run_cmake(NotAFeature_OriginDebugCommand)

run_cmake(generate_feature_list)
file(READ
  "${RunCMake_BINARY_DIR}/generate_feature_list-build/c_features.txt"
  C_FEATURES
)
file(READ
  "${RunCMake_BINARY_DIR}/generate_feature_list-build/cxx_features.txt"
  CXX_FEATURES
)
include("${RunCMake_BINARY_DIR}/generate_feature_list-build/c_standard_default.cmake")
include("${RunCMake_BINARY_DIR}/generate_feature_list-build/cxx_standard_default.cmake")

if (NOT C_FEATURES)
  run_cmake(NoSupportedCFeatures)
  run_cmake(NoSupportedCFeaturesGenex)
endif()

if (NOT CXX_FEATURES)
  run_cmake(NoSupportedCxxFeatures)
  run_cmake(NoSupportedCxxFeaturesGenex)
elseif (cxx_std_98 IN_LIST CXX_FEATURES AND cxx_std_11 IN_LIST CXX_FEATURES)
  if(CXX_STANDARD_DEFAULT EQUAL 98)
    run_cmake(LinkImplementationFeatureCycle)
  endif()
  run_cmake(LinkImplementationFeatureCycleSolved)

  if (cxx_final IN_LIST CXX_FEATURES)
    set(RunCMake_TEST_OPTIONS "-DHAVE_FINAL=1")
  endif()
  run_cmake(NonValidTarget1)
  run_cmake(NonValidTarget2)
  unset(RunCMake_TEST_OPTIONS)
endif()

if(CXX_STANDARD_DEFAULT)
  run_cmake(NotAStandard)

  foreach(standard 98 11)
    file(READ
      "${RunCMake_BINARY_DIR}/generate_feature_list-build/cxx${standard}_flag.txt"
      CXX${standard}_FLAG
    )
    if (CXX${standard}_FLAG STREQUAL NOTFOUND)
      run_cmake(RequireCXX${standard})
      run_cmake(RequireCXX${standard}Variable)
    endif()
    if (CXX${standard}EXT_FLAG STREQUAL NOTFOUND)
      run_cmake(RequireCXX${standard}Ext)
      run_cmake(RequireCXX${standard}ExtVariable)
    endif()
  endforeach()
endif()
