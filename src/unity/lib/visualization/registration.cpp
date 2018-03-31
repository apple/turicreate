#include "show.hpp"
#include "scatter.hpp"
#include "heatmap.hpp"
#include "categorical_heatmap.hpp"
#include "boxes_and_whiskers.hpp"
#include "columnwise_summary.hpp"

namespace turi {
  namespace visualization {
    BEGIN_FUNCTION_REGISTRATION
    REGISTER_FUNCTION(plot, "path_to_client", "x", "y", "xlabel", "ylabel", "title");
    REGISTER_FUNCTION(plot_scatter, "path_to_client", "x", "y", "xlabel", "ylabel", "title"); 
    REGISTER_FUNCTION(plot_heatmap, "path_to_client", "x", "y", "xlabel", "ylabel", "title"); 
    REGISTER_FUNCTION(plot_categorical_heatmap, "path_to_client", "x", "y", "xlabel", "ylabel", "title"); 
    REGISTER_FUNCTION(plot_boxes_and_whiskers, "path_to_client", "x", "y", "xlabel", "ylabel", "title"); 
    REGISTER_FUNCTION(plot_columnwise_summary, "path_to_client", "sf"); 
    REGISTER_FUNCTION(plot_histogram, "path_to_client", "sa", "xlabel", "ylabel", "title"); 
    REGISTER_FUNCTION(plot_item_frequency, "path_to_client", "sa", "xlabel", "ylabel", "title"); 
    END_FUNCTION_REGISTRATION
  }
}
