
#define JOIN_IMPL(A, B) A##B
#define JOIN(A, B) JOIN_IMPL(A, B)

#define CHECK(FEATURE)                                                        \
  (JOIN(PREFIX, JOIN(_COMPILER_, FEATURE)) ==                                 \
   JOIN(EXPECTED_COMPILER_, FEATURE))

#if !CHECK(CXX_DELEGATING_CONSTRUCTORS)
#error cxx_delegating_constructors expected availability did not match.
#endif

#if !CHECK(CXX_VARIADIC_TEMPLATES)
#error cxx_variadic_templates expected availability did not match.
#endif

#if !CHECK(VERSION_MAJOR)
#error Compiler major version did not match.
#endif

#if !CHECK(VERSION_MINOR)
#error Compiler minor version did not match.
#endif

#if !CHECK(VERSION_PATCH)
#error Compiler patch version did not match.
#endif
