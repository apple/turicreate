/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_HASH_VALUE_H_
#define TURI_HASH_VALUE_H_

#include <core/util/cityhash_tc.hpp>
#include <core/util/bitops.hpp>
#include <core/storage/serialization/serialization_includes.hpp>

namespace turi {
/**
 * Defines a Weak version of Token class with a fixed, constant hash
 * value.  Hashes are calculated using the cityhash_gl hash
 * functions.  It is designed as a robust key for hash tables.  The
 * difference between this class and the regular Token class is that
 * this one only stores the hash value, making it better suited for
 * querying over network connections.
 *
 */
class hash_value : public turi::IS_POD_TYPE {
 public:
  /**
   * Creates an empty WeakToken object with the 0 hash.
   */
  hash_value()
      : h_128(0)
  {}

  /// Redirect to the appropriate hash function.
  template <typename... Args>
  inline hash_value(Args... args)
      : h_128(hash128(args...))
  {}

  /// Copy constructor
  hash_value(const hash_value& t) : h_128(t.h_128) {}
  hash_value(hash_value&& t) : h_128(t.h_128) {}
  hash_value& operator=(const hash_value& t) { h_128 = t.h_128; return *this; }

  /// Explicit constructor from a hash value output
  hash_value(uint128_t h) : h_128(h) {}


  inline bool operator==(const hash_value& t) const { return t.hash() == hash(); }

  inline bool operator<(const hash_value& t) const  { return t.hash() < hash(); }

  /// Returns the 128 bit hash value of the token.
  uint128_t hash() const { return h_128;  }

  /// Returns the top number of bits in the hash
  inline size_t n_bit_index(size_t n_bits) const {
    // Need to mix the bits a bit, since the first hash may be just an
    // integer.  multiply by a couple of random primes and xor; good
    // enough for this.
    uint64_t v = (h_2_64.first * 0x7e952a7b972f486fULL) ^ (h_2_64.second * 0xdeb2a42e44aa4c17ULL);
    return v >> (bitsizeof(uint64_t) - n_bits);
  }

  /// Serialization
  void save(oarchive &oarc) const { oarc << h_128; }
  void load(iarchive &iarc) { iarc >> h_128; }

 private:
  union {
    uint128_t h_128;
    std::pair<uint64_t, uint64_t> h_2_64;
  };
};

}

namespace std {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif

template <> struct hash<turi::hash_value> {
  size_t operator()(const turi::hash_value& t) const {
    return size_t(t.hash());
  }
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif

}

#endif
