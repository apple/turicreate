

#include "escape.hpp"

namespace turi {
namespace visualization {

std::string escapeForTable( const flexible_type& value,
                            boost::local_time::time_zone_ptr empty_tz,
                            std::queue<visualization::vega_data::Image>* image_queue,
                            size_t count,
                            const std::string& columnName ){

    using namespace turi;
    using namespace turi::visualization;
    using namespace boost;
    using namespace local_time;
    using namespace gregorian;



    std::stringstream ss;

    switch (value.get_type()) {
      case flex_type_enum::UNDEFINED:
        ss << "null";
      case flex_type_enum::FLOAT:
        {
          flex_float f = value.get<flex_float>();
          if (std::isnan(f)) {
            ss << "\"nan\"";
            break;
          }
          if (std::isinf(f)) {
            if (f > 0) {
              ss << "\"inf\"";
            } else {
              ss << "\"-inf\"";
            }
            break;
          }
        }
      case flex_type_enum::INTEGER:
        ss << value;
        break;
      case flex_type_enum::DATETIME:
        {
          ss << "\"";
          const auto& dt = value.get<flex_date_time>();

          if (dt.time_zone_offset() != flex_date_time::EMPTY_TIMEZONE) {
            std::string prefix = "0.";
            int sign_adjuster = 1;
            if(dt.time_zone_offset() < 0) {
              sign_adjuster = -1;
              prefix = "-0.";
            }

            boost::local_time::time_zone_ptr zone(
                new boost::local_time::posix_time_zone(
                    "GMT" + prefix +
                    std::to_string(sign_adjuster *
                                   dt.time_zone_offset() *
                                   flex_date_time::TIMEZONE_RESOLUTION_IN_MINUTES)));
            boost::local_time::local_date_time az(
                flexible_type_impl::ptime_from_time_t(dt.posix_timestamp(),
                                                      dt.microsecond()), zone);
            ss << az;
          } else {
            boost::local_time::local_date_time az(
                flexible_type_impl::ptime_from_time_t(dt.posix_timestamp(),
                                                      dt.microsecond()),
                empty_tz);
            ss << az;
          }
          ss << "\"";
        }
        break;
      case flex_type_enum::VECTOR:
        {
          const flex_vec& vec = value.get<flex_vec>();

          ss << "[";
          for (size_t i = 0; i < vec.size(); ++i) {
            ss << vec[i];
            if (i + 1 < vec.size()) ss << ", ";
          }
          ss << "]";
        }
        break;
      case flex_type_enum::LIST:
        {
          const flex_list& vec = value.get<flex_list>();
          ss << "[";

          for (size_t i = 0; i < vec.size(); ++i) {
            const flexible_type val = vec[i];
            ss << turi::visualization::escapeForTable(val, empty_tz);
            if (i + 1 < vec.size()) ss << ", ";
          }

          ss << "]";
          break;
        }
      case flex_type_enum::IMAGE:
        {
          if(image_queue != NULL){
            const size_t resized_height = 40;

            flex_image img_temporary = value.get<flex_image>();
            double image_ratio = ((img_temporary.m_width*1.0)/(img_temporary.m_height*1.0));
            double calculated_width = (image_ratio * resized_height);
            size_t resized_width = static_cast<int>(calculated_width);
            flex_image img = turi::image_util::resize_image(img_temporary,
                    resized_width, resized_height, img_temporary.m_channels, img_temporary.is_decoded());
            img = turi::image_util::encode_image(img);

            const unsigned char * image_data = img.get_image_data();

            visualization::vega_data::Image image_temp;

            image_temp.idx = count;
            image_temp.column = visualization::extra_label_escape(columnName);
            image_temp.img = img_temporary;

            image_queue->push(image_temp);

            size_t image_data_size = img.m_image_data_size;
            ss << "{\"width\": " << img.m_width << ", ";
            ss << "\"height\": " << img.m_height << ", ";
            ss << "\"idx\": " << count << ", ";
            ss << "\"column\": " << visualization::extra_label_escape(columnName) << ", ";
            ss << "\"data\": \"";

            std::copy(
              to_base64(image_data),
              to_base64(image_data + image_data_size),
              std::ostream_iterator<char>(ss)
            );

            ss << "\", \"format\": \"";
            switch (img.m_format) {
              case Format::JPG:
                ss << "jpeg";
                break;
              case Format::PNG:
                ss << "png";
                break;
              case Format::RAW_ARRAY:
                ss << "raw";
                break;
              case Format::UNDEFINED:
                ss << "raw";
                break;
            }
            ss << "\"}";
            break;
          }
        }
      default:
        {
          ss << turi::visualization::extra_label_escape(value.to<std::string>());
        }
        break;
    }

    return ss.str();
}
}
}
