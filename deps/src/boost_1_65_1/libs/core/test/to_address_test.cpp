/*
Copyright 2017 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#include <boost/core/pointer_traits.hpp>
#include <boost/core/lightweight_test.hpp>

template<class T>
class pointer {
public:
    typedef typename boost::pointer_traits<T>::element_type element_type;
    pointer(T value)
        : value_(value) { }
    T operator->() const BOOST_NOEXCEPT {
        return value_;
    }
private:
    T value_;
};

template<class T>
class special {
public:
    special(T* value)
        : value_(value) { }
    T* get() const BOOST_NOEXCEPT {
        return value_;
    }
private:
    T* value_;
};

namespace boost {

template<class T>
struct pointer_traits<special<T> > {
    typedef special<T> pointer;
    typedef T element_type;
    typedef std::ptrdiff_t difference_type;
    template<class U>
    struct rebind_to {
        typedef special<U> type;
    };
#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
    template<class U>
    using rebind = typename rebind_to<U>::type;
#endif
    template<class U>
    static pointer pointer_to(U& v) BOOST_NOEXCEPT {
        return pointer(&v);
    }
    static element_type* to_address(const pointer& v) BOOST_NOEXCEPT {
        return v.get();
    }
};

} /* boost */

int main()
{
    int i = 0;
    {
        typedef int* type;
        type p = &i;
        BOOST_TEST(boost::to_address(p) == &i);
    }
    {
        typedef pointer<int*> type;
        type p(&i);
        BOOST_TEST(boost::to_address(p) == &i);
    }
    {
        typedef pointer<pointer<int*> > type;
        type p(&i);
        BOOST_TEST(boost::to_address(p) == &i);
    }
    {
        typedef void* type;
        type p = &i;
        BOOST_TEST(boost::to_address(p) == &i);
    }
    {
        typedef pointer<void*> type;
        type p(&i);
        BOOST_TEST(boost::to_address(p) == &i);
    }
    {
        typedef const int* type;
        type p = &i;
        BOOST_TEST(boost::to_address(p) == &i);
    }
    {
        typedef pointer<const int*> type;
        type p(&i);
        BOOST_TEST(boost::to_address(p) == &i);
    }
    {
        typedef special<int> type;
        type p(&i);
        BOOST_TEST(boost::to_address(p) == &i);
    }
    {
        typedef special<void> type;
        type p(&i);
        BOOST_TEST(boost::to_address(p) == &i);
    }
    {
        typedef special<const int> type;
        type p(&i);
        BOOST_TEST(boost::to_address(p) == &i);
    }
    {
        typedef pointer<special<int> > type;
        type p(&i);
        BOOST_TEST(boost::to_address(p) == &i);
    }
    {
        typedef pointer<special<void> > type;
        type p(&i);
        BOOST_TEST(boost::to_address(p) == &i);
    }
    {
        typedef pointer<special<const int> > type;
        type p(&i);
        BOOST_TEST(boost::to_address(p) == &i);
    }
    return boost::report_errors();
}
