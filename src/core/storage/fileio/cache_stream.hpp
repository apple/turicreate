/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FILEIO_CACHE_STREAM_HPP
#define TURI_FILEIO_CACHE_STREAM_HPP

#include<core/storage/fileio/cache_stream_source.hpp>
#include<core/storage/fileio/cache_stream_sink.hpp>

namespace turi {
namespace fileio{
/**
 * \internal
 * \ingroup fileio
 * icache_stream provides an input stream to a cache object;
 * this should not be used directly.
 * \see general_ifstream
 */
typedef boost::iostreams::stream<turi::fileio_impl::cache_stream_source>
  icache_stream;

/**
 * \internal
 * \ingroup fileio
 * ocache_stream provides an output stream to a cache object;
 * this should not be used directly.
 * \see general_ofstream
 */
typedef boost::iostreams::stream<turi::fileio_impl::cache_stream_sink>
  ocache_stream;

} // end of fileio
} // end of turicreate

#endif
