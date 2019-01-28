#include "plot.hpp"
#import <vega_renderer/VegaRenderer/VegaRenderer.h>

namespace turi {
  namespace visualization {
    bool Plot::render(CGContextRef context, tc_plot_variation variation) {
      std::string spec_with_data = this->get_spec(variation, true /* include_data */);
      NSString *spec = [NSString stringWithUTF8String:spec_with_data.c_str()];
      VegaRenderer *renderer = [[VegaRenderer alloc] initWithSpec:spec];
      CGContextDrawLayerAtPoint(context, CGPointMake(0, 0), renderer.CGLayer);
      return this->finished_streaming();
    }
  }
}