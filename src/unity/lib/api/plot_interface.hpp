#ifndef TURI_PLOT_INTERFACE_HPP
#define TURI_PLOT_INTERFACE_HPP

#include <memory>
#include <vector>
#include <string>
#include <flexible_type/flexible_type.hpp>
#include <unity/lib/api/function_closure_info.hpp>
#include <unity/lib/visualization/plot.hpp>
#include <cppipc/magic_macros.hpp>

namespace turi {
  class plot_base;

  GENERATE_INTERFACE_AND_PROXY(plot_base, plot_proxy,
    (void, show, )
    (void, materialize, )
    (std::string, get_spec, )
    (std::string, get_data, )
  )
}

#endif
