/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TOOLKIT_FUNCTION_WRAPPER_IMPL_HPP
#define TURI_TOOLKIT_FUNCTION_WRAPPER_IMPL_HPP
#include <type_traits>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/fusion/mpl.hpp>
#include <boost/fusion/mpl/at.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/as_vector.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/functional/invocation/invoke.hpp>
#include <boost/type_traits.hpp>
#include <boost/type_traits/function_traits.hpp>
#include <model_server/lib/toolkit_function_invocation.hpp>
#include <model_server/lib/toolkit_function_specification.hpp>
#include <model_server/lib/toolkit_util.hpp>
namespace turi {
namespace toolkit_function_wrapper_impl {

/**
 * Type function which given a function pointer, returns an mpl::vector
 * of the function argument type list.
 *
 * (R (*)(T...)) --> boost::mpl::vector<T...>
 *
 * Example:
 * \code
 * int test(int, double) {
 *  ...
 * }
 *
 * typedef typename function_args_to_mpl_vector<decltype(test)>::type fn_args_type;
 * \endcode
 * will have fn_args_types == boost::mpl::vector<int, double>
 *
 */
template <typename Fn>
struct function_args_to_mpl_vector {
  template <typename R, typename... T>
  static boost::mpl::vector<T...> function_args_to_mpl_vector_helper(R (*)(T...)) {
    return boost::mpl::vector<T...>();
  }

  typedef decltype(function_args_to_mpl_vector_helper(reinterpret_cast<Fn>(NULL))) type;
};


/**
 * Provides a wrapper around the return type of a function call.
 * For instance:
 * \code
 * int test() {
 *   ...
 * }
 * typedef boost::function_traits<decltype(test)> fntraits;
 *
 * result_of_function_wrapper<fntraits::result_type> res;
 * res.call(test);
 *
 * // after which res.ret_value will contain the return value of res
 * // (which is an int).
 * \endcode
 *
 * The key reason for this struct is to handle the void return case
 * where the result type of the function is void. This struct specializes that
 * case by making ret_value an integer which is 0 prior to calling the function,
 * and 1 after successful calling of the function.
 *
 * i.e.
 * \code
 * void nothing() {
 *   ...
 * }
 * typedef boost::function_traits<decltype(nothing)> fntraits;
 *
 * result_of_function_wrapper<fntraits::result_type> res;
 * // at this point res.ret_value is an integer and has value 0
 * try {
 *   res.call(nothing);
 * } catch (...) {
 *   // function was called, but it didn't return properly,
 *   // res.ret_value is still 0
 * }
 * // function call was completed successfully. res.ret_value == 1
 * \endcode
 *
 * Note that the function must take no arguments. If there are arguments,
 * it can be wrapped in a lambda. For instance:
 *
 * \code
 * void has_arguments(int) {
 *   ...
 * }
 * typedef boost::function_traits<decltype(has_arguments)> fntraits;
 * result_of_function_wrapper<fntraits::result_type> res;
 * res.call([](){ return has_arguments(5);});
 *
 * \endcode
 */
template <typename T>
struct result_of_function_wrapper {
  T ret_value;
  static constexpr bool is_void = false;
  template <typename F>
  void call(F f) {
    ret_value = f();
  }
};

/** specialization for the void return case;
 * in which case the return value is an integer which is 0 if the function
 * is not called, and 1 if the function is called.
 */
template <>
struct result_of_function_wrapper<void> {
  int ret_value = 0;
  static constexpr bool is_void = true;
  template <typename F>
  void call(F f) {
    f();
    ret_value = 1;
  }
};


/**
 * A boost fusion type function that achieves:
 *
 *    T -> T*
 */
struct make_pointer_to {
    template<typename Sig>
    struct result;

    template<typename T>
    struct result<make_pointer_to(T&)>
    {
        typedef T* type;
    };

