cmake_minimum_required(VERSION 2.8)

project(CheckStructHasMember)

set(CMAKE_REQUIRED_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}")

include(CheckStructHasMember)

foreach(_config_type Release RelWithDebInfo MinSizeRel Debug)
    set(CMAKE_TRY_COMPILE_CONFIGURATION ${_config_type})
    unset(CSHM_RESULT_S1_${_config_type} CACHE)
    unset(CSHM_RESULT_S2_${_config_type} CACHE)
    unset(CSHM_RESULT_S3_${_config_type} CACHE)
    message(STATUS "Testing configuration ${_config_type}")

    check_struct_has_member("struct non_existent_struct" "foo" "cm_cshm.h" CSHM_RESULT_S1_${_config_type})
    check_struct_has_member("struct struct_with_member" "non_existent_member" "cm_cshm.h" CSHM_RESULT_S2_${_config_type})
    check_struct_has_member("struct struct_with_member" "member" "cm_cshm.h" CSHM_RESULT_S3_${_config_type})

    if(CSHM_RESULT_S1_${_config_type} OR CSHM_RESULT_S2_${_config_type})
        message(SEND_ERROR "CheckStructHasMember reported a nonexistent member as existing in configuration ${_config_type}")
    endif()

    if(NOT CSHM_RESULT_S3_${_config_type})
        message(SEND_ERROR "CheckStructHasMember did not report an existent member as existing in configuration ${_config_type}")
    endif()
endforeach()

foreach(_config_type Release RelWithDebInfo MinSizeRel Debug)
    set(CMAKE_TRY_COMPILE_CONFIGURATION ${_config_type})
    unset(CSHM_RESULT_S1_${_config_type}_C CACHE)
    unset(CSHM_RESULT_S2_${_config_type}_C CACHE)
    unset(CSHM_RESULT_S3_${_config_type}_C CACHE)
    message(STATUS "Testing configuration ${_config_type}")

    check_struct_has_member("struct non_existent_struct" "foo" "cm_cshm.h" CSHM_RESULT_S1_${_config_type}_C LANGUAGE C)
    check_struct_has_member("struct struct_with_member" "non_existent_member" "cm_cshm.h" CSHM_RESULT_S2_${_config_type}_C LANGUAGE C)
    check_struct_has_member("struct struct_with_member" "member" "cm_cshm.h" CSHM_RESULT_S3_${_config_type}_C LANGUAGE C)

    if(CSHM_RESULT_S1_${_config_type}_C OR CSHM_RESULT_S2_${_config_type}_C)
        message(SEND_ERROR "CheckStructHasMember reported a nonexistent member as existing in configuration ${_config_type}")
    endif()

    if(NOT CSHM_RESULT_S3_${_config_type}_C)
        message(SEND_ERROR "CheckStructHasMember did not report an existent member as existing in configuration ${_config_type}")
    endif()
endforeach()

foreach(_config_type Release RelWithDebInfo MinSizeRel Debug)
    set(CMAKE_TRY_COMPILE_CONFIGURATION ${_config_type})
    unset(CSHM_RESULT_S1_${_config_type}_CXX CACHE)
    unset(CSHM_RESULT_S2_${_config_type}_CXX CACHE)
    unset(CSHM_RESULT_S3_${_config_type}_CXX CACHE)
    unset(CSHM_RESULT_C1_${_config_type}_CXX CACHE)
    unset(CSHM_RESULT_C2_${_config_type}_CXX CACHE)
    unset(CSHM_RESULT_C3_${_config_type}_CXX CACHE)

    message(STATUS "Testing configuration ${_config_type}")

    check_struct_has_member("non_existent_struct" "foo" "cm_cshm.h" CSHM_RESULT_S1_${_config_type}_CXX LANGUAGE CXX)
    check_struct_has_member("struct_with_non_existent_members" "non_existent_member" "cm_cshm.h" CSHM_RESULT_S2_${_config_type}_CXX LANGUAGE CXX)
    check_struct_has_member("struct struct_with_member" "member" "cm_cshm.h" CSHM_RESULT_S3_${_config_type}_CXX LANGUAGE CXX)
    check_struct_has_member("ns::non_existent_class" "foo" "cm_cshm.hxx" CSHM_RESULT_C1_${_config_type}_CXX LANGUAGE CXX)
    check_struct_has_member("ns::class_with_non_existent_members" "foo" "cm_cshm.hxx" CSHM_RESULT_C2_${_config_type}_CXX LANGUAGE CXX)
    check_struct_has_member("ns::class_with_member" "foo" "cm_cshm.hxx" CSHM_RESULT_C3_${_config_type}_CXX LANGUAGE CXX)

    if(CSHM_RESULT_S1_${_config_type}_CXX OR CSHM_RESULT_S2_${_config_type}_CXX OR CSHM_RESULT_C1_${_config_type}_CXX OR CSHM_RESULT_C2_${_config_type}_CXX)
        message(SEND_ERROR "CheckStructHasMember reported a nonexistent member as existing in configuration ${_config_type}")
    endif()

    if(NOT CSHM_RESULT_S3_${_config_type}_CXX OR NOT CSHM_RESULT_C3_${_config_type}_CXX)
        message(SEND_ERROR "CheckStructHasMember did not report an existent member as existing in configuration ${_config_type}")
    endif()
endforeach()


set(CMAKE_TRY_COMPILE_CONFIGURATION ${CMAKE_BUILD_TYPE})

if (CMAKE_COMPILER_IS_GNUCC)
    string(APPEND CMAKE_C_FLAGS " -O3")
    unset(CSHM_RESULT_O3 CACHE)
    unset(CSHM_RESULT_O3_C CACHE)
    unset(CSHM_RESULT_O3_CXX CACHE)
    message(STATUS "Testing with optimization -O3")

    check_struct_has_member("class_with_non_existent_members" foo "cm_cshm.h" CSHM_RESULT_O3)
    check_struct_has_member("class_with_non_existent_members" foo "cm_cshm.h" CSHM_RESULT_O3_C LANGUAGE C)
    check_struct_has_member("class_with_non_existent_members" foo "cm_cshm.h" CSHM_RESULT_O3_CXX LANGUAGE CXX)

  if (CSE_RESULT_O3 OR CSHM_RESULT_O3_C OR CSHM_RESULT_O3_CXX)
    message(SEND_ERROR "CheckSymbolExists reported a nonexistent symbol as existing with optimization -O3")
  endif ()
endif ()
