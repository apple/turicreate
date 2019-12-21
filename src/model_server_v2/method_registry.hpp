/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_METHOD_REGISTRY_HPP_
#define TURI_METHOD_REGISTRY_HPP_

#include <core/export.hpp>
#include <model_server/lib/variant.hpp>
#include <map>
#include <model_server_v2/method_wrapper.hpp>
#include <model_server/lib/variant_converter.hpp>

namespace turi {
namespace v2 { 


/** Manager all the methods in a given class / model. 
 *
 *  This class exists to manage a collection of methods associated with a given 
 *  class.  It provides an interface to call previously registered methods on 
 *  this class by name, along with helpful error messages if the call is wrong. 
 *
 *  If the BaseClass is void, it provides a registry for standalone functions.
 *  TODO: implement this. 
 */
template <typename BaseClass>
class method_registry {
  public:

   method_registry()
     : m_class_name()
    {}

   method_registry(const std::string& _name)
     : m_class_name(_name)
    {}

   /** Register a new method.  
    *
    *  See method_wrapper<BaseClass>::create for an explanation of the arguments.
    * 
    */
   template <typename... RegisterMethodArgs> 
     void register_method(const std::string& name, RegisterMethodArgs&&... rmargs) { 

      try { 

        auto wrapper = method_wrapper<BaseClass>::create(rmargs...);
      
        m_method_lookup[name] = wrapper; 
      } catch(...) {
        // TODO: Expand these exceptions to make them informative.
        process_exception(std::current_exception());
      }
    }

   // Lookup a call function information.
   std::shared_ptr<method_wrapper<BaseClass> > lookup(const std::string& name) const { 

     // TODO: proper error message here
     return m_method_lookup.at(name);
   }

   /** Call a given const method registered previously.  
    */ 
   variant_type call_method(const BaseClass* inst, const std::string& name, 
       const argument_pack& arguments) const { 

      try { 
         return lookup(name)->call(inst, arguments); 

      } catch(...) {
        process_exception(std::current_exception());
      }
   }

   /** Call a given const or non-const method registered previously. 
    */
   variant_type call_method(BaseClass* inst, const std::string& name, 
       const argument_pack& arguments) const { 

      try { 
         return lookup(name)->call(inst, arguments); 
      } catch(...) {
        process_exception(std::current_exception());
      }
   }

   private:

    [[noreturn]] void process_exception(std::exception_ptr e) const { 
      // TODO: Expand these exceptions to make them informative.

      std::rethrow_exception(e);
    }

   // Unpack arguments.
     template <size_t i, size_t N, typename Tuple, enable_if_<i != N> = 0>
     inline void _arg_unpack(std::vector<variant_type>& dest, const Tuple& t) const { 
        dest[i] = to_variant(std::get<i>(t)); 
        _arg_unpack<i + 1, N>(dest, t);
     }

     template <size_t i, size_t N, typename Tuple, enable_if_<i == N> = 0>
     inline void _arg_unpack(std::vector<variant_type>& dest, const Tuple& t) const { 
     }


   public: 

     // Call a method with the arguments explicitly. 
     template <typename BC, typename... Args, 
               enable_if_<std::is_same<typename std::remove_const<BC>::type, BaseClass>::value> = 0>
     variant_type call_method(BC* inst, const std::string& name, const Args&... args) const { 

     argument_pack arg_list; 
     arg_list.ordered_arguments.resize(sizeof...(Args));

     _arg_unpack<0, sizeof...(Args)>(arg_list.ordered_arguments, std::make_tuple<const Args&...>(args...));

     return call_method(inst, name, arg_list); 
   }
  
  private:

   std::string m_class_name; 

   std::unordered_map<std::string, std::shared_ptr<method_wrapper<BaseClass> > >
     m_method_lookup;
};

}
}

#endif
