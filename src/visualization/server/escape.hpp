
#ifndef __TC_ESCAPE
#define __TC_ESCAPE

#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <flexible_type/flexible_type.hpp>
#include <flexible_type/ndarray.hpp>
#include <unity/lib/visualization/vega_data.hpp>
#include <unity/lib/image_util.hpp>
#include <string>
#include <vector>
#include <queue>

namespace turi {
namespace visualization {

  typedef boost::archive::iterators::base64_from_binary<
    boost::archive::iterators::transform_width<
      const unsigned char *,
      6,
      8
    >
  > to_base64;

  std::string escape_float(flex_float value);

  std::string escapeForTable( const flexible_type& value,
                              boost::local_time::time_zone_ptr empty_tz,
                              std::queue<vega_data::Image>* image_queue = NULL,
                              size_t count = -1,
                              const std::string& columnName = "");

  std::string replace_all(std::string str, const std::string& from, const std::string& to);
  std::string escape_string(const std::string& str, bool include_quotes=true);
  std::string extra_label_escape(const std::string& str, bool include_quotes=true);
}}


#endif
