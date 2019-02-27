#if defined(_MSC_VER) && _MSC_VER == 1800 && _MSC_FULL_VER < 180030723
#  error "VS 2013 safely supports this only with Update 3 or greater"
#endif

// Dummy implementation. Test only the compiler feature.
namespace std {
typedef decltype(sizeof(int)) size_t;
template <class _E>
class initializer_list
{
  const _E* __begin_;
  size_t __size_;

#ifdef __INTEL_COMPILER
  // The Intel compiler internally asserts the constructor overloads, so
  // reproduce the constructor used in its <initializer_list> header.
  initializer_list(const _E*, size_t) {}
#else
public:
  template <typename T1, typename T2>
  initializer_list(T1, T2)
  {
  }
#endif
};
}

template <typename T>
struct A
{
  A(std::initializer_list<T>) {}
};

void someFunc()
{
  A<int> as = { 1, 2, 3, 4 };
}
