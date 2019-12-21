/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_METHOD_WRAPPER_HPP_
#define TURI_METHOD_WRAPPER_HPP_

#include <core/export.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server_v2/method_parameters.hpp>

namespace turi {

// Helper for enable if in templates. 
// TODO: move this to a common location
template <bool B> using enable_if_ = typename std::enable_if<B,int>::type;

namespace v2 { 


/** Base class for wrapper around a specific method. 
 *
 *  This class provides an interface to call a method 
 *  or function using generic arguments.  The interface is contained in a
 *  templated instance of the class derived from this.  It's created 
 *  through the `create` static factory method.
 *
 *  This class is meant to be a member of the  
 *   
 */
template <typename BaseClass> class method_wrapper { 

  public:
   
   virtual ~method_wrapper(){}

   /** The type of the Class on which our method rests.
    *  
    *  May be , in which case it's a standalone function.
    */
   typedef BaseClass class_type;  
  
   /// Calling method for const base classes.
   virtual variant_type call(const BaseClass* _inst, const argument_pack& args) const = 0;

   /// Calling method for non-const base classes. 
   virtual variant_type call(BaseClass* C, const argument_pack& args) const = 0;
   
   /// Calling method for standalone functions. 
   variant_type call(const argument_pack& args) const {
     return call(nullptr, args);
   }

   /// Returns the parameter info struct for a particular parameter 
   inline const Parameter& parameter_info(size_t n) const { 
     return m_parameter_list.at(n);
   }

   /// Returns the name of the parameter 
   inline const std::string& parameter_name(size_t n) const {
     return parameter_info(n).name; 
   }

   /** Factory method.
    *
    *  Call this method to create the interface wrapper around 
    *  the method.
    */
   template <typename Class, typename RetType, 
            typename... FuncParams, typename... ParamDefs> 
   static std::shared_ptr<method_wrapper> 
   create(RetType(Class::*method)(FuncParams...) const, const ParamDefs&...);
   
   /** Overload of above factory method for non-const methods
    *
    */ 
   template <typename Class, typename RetType, 
            typename... FuncParams, typename... ParamDefs> 
   static std::shared_ptr<method_wrapper> 
   create(RetType(Class::*method)(FuncParams...), const ParamDefs&...);
   

   /** Factory method for non-method functions.
    *
    */
   template <typename RetType, typename... FuncParams, typename... ParamDefs> 
   static std::shared_ptr<method_wrapper> 
   create(RetType(*function)(FuncParams...), const ParamDefs&...);

  protected:

   // To be called only from the instantiating class
   method_wrapper(const std::vector<Parameter>& _parameter_list) 
     : m_parameter_list(_parameter_list)
   { }
   
   // Information about the function / method 
   std::vector<Parameter> m_parameter_list;
 };


/////////////////////////////////////////////////////////////////////////////////
//
//  Implementation details of the above.
//  

// If there is no class, then this is used instead of Class 
// to denote that there is no class. 
struct __NoClass {};

/** Child class for resolving the arguments passed into a function call.  
 *
 *  This class is mainly a container to define the types present 
 *  during the recursive parameter expansion stage.  
 *
 */ 
template <typename Class, typename BaseClass, bool is_const_method,
          typename RetType, typename... FuncParams> 
  class method_wrapper_impl : public method_wrapper<BaseClass> {
 private:  

  /// The number of parameters required for the function.
  static constexpr size_t N = sizeof...(FuncParams);
 
  /// Are we in a class instance or just a standalone function? 
  static constexpr bool is_method = !std::is_same<Class, __NoClass>::value;

  // logic below requires is_const_method to be false when is_method is false
  static_assert(is_method || !is_const_method, 
      "is_const_method=1 when is_method =0");

  /// Set the method type -- general class vs standalone function 
  template <int> struct method_type_impl {};

  // Non-method case 
  template <> struct method_type_impl<0> { 
     typedef RetType (*type)(FuncParams...);
  }; 
  
  // Method non-const case.
  template <> struct method_type_impl<1> { 
     typedef RetType (Class::*type)(FuncParams...);
  }; 

  // Method const case.
  template <> struct method_type_impl<2> { 
     typedef RetType (Class::*type)(FuncParams...) const;
  }; 

  typedef typename method_type_impl<is_method + is_const_method>::type method_type;
  
  /// Function pointer to the method we're calling.
  method_type m_method;

