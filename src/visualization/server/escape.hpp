
#ifndef __TC_ESCAPE
#define __TC_ESCAPE

#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/data/flexible_type/ndarray.hpp>
#include <visualization/server/vega_data.hpp>
#include <model_server/lib/image_util.hpp>
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
  std::string escape_image(flex_image value, size_t resized_height,
                              size_t row_index,
                              const std::string& columnName);

  std::string escapeForTable( const flexible_type& value,
                              size_t row_index = -1,
                              const std::string& columnName = "");

  std::string replace_all(std::string str, const std::string& from, const std::string& to);
  std::string escape_string(const std::string& str, bool include_quotes=true);
  std::string extra_label_escape(const std::string& str, bool include_quotes=true);
}}


#endif
