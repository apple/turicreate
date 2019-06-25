#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/logging/logger.hpp>
#include <core/storage/sframe_data/integer_pack.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
using namespace turi;
using namespace integer_pack;
struct integer_pack_test {
 public:
  void test_variable_code() {
    for (size_t shift = 0; shift < 64; shift += 8) {
      for (uint64_t i = 0; i < 256; ++i) {
        oarchive oarc;
        variable_encode(oarc, i << shift);
        uint64_t j;
        iarchive iarc(oarc.buf, oarc.off);
        variable_decode(iarc, j);
        TS_ASSERT_EQUALS(oarc.off, iarc.off);
        free(oarc.buf);
        TS_ASSERT_EQUALS(i << shift, j);
      }
    }
  }
  void test_pack() {
    {
      size_t len = 8;
      uint64_t in[8] = {19,20,21,22,23,24,25,26};
      uint64_t out[8];
      oarchive oarc;
      frame_of_reference_encode_128(in, 8, oarc);

      iarchive iarc(oarc.buf, oarc.off);
      frame_of_reference_decode_128(iarc, 8, out);
      TS_ASSERT_EQUALS(oarc.off, iarc.off);
      free(oarc.buf);

      for (size_t i = 0;i < len; ++i) {
        TS_ASSERT_EQUALS(in[i], out[i]);
      }
    }
    // simple cases
    for (size_t mod = 1; mod < 63; ++mod) {
      for (size_t len = 0; len <= 128; ++len) {
        uint64_t in[len];
        uint64_t out[len];
        for (size_t i = 0;i < len; ++i) {
          if (mod == 0) {
            in[i] = 0;
          } else {
            in[i] = (i % mod) & (1 << (mod - 1));
          }
        }
        oarchive oarc;
        frame_of_reference_encode_128(in, len, oarc);

        iarchive iarc(oarc.buf, oarc.off);
        frame_of_reference_decode_128(iarc, len, out);
        TS_ASSERT_EQUALS(oarc.off, iarc.off);
        free(oarc.buf);

        for (size_t i = 0;i < len; ++i) {
          if (in[i] != out[i]) std::cout << mod << " " << len << " " << i << "\n";
          TS_ASSERT_EQUALS(in[i], out[i]);
        }
      }
    }
    
    // harder cases
    for (size_t multiplier = 1; multiplier < 63; ++multiplier) {
      for (size_t shift = 1; shift < 63; ++shift) {
        size_t len = 128;
        uint64_t in[len];
        uint64_t out[len];
        for (size_t i = 0;i < len; ++i) {
          in[i] = shift + (multiplier * i);
        }
        oarchive oarc;
        frame_of_reference_encode_128(in, len, oarc);

        iarchive iarc(oarc.buf, oarc.off);
        frame_of_reference_decode_128(iarc, len, out);
        TS_ASSERT_EQUALS(oarc.off, iarc.off);
        free(oarc.buf);

        for (size_t i = 0;i < len; ++i) {
          TS_ASSERT_EQUALS(in[i], out[i]);
        }
      }
      for (size_t mod = 1; mod < 63; ++mod) {
        size_t len = 128;
        uint64_t in[len];
        uint64_t out[len];
        for (size_t i = 0;i < len; ++i) {
          in[i] = (multiplier * i) % mod;
        }
        oarchive oarc;
        frame_of_reference_encode_128(in, len, oarc);

        iarchive iarc(oarc.buf, oarc.off);
        frame_of_reference_decode_128(iarc, len, out);
        TS_ASSERT_EQUALS(oarc.off, iarc.off);
        free(oarc.buf);

        for (size_t i = 0;i < len; ++i) {
          TS_ASSERT_EQUALS(in[i], out[i]);
        }
      }
    }
    
    // integer boundary cases
    int64_t maxint = std::numeric_limits<int64_t>::max() >> 4;

    //
    // FIXME: The following loop is a no-op; did the author mean for multiplier
    // to range from (maxint >> 4) to maxint by powers of two?
    //
    for (size_t multiplier = static_cast<size_t>(maxint);
         multiplier < static_cast<size_t>(maxint);
         ++multiplier) {
      size_t len = 128;
      uint64_t in[len];
      uint64_t out[len];
      for (size_t i = 0;i < len; ++i) {
        in[i] = (multiplier * i);
      }
      oarchive oarc;
      frame_of_reference_encode_128(in, len, oarc);

      iarchive iarc(oarc.buf, oarc.off);
      frame_of_reference_decode_128(iarc, len, out);
      TS_ASSERT_EQUALS(oarc.off, iarc.off);
      free(oarc.buf);

      for (size_t i = 0;i < len; ++i) {
        TS_ASSERT_EQUALS(in[i], out[i]);
      }
    }
  }
  void test_shift_encode() {
    int64_t maxint = std::numeric_limits<int64_t>::max();
    int64_t minint = std::numeric_limits<int64_t>::min();
    for (int64_t i = maxint - 256; i < maxint; ++i) {
      uint64_t j = shifted_integer_encode(i);
      int64_t i2 = shifted_integer_decode(j);
      TS_ASSERT_EQUALS(i, i2);
    }
    for (int64_t i = minint; i < minint + 256; ++i) {
      uint64_t j = shifted_integer_encode(i);
      int64_t i2 = shifted_integer_decode(j);
      TS_ASSERT_EQUALS(i, i2);
    }
    for (int64_t i = -256; i < 256; ++i) {
      uint64_t j = shifted_integer_encode(i);
      int64_t i2 = shifted_integer_decode(j);
      TS_ASSERT_EQUALS(i, i2);
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_integer_pack_test, integer_pack_test)
BOOST_AUTO_TEST_CASE(test_variable_code) {
  integer_pack_test::test_variable_code();
}
BOOST_AUTO_TEST_CASE(test_pack) {
  integer_pack_test::test_pack();
}
BOOST_AUTO_TEST_CASE(test_shift_encode) {
  integer_pack_test::test_shift_encode();
}
BOOST_AUTO_TEST_SUITE_END()