 public: 
  /** Constructor.  
   */
  method_wrapper_impl(
      std::vector<Parameter>&& _parameter_list, method_type method)
    : method_wrapper<BaseClass>(std::move(_parameter_list))
    , m_method(method)
  {
    validate_parameter_list<FuncParams...>(this->m_parameter_list);
  }

 private:


  
  //////////////////////////////////////////////////////////////////////////////
  //
  //  Calling methods

  /** A handy way to refer to the type of the nth argument.
   */
  template <int idx>
  struct nth_param_type { 
    typedef typename std::tuple_element<idx, std::tuple<FuncParams...> >::type raw_type;
    typedef typename std::decay<raw_type>::type type;
  };
  
  /// Container for passing the calling arguments around after unpacking from argument_list
  typedef std::array<const variant_type*, N> arg_v_type;
  
  //////////////////////////////////////////////////////////////////////////////
  // Entrance methods to this process. 

  /** Non-const calling method.
   *
   */ 
  variant_type call(BaseClass* inst, const argument_pack& args) const override {
    return _choose_call_path(inst, args); 
  }
  
  /** Const calling method.
   *
   */ 
  variant_type call(const BaseClass* inst, const argument_pack& args) const override {
    return _choose_call_path(inst, args); 
  }

  //////////////////////////////////////////////////////////////////////////////
  // Step 1: Determine the evaluation path and Class pointer type depending 
  //         on whether it's a const method, regular method, or function.
  
  template <class C>
  struct _call_chooser {
    static constexpr bool func_path         = !is_method; 
    static constexpr bool const_method      = is_const_method;
    static constexpr bool _non_const_method = is_method && !is_const_method;
    static constexpr bool bad_const_call    = _non_const_method && std::is_const<C>::value;
    static constexpr bool method_path       = _non_const_method && !std::is_const<C>::value;
  }; 

  // If it's a regular function.
  template <class C, enable_if_<_call_chooser<C>::func_path> = 0>
  variant_type _choose_call_path(C* inst, const argument_pack& args) const {
    return _call<void>(nullptr, args);
  }

  // If it's a bad const call.
  template <class C, enable_if_<_call_chooser<C>::bad_const_call> = 0>
  [[noreturn]] 
  variant_type _choose_call_path(C* inst, const argument_pack& args) const {
    // Null implementation of the above to intercept compilation of the in-formed 
    // case where it's a const class and the method is non-const. 
    throw std::invalid_argument("Non-const method call attempted on const class pointer.");
  }

  // Const method.
  template <class C, enable_if_<_call_chooser<C>::const_method> = 0>
  variant_type _choose_call_path(C* inst, const argument_pack& args) const {
    return _call(dynamic_cast<const Class*>(inst), args);
  }

