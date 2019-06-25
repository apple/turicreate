/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <iostream>
#include <core/logging/logger.hpp>
#include <boost/algorithm/string.hpp>
#include <core/storage/fileio/sanitize_url.hpp>
#include <core/storage/fileio/general_fstream.hpp>

namespace turi {

/*****************************************************************************/
/*                                                                           */
/*                     general_ifstream implementation                       */
/*                                                                           */
/*****************************************************************************/

general_ifstream::general_ifstream(std::string filename)
    // this is the function try block syntax which, together with the member
    // function pointer syntax, is probably the ugliest C++ syntactic element
    // every conceieved.
    try :general_ifstream_base(filename), opened_filename(filename)
    {
    }  catch (const std::exception& e) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename) + " for read. " + e.what());
    } catch (std::string e) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename) + " for read. " + e);
    } catch (...) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename));
    }


general_ifstream::general_ifstream(std::string filename, bool gzip_compressed)
    try :general_ifstream_base(filename, gzip_compressed),
     opened_filename(filename) {
     } catch (const std::exception& e) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename) + " for read. " + e.what());
    } catch (std::string e) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename) + " for read. " + e);
    } catch (...) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename));
    }

size_t general_ifstream::file_size() {
  return this->general_ifstream_base::operator->()->file_size();
}

size_t general_ifstream::get_bytes_read() {
  return this->general_ifstream_base::operator->()->get_bytes_read();
}

std::string general_ifstream::filename() const {
  return opened_filename;
}

std::shared_ptr<std::istream> general_ifstream::get_underlying_stream() {
  return this->general_ifstream_base::operator->()->get_underlying_stream();
}
/*****************************************************************************/
/*                                                                           */
/*                     general_ofstream implementation                       */
/*                                                                           */
/*****************************************************************************/

general_ofstream::general_ofstream(std::string filename)
    try :general_ofstream_base(filename), opened_filename(filename) {
      // this space intentionally left blank
    } catch (const std::ios_base::failure& e) {
      // an I/O error is already the exception type - just re-throw
      throw;
    } catch (const std::exception& e) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename) + " for write. " + e.what());
    } catch (std::string e) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename) + " for write. " + e);
    } catch (...) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename));
    }


general_ofstream::general_ofstream(std::string filename, bool gzip_compress)
    try :general_ofstream_base(filename, gzip_compress),
     opened_filename(filename) { }  catch (std::exception e) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename) + " for write. " + e.what());
    } catch (std::string e) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename) + " for write. " + e);
    } catch (...) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename));
    }


bool general_ofstream::good() const {
  // annoyingly the boost stream object does not have a const operator-> !
  const general_ofstream_base* const_base = this;
  general_ofstream_base* base = const_cast<general_ofstream_base*>(const_base);
  return base->good();
}

bool general_ofstream::bad() const {
  // annoyingly the boost stream object does not have a const operator-> !
  const general_ofstream_base* const_base = this;
  general_ofstream_base* base = const_cast<general_ofstream_base*>(const_base);
  return base->bad();
}

bool general_ofstream::fail() const {
  // annoyingly the boost stream object does not have a const operator-> !
  const general_ofstream_base* const_base = this;
  general_ofstream_base* base = const_cast<general_ofstream_base*>(const_base);
  return base->fail();
}

size_t general_ofstream::get_bytes_written() const {
  const general_ofstream_base* const_base = this;
  general_ofstream_base* base = const_cast<general_ofstream_base*>(const_base);
  return (*base)->get_bytes_written();
}

std::string general_ofstream::filename() const {
  return opened_filename;
}

} // namespace turi
