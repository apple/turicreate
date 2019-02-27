
#if defined(__ELF__)
#  if !defined(__PIC__) && !defined(__PIE__)
#    error "The POSITION_INDEPENDENT_CODE property should cause __PIC__ or __PIE__ to be defined on ELF platforms."
#  endif
#endif

#if defined(PIC_TEST_STATIC_BUILD)
#  define PIC_TEST_EXPORT
#else
#  if defined(_WIN32) || defined(WIN32) /* Win32 version */
#    ifdef PIC_TEST_BUILD_DLL
#      define PIC_TEST_EXPORT __declspec(dllexport)
#    else
#      define PIC_TEST_EXPORT __declspec(dllimport)
#    endif
#  else
#    define PIC_TEST_EXPORT
#  endif
#endif
