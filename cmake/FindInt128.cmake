# - this module looks for 128 bit integer support.  It sets up the
# type defs in util/int128_types.hpp.  Simply add ${INT128_FLAGS} to the
# compiler flags.

include(CheckTypeSize)

MACRO(CHECK_128_BIT_HASH_FUNCTION VAR_NAME DEF_NAME)

  message("Testing for presence of 128 bit unsigned integer hash function for ${VAR_NAME}.")


  CHECK_CXX_SOURCE_COMPILES("
  #include <functional>
  #include <cstdint>
  int main(int argc, char** argv) {
    std::hash<${VAR_NAME}>()(0);
  return 0;
  }"
  has_hash_${VAR_NAME})

  if(has_hash_${VAR_NAME})
    message("std::hash<${VAR_NAME}> defined.")
    SET(${DEF_NAME} 1)
  else()
    message("std::hash<${VAR_NAME}> not defined.")
  endif()
endmacro()


MACRO(CHECK_INT128 INT128_NAME VARIABLE DEFINE_NAME)

  if(NOT INT128_FOUND)
    message("Testing for 128 bit integer support with ${INT128_NAME}.")
    check_type_size("${INT128_NAME}" int128_t_${DEFINE_NAME})
    if(HAVE_int128_t_${DEFINE_NAME})
      if(int128_t_${DEFINE_NAME} EQUAL 16)
	message("Found: Enabling support for 128 bit integers using ${INT128_NAME}.")
	SET(INT128_FOUND 1)
  CHECK_128_BIT_HASH_FUNCTION(${INT128_NAME} HAS_INT128_STD_HASH)

	SET(${VARIABLE} "${DEFINE_NAME}")
      else()
	message("${INT128_NAME} has incorrect size, can't use.")
      endif()
    endif()
  endif()
endmacro()

MACRO(CHECK_UINT128 UINT128_NAME VARIABLE DEFINE_NAME)

  if(NOT UINT128_FOUND)
    message("Testing for 128 bit unsigned integer support with ${UINT128_NAME}.")
    check_type_size("${UINT128_NAME}" uint128_t_${DEFINE_NAME})
    if(HAVE_uint128_t_${DEFINE_NAME})
      if(uint128_t_${DEFINE_NAME} EQUAL 16)
	message("Found: Enabling support for 128 bit integers using ${UINT128_NAME}.")
	SET(UINT128_FOUND 1)
  CHECK_128_BIT_HASH_FUNCTION(${UINT128_NAME} HAS_UINT128_STD_HASH)
	SET(${VARIABLE} "${DEFINE_NAME}")
      else()
	message("${UINT128_NAME} has incorrect size, can't use.")
      endif()
    endif()
  endif()
endmacro()

MACRO(FIND_INT128_TYPES)

  Check_Int128("__int128_t" INT128_DEF "HAVE__int128_t")
  Check_Int128("long long"  INT128_DEF "HAVEint128_as_long_long")
  Check_Int128("int128_t"   INT128_DEF "HAVEint128_t")
  Check_Int128("__int128"   INT128_DEF "HAVE__int128")
  Check_Int128("int128"     INT128_DEF "HAVEint128")

  if(INT128_FOUND)
    set(INT128_FLAGS "-D${INT128_DEF}")

    if(HAS_INT128_STD_HASH)
      set(INT128_FLAGS "${INT128_FLAGS} -DHASH_FOR_INT128_DEFINED")
    endif()

  else()
    message("Compiler/platform support for 128 bit integers not found, falling back to boost mpfr.")
    set(INT128_FLAGS "")
  endif()

  Check_UInt128("__uint128_t"         UINT128_DEF "HAVE__uint128_t")
  Check_UInt128("unsigned long long"  UINT128_DEF "HAVEuint128_as_u_long_long")
  Check_UInt128("uint128_t"           UINT128_DEF "HAVEuint128_t")
  Check_UInt128("__uint128"           UINT128_DEF "HAVE__uint128")
  Check_UInt128("uint128"             UINT128_DEF "HAVEuint128")
  Check_UInt128("unsigned __int128_t" UINT128_DEF "HAVEunsigned__int128_t")
  Check_UInt128("unsigned int128_t"   UINT128_DEF "HAVEunsignedint128_t")
  Check_UInt128("unsigned __int128"   UINT128_DEF "HAVEunsigned__int128")
  Check_UInt128("unsigned int128"     UINT128_DEF "HAVEunsignedint128")

  if(UINT128_FOUND)
    set(INT128_FLAGS "${INT128_FLAGS} -D${UINT128_DEF}")

    if(HAS_UINT128_STD_HASH)
      set(INT128_FLAGS "${INT128_FLAGS} -DHASH_FOR_UINT128_DEFINED")
    endif()

  else()
    message("Compiler/platform support for unsigned 128 bit integers not found, falling back to boost mpfr.")
  endif()

endmacro()
