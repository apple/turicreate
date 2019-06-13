/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_IPC_DESERIALIZER_MINIMAL_HPP
#define CPPIPC_IPC_DESERIALIZER_MINIMAL_HPP
#include <type_traits>
#include <core/storage/serialization/iarchive.hpp>
#include <core/system/cppipc/ipc_object_base.hpp>

namespace turi {
namespace archive_detail {

template <typename OutArcType, typename T>
struct serialize_impl<OutArcType, std::shared_ptr<T>, false,
    typename std::enable_if<std::is_convertible<T*, cppipc::ipc_object_base*>::value>::type
    > {
  inline static
      void
      exec(OutArcType& oarc, const std::shared_ptr<T> value) {
        oarc << (*value);
  }
};


template <typename InArcType, typename T>
struct deserialize_impl<InArcType, std::shared_ptr<T>, false,
    typename std::enable_if<std::is_convertible<T*, cppipc::ipc_object_base*>::value>::type
    > {
  inline static
      void exec(InArcType& iarc, std::shared_ptr<T>& value) {
        iarc >> (*value);
  }
};
} // archive_detail
} // turicreate

#endif
