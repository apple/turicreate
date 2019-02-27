/*
  Copyright Kitware, Inc.
  Distributed under the OSI-approved BSD 3-Clause License.
  See accompanying file Copyright.txt for details.
*/
#ifdef __cplusplus
extern "C" {
#endif
extern int test_abi_C(void);
extern int test_int_C(void);
extern int test_abi_CXX(void);
extern int test_int_CXX(void);
extern int test_include_C(void);
extern int test_include_CXX(void);
#ifdef __cplusplus
} // extern "C"
#endif

int main(void)
{
  int result = 1;
#ifdef KWIML_LANGUAGE_C
  result = test_abi_C() && result;
  result = test_int_C() && result;
  result = test_include_C() && result;
#endif
#ifdef KWIML_LANGUAGE_CXX
  result = test_abi_CXX() && result;
  result = test_int_CXX() && result;
  result = test_include_CXX() && result;
#endif
  return result? 0 : 1;
}
