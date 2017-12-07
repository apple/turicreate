
template <long l>
struct Outputter;

#if DEFAULT_CXX17
#if __cplusplus <= 201402L
Outputter<__cplusplus> o;
#endif
#elif DEFAULT_CXX14
#if __cplusplus != 201402L
Outputter<__cplusplus> o;
#endif
#elif DEFAULT_CXX11
#if __cplusplus != 201103L
Outputter<__cplusplus> o;
#endif
#else
#if !DEFAULT_CXX98
#error Buildsystem error
#endif
#if __cplusplus != 199711L && __cplusplus != 1 &&                             \
  !defined(__GXX_EXPERIMENTAL_CXX0X__)
Outputter<__cplusplus> o;
#endif
#endif

int main()
{
  return 0;
}