    template<typename T>
    T* operator()(T& t) const
    {
        return &t;
    }
};

/**
 * Reads a typed function parameter from a variant argument. In most cases, this
 * just boils down to variant_get_value<T>.
 */
template <typename T>
inline T read_arg(const variant_type& var) {
  return variant_get_value<T>(var);
}

/**
 * Specializes read_arg<variant_map_type>. The Python integration must convert
 * Python dictionaries to either variant_map_type or flexible_type without any
 * knowledge of the type required on the C++ end. Python prefers flexible_type
 * when both are possible, so the C++ side must convert if necessary.
 */
template <>
inline variant_map_type read_arg<variant_map_type>(const variant_type& var) {
  if (var.which() == 0) {
    const flexible_type& ft = variant_get_ref<flexible_type>(var);
    if (ft.get_type() == flex_type_enum::DICT) {
      // The argument is a flex_dict but we expected a variant_map_type. Attempt
      // a conversion. Note that this will fail if any flex_dict keys are not
      // strings, but we would have failed anyway in variant_get_value below.
      variant_map_type ret;
      for (const auto& kv : ft.get<flex_dict>()) {
        ret[kv.first.get<flex_string>()] = to_variant(kv.second);
      }
      return ret;
    }
  }

  return variant_get_value<variant_map_type>(var);
}

/**
 * Fills in a boost::fusion::vector<T ...> with parameters from the
 * toolkit invocation object.
 *
 * InArgType is a boost::fusion::vector<T ...> representing the input arguments
 * of the user defined function.
 *
 * inargnames is a vector of strings naming each of the argument, and should
 * preferably, (but not necessary) be of the same length as InArgType.
 *
 * Essentially, this performs the following simple task:
 *
 * inargs[n] = params[inargnames[n]]
 *
 * Except this cannot be done so easily because each entry in the
 * boost fusion vector is of a different type. So a bit more work is needed.
 *
 * If inargnames is shorter than the actual number of arguments, only
 * inargnames.size() number of arguments is filled. If it is longer, only the
 * first inargs.size() names are taken.
 */
template <typename InArgType>
struct fill_named_in_args {
  InArgType* inargs; // pointer to the input arguments
  std::vector<std::string> inargnames;  // name of each input argument
  variant_map_type* params;  // pointer to the user's input which called the function

  template<int n>
  void operator()(boost::mpl::integral_c<int, n> t) const {
    // the type of inargs[n] the use of std::decay removes all references
    typedef typename std::decay<decltype(boost::fusion::at_c<n>(*inargs))>::type element_type;
    if (n < inargnames.size()) {
      variant_map_type::const_iterator kv = params->find(inargnames[n]);
      if (kv == params->end()) {
        std_log_and_throw(std::invalid_argument,
            "Missing toolkit function parameter: " + inargnames[n]);
      }

      boost::fusion::at_c<n>(*inargs) = read_arg<element_type>(kv->second);
    }
  }
};


/**
 * Fills in a boost::fusion::vector<T ...> with parameters from the
 * std::vector<variant_type>.
 *
 * InArgType is a boost::fusion::vector<T ...> representing the input arguments
 * of the user defined function.
 *
 * params is a vector of variants of the same length as InArgType.
 *
 * Essentially, this performs the following simple task:
 *
 * inargs[n] = params[n]
 *
 * Except this cannot be done so easily because each entry in the
 * boost fusion vector is of a different type. So a bit more work is needed.
 */
template <typename InArgType>
struct fill_in_args {
  InArgType* inargs; // pointer to the input arguments
  const std::vector<variant_type>* params;  // pointer to the user's input which called the function

