#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <algorithm>
#include <core/util/cityhash_tc.hpp>
#include <core/util/int128_types.hpp>
#include <core/data/flexible_type/flexible_type.hpp>

using namespace turi;

static const size_t test_chain_length = 50000;
static const size_t K = 10;

template <typename H, typename V>
class hash_tracker {
 public:
  void check_and_add(H h, V v) {

    auto it = seen_hashes.find(h);

    if(it != seen_hashes.end()) {

      V old_value = it->second;

      if(old_value != v) {
        std::ostringstream ss;

        ss << "Hash of '" << old_value << "' and '" << v << "' map collide.";

        TS_FAIL(ss.str());
      }
    }

    seen_hashes[h] = v;
  }

  std::map<H, V> seen_hashes;
};

template <typename HT>
void stress_test_flex_type(std::function<HT(flexible_type)> hf) {

  hash_tracker<HT, flexible_type> htest;

  // Do several things to make sure we don't have collisions
  for(long i1 = 0; i1 < static_cast<int64_t>(K); ++i1) {
    for(long i2 = 0; i2 < static_cast<int64_t>(K); ++i2) {
      for(long i3 = 0; i3 < static_cast<int64_t>(K); ++i3) {
        {
          flexible_type k = std::vector<flexible_type>{flexible_type(i1),
                                                       flexible_type(i2),
                                                       flexible_type(i3)};

          htest.check_and_add(hf(k), k);
        }

        {
          flexible_type k = std::vector<flexible_type>{flexible_type(std::to_string(i1)),
                                                       flexible_type(std::to_string(i2)),
                                                       flexible_type(std::to_string(i3))};

          htest.check_and_add(hf(k), k);
        }

        {
          flexible_type k = i1*K*K + i2*K + i3;

          htest.check_and_add(hf(k), k);
        }

        {
          flexible_type k = std::to_string(i1*K*K + i2*K + i3);

          htest.check_and_add(hf(k), k);
        }

        {
          flexible_type k = std::vector<flexible_type>{flexible_type(i1*K*K + i2*K + i3)};

          htest.check_and_add(hf(k), k);
        }

        {
          flexible_type k = std::vector<flexible_type>{
            flexible_type(i1),
            flexible_type(std::vector<flexible_type>{i2, i3})};

          htest.check_and_add(hf(k), k);
        }

        {
          flexible_type k = std::vector<flexible_type>{
            flexible_type(std::to_string(i1)),
            flexible_type(std::vector<flexible_type>{std::to_string(i2), std::to_string(i3)})};

          htest.check_and_add(hf(k), k);
        }
      }
    }
  }
}

struct flexible_type_hash_test  {
 public:

  flexible_type_hash_test()
      : values(2*test_chain_length)
  {
    std::default_random_engine generator(0);

    for(size_t i = 0; i < test_chain_length; ++i) {
      values[i] = long(i);
    }

    for(size_t i = 0; i < test_chain_length; ++i) {
      int bit = std::uniform_int_distribution<int>(0,8*sizeof(long))(generator);

      values[test_chain_length + i] = values[test_chain_length + i - 1] ^ long(1UL << bit);
    }
  }

  std::vector<long> values;

  // Just make sure they equal the original string ones
  void test_ft_string_hashes_128() {
    for(long v : values) {
      std::string s = std::to_string(v);

      uint128_t h1 = flexible_type(s).hash128();
      uint128_t h2 = hash128(s);

      // std::cout << "h1 = " << h1 << std::endl;
      // std::cout << "h2 = " << h2 << std::endl;

      BOOST_CHECK(h1 == h2);
    }
  }

  void test_ft_string_hashes_64() {
    for(long v : values) {
      std::string s = std::to_string(v);

      uint64_t h1 = flexible_type(s).hash();
      uint64_t h2 = hash64(s);

      TS_ASSERT_EQUALS(h1, h2);
    }
  }

  void test_ft_integer_hashes_128() {
    for(long v : values)
      BOOST_CHECK(hash128(v) == flexible_type(v).hash128());
  }

  ////////////////////////////////////////////////////////////////////////////////
  //
  //    THIS TEST IS KNOWN TO FAIL -- SEE ISSUE 475!!!
  //
  ////////////////////////////////////////////////////////////////////////////////

  // void test_ft_integer_hashes_64() {
  //   for(long v : values) {
  //     TS_ASSERT_EQUALS(hash64(v), flexible_type(v).hash());
  //   }
  // }

  void test_ft_vector_hashes_64() {
    stress_test_flex_type<uint64_t>([](flexible_type f) {return f.hash(); });
  }

  void test_ft_vector_hashes_128() {
    stress_test_flex_type<uint128_t>([](flexible_type f) {return f.hash128(); });
  }

  void test_nd_vec_hashability() {
    flex_nd_vec vec({0,5,1,6,2,7,3,8,4,9},
                    {2,5},
                    {1,2});
    flexible_type(vec).hash();
  }
};

BOOST_FIXTURE_TEST_SUITE(_flexible_type_hash_test, flexible_type_hash_test)
BOOST_AUTO_TEST_CASE(test_ft_string_hashes_128) {
  flexible_type_hash_test::test_ft_string_hashes_128();
}
BOOST_AUTO_TEST_CASE(test_ft_string_hashes_64) {
  flexible_type_hash_test::test_ft_string_hashes_64();
}
BOOST_AUTO_TEST_CASE(test_ft_integer_hashes_128) {
  flexible_type_hash_test::test_ft_integer_hashes_128();
}
BOOST_AUTO_TEST_CASE(test_ft_vector_hashes_64) {
  flexible_type_hash_test::test_ft_vector_hashes_64();
}
BOOST_AUTO_TEST_CASE(test_ft_vector_hashes_128) {
  flexible_type_hash_test::test_ft_vector_hashes_128();
}
BOOST_AUTO_TEST_SUITE_END()
