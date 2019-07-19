#include "plot.hpp"
#include "show.hpp"
#include "scatter.hpp"
#include "heatmap.hpp"
#include "categorical_heatmap.hpp"
#include "boxes_and_whiskers.hpp"
#include "columnwise_summary.hpp"

namespace turi {
  namespace visualization {
    BEGIN_FUNCTION_REGISTRATION
    REGISTER_FUNCTION(plot, "x", "y", "xlabel", "ylabel", "title");
    REGISTER_FUNCTION(plot_scatter, "x", "y", "xlabel", "ylabel", "title");
    REGISTER_FUNCTION(plot_heatmap, "x", "y", "xlabel", "ylabel", "title");
    REGISTER_FUNCTION(plot_categorical_heatmap, "x", "y", "xlabel", "ylabel", "title");
    REGISTER_FUNCTION(plot_boxes_and_whiskers, "x", "y", "xlabel", "ylabel", "title");
    REGISTER_FUNCTION(plot_columnwise_summary, "sf");
    REGISTER_FUNCTION(plot_histogram, "sa", "xlabel", "ylabel", "title");
    REGISTER_FUNCTION(plot_item_frequency, "sa", "xlabel", "ylabel", "title");
    REGISTER_FUNCTION(plot_from_vega_spec, "vega_spec");
    END_FUNCTION_REGISTRATION
  }
}
