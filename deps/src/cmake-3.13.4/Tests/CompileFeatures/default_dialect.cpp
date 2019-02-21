
template <long l>
struct Outputter;

#if defined(_MSC_VER) && defined(_MSVC_LANG)
#  define CXX_STD _MSVC_LANG
#else
#  define CXX_STD __cplusplus
#endif

#if DEFAULT_CXX20
#  if CXX_STD <= 201703L
Outputter<CXX_STD> o;
#  endif
#elif DEFAULT_CXX17
#  if CXX_STD <= 201402L
Outputter<CXX_STD> o;
#  endif
#elif DEFAULT_CXX14
#  if CXX_STD != 201402L
Outputter<CXX_STD> o;
#  endif
#elif DEFAULT_CXX11
#  if CXX_STD != 201103L
Outputter<CXX_STD> o;
#  endif
#else
#  if !DEFAULT_CXX98
#    error Buildsystem error
#  endif
#  if CXX_STD != 199711L && CXX_STD != 1 &&                                   \
    !defined(__GXX_EXPERIMENTAL_CXX0X__)
Outputter<CXX_STD> o;
#  endif
#endif

int main()
{
  return 0;
}
