
#ifndef CMAKE_IS_FUN
#  error Expect CMAKE_IS_FUN definition
#endif

#if CMAKE_IS != Fun
#  error Expect CMAKE_IS=Fun definition
#endif

template <bool test>
struct CMakeStaticAssert;

template <>
struct CMakeStaticAssert<true>
{
};

static const char fun_string[] = CMAKE_IS_;
#ifndef NO_SPACES_IN_DEFINE_VALUES
static const char very_fun_string[] = CMAKE_IS_REALLY;
#endif

enum
{
  StringLiteralTest1 =
    sizeof(CMakeStaticAssert<sizeof(CMAKE_IS_) == sizeof("Fun")>),
#ifndef NO_SPACES_IN_DEFINE_VALUES
  StringLiteralTest2 =
    sizeof(CMakeStaticAssert<sizeof(CMAKE_IS_REALLY) == sizeof("Very Fun")>),
#endif
#ifdef TEST_GENERATOR_EXPRESSIONS
  StringLiteralTest3 =
    sizeof(CMakeStaticAssert<sizeof(LETTER_LIST1) == sizeof("A,B,C,D")>),
  StringLiteralTest4 =
    sizeof(CMakeStaticAssert<sizeof(LETTER_LIST2) == sizeof("A,,B,,C,,D")>),
  StringLiteralTest5 =
    sizeof(CMakeStaticAssert<sizeof(LETTER_LIST3) == sizeof("A,-B,-C,-D")>),
  StringLiteralTest6 =
    sizeof(CMakeStaticAssert<sizeof(LETTER_LIST4) == sizeof("A-,-B-,-C-,-D")>),
  StringLiteralTest7 =
    sizeof(CMakeStaticAssert<sizeof(LETTER_LIST5) == sizeof("A-,B-,C-,D")>)
#endif
};

#ifdef TEST_GENERATOR_EXPRESSIONS
#  ifndef CMAKE_IS_DECLARATIVE
#    error Expect declarative definition
#  endif
#  ifdef GE_NOT_DEFINED
#    error Expect not defined generator expression
#  endif

#  ifndef ARGUMENT
#    error Expected define expanded from list
#  endif
#  ifndef LIST
#    error Expected define expanded from list
#  endif

#  ifndef PREFIX_DEF1
#    error Expect PREFIX_DEF1
#  endif

#  ifndef PREFIX_DEF2
#    error Expect PREFIX_DEF2
#  endif

#  ifndef LINK_CXX_DEFINE
#    error Expected LINK_CXX_DEFINE
#  endif
#  ifndef LINK_LANGUAGE_IS_CXX
#    error Expected LINK_LANGUAGE_IS_CXX
#  endif

#  ifdef LINK_C_DEFINE
#    error Unexpected LINK_C_DEFINE
#  endif
#  ifdef LINK_LANGUAGE_IS_C
#    error Unexpected LINK_LANGUAGE_IS_C
#  endif

// TEST_GENERATOR_EXPRESSIONS
#endif

#ifndef BUILD_IS_DEBUG
#  error "BUILD_IS_DEBUG not defined!"
#endif
#ifndef BUILD_IS_NOT_DEBUG
#  error "BUILD_IS_NOT_DEBUG not defined!"
#endif

// Check per-config definitions.
#ifdef TEST_CONFIG_DEBUG
#  if !BUILD_IS_DEBUG
#    error "BUILD_IS_DEBUG false with TEST_CONFIG_DEBUG!"
#  endif
#  if BUILD_IS_NOT_DEBUG
#    error "BUILD_IS_NOT_DEBUG true with TEST_CONFIG_DEBUG!"
#  endif
#else
#  if BUILD_IS_DEBUG
#    error "BUILD_IS_DEBUG true without TEST_CONFIG_DEBUG!"
#  endif
#  if !BUILD_IS_NOT_DEBUG
#    error "BUILD_IS_NOT_DEBUG false without TEST_CONFIG_DEBUG!"
#  endif
#endif

int main(int argc, char** argv)
{
  return 0;
}
