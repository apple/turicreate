/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/logging/assertions.hpp>
#include <core/data/image/image_util_impl.hpp>

// contains some of the bigger functions I do not want to inline
namespace turi {
constexpr int64_t flex_date_time::MICROSECONDS_PER_SECOND;
constexpr double flex_date_time::MICROSECOND_EPSILON;
constexpr int32_t flex_date_time::TIMEZONE_LOW;
constexpr int32_t flex_date_time::TIMEZONE_HIGH;
constexpr int32_t flex_date_time::EMPTY_TIMEZONE;
constexpr int32_t flex_date_time::TIMEZONE_RESOLUTION_IN_SECONDS;
constexpr int32_t flex_date_time::TIMEZONE_RESOLUTION_IN_MINUTES;
constexpr double flex_date_time::TIMEZONE_RESOLUTION_IN_HOURS;
constexpr int32_t flex_date_time::_LEGACY_TIMEZONE_SHIFT;

namespace flexible_type_impl {


boost::posix_time::ptime ptime_from_time_t(std::time_t offset, int32_t microseconds){
  static const boost::posix_time::ptime time_t_epoch = boost::posix_time::from_time_t(0);
  static const int32_t max_int32 = std::numeric_limits<int32_t>::max();
  static const int32_t min_int32 = (std::numeric_limits<int32_t>::min() + 1);
  boost::posix_time::ptime accum = time_t_epoch;
  // this is really ugly but boost posix time has some annoying 32 bit issues
  if(offset < 0 ){
    while (offset < min_int32)
    {
      accum  += boost::posix_time::seconds(min_int32);
      offset -= min_int32;
    }
    accum += boost::posix_time::seconds(offset);
  } else {

    while (offset > max_int32)
    {
      accum  += boost::posix_time::seconds(max_int32);
      offset -= max_int32;
    }
    accum += boost::posix_time::seconds(offset);
  }

  accum += boost::posix_time::microseconds(microseconds);
  return accum;
}

flex_int ptime_to_time_t(const boost::posix_time::ptime & time) {
  boost::posix_time::time_duration dur = time - boost::posix_time::from_time_t(0);
  return static_cast<flex_int>(dur.ticks() / dur.ticks_per_second());
}

flex_int ptime_to_fractional_microseconds(const boost::posix_time::ptime & time) {
  // get the time difference between the time and the integer rounded time_t
  // time
  boost::posix_time::time_duration dur = time -
      boost::posix_time::from_time_t(ptime_to_time_t(time));
  double fractional_seconds = double(dur.ticks()) / dur.ticks_per_second();
  return fractional_seconds * flex_date_time::MICROSECONDS_PER_SECOND;
}


std::string date_time_to_string(const flex_date_time& i) {
 return boost::posix_time::to_iso_string(
     ptime_from_time_t(i.shifted_posix_timestamp(),
                       i.microsecond()));
}

date_time_string_reader::date_time_string_reader(std::string format)
    : format_(std::move(format)) {
  if (format_.empty()) {
    format_ = "%Y%m%dT%H%M%S%F%q";
  }

  auto* facet = new boost::local_time::local_time_input_facet(format_);
  stream_.imbue(std::locale(stream_.getloc(), facet));
  stream_.exceptions(std::ios_base::failbit);
}

flex_date_time date_time_string_reader::read(const flex_string& input) {
  try {
    boost::local_time::local_date_time ldt(boost::posix_time::not_a_date_time);
    stream_.str(input);
    stream_ >> ldt; // do the parse

    boost::posix_time::ptime p = ldt.utc_time();
    std::time_t time = ptime_to_time_t(p);
    int32_t microseconds = ptime_to_fractional_microseconds(p);
    int32_t timezone_offset = flex_date_time::EMPTY_TIMEZONE;
    if (ldt.zone()) {
      timezone_offset =
        static_cast<int32_t>(ldt.zone()->base_utc_offset().total_seconds()) /
        flex_date_time::TIMEZONE_RESOLUTION_IN_SECONDS;
    }
    return flex_date_time(time, timezone_offset, microseconds);
  } catch (std::exception& ex) {
    log_and_throw("Unable to interpret " + input + " as string with " +
                  format_ + " format");
  }
}

flex_date_time get_datetime_visitor::operator()(const flex_string& s) const {
  date_time_string_reader reader("ISO");
  return reader.read(s);
}

flex_string get_string_visitor::operator()(const flex_vec& vec) const {
  std::stringstream strm; strm << "[";
  for (size_t i = 0; i < vec.size(); ++i) {
    strm << vec[i];
    if (i + 1 < vec.size()) strm << " ";
  }
  strm << "]";
  return strm.str();
}

flex_string get_string_visitor::operator()(const flex_nd_vec& vec) const {
  std::stringstream strm;
  strm << vec;
  return strm.str();
}

flex_string get_string_visitor::operator()(const flex_date_time& i) const {
  return date_time_to_string(i);
}
flex_string get_string_visitor::operator()(const flex_list& vec) const {
  std::stringstream strm; strm << "[";
  for (size_t i = 0; i < vec.size(); ++i) {
    if (vec[i].get_type() == flex_type_enum::STRING) {
      strm << "\"" << (flex_string)(vec[i]) << "\"";
    } else {
      strm << (flex_string)(vec[i]);
    }
    if (i + 1 < vec.size()) strm << ",";
  }
  strm << "]";
  return strm.str();
}

flex_string get_string_visitor::operator()(const flex_dict& vec) const {
  std::stringstream strm; strm << "{";
  size_t vecsize = vec.size();
  size_t i = 0;
  for(const auto& val: vec) {
    if (val.first.get_type() == flex_type_enum::STRING) {
      strm << "\"" << (flex_string)(val.first) << "\"";
    } else {
      strm << (flex_string)(val.first);
    }
    strm << ":";
    if (val.second.get_type() == flex_type_enum::STRING) {
      strm << "\"" << (flex_string)(val.second) << "\"";
    } else {
      strm << (flex_string)(val.second);
    }
    if (i + 1 < vecsize) strm << ", ";
    ++i;
  }
  strm << "}";
  return strm.str();
}

flex_string get_string_visitor::operator() (const flex_image& img) const {
  std::stringstream strm;
  strm << "Height: " << img.m_height;
  strm << " Width: " << img.m_width;

  return strm.str();
}

flex_vec get_vec_visitor::operator() (const flex_image& img) const {
  flex_vec vec;
  if(img.m_format == Format::RAW_ARRAY) {
    for (size_t i = 0 ; i < img.m_image_data_size; ++i){
      vec.push_back(static_cast<double>(static_cast<unsigned char>(img.m_image_data[i])));
    }
  } else {
    // pay the price to decode
    flex_image newimg = img;
    decode_image_inplace(newimg);
    ASSERT_TRUE(newimg.m_format == Format::RAW_ARRAY);
    for (size_t i = 0 ; i < newimg.m_image_data_size; ++i){
      vec.push_back(static_cast<double>(static_cast<unsigned char>(newimg.m_image_data[i])));
    }
  }
  return vec;
}

/**
 * Flatten a potentially recursive flexible_type to nd_vec.
 *
 * Recursively breaks down the flexible_type flattening it into the ret array
 * with the canonical ordering.
 *
 * Returns true on success, false on any shape error.
 *
 * \param f The flexible_type (and a few overloads of it) to flatten
 * \param shape The target output shape.
 * \param shape_index The current shape index of the recursive decomposition.
 */
static bool flexible_type_flatten_to_nd_vec(const flexible_type& f,
                                            const std::vector<size_t>& shape,
                                            size_t shape_index,
                                            std::vector<double>& ret);
static bool flexible_type_flatten_to_nd_vec(const flex_vec& f,
                                            const std::vector<size_t>& shape,
                                            size_t shape_index,
                                            std::vector<double>& ret);
static bool flexible_type_flatten_to_nd_vec(const flex_list& f,
                                            const std::vector<size_t>& shape,
                                            size_t shape_index,
                                            std::vector<double>& ret);
static bool flexible_type_flatten_to_nd_vec(const flex_nd_vec& f,
                                            const std::vector<size_t>& shape,
                                            size_t shape_index,
                                            std::vector<double>& ret);

static bool flexible_type_flatten_to_nd_vec(const flexible_type& f,
                                            const std::vector<size_t>& shape,
                                            size_t shape_index,
                                            std::vector<double>& ret) {
  if (f.get_type() == flex_type_enum::VECTOR) {
    return flexible_type_flatten_to_nd_vec(f.get<flex_vec>(), shape, shape_index, ret);
  } else if (f.get_type() == flex_type_enum::ND_VECTOR) {
    return flexible_type_flatten_to_nd_vec(f.get<flex_nd_vec>(), shape, shape_index, ret);
  } else if (f.get_type() == flex_type_enum::LIST) {
    return flexible_type_flatten_to_nd_vec(f.get<flex_list>(), shape, shape_index, ret);
  } else {
    return false;
  }
}

static bool flexible_type_flatten_to_nd_vec(const flex_vec& f,
                                            const std::vector<size_t>& shape,
                                            size_t shape_index,
                                            std::vector<double>& ret) {
  // check shape.
  if (shape_index == shape.size() - 1 && f.size() == shape[shape_index]) {
    // shape is good
    std::copy(f.begin(), f.end(), std::inserter(ret, ret.end()));
    return true;
  }
  return false;
}

static bool flexible_type_flatten_to_nd_vec(const flex_list& f,
                                            const std::vector<size_t>& shape,
                                            size_t shape_index,
                                            std::vector<double>& ret) {
  // check shape
  if (shape_index < shape.size() && f.size() == shape[shape_index]) {
    // shape is good
    if (shape_index == shape.size() - 1) {
      // fast path for last entry is numeric
      for (size_t i = 0;i < f.size(); ++i) {
        if (flex_type_is_convertible(f[i].get_type(), flex_type_enum::FLOAT)) {
          ret.push_back(f[i].to<double>());
        } else {
          return false;
        }
      }
    } else {
      for (size_t i = 0;i < f.size(); ++i) {
        if (!flexible_type_flatten_to_nd_vec(f[i], shape, shape_index+1, ret)) {
          return false;
        }
      }
    }
    return true;
  }
  return false;
}


static bool flexible_type_flatten_to_nd_vec(const flex_nd_vec& f,
                                            const std::vector<size_t>& shape,
                                            size_t shape_index,
                                            std::vector<double>& ret) {
  // check shape
  if (shape.size() - shape_index == f.shape().size()) {
    bool shape_good = true;
    for (size_t i = shape_index; i < shape.size(); ++i) {
      shape_good &= (shape[i] == f.shape()[i-shape_index]);
    }
    if (shape_good == false) return false;
    // shape is good

    if (f.num_elem() == 0) return true;
    std::vector<size_t> idx(f.shape().size(), 0);
    do {
      ret.push_back(f[f.fast_index(idx)]);
    } while(f.increment_index(idx));
    return true;
  }
  return false;
}


flex_nd_vec get_ndvec_visitor::operator()(flex_list u) const {
  // find dimensionality.
  std::vector<size_t> shape;

  // we try to derive from u[0]. But it could be recursive hierarchies of stuff.
  const flex_list* l = &u;
  while(1) {
    shape.push_back(l->size());
    if (l->size() == 0) {
      break;
    }
    const flexible_type& f = (*l)[0];
    if (f.get_type() == flex_type_enum::VECTOR) {
      // list contains a vector.
      // this is the last shape dimension.
      shape.push_back(f.size());
      break;
    } else if (f.get_type() == flex_type_enum::ND_VECTOR) {
      // list contains a nd vector.
      // this is the last shape dimension.
      const flex_nd_vec& v = f.get<flex_nd_vec>();
      for (size_t j = 0; j < v.shape().size(); ++j) shape.push_back(v.shape()[j]);
      break;
    } else if (f.get_type() == flex_type_enum::LIST) {
      // we have another list to break down
      l  = &(f.get<flex_list>());
      continue;
    } else if (flex_type_is_convertible(f.get_type(), flex_type_enum::FLOAT)) {
      // list contains a scalar, we are now at the deepest recursive level
      break;
    } else {
      log_and_throw("list contains non-numeric type. Cannot convert to ndarray");
    }
  }

  // we have a shape
  // empty shape
  if (shape.size() == 0) {
    return flex_nd_vec();
  }
  // 0 element shape
  size_t numel = 1;
  for (size_t i = 0; i < shape.size(); ++i) numel *= shape[i];
  if (numel == 0) {
    return flex_nd_vec(flex_nd_vec::container_type(), shape);
  }

  // n element shape
  auto elems = std::make_shared<flex_nd_vec::container_type>() ;
  elems->reserve(numel);
  if (flexible_type_flatten_to_nd_vec(u, shape, 0, *elems) == false) {
    log_and_throw("list shape invalid");
  }
  return flex_nd_vec(elems, shape);
}

flex_nd_vec get_ndvec_visitor::operator()(const flex_image& img) const {
  flex_vec flattened = get_vec_visitor()(img);
  auto elem = std::make_shared<flex_nd_vec::container_type>();
  (*elem) = std::move(flattened);
  if (img.m_channels == 1) {
    return flex_nd_vec(elem, {img.m_height, img.m_width});
  } else {
    return flex_nd_vec(elem, {img.m_height, img.m_width, img.m_channels});
  }
}

flex_image get_img_visitor::operator()(const flex_nd_vec& v) const {
  ASSERT_MSG(v.shape().size() == 2 || v.shape().size() == 3, "Cannot convert nd array to image");
  size_t channels = 1, height = 0, width = 0;
  if (v.shape().size() == 2) {
    height = v.shape()[0];
    width = v.shape()[1];
  } else if (v.shape().size() == 3) {
    height = v.shape()[0];
    width = v.shape()[1];
    channels = v.shape()[2];
  }
  ASSERT_MSG(channels == 1 || channels == 3 || channels == 4, "Channels must be 1, 3 or 4");

  size_t npixels = channels * height * width;
  if (npixels == 0) {
    return flex_image(nullptr, height, width, channels, 0,
                      IMAGE_TYPE_CURRENT_VERSION, int(Format::RAW_ARRAY));
  }
  std::vector<unsigned char> pixels(npixels, 0);

  // loop through v converting it to pixels
  std::vector<size_t> idx(v.shape().size(), 0);
  size_t ctr = 0;
  do {
    pixels[ctr] = v[v.fast_index(idx)];
    ++ctr;
  } while(v.increment_index(idx));
  return flex_image((const char*)(pixels.data()), height, width, channels, pixels.size(),
                    IMAGE_TYPE_CURRENT_VERSION, int(Format::RAW_ARRAY));
}

void soft_assignment_visitor::operator()(flex_vec& t, const flex_list& u) const {
  t.resize(u.size());
  flexible_type ft = flex_float();

  for (size_t i = 0; i < u.size(); ++i) {
    ft.soft_assign(u[i]);
    t[i] = ft.get<flex_float>();
  }
}


bool approx_equality_operator::operator()(const flex_dict& t, const flex_dict& u) const {
    if (t.size() != u.size()) return false;

    std::unordered_multimap<flexible_type, flexible_type> d1(t.begin(), t.end());
    std::unordered_multimap<flexible_type, flexible_type> d2(u.begin(), u.end());

    return d1 == d2;
}

bool approx_equality_operator::operator()(const flex_list& t, const flex_list& u) const {
  if (t.size() != u.size()) return false;
  for (size_t i = 0; i < t.size(); ++i) if (t[i] != u[i]) return false;
  return true;
}

size_t city_hash_visitor::operator()(const flex_list& t) const {
  size_t h = 0;
  for (size_t i = 0; i < t.size(); ++i) {
    h = hash64_combine(h, t[i].hash());
  }
  // The final hash is needed to distinguish nested types
  return turi::hash64(h);
}

size_t city_hash_visitor::operator()(const flex_dict& t) const {
  size_t key_hash = 0;
  size_t value_hash = 0;
  for(const auto& val: t) {
    // Use xor to ignore the order of the pairs.
    key_hash |= val.first.hash();
    value_hash |= val.first.hash();
  }
  return turi::hash64_combine(key_hash, value_hash);
}

uint128_t city_hash128_visitor::operator()(const flex_list& t) const {
  uint128_t h = 0;
  for (size_t i = 0; i < t.size(); ++i) {
    h = hash128_combine(h, t[i].hash128());
  }
  // The final hash is needed to distinguish nested types
  return turi::hash128(h);
}

uint128_t city_hash128_visitor::operator()(const flex_dict& t) const {
  uint128_t key_hash = 0;
  uint128_t value_hash = 0;
  for(const auto& val: t) {
    // Use xor to ignore the order of the pairs.
    key_hash |= val.first.hash128();
    value_hash |= val.first.hash128();
  }
  return turi::hash128_combine(key_hash, value_hash);
}
} // namespace flexible_type_impl


void flexible_type::erase(const flexible_type& index) {
  ensure_unique();
  switch(get_type()) {
   case flex_type_enum::DICT: {
     flex_dict& value = val.dictval->second;
     for(auto iter = value.begin(); iter != value.end(); iter++) {
       if ((*iter).first == index) {
         value.erase(iter);
         break;
       }
     }
    return;
   }
   default:
     FLEX_TYPE_ASSERT(false);
  }
}

bool flexible_type::is_zero() const {
  switch(get_type()) {
    case flex_type_enum::INTEGER:
      return get<flex_int>() == 0;
    case flex_type_enum::FLOAT:
      return get<flex_float>() == 0.0;
    case flex_type_enum::STRING:
      return get<flex_string>().empty();
    case flex_type_enum::VECTOR:
      return get<flex_vec>().empty();
    case flex_type_enum::LIST:
      return get<flex_list>().empty();
    case flex_type_enum::DICT:
      return get<flex_dict>().empty();
    case flex_type_enum::IMAGE:
      return get<flex_image>().m_format == Format::UNDEFINED;
    case flex_type_enum::UNDEFINED:
      return true;
    default:
      log_and_throw("Unexpected type!");
  };
  __builtin_unreachable();
}

bool flexible_type::is_na() const {
  auto the_type = get_type();
  return (the_type == flex_type_enum::UNDEFINED) ||
          (the_type == flex_type_enum::FLOAT && std::isnan(get<flex_float>()));
}


void flexible_type_fail(bool success) {
  if(!success) {
    log_and_throw("Invalid type conversion");
  }
}
} // namespace turi
