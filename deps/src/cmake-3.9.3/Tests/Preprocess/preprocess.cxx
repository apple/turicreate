#include <preprocess.h>

#include FILE_PATH
#include TARGET_PATH

#include <stdio.h>
#include <string.h>

extern "C" int check_defines_C(void);

int check_defines_CXX()
{
  int result = 1;
  if (strcmp(FILE_STRING, STRING_VALUE) != 0) {
    fprintf(stderr, "FILE_STRING has wrong value in CXX [%s]\n", FILE_STRING);
    result = 0;
  }
  if (strcmp(TARGET_STRING, STRING_VALUE) != 0) {
    fprintf(stderr, "TARGET_STRING has wrong value in CXX [%s]\n",
            TARGET_STRING);
    result = 0;
  }
  {
    int x = 2;
    int y = 3;
    if ((FILE_EXPR) != (EXPR)) {
      fprintf(stderr, "FILE_EXPR did not work in CXX [%s]\n",
              TO_STRING(FILE_EXPR));
      result = 0;
    }
    if ((TARGET_EXPR) != (EXPR)) {
      fprintf(stderr, "TARGET_EXPR did not work in CXX [%s]\n",
              TO_STRING(FILE_EXPR));
      result = 0;
    }
  }
#ifdef NDEBUG
#ifdef FILE_DEF_DEBUG
  {
    fprintf(stderr, "FILE_DEF_DEBUG should not be defined in CXX\n");
    result = 0;
  }
#endif
#ifdef TARGET_DEF_DEBUG
  {
    fprintf(stderr, "TARGET_DEF_DEBUG should not be defined in CXX\n");
    result = 0;
  }
#endif
#ifdef DIRECTORY_DEF_DEBUG
  {
    fprintf(stderr, "DIRECTORY_DEF_DEBUG should not be defined in CXX\n");
    result = 0;
  }
#endif
#ifndef FILE_DEF_RELEASE
#ifndef PREPROCESS_XCODE
  {
    fprintf(stderr, "FILE_DEF_RELEASE should be defined in CXX\n");
    result = 0;
  }
#endif
#endif
#ifndef TARGET_DEF_RELEASE
  {
    fprintf(stderr, "TARGET_DEF_RELEASE should be defined in CXX\n");
    result = 0;
  }
#endif
#ifndef DIRECTORY_DEF_RELEASE
  {
    fprintf(stderr, "DIRECTORY_DEF_RELEASE should be defined in CXX\n");
    result = 0;
  }
#endif
#endif
#ifdef PREPROCESS_DEBUG
#ifndef FILE_DEF_DEBUG
#ifndef PREPROCESS_XCODE
  {
    fprintf(stderr, "FILE_DEF_DEBUG should be defined in CXX\n");
    result = 0;
  }
#endif
#endif
#ifndef TARGET_DEF_DEBUG
  {
    fprintf(stderr, "TARGET_DEF_DEBUG should be defined in CXX\n");
    result = 0;
  }
#endif
#ifndef DIRECTORY_DEF_DEBUG
  {
    fprintf(stderr, "DIRECTORY_DEF_DEBUG should be defined in CXX\n");
    result = 0;
  }
#endif
#ifdef FILE_DEF_RELEASE
  {
    fprintf(stderr, "FILE_DEF_RELEASE should not be defined in CXX\n");
    result = 0;
  }
#endif
#ifdef TARGET_DEF_RELEASE
  {
    fprintf(stderr, "TARGET_DEF_RELEASE should not be defined in CXX\n");
    result = 0;
  }
#endif
#ifdef DIRECTORY_DEF_RELEASE
  {
    fprintf(stderr, "DIRECTORY_DEF_RELEASE should not be defined in CXX\n");
    result = 0;
  }
#endif
#endif
#if defined(FILE_DEF_DEBUG) || defined(TARGET_DEF_DEBUG)
#if !defined(FILE_DEF_DEBUG) || !defined(TARGET_DEF_DEBUG)
#ifndef PREPROCESS_XCODE
  {
    fprintf(stderr,
            "FILE_DEF_DEBUG and TARGET_DEF_DEBUG inconsistent in CXX\n");
    result = 0;
  }
#endif
#endif
#if defined(FILE_DEF_RELEASE) || defined(TARGET_DEF_RELEASE)
  {
    fprintf(stderr, "DEBUG and RELEASE definitions inconsistent in CXX\n");
    result = 0;
  }
#endif
#endif
#if defined(FILE_DEF_RELEASE) || defined(TARGET_DEF_RELEASE)
#if !defined(FILE_DEF_RELEASE) || !defined(TARGET_DEF_RELEASE)
#ifndef PREPROCESS_XCODE
  {
    fprintf(stderr,
            "FILE_DEF_RELEASE and TARGET_DEF_RELEASE inconsistent in CXX\n");
    result = 0;
  }
#endif
#endif
#if defined(FILE_DEF_DEBUG) || defined(TARGET_DEF_DEBUG)
  {
    fprintf(stderr, "RELEASE and DEBUG definitions inconsistent in CXX\n");
    result = 0;
  }
#endif
#endif
#ifndef FILE_PATH_DEF
  {
    fprintf(stderr, "FILE_PATH_DEF not defined in CXX\n");
    result = 0;
  }
#endif
#ifndef TARGET_PATH_DEF
  {
    fprintf(stderr, "TARGET_PATH_DEF not defined in CXX\n");
    result = 0;
  }
#endif
#ifndef FILE_DEF
  {
    fprintf(stderr, "FILE_DEF not defined in CXX\n");
    result = 0;
  }
#endif
#ifndef TARGET_DEF
  {
    fprintf(stderr, "TARGET_DEF not defined in CXX\n");
    result = 0;
  }
#endif
#ifndef DIRECTORY_DEF
  {
    fprintf(stderr, "DIRECTORY_DEF not defined in CXX\n");
    result = 0;
  }
#endif
#ifndef OLD_DEF
  {
    fprintf(stderr, "OLD_DEF not defined in CXX\n");
    result = 0;
  }
#endif
#if !defined(OLD_EXPR) || OLD_EXPR != 2
  {
    fprintf(stderr, "OLD_EXPR id not work in C [%s]\n", TO_STRING(OLD_EXPR));
    result = 0;
  }
#endif
  return result;
}

int main()
{
  int result = 1;

  if (!check_defines_C()) {
    result = 0;
  }

  if (!check_defines_CXX()) {
    result = 0;
  }

  if (result) {
    printf("All preprocessor definitions are correct.\n");
    return 0;
  } else {
    return 1;
  }
}
