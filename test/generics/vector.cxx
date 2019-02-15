/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <iostream>
#include <typeinfo>       // operator typeid
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <iterator>


#include <generics/vector.hpp>
#include <util/testing_utils.hpp>

using namespace turi;

template <typename T>
void verify_serialization(const gl_vector<T>& v) {
  
  // into gl_vector
  gl_vector<T> v1; 
  save_and_load_object(v1, v);
  DASSERT_TRUE(v == v1);

  // Into std
  std::vector<T> v2; 
  save_and_load_object(v2, v);
  DASSERT_EQ(v2.size(), v.size());
  DASSERT_TRUE(std::equal(v2.begin(), v2.end(), v.begin()));

  // from std into empty
  std::vector<T> v3; 
  v3 = v;
  gl_vector<T> v4; 
  save_and_load_object(v4, v3);
  DASSERT_TRUE(v4 == v);
  
  // from std into non-empty
  save_and_load_object(v1, v3);
  DASSERT_TRUE(v1 == v);
}

template <typename T> 
void verify_consistency(const gl_vector<T>& v) {

  // Constructor 1
  {
    gl_vector<T> v2(v);
    DASSERT_TRUE(v == v2);
  }
  
  // Constructor 2
  {
    gl_vector<T> v2(v.begin(), v.end());
    DASSERT_TRUE(v == v2);
  }
  
  // Assignment
  {
    gl_vector<T> v2;
    v2 = v;
    DASSERT_TRUE(v == v2);
  }

  // Assignment
  {
    gl_vector<T> v2;
    v2.assign(v.begin(), v.end());
    DASSERT_TRUE(v == v2);
  }
  
  // Assignment by insert
  {
    gl_vector<T> v2;
    v2.insert(v2.end(), v.begin(), v.end());
    DASSERT_TRUE(v == v2);
  }

  // Assignment by insert into cleared vector
  {
    gl_vector<T> v2;
    v2.resize(1);
    v2.clear();
    v2.insert(v2.end(), v.begin(), v.end());
    DASSERT_TRUE(v == v2);
  }
  
  // construction by empty, then resize, then index fill
  {
    gl_vector<T> v2;
    v2.resize(v.size());
    for(size_t i = 0; i < v.size(); ++i) {
      v2[i] = v[i];
    }
    DASSERT_TRUE(v == v2);
  }

  // construction by empty, then resize, then 
  {
    gl_vector<T> v2;
    v2.reserve(v.size());
    for(auto e : v) {
      v2.push_back(e);
    }
    DASSERT_TRUE(v == v2);
  }
  
  // construction by empty, then resize, then iteration
  {
    gl_vector<T> v2;
    v2.resize(v.size());
    auto it = v2.begin(); 
    for(size_t i = 0; i < v.size(); ++i, ++it) {
      *it = v[i]; 
    }
    DASSERT_TRUE(v == v2);
  }

  // construction by empty, then resize, then reverse iteration
  {
    gl_vector<T> v2;
    v2.resize(v.size());
    auto it = v2.rbegin(); 
    for(size_t i = v.size(); (i--) != 0; ++it) {
      *it = v[i]; 
    }
    DASSERT_TRUE(v == v2);
  }
  
  // Assignment by insert into cleared vector.
  {
    gl_vector<T> v2;
    v2.resize(1);
    v2.insert(v2.begin(), v.begin(), v.end());
    v2.resize(v.size());
    DASSERT_TRUE(v == v2);
  }

  // Assignment by insert into cleared vector, then erase
  {
    gl_vector<T> v2;
    v2.resize(1);
    v2.insert(v2.end(), v.begin(), v.end());
    v2.erase(v2.begin());
    DASSERT_TRUE(v == v2);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // test casting. 
  {
    std::vector<T> v_stl;

    v_stl = v;

    TS_ASSERT_EQUALS(v_stl.size(), v.size());
    TS_ASSERT(std::equal(v.begin(), v.end(), v_stl.begin()));

    // Assignment 
    gl_vector<T> v2(20);

    v2 = v_stl;

    TS_ASSERT_EQUALS(v2, v); 
  
    // Construction 
    gl_vector<T> v3(v_stl);

    TS_ASSERT_EQUALS(v3, v);
  }
}

using namespace turi;

// Full compiler initialization
template class turi::gl_vector<int>;
template class turi::gl_vector<gl_vector<int> >;

class A
{
  int i_;
  double d_;

  A(const A&);
  A& operator=(const A&);
 public:
  A(int i, double d)
      : i_(i), d_(d) {}

  A(A&& a)
  : i_(a.i_),
    d_(a.d_)
  {
    a.i_ = 0;
    a.d_ = 0;
  }

  A& operator=(A&& a)
  {
    i_ = a.i_;
    d_ = a.d_;
    a.i_ = 0;
    a.d_ = 0;
    return *this;
  }

  int geti() const {return i_;}
  double getd() const {return d_;}
};

////////////////////////////////////////////////////////////////////////////////

class Copyable
{
 public:
};

////////////////////////////////////////////////////////////////////////////////

class MoveOnly
{
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;

  int data_;
 public:
  MoveOnly(int data = 1) : data_(data) {}
  MoveOnly(MoveOnly&& x)
  : data_(x.data_) {x.data_ = 0;}
  MoveOnly& operator=(MoveOnly&& x)
  {data_ = x.data_; x.data_ = 0; return *this;}

  int get() const {return data_;}
  
  bool operator==(const MoveOnly& x) const {return data_ == x.data_;}
  bool operator< (const MoveOnly& x) const {return data_ <  x.data_;}
};

////////////////////////////////////////////////////////////////////////////////

template <class C, class Iterator>
void test_iterator_fill(Iterator first, Iterator last)
{
  C c(first, last);
  TS_ASSERT(c.size() == size_t(std::distance(first, last)));
  for (typename C::const_iterator i = c.cbegin(), e = c.cend(); i != e; ++i, ++first)
    TS_ASSERT(*i == *first);
}

////////////////////////////////////////////////////////////////////////////////

namespace std {

template <>
struct hash<MoveOnly>
    : public std::unary_function<MoveOnly, std::size_t>
{
  std::size_t operator()(const MoveOnly& x) const {return x.get();}
};

}

////////////////////////////////////////////////////////////////////////////////

struct gl_vector_datatype_test  {

 public:

  void test_sanity() {
    gl_vector<int> v;
    TS_ASSERT_EQUALS(v.size(), 0);
    verify_consistency(v);
    verify_serialization(v);
  }

  void test_default_size_constructor() {
    size_t n = 10;

    gl_vector<int> c(n);
    TS_ASSERT(c.size() == n);

    for (typename gl_vector<int>::const_iterator i = c.cbegin(), e = c.cend(); i != e; ++i)
      TS_ASSERT(*i == typename gl_vector<int>::value_type());

    verify_consistency(c);
    verify_serialization(c);
  }

  void test_default_size_constructor_2() {
    size_t n = 10;
    gl_vector<int> c(n, 5);
    TS_ASSERT(c.size() == n);

    for (typename gl_vector<int>::const_iterator i = c.cbegin(), e = c.cend(); i != e; ++i)
      TS_ASSERT(*i == 5);
    verify_consistency(c);
    verify_serialization(c);
  }

  void test_fill_from_iterator() {
    int a[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 8, 7, 6, 5, 4, 3, 1, 0};
    int* an = a + sizeof(a)/sizeof(a[0]);

    test_iterator_fill<gl_vector<int> >(a, an);

    std::list<int> al(a, an);
    test_iterator_fill<gl_vector<int> >(al.begin(), al.end());
  }

  void test_back() {
    typedef int T;
    typedef gl_vector<T> C;
    C c(1, 0);
    TS_ASSERT_EQUALS(c.back(), 0);
    verify_consistency(c);
    verify_serialization(c);
  }

  void test_front() {
    typedef int T;
    typedef gl_vector<T> C;
    C c(1, 0);
    TS_ASSERT_EQUALS(c.front(), 0);
    verify_consistency(c);
    verify_serialization(c);
  }

  void test_emplace() {
    gl_vector<A> c;
    gl_vector<A>::iterator i = c.emplace(c.cbegin(), 2, 3.5);

    TS_ASSERT(i == c.begin());
    TS_ASSERT_EQUALS(c.size(), 1);
    TS_ASSERT_EQUALS(c.front().geti(), 2);
    TS_ASSERT_EQUALS(c.front().getd(), 3.5);
    i = c.emplace(c.cend(), 3, 4.5);

    TS_ASSERT(i == c.end()-1);
    TS_ASSERT_EQUALS(c.size(), 2);
    TS_ASSERT_EQUALS(c.front().geti(), 2);
    TS_ASSERT_EQUALS(c.front().getd(), 3.5);
    TS_ASSERT_EQUALS(c.back().geti(), 3);
    TS_ASSERT_EQUALS(c.back().getd(), 4.5);

    i = c.emplace(c.cbegin()+1, 4, 6.5);
    TS_ASSERT(i == c.begin()+1);
    TS_ASSERT_EQUALS(c.size(), 3);
    TS_ASSERT_EQUALS(c.front().geti(), 2);
    TS_ASSERT_EQUALS(c.front().getd(), 3.5);
    TS_ASSERT_EQUALS(c[1].geti(), 4);
    TS_ASSERT_EQUALS(c[1].getd(), 6.5);
    TS_ASSERT_EQUALS(c.back().geti(), 3);
    TS_ASSERT_EQUALS(c.back().getd(), 4.5);
  }

  void test_emplace_back() {
    gl_vector<A> c;
    c.emplace_back(2, 3.5);
    TS_ASSERT_EQUALS(c.size(), 1);
    TS_ASSERT_EQUALS(c.front().geti(), 2);
    TS_ASSERT_EQUALS(c.front().getd(), 3.5);

    c.emplace_back(3, 4.5);
    TS_ASSERT_EQUALS(c.size(), 2);
    TS_ASSERT_EQUALS(c.front().geti(), 2);
    TS_ASSERT_EQUALS(c.front().getd(), 3.5);
    TS_ASSERT_EQUALS(c.back().geti(), 3);
    TS_ASSERT_EQUALS(c.back().getd(), 4.5);
  }

  void test_erase() {
    int a1[] = {1, 2, 3};
    gl_vector<int> l1(a1, a1+3);
    gl_vector<int>::const_iterator i = l1.begin();
    ++i;
    gl_vector<int>::iterator j = l1.erase(i);
    TS_ASSERT_EQUALS(l1.size(), 2);
    TS_ASSERT_EQUALS(std::distance(l1.begin(), l1.end()), 2);
    TS_ASSERT_EQUALS(*j, 3);
    TS_ASSERT_EQUALS(*l1.begin(), 1);
    TS_ASSERT_EQUALS(*std::next(l1.begin()), 3);

    verify_consistency(l1);
    verify_serialization(l1);

    j = l1.erase(j);
    TS_ASSERT_EQUALS(j, l1.end());
    TS_ASSERT_EQUALS(l1.size(), 1);
    TS_ASSERT_EQUALS(std::distance(l1.begin(), l1.end()), 1);
    TS_ASSERT_EQUALS(*l1.begin(), 1);

    verify_consistency(l1);
    verify_serialization(l1);

    j = l1.erase(l1.begin());
    TS_ASSERT_EQUALS(j, l1.end());
    TS_ASSERT_EQUALS(l1.size(), 0);
    TS_ASSERT_EQUALS(std::distance(l1.begin(), l1.end()), 0);

    verify_consistency(l1);
    verify_serialization(l1);
    
  }

  void test_erase_iter_iter() {

    int a1[] = {1, 2, 3};
    {
      gl_vector<int> l1(a1, a1+3);
      gl_vector<int>::iterator i = l1.erase(l1.cbegin(), l1.cbegin());
      TS_ASSERT_EQUALS(l1.size(), 3);
      TS_ASSERT(std::distance(l1.cbegin(), l1.cend()) == 3);
      TS_ASSERT(i == l1.begin());
      
      verify_consistency(l1);
      verify_serialization(l1);
    }
    {
      gl_vector<int> l1(a1, a1+3);
      gl_vector<int>::iterator i = l1.erase(l1.cbegin(), std::next(l1.cbegin()));
      TS_ASSERT_EQUALS(l1.size(), 2);
      TS_ASSERT_EQUALS(std::distance(l1.cbegin(), l1.cend()), 2);
      TS_ASSERT_EQUALS(i, l1.begin());
      TS_ASSERT(l1 == gl_vector<int>(a1+1, a1+3));
      
      verify_consistency(l1);
      verify_serialization(l1);
    }
    {
      gl_vector<int> l1(a1, a1+3);
      gl_vector<int>::iterator i = l1.erase(l1.cbegin(), std::next(l1.cbegin(), 2));
      TS_ASSERT_EQUALS(l1.size(), 1);
      TS_ASSERT_EQUALS(std::distance(l1.cbegin(), l1.cend()), 1);
      TS_ASSERT(i == l1.begin());
      TS_ASSERT(l1 == gl_vector<int>(a1+2, a1+3));
      verify_consistency(l1);
      verify_serialization(l1);
    }
    {
      gl_vector<int> l1(a1, a1+3);
      gl_vector<int>::iterator i = l1.erase(l1.cbegin(), std::next(l1.cbegin(), 3));
      TS_ASSERT_EQUALS(l1.size(), 0);
      TS_ASSERT_EQUALS(std::distance(l1.cbegin(), l1.cend()), 0);
      TS_ASSERT(i == l1.begin());
      verify_consistency(l1);
      verify_serialization(l1);
    }
    {
      gl_vector<gl_vector<int> > outer(2, gl_vector<int>(1));
      outer.erase(outer.begin(), outer.begin());
      TS_ASSERT_EQUALS(outer.size(), 2);
      TS_ASSERT_EQUALS(outer[0].size(), 1);
      TS_ASSERT_EQUALS(outer[1].size(), 1);
      verify_consistency(outer);
      verify_serialization(outer);
    }
  }

  void test_insert_initializer_list() {
    gl_vector<int> d(10, 1);
    gl_vector<int>::iterator i = d.insert(d.cbegin() + 2, {3, 4, 5, 6});
    TS_ASSERT(d.size() == 14);
    TS_ASSERT(i == d.begin() + 2);
    TS_ASSERT_EQUALS(d[0], 1);
    TS_ASSERT_EQUALS(d[1], 1);
    TS_ASSERT_EQUALS(d[2], 3);
    TS_ASSERT_EQUALS(d[3], 4);
    TS_ASSERT_EQUALS(d[4], 5);
    TS_ASSERT_EQUALS(d[5], 6);
    TS_ASSERT_EQUALS(d[6], 1);
    TS_ASSERT_EQUALS(d[7], 1);
    TS_ASSERT_EQUALS(d[8], 1);
    TS_ASSERT_EQUALS(d[9], 1);
    TS_ASSERT_EQUALS(d[10], 1);
    TS_ASSERT_EQUALS(d[11], 1);
    TS_ASSERT_EQUALS(d[12], 1);
    TS_ASSERT_EQUALS(d[13], 1);

    verify_consistency(d);
    verify_serialization(d);
  }

  void test_move()
  {
    gl_vector<MoveOnly> v(100);
    gl_vector<MoveOnly>::iterator i = v.insert(v.cbegin() + 10, MoveOnly(3));
    TS_ASSERT_EQUALS(v.size(), 101);
    TS_ASSERT(i == v.begin() + 10);
    int j;
    for (j = 0; j < 10; ++j)
      TS_ASSERT(v[j] == MoveOnly());
    TS_ASSERT(v[j] == MoveOnly(3));
    for (++j; j < 101; ++j)
      TS_ASSERT(v[j] == MoveOnly());
  }


  void test_insert_by_value_1()
  {
    gl_vector<int> v(100);
    int j;

    for (j = 0; j < 100; ++j)
      TS_ASSERT_EQUALS(v[j], 0);

    gl_vector<int>::iterator i = v.insert(v.cbegin() + 10, 1);

    TS_ASSERT_EQUALS(v.size(), 101);
    TS_ASSERT(i == v.begin() + 10);
    for (j = 0; j < 10; ++j)
      TS_ASSERT_EQUALS(v[j], 0);
    for (; j < 11; ++j)
      TS_ASSERT_EQUALS(v[j], 1);
    for (++j; j < 101; ++j)
      TS_ASSERT_EQUALS(v[j], 0);

    verify_consistency(v);
    verify_serialization(v);
  }

  void test_insert_by_value_2()
  {
    gl_vector<int> v(100);
    gl_vector<int>::iterator i = v.insert(v.cbegin() + 10, 5, 1);
    TS_ASSERT_EQUALS(v.size(), 105);
    TS_ASSERT(i == v.begin() + 10);
    int j;
    for (j = 0; j < 10; ++j)
      TS_ASSERT_EQUALS(v[j], 0);
    for (; j < 15; ++j)
      TS_ASSERT_EQUALS(v[j], 1);
    for (++j; j < 105; ++j)
      TS_ASSERT_EQUALS(v[j], 0);

    verify_consistency(v);
    verify_serialization(v);
  }

  void test_insert_by_iter()
  {
    gl_vector<int> v(100);
    int a[] = {1, 2, 3, 4, 5};
    const int N = sizeof(a)/sizeof(a[0]);
    gl_vector<int>::iterator i = v.insert(v.cbegin() + 10, (const int*)a, (const int*)(a + N));

    TS_ASSERT_EQUALS(v.size(), 100 + N);
    TS_ASSERT(i == v.begin() + 10);
    int j;
    for (j = 0; j < 10; ++j)
      TS_ASSERT_EQUALS(v[j], 0);
    for (int k = 0; k < N; ++j, ++k)
      TS_ASSERT_EQUALS(v[j], a[k]);
    for (; j < 105; ++j)
      TS_ASSERT_EQUALS(v[j], 0);
    
    verify_consistency(v);
    verify_serialization(v);
  }

  void test_insert_by_iter_move()
  {
    gl_vector<MoveOnly> v(100);
    gl_vector<MoveOnly>::iterator i = v.insert(v.cbegin() + 10, MoveOnly(3));

    TS_ASSERT(v.size() == 101);
    TS_ASSERT(i == v.begin() + 10);

    int j;
    for (j = 0; j < 10; ++j)
      TS_ASSERT(v[j] == MoveOnly());

    TS_ASSERT(v[j] == MoveOnly(3));

    for (++j; j < 101; ++j)
      TS_ASSERT(v[j] == MoveOnly());
  }

  void test_iterators_1()
  {
    typedef int T;
    typedef gl_vector<T> C;
    C c(1);
    C::iterator i = c.end();
    --i;
    TS_ASSERT(i == c.begin());
    TS_ASSERT(i == c.cbegin());

    verify_serialization(c);
    verify_consistency(c); 
  }

  void test_iterators_1_const()
  {
    typedef int T;
    typedef gl_vector<T> C;
    C c(1);
    auto i = c.cend();
    --i;
    TS_ASSERT(i == c.begin());
    TS_ASSERT(i == c.cbegin());

    verify_serialization(c);
    verify_consistency(c); 
  }

  void test_swap_1()
  {
    gl_vector<int> v1(100);
    gl_vector<int> v2(200);

    int* ptr_1 = v1.data();
    int* ptr_2 = v2.data();

    v1.swap(v2);

    TS_ASSERT_EQUALS(v1.size(), 200);
    TS_ASSERT_EQUALS(v2.size(), 100);
    TS_ASSERT_EQUALS(v1.capacity(), 200);
    TS_ASSERT_EQUALS(v2.capacity(), 100);
    TS_ASSERT_EQUALS(v1.data(), ptr_2);
    TS_ASSERT_EQUALS(v2.data(), ptr_1);

    verify_serialization(v1);
    verify_consistency(v1); 
    verify_serialization(v2);
    verify_consistency(v2); 
  }

  void test_swap_2()
  {
    int a1[] = {1, 3, 7, 9, 10};
    int a2[] = {0, 2, 4, 5, 6, 8, 11};
    gl_vector<int> c1(a1, a1+sizeof(a1)/sizeof(a1[0]));
    gl_vector<int> c2(a2, a2+sizeof(a2)/sizeof(a2[0]));

    verify_serialization(c1);
    verify_consistency(c1); 
    verify_serialization(c2);
    verify_consistency(c2); 
    
    std::swap(c1, c2);

    TS_ASSERT(c1 == gl_vector<int>(a2, a2+sizeof(a2)/sizeof(a2[0])));
    TS_ASSERT(c2 == gl_vector<int>(a1, a1+sizeof(a1)/sizeof(a1[0])));

    verify_serialization(c1);
    verify_consistency(c1); 
    verify_serialization(c2);
    verify_consistency(c2); 
  }

  void test_shrink_to_fit_1()
  {
    gl_vector<int> v(100);
    v.push_back(1);
    v.shrink_to_fit();
    TS_ASSERT_EQUALS(v.capacity(), 101);
    TS_ASSERT_EQUALS(v.size(), 101);
    
    verify_serialization(v);
    verify_consistency(v); 
  }

  void test_shrink_to_fit_2()
  {
    gl_vector<int> v(100);
    v.reserve(200);
    v.shrink_to_fit();
    TS_ASSERT_EQUALS(v.capacity(), 100);
    TS_ASSERT_EQUALS(v.size(), 100);
    
    verify_serialization(v);
    verify_consistency(v); 
  }

  void test_resize_1()
  {
    gl_vector<int> v(100);
    v.resize(50, 1);
    TS_ASSERT(v.size() == 50);
    TS_ASSERT(v.capacity() == 100);
    TS_ASSERT(v == gl_vector<int>(50));
    v.resize(200, 1);
    TS_ASSERT(v.size() == 200);
    TS_ASSERT(v.capacity() >= 200);
    for (unsigned i = 0; i < 50; ++i)
      TS_ASSERT(v[i] == 0);
    for (unsigned i = 50; i < 200; ++i)
      TS_ASSERT(v[i] == 1);
    
    verify_serialization(v);
    verify_consistency(v); 
  }

  void test_resize_2()
  {
    gl_vector<int> v(100);
    v.resize(50);
    TS_ASSERT(v.size() == 50);
    TS_ASSERT(v.capacity() == 100);

    v.resize(200);
    TS_ASSERT(v.size() == 200);
    TS_ASSERT(v.capacity() >= 200);

    verify_serialization(v);
    verify_consistency(v); 
  }

  void test_resize_2_move()
  {
    gl_vector<MoveOnly> v(100);
    v.resize(50);
    TS_ASSERT(v.size() == 50);
    TS_ASSERT(v.capacity() == 100);

    v.resize(200);
    TS_ASSERT(v.size() == 200);
    TS_ASSERT(v.capacity() >= 200);
  }

  void test_reserve_1() {
    gl_vector<int> v;
    v.reserve(10);
    TS_ASSERT(v.capacity() >= 10);

    verify_serialization(v);
    verify_consistency(v); 
  }

  void test_reserve_2() {
    gl_vector<int> v(100);
    TS_ASSERT(v.capacity() == 100);
    v.reserve(50);
    TS_ASSERT(v.size() == 100);
    TS_ASSERT(v.capacity() == 100);
    v.reserve(150);
    TS_ASSERT(v.size() == 100);
    TS_ASSERT(v.capacity() == 150);

    verify_serialization(v);
    verify_consistency(v); 
  }

  void test_reserve_2_move() {
    gl_vector<MoveOnly> v(100);
    TS_ASSERT(v.capacity() == 100);
    v.reserve(50);
    TS_ASSERT(v.size() == 100);
    TS_ASSERT(v.capacity() == 100);
    v.reserve(150);
    TS_ASSERT(v.size() == 100);
    TS_ASSERT(v.capacity() == 150);
  }

  void test_assign_1() {
    gl_vector<int> l(3, 2);
    gl_vector<int> l2(l);
    l2 = l;

    TS_ASSERT(l2 == l);

    verify_serialization(l);
    verify_consistency(l); 
    verify_serialization(l2);
    verify_consistency(l2); 
  }

  void test_assign_initializers() {
    gl_vector<int> d;
    d.assign({3, 4, 5, 6});
    TS_ASSERT(d.size() == 4);
    TS_ASSERT(d[0] == 3);
    TS_ASSERT(d[1] == 4);
    TS_ASSERT(d[2] == 5);
    TS_ASSERT(d[3] == 6);

    verify_serialization(d);
    verify_consistency(d); 
  }

  void test_assign_move() {
    gl_vector<MoveOnly> l;
    gl_vector<MoveOnly> lo;

    for (int i = 1; i <= 3; ++i)
    {
      l.push_back(i);
      lo.push_back(i);
    }

    gl_vector<MoveOnly> l2;
    l2 = std::move(l);

    TS_ASSERT(l2 == lo);
  }

  void test_pop_back_1() {
    gl_vector<int> c;
    c.push_back(1);
    TS_ASSERT(c.size() == 1);

    verify_serialization(c);
    verify_consistency(c);
    
    c.pop_back();
    TS_ASSERT(c.size() == 0);
    
    verify_serialization(c);
    verify_consistency(c); 
  }

  void test_pop_back_2() {
    gl_vector<std::shared_ptr<int> > c;

    std::shared_ptr<int> i(new int);

    TS_ASSERT(i.use_count() == 1);

    c.emplace_back(i);

    TS_ASSERT(i.use_count() == 2);
    TS_ASSERT(c.size() == 1);

    verify_consistency(c); 

    c.pop_back();
    TS_ASSERT(c.size() == 0);
    TS_ASSERT(i.use_count() == 1);

    verify_consistency(c); 
  }


};

////////////////////////////////////////////////////////////////////////////////
// Stuff for the typing

template <class T> void _test_types()
{
  typedef gl_vector<T> C;

    static_assert((std::is_same<typename C::value_type, T>::value), "");
    
    static_assert((std::is_same<
        typename std::iterator_traits<typename C::iterator>::iterator_category,
        std::random_access_iterator_tag>::value), "");

    static_assert((std::is_same<
        typename std::iterator_traits<typename C::const_iterator>::iterator_category,
        std::random_access_iterator_tag>::value), "");

    static_assert((std::is_same<
        typename C::reverse_iterator,
        std::reverse_iterator<typename C::iterator> >::value), "");

    static_assert((std::is_same<
        typename C::const_reverse_iterator,
        std::reverse_iterator<typename C::const_iterator> >::value), "");
}

////////////////////////////////////////////////////////////////////////////////

struct gl_vector_types_test   {
 public:
  void test_int() {
    _test_types<int>();
  }

  void test_int_ptr() {
    _test_types<int*>();
  }

  void test_copyable() {
    _test_types<Copyable>();
  }

  void test_nested() {
    _test_types<gl_vector<int> >();
  }

};

BOOST_FIXTURE_TEST_SUITE(_gl_vector_datatype_test, gl_vector_datatype_test)
BOOST_AUTO_TEST_CASE(test_sanity) {
  gl_vector_datatype_test::test_sanity();
}
BOOST_AUTO_TEST_CASE(test_default_size_constructor) {
  gl_vector_datatype_test::test_default_size_constructor();
}
BOOST_AUTO_TEST_CASE(test_default_size_constructor_2) {
  gl_vector_datatype_test::test_default_size_constructor_2();
}
BOOST_AUTO_TEST_CASE(test_fill_from_iterator) {
  gl_vector_datatype_test::test_fill_from_iterator();
}
BOOST_AUTO_TEST_CASE(test_back) {
  gl_vector_datatype_test::test_back();
}
BOOST_AUTO_TEST_CASE(test_front) {
  gl_vector_datatype_test::test_front();
}
BOOST_AUTO_TEST_CASE(test_emplace) {
  gl_vector_datatype_test::test_emplace();
}
BOOST_AUTO_TEST_CASE(test_emplace_back) {
  gl_vector_datatype_test::test_emplace_back();
}
BOOST_AUTO_TEST_CASE(test_erase) {
  gl_vector_datatype_test::test_erase();
}
BOOST_AUTO_TEST_CASE(test_erase_iter_iter) {
  gl_vector_datatype_test::test_erase_iter_iter();
}
BOOST_AUTO_TEST_CASE(test_insert_initializer_list) {
  gl_vector_datatype_test::test_insert_initializer_list();
}
BOOST_AUTO_TEST_CASE(test_move) {
  gl_vector_datatype_test::test_move();
}
BOOST_AUTO_TEST_CASE(test_insert_by_value_1) {
  gl_vector_datatype_test::test_insert_by_value_1();
}
BOOST_AUTO_TEST_CASE(test_insert_by_value_2) {
  gl_vector_datatype_test::test_insert_by_value_2();
}
BOOST_AUTO_TEST_CASE(test_insert_by_iter) {
  gl_vector_datatype_test::test_insert_by_iter();
}
BOOST_AUTO_TEST_CASE(test_insert_by_iter_move) {
  gl_vector_datatype_test::test_insert_by_iter_move();
}
BOOST_AUTO_TEST_CASE(test_iterators_1) {
  gl_vector_datatype_test::test_iterators_1();
}
BOOST_AUTO_TEST_CASE(test_iterators_1_const) {
  gl_vector_datatype_test::test_iterators_1_const();
}
BOOST_AUTO_TEST_CASE(test_swap_1) {
  gl_vector_datatype_test::test_swap_1();
}
BOOST_AUTO_TEST_CASE(test_swap_2) {
  gl_vector_datatype_test::test_swap_2();
}
BOOST_AUTO_TEST_CASE(test_shrink_to_fit_1) {
  gl_vector_datatype_test::test_shrink_to_fit_1();
}
BOOST_AUTO_TEST_CASE(test_shrink_to_fit_2) {
  gl_vector_datatype_test::test_shrink_to_fit_2();
}
BOOST_AUTO_TEST_CASE(test_resize_1) {
  gl_vector_datatype_test::test_resize_1();
}
BOOST_AUTO_TEST_CASE(test_resize_2) {
  gl_vector_datatype_test::test_resize_2();
}
BOOST_AUTO_TEST_CASE(test_resize_2_move) {
  gl_vector_datatype_test::test_resize_2_move();
}
BOOST_AUTO_TEST_CASE(test_reserve_1) {
  gl_vector_datatype_test::test_reserve_1();
}
BOOST_AUTO_TEST_CASE(test_reserve_2) {
  gl_vector_datatype_test::test_reserve_2();
}
BOOST_AUTO_TEST_CASE(test_reserve_2_move) {
  gl_vector_datatype_test::test_reserve_2_move();
}
BOOST_AUTO_TEST_CASE(test_assign_1) {
  gl_vector_datatype_test::test_assign_1();
}
BOOST_AUTO_TEST_CASE(test_assign_initializers) {
  gl_vector_datatype_test::test_assign_initializers();
}
BOOST_AUTO_TEST_CASE(test_assign_move) {
  gl_vector_datatype_test::test_assign_move();
}
BOOST_AUTO_TEST_CASE(test_pop_back_1) {
  gl_vector_datatype_test::test_pop_back_1();
}
BOOST_AUTO_TEST_CASE(test_pop_back_2) {
  gl_vector_datatype_test::test_pop_back_2();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_gl_vector_types_test, gl_vector_types_test)
BOOST_AUTO_TEST_CASE(test_int) {
  gl_vector_types_test::test_int();
}
BOOST_AUTO_TEST_CASE(test_int_ptr) {
  gl_vector_types_test::test_int_ptr();
}
BOOST_AUTO_TEST_CASE(test_copyable) {
  gl_vector_types_test::test_copyable();
}
BOOST_AUTO_TEST_CASE(test_nested) {
  gl_vector_types_test::test_nested();
}
BOOST_AUTO_TEST_SUITE_END()