  template<int n>
  void operator()(boost::mpl::integral_c<int, n> t) const {
    // the type of inargs[n] the use of std::decay removes all references
    typedef typename std::decay<decltype(boost::fusion::at_c<n>(*inargs))>::type element_type;
    if (n < params->size()) {
      boost::fusion::at_c<n>(*inargs) = variant_get_value<element_type>((*params)[n]);
    }
  }
};

/**
 * A type function that maps int --> range<int>(0, N).
 *
 * Given a number N, make_range<N>::type is a boost mpl range type representing
 * a sequence from 0 to N exclusive.
 *
 */
template <size_t N>
struct make_range {
  typedef typename boost::mpl::range_c<int, 0, N>::type type;
};


/**
 * A type function that maps (int, int) --> range<int>(Start, End).
 *
 * Given two numbers Start, End , make_range<Start, End>::type is a boost mpl
 * range type representing a sequence from Start to End exclusive.
 *
 */
template <size_t Start, size_t End>
struct make_range2 {
  typedef typename boost::mpl::range_c<int, Start, End>::type type;
};


/**
 * Wraps a function f(...) with a function that takes a variant_map_type
 * and returns a variant_type.
 * Essentially, given a function f of type ret(in1, in2, in3 ...),
 * returns a function g of type variant_type(variant_map_type) where g
 * performs the equivalent of:
 *
 * \code
 * variant_type g(variant_map_type input) {
 *   return variant_encode( f (variant_decode(input[inargnames[0]),
 *                             variant_decode(input[inargnames[1]),
 *                             variant_decode(input[inargnames[2]),
 *                             ...));
 * }
 *
 * \endcode
 */
template <size_t NumInArgs, typename Function>
std::function<variant_type(variant_map_type)>
generate_function_wrapper(Function fn, std::vector<std::string> inargnames) {
  // import some mpl stuff we will be using
  using boost::mpl::erase;
  using boost::mpl::size;
  using boost::mpl::int_;
  using boost::mpl::_1;

  // the actual spec object we are returning
  toolkit_function_specification spec;
  // Get a function traits object from the function type
  typedef boost::function_traits<typename std::remove_pointer<Function>::type > fntraits;
  // Get an mpl::vector containing all the arguments of the user defined function
  typedef typename function_args_to_mpl_vector<Function>::type fn_args_type_original;
  // decay it to element const, references, etc.
  typedef typename boost::mpl::transform<fn_args_type_original, std::decay<_1>>::type fn_args_type;

  static_assert(size<fn_args_type>::value == NumInArgs,
                "Invalid number arguments. #input != #function arguments.");
  typedef fn_args_type in_arg_types;

  auto fnwrapper =
      [fn, inargnames](variant_map_type args)->variant_type {
        turi::toolkit_function_response_type ret;
        // we need to fill the input arguments and output arguments
        typename boost::fusion::result_of::as_vector<in_arg_types>::type in_args;
        /*
         * Essentially:
         *
         * for i = 0 to #inargs - 1:
         *    inargs[i] = invoke->params[inargnames[i]]
         *
         * The fill_named_in_args struct performs the body of the for loop.
         */
        fill_named_in_args<decltype(in_args)> in_arg_filler;
        in_arg_filler.params = &args;
        in_arg_filler.inargnames = inargnames;
        in_arg_filler.inargs = &in_args;
        typename make_range<size<decltype(in_args)>::value>::type in_arg_range;
        boost::fusion::for_each(in_arg_range, in_arg_filler);

        // Invoke the function call, storing the return value
        result_of_function_wrapper<typename std::decay<typename fntraits::result_type>::type> retval;
        retval.call([&](){
          return boost::fusion::invoke(fn, in_args);
        });
        if (retval.is_void) return to_variant(FLEX_UNDEFINED);
        else return to_variant(retval.ret_value);
      };
  return fnwrapper;
}


/**
 * Wraps a function f(...) with a function that takes a variant_map_type
 * and returns a variant_type.
 * Essentially, given a function f of type ret(in1, in2, in3 ...),
 * returns a function g of type variant_type(variant_map_type) where g
 * performs the equivalent of:
 *
 * \code
 * variant_type g(variant_map_type input) {
 *   return variant_encode( f (variant_decode(input[inargnames[0]),
 *                             variant_decode(input[inargnames[1]),
 *                             variant_decode(input[inargnames[2]),
 *                             ...));
 * }
 *
 * \endcode
 */
template <size_t NumInArgs, typename Function>
std::function<variant_type(const std::vector<variant_type>&)>
generate_native_function_wrapper(Function fn) {
  // import some mpl stuff we will be using
  using boost::mpl::erase;
  using boost::mpl::size;
  using boost::mpl::int_;
  using boost::mpl::_1;

  // the actual spec object we are returning
  toolkit_function_specification spec;
  // Get a function traits object from the function type
  typedef boost::function_traits<typename std::remove_pointer<Function>::type > fntraits;
  // Get an mpl::vector containing all the arguments of the user defined function
  typedef typename function_args_to_mpl_vector<Function>::type fn_args_type_original;
  // decay it to element const, references, etc.
  typedef typename boost::mpl::transform<fn_args_type_original, std::decay<_1>>::type fn_args_type;

  static_assert(size<fn_args_type>::value == NumInArgs,
                "Invalid number arguments. #input != #function arguments.");
  typedef fn_args_type in_arg_types;

  auto fnwrapper =
      [fn](const std::vector<variant_type>& args)->variant_type {
        if (args.size() != fntraits::arity) {
          throw std::string("Insufficient arguments");
        }
        // we need to fill the input arguments and output arguments
        typename boost::fusion::result_of::as_vector<in_arg_types>::type in_args;
        /*
         * Essentially:
         *
         * for i = 0 to #inargs - 1:
         *    inargs[i] = invoke->params[i]
         *
         * The fill_in_args struct performs the body of the for loop.
         */
        fill_in_args<decltype(in_args)> in_arg_filler;
        in_arg_filler.params = &args;
        in_arg_filler.inargs = &in_args;
        typename make_range<size<decltype(in_args)>::value>::type in_arg_range;
        boost::fusion::for_each(in_arg_range, in_arg_filler);

        // Invoke the function call, storing the return value
        result_of_function_wrapper<typename std::decay<typename fntraits::result_type>::type> retval;
        retval.call([&](){
          return boost::fusion::invoke(fn, in_args);
        });
        if (retval.is_void) return to_variant(FLEX_UNDEFINED);
        else return to_variant(retval.ret_value);
      };
  return fnwrapper;
}




/**
 * Wraps a member function T::f(...) with a function that takes a
 * variant_map_type and returns a variant_type.
 * Essentially, given a function f of type ret(in1, in2, in3 ...),
 * returns a function g of type variant_type(variant_map_type) where g
 * performs the equivalent of:
 *
 * \code
 * variant_type g(T* t, variant_map_type input) {
 *   return variant_encode( t->f (variant_decode(input[inargnames[0]),
 *                             variant_decode(input[inargnames[1]),
 *                             variant_decode(input[inargnames[2]),
 *                             ...));
 * }
 *
 * \endcode
 */
template <size_t NumInArgs, typename T, typename Ret, typename... Args>
std::function<variant_type(T*, variant_map_type)>
generate_member_function_wrapper(Ret (T::* fn)(Args...),
                                 std::vector<std::string> inargnames) {
  // import some mpl stuff we will be using
  using boost::mpl::erase;
  using boost::mpl::size;
  using boost::mpl::int_;
  using boost::mpl::_1;

  // the actual spec object we are returning
  toolkit_function_specification spec;
  // convert Ret (T::* fn)(Args...) to Ret fn(T*, Args...)
  auto member_fn = std::mem_fn(fn);
  // Get an mpl::vector containing all the arguments of the member function
  typedef typename boost::mpl::vector<T*, Args...>::type fn_args_type_original;
  // decay it to element const, references, etc.
  typedef typename boost::mpl::transform<fn_args_type_original, std::decay<_1>>::type fn_args_type;

  static_assert(size<fn_args_type>::value == NumInArgs + 1,
                "Invalid number arguments. #input != #function arguments.");
  typedef fn_args_type in_arg_types;
  // pad inargnames in front with one more element to cover for the "this" argument
  inargnames.insert(inargnames.begin(), "");

  auto fnwrapper =
      [member_fn, inargnames](T* t, variant_map_type args)->variant_type {
        turi::toolkit_function_response_type ret;
        // we need to fill the input arguments and output arguments
        typename boost::fusion::result_of::as_vector<in_arg_types>::type in_args;
        /*
         * Essentially:
         * inargs[0] = t
         * for i = 1 to #inargs - 1:
         *    inargs[i] = invoke->params[inargnames[i]]
         *
         * The fill_named_in_args struct performs the body of the for loop.
         */
        boost::fusion::at_c<0>(in_args) = t;
        fill_named_in_args<decltype(in_args)> in_arg_filler;
        in_arg_filler.params = &args;
        in_arg_filler.inargnames = inargnames;
        in_arg_filler.inargs = &in_args;
        typename make_range2<1, size<decltype(in_args)>::value>::type in_arg_range;
        boost::fusion::for_each(in_arg_range, in_arg_filler);

        // Invoke the function call, storing the return value
        result_of_function_wrapper<typename std::decay<Ret>::type> retval;
        retval.call([&](){
          return boost::fusion::invoke(member_fn, in_args);
        });
        if (retval.is_void) return to_variant(FLEX_UNDEFINED);
        else return to_variant(retval.ret_value);
      };
  return fnwrapper;
}


/**
 * Wraps a const member function T::f(...) const with a function that takes a
 * variant_map_type and returns a variant_type.
 * Essentially, given a function f of type ret(in1, in2, in3 ...),
 * returns a function g of type variant_type(variant_map_type) where g
 * performs the equivalent of:
 *
 * \code
 * variant_type g(T* t, variant_map_type input) {
 *   return variant_encode( t->f (variant_decode(input[inargnames[0]),
 *                             variant_decode(input[inargnames[1]),
 *                             variant_decode(input[inargnames[2]),
 *                             ...));
 * }
 *
 * \endcode
 *
 * \note Annoyingly we need a seperate specialization for this even
 * though the code is... *identical*. I can't figure out how to templatize
 * around the const.
 */
template <size_t NumInArgs, typename T, typename Ret, typename... Args>
std::function<variant_type(T*, variant_map_type)>
generate_const_member_function_wrapper(Ret (T::* fn)(Args...) const,
                                       std::vector<std::string> inargnames) {
  // import some mpl stuff we will be using
  using boost::mpl::erase;
  using boost::mpl::size;
  using boost::mpl::int_;
  using boost::mpl::_1;

  // the actual spec object we are returning
  toolkit_function_specification spec;
  // convert Ret (T::* fn)(Args...) to Ret fn(T*, Args...)
  auto member_fn = std::mem_fn(fn);
  // Get an mpl::vector containing all the arguments of the member function
  typedef typename boost::mpl::vector<T*, Args...>::type fn_args_type_original;
  // decay it to element const, references, etc.
  typedef typename boost::mpl::transform<fn_args_type_original, std::decay<_1>>::type fn_args_type;

  static_assert(size<fn_args_type>::value == NumInArgs + 1,
                "Invalid number arguments. #input != #function arguments.");
  typedef fn_args_type in_arg_types;
  // pad inargnames in front with one more element to cover for the "this" argument
  inargnames.insert(inargnames.begin(), "");

  auto fnwrapper =
      [member_fn, inargnames](T* t, variant_map_type args)->variant_type {
        turi::toolkit_function_response_type ret;
        // we need to fill the input arguments and output arguments
        typename boost::fusion::result_of::as_vector<in_arg_types>::type in_args;
        /*
         * Essentially:
         * inargs[0] = t
         * for i = 1 to #inargs - 1:
         *    inargs[i] = invoke->params[inargnames[i]]
         *
         * The fill_named_in_args struct performs the body of the for loop.
         */
        boost::fusion::at_c<0>(in_args) = t;
        fill_named_in_args<decltype(in_args)> in_arg_filler;
        in_arg_filler.params = &args;
        in_arg_filler.inargnames = inargnames;
        in_arg_filler.inargs = &in_args;
        typename make_range2<1, size<decltype(in_args)>::value>::type in_arg_range;
        boost::fusion::for_each(in_arg_range, in_arg_filler);

        // Invoke the function call, storing the return value
        result_of_function_wrapper<typename std::decay<Ret>::type> retval;
        retval.call([&](){
          return boost::fusion::invoke(member_fn, in_args);
        });
        if (retval.is_void) return to_variant(FLEX_UNDEFINED);
        else return to_variant(retval.ret_value);
      };
  return fnwrapper;
}


/**
 * Generates a toolkit specification object for a user defined function which
 * wraps the user defined function with a helper that provides type checking,
 * argument filling and exception handling.
 *
 * The basic model of toolkit function publishing is that the user
 * define a function of the form:
 * \code
 * toolkit_function_response_type user_function(toolkit_function_invocation& invoke)
 * \endcode
 * Where toolkit_function_invocation essentially stores a dictionary of input arguments,
 * and toolkit_function_response_type stores a dictionary of outputs.
 *
 * However, this can be quite difficult to use in practice, especially due to
 * dynamic typing which requires a lot of additional typechecking and validation
 * overhead.
 *
 * The basic idea behind this function is to allow the user can publish
 * arbitrary functions of the form:
 *
 * \code
 * return_type function_name(InArg1 arg1, InArg2 arg2...)
 * \endcode
 *
 * Note that all output argument must appear AFTER all the input arguments.
 *
 * Input argument types are one of:
 *  - flexible_type
 *  - unity_sarray*
 *  - unity_sframe*
 *  - unity_sgraph*
 *  - any type contained by flexible_type: i.e. flex_int, flex_vec, etc.
 *
 * The return type can similarly be any input argument type, or any type which
 * can be converted into a flexible_type.
 *
 * For instance
 * \code
 * size_t demo(flex_int arg1, unity_sarray* arg2) {
 *   ...
 * }
 * \endcode
 *
 * Then to publish it:
 * \code
 * toolkit_function_specification spec =
 *      make_spec<2>(demo, "demo", {"arg1name", "arg2name"});
 * \endcode
 *
 * Will return a toolkit specification object that publishes the user defined
 * function to Python under the name "fnname". And mapping the input
 * dictionary to the input arguments of the function using "inargnames"
 * and mapping output arguments to the output dictionary using outargnames.
 *
 * Essentially, with reference to the example demo function above, a helper
 * function is produced which performs the following, but with more error
 * checking and validation.
 * \code
 * toolkit_function_response_type helper(toolkit_function_invocation& invoke) {
 *    // perform the call
 *    flex_int arg1 =  invoke.params["arg1name"];
 *    unity_sarray* arg2 =  invoke.params["arg2name"];
 *    auto result = demo(arg1, arg2);
 *
 *    // generate response
 *    toolkit_function_response_type ret;
 *    ret.params["return_value"] = result;
 *    ret.success = true;
 *    return ret;
 * }
 * \endcode
 * This helper is then published under the name specified in fnname, and will be
 * callable from python via
 * \code
 * import turicreate.toolkits.main as main
 * ret = main.run("demo", {'arg1name':5, 'arg2name':array})
 * \endcode
 *
 *
 * \tparam NumInArgs The number of input arguments of the user function
 * \tparam Function The type of the function. Usually does not need to be
 *                  specified. This should be automatically inferable.
 *
 * \param fn Pointer to the user function
 * \param fnname The published name of the function. (what Python sees)
 * \param inargnames The published input argument names (what Python sees)
 */
template <size_t NumInArgs, typename Function>
toolkit_function_specification make_spec(Function fn, std::string fnname,
                                std::vector<std::string> inargnames) {
  // the actual spec object we are returning
  toolkit_function_specification spec;
  auto fnwrapper = generate_function_wrapper<NumInArgs, Function>(fn, inargnames);
  auto native_fn_wrapper = generate_native_function_wrapper<NumInArgs, Function>(fn);

  auto invoke_fn = [fnwrapper,inargnames](turi::toolkit_function_invocation& invoke)->turi::toolkit_function_response_type {
        turi::toolkit_function_response_type ret;
        // we are inside the actual toolkit call now.
        try {
          variant_type val = fnwrapper(invoke.params);
          ret.params["return_value"] = val;
          ret.success = true;
          // final bit of error handling in the case of exceptions
        } catch (std::string err) {
          ret.message = err;
          ret.success = false;
        } catch (const char* err) {
          ret.message = err;
          ret.success = false;
        } catch (std::exception& e) {
          ret.message = e.what();
          ret.success = false;
        } catch (...) {
          ret.message = "Unknown Exception";
          ret.success = false;
        }
        return ret;
      };

  // complete the function registration.
  // get the python exposed function name. Remove any namespacing is any.
  auto last_colon = fnname.find_last_of(":");
  if (last_colon == std::string::npos) spec.name = fnname;
  else spec.name = fnname.substr(last_colon + 1);

  // store the function
  spec.toolkit_execute_function = invoke_fn;
  spec.native_execute_function = native_fn_wrapper;
  spec.description["arguments"] =
      flexible_type_converter<decltype(inargnames)>().set(inargnames);
  spec.description["_raw_fn_pointer_"] = reinterpret_cast<size_t>(fn);

  return spec;
}



/**
 * Generates a toolkit specification object for a user defined function which
 * wraps the user defined function with a helper that provides type checking,
 * argument filling and exception handling.
 *
 * Performs exactly the same thing as make_spec, but takes the final inargnames
 * argument as a variable length argument.
 */
template <typename Function, typename... Args>
toolkit_function_specification make_spec_indirect(Function fn, std::string fnname,
                                         Args... args) {
  // Get a function traits object from the function type
  typedef boost::function_traits<typename std::remove_pointer<Function>::type> fntraits;
  static_assert(fntraits::arity == sizeof...(Args),
                "Incorrect number input parameter names specified.");
  return make_spec<sizeof...(Args)>(fn, fnname, {args...});
}



} // toolkit_function_wrapper_impl
} // turicreate

#endif
