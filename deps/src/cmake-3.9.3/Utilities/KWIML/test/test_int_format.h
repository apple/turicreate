/*
  Copyright Kitware, Inc.
  Distributed under the OSI-approved BSD 3-Clause License.
  See accompanying file Copyright.txt for details.
*/
#include <stdio.h>
#include <string.h>

#if defined(_MSC_VER)
# pragma warning (push)
# pragma warning (disable:4310) /* cast truncates constant value */
#endif

#ifdef __cplusplus
# define LANG "C++ "
#else
# define LANG "C "
#endif

#define VALUE(T, U) (T)((U)0xab << ((sizeof(T)-1)<<3))

#define TEST_C_(C, V, PRI, T, U)                                        \
  {                                                                     \
  T const x = VALUE(T, U);                                              \
  T y = C(V);                                                           \
  printf(LANG #C ":"                                                    \
         " expression [%" KWIML_INT_PRI##PRI "],"                       \
         " literal [%" KWIML_INT_PRI##PRI "]", x, y);                   \
  if(x == y)                                                            \
    {                                                                   \
    printf(", PASSED\n");                                               \
    }                                                                   \
  else                                                                  \
    {                                                                   \
    printf(", FAILED\n");                                               \
    result = 0;                                                         \
    }                                                                   \
  }

#define TEST_PRI_(PRI, T, U, STR)                                       \
  {                                                                     \
  T const x = VALUE(T, U);                                              \
  char const* str = STR;                                                \
  sprintf(buf, "%" KWIML_INT_PRI##PRI, x);                              \
  printf(LANG "KWIML_INT_PRI" #PRI ":"                                  \
         " expected [%s], got [%s]", str, buf);                         \
  if(strcmp(str, buf) == 0)                                             \
    {                                                                   \
    printf(", PASSED\n");                                               \
    }                                                                   \
  else                                                                  \
    {                                                                   \
    printf(", FAILED\n");                                               \
    result = 0;                                                         \
    }                                                                   \
  }

#define TEST_SCN_(SCN, T, U, STR) TEST_SCN2_(SCN, SCN, T, U, STR)
#define TEST_SCN2_(PRI, SCN, T, U, STR)                                 \
  {                                                                     \
  T const x = VALUE(T, U);                                              \
  T y;                                                                  \
  char const* str = STR;                                                \
  if(sscanf(str, "%" KWIML_INT_SCN##SCN, &y) != 1)                      \
    {                                                                   \
    y = 0;                                                              \
    }                                                                   \
  printf(LANG "KWIML_INT_SCN" #SCN ":"                                  \
         " expected [%" KWIML_INT_PRI##PRI "],"                         \
         " got [%" KWIML_INT_PRI##PRI "]", x, y);                       \
  if(x == y)                                                            \
    {                                                                   \
    printf(", PASSED\n");                                               \
    }                                                                   \
  else                                                                  \
    {                                                                   \
    printf(", FAILED\n");                                               \
    result = 0;                                                         \
    }                                                                   \
  }

#define TEST_(FMT, T, U, STR) TEST2_(FMT, FMT, T, U, STR)
#define TEST2_(PRI, SCN, T, U, STR)                                     \
  TEST_PRI_(PRI, T, U, STR)                                             \
  TEST_SCN2_(PRI, SCN, T, U, STR)

/* Concatenate T and U now to avoid expanding them.  */
#define TEST(FMT, T, U, STR) \
        TEST_(FMT, KWIML_INT_##T, KWIML_INT_##U, STR)
#define TEST2(PRI, SCN, T, U, STR) \
        TEST2_(PRI, SCN, KWIML_INT_##T, KWIML_INT_##U, STR)
#define TEST_C(C, V, PRI, T, U) \
        TEST_C_(KWIML_INT_##C, V, PRI, KWIML_INT_##T, KWIML_INT_##U)
#define TEST_PRI(PRI, T, U, STR) \
        TEST_PRI_(PRI, KWIML_INT_##T, KWIML_INT_##U, STR)
#define TEST_SCN(SCN, T, U, STR) \
        TEST_SCN_(SCN, KWIML_INT_##T, KWIML_INT_##U, STR)
#define TEST_SCN2(PRI, SCN, T, U, STR) \
        TEST_SCN2_(PRI, SCN, KWIML_INT_##T, KWIML_INT_##U, STR)

static int test_int_format(void)
{
  int result = 1;
  char buf[256];
  TEST_PRI(i8, int8_t, uint8_t, "-85")
#if defined(KWIML_INT_SCNi8)
  TEST_SCN(i8, int8_t, uint8_t, "-85")
#endif
  TEST_PRI(d8, int8_t, uint8_t, "-85")
#if defined(KWIML_INT_SCNd8)
  TEST_SCN(d8, int8_t, uint8_t, "-85")
#endif
  TEST_PRI(o8, uint8_t, uint8_t, "253")
#if defined(KWIML_INT_SCNo8)
  TEST_SCN(o8, uint8_t, uint8_t, "253")
#endif
  TEST_PRI(u8, uint8_t, uint8_t, "171")
#if defined(KWIML_INT_SCNu8)
  TEST_SCN(u8, uint8_t, uint8_t, "171")
#endif
  TEST_PRI(x8, uint8_t, uint8_t, "ab")
  TEST_PRI(X8, uint8_t, uint8_t, "AB")
#if defined(KWIML_INT_SCNx8)
  TEST_SCN(x8, uint8_t, uint8_t, "ab")
  TEST_SCN2(X8, x8, uint8_t, uint8_t, "AB")
#endif

  TEST(i16, int16_t, uint16_t, "-21760")
  TEST(d16, int16_t, uint16_t, "-21760")
  TEST(o16, uint16_t, uint16_t, "125400")
  TEST(u16, uint16_t, uint16_t, "43776")
  TEST(x16, uint16_t, uint16_t, "ab00")
  TEST2(X16, x16, uint16_t, uint16_t, "AB00")

  TEST(i32, int32_t, uint32_t, "-1426063360")
  TEST(d32, int32_t, uint32_t, "-1426063360")
  TEST(o32, uint32_t, uint32_t, "25300000000")
  TEST(u32, uint32_t, uint32_t, "2868903936")
  TEST(x32, uint32_t, uint32_t, "ab000000")
  TEST2(X32, x32, uint32_t, uint32_t, "AB000000")

  TEST_PRI(i64, int64_t, uint64_t, "-6124895493223874560")
#if defined(KWIML_INT_SCNi64)
  TEST_SCN(i64, int64_t, uint64_t, "-6124895493223874560")
#endif
  TEST_PRI(d64, int64_t, uint64_t, "-6124895493223874560")
#if defined(KWIML_INT_SCNd64)
  TEST_SCN(d64, int64_t, uint64_t, "-6124895493223874560")
#endif
  TEST_PRI(o64, uint64_t, uint64_t, "1254000000000000000000")
#if defined(KWIML_INT_SCNo64)
  TEST_SCN(o64, uint64_t, uint64_t, "1254000000000000000000")
#endif
  TEST_PRI(u64, uint64_t, uint64_t, "12321848580485677056")
#if defined(KWIML_INT_SCNu64)
  TEST_SCN(u64, uint64_t, uint64_t, "12321848580485677056")
#endif
  TEST_PRI(x64, uint64_t, uint64_t, "ab00000000000000")
  TEST_PRI(X64, uint64_t, uint64_t, "AB00000000000000")
#if defined(KWIML_INT_SCNx64)
  TEST_SCN(x64, uint64_t, uint64_t, "ab00000000000000")
  TEST_SCN2(X64, x64, uint64_t, uint64_t, "AB00000000000000")
#endif

#if !defined(KWIML_INT_NO_INTPTR_T)
# if KWIML_ABI_SIZEOF_DATA_PTR == 4
  TEST(iPTR, intptr_t, uint32_t, "-1426063360")
  TEST(dPTR, intptr_t, uint32_t, "-1426063360")
# else
  TEST(iPTR, intptr_t, uint64_t, "-6124895493223874560")
  TEST(dPTR, intptr_t, uint64_t, "-6124895493223874560")
# endif
#endif

#if !defined(KWIML_INT_NO_UINTPTR_T)
# if KWIML_ABI_SIZEOF_DATA_PTR == 4
  TEST(oPTR, uintptr_t, uintptr_t, "25300000000")
  TEST(uPTR, uintptr_t, uintptr_t, "2868903936")
  TEST(xPTR, uintptr_t, uintptr_t, "ab000000")
  TEST2(XPTR, xPTR, uintptr_t, uintptr_t, "AB000000")
# else
  TEST(oPTR, uintptr_t, uintptr_t, "1254000000000000000000")
  TEST(uPTR, uintptr_t, uintptr_t, "12321848580485677056")
  TEST(xPTR, uintptr_t, uintptr_t, "ab00000000000000")
  TEST2(XPTR, xPTR, uintptr_t, uintptr_t, "AB00000000000000")
# endif
#endif

  TEST_C(INT8_C,  -0x55, i8, int8_t, uint8_t)
  TEST_C(UINT8_C,  0xAB, u8, uint8_t, uint8_t)
  TEST_C(INT16_C, -0x5500, i16, int16_t, uint16_t)
  TEST_C(UINT16_C, 0xAB00, u16, uint16_t, uint16_t)
  TEST_C(INT32_C, -0x55000000, i32, int32_t, uint32_t)
  TEST_C(UINT32_C, 0xAB000000, u32, uint32_t, uint32_t)
  TEST_C(INT64_C, -0x5500000000000000, i64, int64_t, uint64_t)
  TEST_C(UINT64_C, 0xAB00000000000000, u64, uint64_t, uint64_t)

  return result;
}

#if defined(_MSC_VER)
# pragma warning (pop)
#endif