  // Non-const method.
  template <class C, enable_if_<_call_chooser<C>::method_call> = 0>
  variant_type _choose_call_path(C* inst, const argument_pack& args) const {
    return _call(dynamic_cast<Class*>(inst), args);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Step 2: Unpack and resolve arguments. 

  template <class C> 
  variant_type _call(C* inst, const argument_pack& args) const {
    arg_v_type arg_v; 
   
    // Resolve and unpack the incoming arguments.
    resolve_method_arguments<N>(arg_v, this->m_parameter_list, args); 

    // Now that the argument list arg_v is filled out, we can call the 
    // recursive calling function and return the value.
    return __call<0>(inst, arg_v);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Step 3: Recursively unpack the parameters into a parameter pack with the 
  //         correct values.  Checks performed here.

  template <int arg_idx, typename C, typename... Expanded, enable_if_<arg_idx != N> = 0>
    variant_type __call(C* inst, const arg_v_type& arg_v, const Expanded&... args) const {

    // TODO: Separate out the case where the unpacking can be done by 
    // reference.
    typedef typename nth_param_type<arg_idx>::type arg_type;

    // TODO: Add intelligent error messages here on failure
    arg_type next_arg = variant_get_value<arg_type>(*(arg_v[arg_idx]));
   
    // Call the next unpacking routine. 
    return __call<arg_idx+1, C, Expanded..., arg_type>(inst, arg_v, args..., next_arg);
  }
 
  //////////////////////////////////////////////////////////////////////////////
  // Step 4: Call the function / method. 
  //
  // This is the stopping case of expansion -- we've unpacked and translated all the 
  // arguments, now it's time to actually call the method.
  
  // First case: class method, void return type.
  template <int arg_idx, typename C, typename... Expanded, 
           enable_if_<arg_idx == N && is_method && std::is_void<RetType>::value> = 0>
    variant_type __call(C* inst, const arg_v_type& arg_v, const Expanded&... args) const {
     (inst->*m_method)(args...); 
     return variant_type(); 
  }
  
  // Second case: class method, non-void return type.
  template <int arg_idx, typename C, typename... Expanded, 
           enable_if_<arg_idx == N && is_method && !std::is_void<RetType>::value> = 0>
    variant_type __call(C* inst, const arg_v_type& arg_v, const Expanded&... args) const {
    
     return to_variant( (inst->*m_method)(args...) );
  }
  
  // Third case: standalone function, void return type
  template <int arg_idx, typename C, typename... Expanded, 
           enable_if_<arg_idx == N && !is_method && std::is_void<RetType>::value> = 0>
    variant_type __call(C*, const arg_v_type& arg_v, const Expanded&... args) const {

     m_method(args...);
     return variant_type(); 
  }

  // Fourth case: standalone function, non-void return type.
  template <int arg_idx, typename C, typename... Expanded, 
           enable_if_<arg_idx == N && !is_method && !std::is_void<RetType>::value> = 0>
    variant_type __call(C*, const arg_v_type& arg_v, const Expanded&... args) const {
   
     return to_variant(m_method(args...) );
  }
}; 


//////////////////////////////////////////////////////////
// Some utility functions to help unpack the variadic arguments into 
// a vector of parameters.

template <int n>
void __unpack_parameters(
    std::vector<Parameter>& dest,
    const std::vector<Parameter>& vp) {
  dest = vp;
}

/** Recursive function to unpack the parameter list. 
 */
template <int idx, typename... Params, enable_if_<idx != sizeof...(Params)> = 0> 
void __unpack_parameters(std::vector<Parameter>& dest, const Params&... pv) {
  dest[idx] = Parameter(std::get<idx>(std::make_tuple<const Params&...>(pv...)));
  __unpack_parameters<idx + 1>(dest, pv...);
}

/** Stopping case of recursive unpack function.
 *
 */
template <int idx, typename... Params, enable_if_<idx == sizeof...(Params)> = 0> 
void __unpack_parameters(std::vector<Parameter>& dest, const Params&... pv) {
}
   
/** Implementation of the factory method for non-const methods.
 */
template <typename BaseClass>
template <typename Class, typename RetType, typename... FuncParams, typename... ParamDefs>
std::shared_ptr<method_wrapper<BaseClass> > method_wrapper<BaseClass>::create(
    RetType(Class::*method)(FuncParams...), 
    const ParamDefs&... param_defs) { 

  std::vector<Parameter> params; 
  params.resize(sizeof...(ParamDefs));
  __unpack_parameters<0>(params, param_defs...);

  return std::make_shared<method_wrapper_impl<Class, BaseClass, false, RetType, FuncParams...> >
        (std::move(params), method);
 };

/**  Const overload of the method interface factory method.
 */
template <typename BaseClass>
template <typename Class, typename RetType, typename... FuncParams, typename... ParamDefs>
std::shared_ptr<method_wrapper<BaseClass> > method_wrapper<BaseClass>::create(
    RetType(Class::*method)(FuncParams...) const, 
    const ParamDefs&... param_defs) { 

  std::vector<Parameter> params; 
  params.resize(sizeof...(ParamDefs));
  __unpack_parameters<0>(params, param_defs...);

  return std::make_shared<method_wrapper_impl<Class, BaseClass, true, RetType, FuncParams...> >
        (std::move(params), method);
 };

/** Factory method for non-method functions.
 *
 */
template <class BaseClass>
   template <typename RetType, typename... FuncParams, typename... ParamDefs> 
   std::shared_ptr<method_wrapper<BaseClass> > 
    method_wrapper<BaseClass>::create(
        RetType(*function)(FuncParams...), 
        const ParamDefs&... param_defs) {

  std::vector<Parameter> params; 
  params.resize(sizeof...(ParamDefs));
  __unpack_parameters<0>(params, param_defs...);

  return std::make_shared<method_wrapper_impl<__NoClass, BaseClass, false, RetType, FuncParams...> >
        (std::move(params), function);

   }


}
}

#endif
