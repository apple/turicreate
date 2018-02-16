#include "plot.hpp"

namespace turi{
  namespace visualization{

    void Plot::show() {

    }

    void Plot::materialize() {

    }

    std::string Plot::get_spec() {
      return "hello";
    }
  }
}

using namespace turi;

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(turi::visualization::Plot)
END_CLASS_REGISTRATION
