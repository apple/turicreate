/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/server/registration.hpp>
#include <model_server/server/unity_server_init.hpp>
#include <toolkits/recsys/models/itemcf.hpp>

namespace turi {


  // Do the registration here, so that we only get the model that we need for this.

  namespace recsys { namespace ios {
  BEGIN_CLASS_REGISTRATION
  REGISTER_CLASS(recsys_itemcf)
  END_CLASS_REGISTRATION
  }  // namespace ios
  }  // namespace recsys

  class EXPORT recommender_server_initializer
      : public unity_server_initializer {
   public:
    /**
     * Fill the registry of internal toolkits
     */
    void init_toolkits(toolkit_function_registry& registry) const {}

    /**
     * Fill the registry of internal models
     */
    void init_models(toolkit_class_registry& registry) const {
      // Recsys Models
      registry.register_toolkit_class(
          turi::recsys::ios::get_toolkit_class_registration());
    }

};

/** The server initializer function.
 *
 *  This function overrides the capi_server_initializer function in the
 *
 *  In some cases, it may be desirable to have a custom server initializer, for
 *  example if only a subset of the models are needed.  In this case, define the
 *  macro CAPI_DISABLE_DEFAULT_SERVER_INITIALIZER and create a custom
 *  implementation of that function to be compiled in.  In addition, the CMakeLists.txt
 */
EXPORT std::shared_ptr<turi::unity_server_initializer> capi_server_initializer() {
 return std::make_shared<turi::recommender_server_initializer>();
}

}
