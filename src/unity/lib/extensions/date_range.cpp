#include <flexible_type/flexible_type.hpp>
#include <unity/lib/gl_sarray.hpp>

namespace turi {
    namespace date_range {

        gl_sarray date_range(const flexible_type &start_time,const flexible_type
            &end_time,const flexible_type & period) {
            gl_sarray_writer writer(flex_type_enum::DATETIME,1);
            flexible_type current_time(start_time);

            while(current_time <= end_time) {
              writer.write(current_time,0);
              current_time += period;
            }
            return writer.close();
        }

        BEGIN_FUNCTION_REGISTRATION
        REGISTER_FUNCTION(date_range, "start_time", "end_time",
            "period")
        END_FUNCTION_REGISTRATION

    }
}