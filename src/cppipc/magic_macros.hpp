/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_MAGIC_MACROS_HPP
#define CPPIPC_MAGIC_MACROS_HPP
#include <cppipc/ipc_object_base.hpp>
#include <cppipc/registration_macros.hpp>
#ifndef DISABLE_TURI_CPPIPC_PROXY_GENERATION
#include <cppipc/server/comm_server.hpp>
#include <cppipc/client/object_proxy.hpp>
#include <cppipc/cppipc.hpp>
#endif
#include <cppipc/common/ipc_deserializer.hpp>
#include <serialization/serialization_includes.hpp>
#include <boost/preprocessor.hpp>
#define __GENERATE_REGISTRATION__(r, base_name, elem) \
    REGISTER(base_name::BOOST_PP_TUPLE_ELEM(3,1,elem))


#define __GENERATE_PROXY_ARGS__(r, ni, i, elem) elem BOOST_PP_CAT(arg_,i) \
    BOOST_PP_COMMA_IF(BOOST_PP_GREATER(ni, BOOST_PP_ADD(i, 1)))

#define __GENERATE_PROXY_CALL_ARGS__(r, ni, i, elem) BOOST_PP_CAT(arg_,i) \
    BOOST_PP_COMMA_IF(BOOST_PP_GREATER(ni, BOOST_PP_ADD(i, 1)))


#define __GENERATE_BASE__(r, _, elem) \
    virtual BOOST_PP_TUPLE_ELEM(3, 0, elem)  \
            BOOST_PP_TUPLE_ELEM(3,1, elem) (  \
             BOOST_PP_SEQ_FOR_EACH_I(__GENERATE_PROXY_ARGS__,  \
                                     BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(3, 2,elem)), \
                                     BOOST_PP_TUPLE_ELEM(3,2, elem)) \
             ) = 0;

#define __GENERATE_PROXY_CALLS__(r, base_name, elem) \
    inline BOOST_PP_TUPLE_ELEM(3, 0, elem)  \
            BOOST_PP_TUPLE_ELEM(3,1, elem) \
            ( BOOST_PP_SEQ_FOR_EACH_I(__GENERATE_PROXY_ARGS__,  \
                                      BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(3, 2,elem)), \
                                      BOOST_PP_TUPLE_ELEM(3,2, elem)) ) { \
    return proxy.call(&base_name::BOOST_PP_TUPLE_ELEM(3,1,elem)  \
                      BOOST_PP_COMMA_IF(BOOST_PP_GREATER(BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(3,2,elem)),0))  \
                      BOOST_PP_SEQ_FOR_EACH_I(__GENERATE_PROXY_CALL_ARGS__,  \
                                              BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(3, 2,elem)), \
                                              BOOST_PP_TUPLE_ELEM(3, 2, elem)) \
            ); \
            }


#define __ADD_PAREN_1__(A, B, C) ((A, B, C)) __ADD_PAREN_2__
#define __ADD_PAREN_2__(A, B, C) ((A, B, C)) __ADD_PAREN_1__
#define __ADD_PAREN_1___END
#define __ADD_PAREN_2___END
#define __ADD_PARENS__(INPUT) BOOST_PP_CAT(__ADD_PAREN_1__ INPUT,_END)


/**
 * \ingroup cppipc
 * Magic Interface generating macro.
 * Like \ref GENERATE_INTERFACE_AND_PROXY but only generates the interface.
 *
 * To use, call with the 1st argument as the base name of the interface,
 * and the 2nd argument as a sequence of:
 * \code
 * (return_type, function_name, (arg1type)(arg2type)(arg3type))
 * \endcode
 * To get a function with no arguments simply have an empty 3rd argument. 
 * (the 3rd argument is still needed. Observe the comma. It is just empty.) 
 * \code
 * (return_type, function_name, )
 * \endcode
 * For instance, 
 * \code
 * GENERATE_INTERFACE(object_base, object_proxy,
 *                     (std::string, ping, (std::string))
 *                     (int, add_one, (int))
 *                     (int, add, (int)(int))
 *                   )
 * \endcode
 * will create a base class called object_base with 3 functions, ping, 
 * add_one, and add, with the appropriate registration functions.
 *
 * The above macro generates the following code:
 * \code
 * class object_base : public cppipc::ipc_object_base{
 *  public:
 *   typedef object_proxy proxy_object_type;
 *   virtual ~object_base() { }
 *   virtual std::string ping(std::string) = 0;
 *   virtual int add_one(int) = 0;
 *   virtual int add_one(int, int) = 0;
 * 
 *   virtual void save(turi::oarchive& oarc) const {}
 *   virtual void load(turi::iarchive& iarc) {}
 * 
 *   REGISTRATION_BEGIN(test_object_base)
 *   REGISTER(test_object_base::ping)
 *   REGISTER(test_object_base::add)
 *   REGISTER(test_object_base::add_one)
 *   REGISTRATION_END
 * };
 * \endcode
 */
#define GENERATE_INTERFACE(base_name, proxy_name, functions) \
    class base_name : public cppipc::ipc_object_base { \
     public: \
      typedef proxy_name proxy_object_type; \
      inline virtual ~base_name() { } \
      inline virtual void save(turi::oarchive& oarc) const {} \
      inline virtual void load(turi::iarchive& iarc) {}       \
       BOOST_PP_SEQ_FOR_EACH(__GENERATE_BASE__, _, __ADD_PARENS__(functions)) \
       REGISTRATION_BEGIN(base_name) \
       BOOST_PP_SEQ_FOR_EACH(__GENERATE_REGISTRATION__, base_name, __ADD_PARENS__(functions)) \
       REGISTRATION_END \
    }; 




/**
 * \ingroup cppipc
 * Magic proxy generating macro.
 *
 * Like \ref GENERATE_INTERFACE_AND_PROXY but only generates the proxy.
 *
 * To use, call with the 1st argument as the base name of the interface,
 * the 2nd argument as the name of the proxy class,
 * and the 3nd argument as a sequence of:
 * \code
 * (return_type, function_name, (arg1type)(arg2type)(arg3type))
 * \endcode
 * To get a function with no arguments simply have an empty 3rd argument. 
 * (the 3rd argument is still needed. Observe the comma. It is just empty.) 
 * \code
 * (return_type, function_name, )
 * \endcode
 *
 * For instance, 
 * \code
 * GENERATE_PROXY(object_base, object_proxy,
 *                               (std::string, ping, (std::string))
 *                               (int, add_one, (int))
 *                               (int, add, (int)(int))
 *                             )
 * \endcode
 * will create a base class called object_base with 3 functions, ping, 
 * add_one, and add, with the appropriate registration functions, as well as
 * a proxy object with the appropriate proxy forwarding calls.
 *
 * The above macro generates all of the following code:
 * \code
 * class object_proxy : public object_base { 
 *  public:
 *   cppipc::object_proxy<object_base> proxy;
 *
 *   inline test_object_proxy(cppipc::comm_client& comm, 
 *                            bool auto_create = true,
 *                            size_t object_id = (size_t)(-1)):
 *                                proxy(comm, auto_create, object_id){ } 
 *
 *   inline test_object_proxy(cppipc::comm_client& comm):proxy(comm){ } 
 *
 *   inline std::string ping(std::string arg_0) { 
 *     return proxy.call(&object_base::ping, arg_0) 
 *   } 
 *   inline int add_one(int arg_0) { 
 *     return proxy.call(&object_base::add_one, arg_0) 
 *   } 
 *   inline int add(int arg_0, int arg_1) { 
 *     return proxy.call(&object_base::add, arg_0 , arg_1);
 *   }
 *   inline void save(turi::oarchive& oarc) const {
 *     oarc << proxy.get_object_id();
 *   }
 *   inline void load(turi::iarchive& iarc) {
 *     size_t objid; iarc >> objid;
 *     proxy.set_object_id(objid);
 *   }
 * };
 * \endcode
 */
#define GENERATE_PROXY(base_name, proxy_name, functions) \
    class proxy_name : public base_name { \
     public: \
      cppipc::object_proxy<base_name> proxy; \
      inline proxy_name(cppipc::comm_client& comm,        \
                               bool auto_create = true,   \
                               size_t object_id = (size_t)(-1)): \
          proxy(comm, auto_create, object_id){ }        \
      inline void save(turi::oarchive& oarc) const {  \
        oarc << proxy.get_object_id();                    \
      }                                                   \
      inline size_t __get_object_id() const {               \
        return proxy.get_object_id();                     \
      }                                                   \
      inline void load(turi::iarchive& iarc) {        \
        size_t objid; iarc >> objid;                      \
        proxy.set_object_id(objid);                       \
      }                                                   \
      BOOST_PP_SEQ_FOR_EACH(__GENERATE_PROXY_CALLS__, base_name, __ADD_PARENS__(functions)) \
    };



/**
 * \ingroup cppipc
 * Magic Interface and proxy generating macro.
 *
 * To use, call with the 1st argument as the base name of the interface,
 * the 2nd argument as the name of the proxy class,
 * and the 3nd argument as a sequence of:
 * \code
 * (return_type, function_name, (arg1type)(arg2type)(arg3type))
 * \endcode
 * To get a function with no arguments simply have an empty 3rd argument. 
 * (the 3rd argument is still needed. Observe the comma. It is just empty.) 
 * \code
 * (return_type, function_name, )
 * \endcode
 *
 * For instance, 
 * \code
 * GENERATE_INTERFACE_AND_PROXY(object_base, object_proxy,
 *                               (std::string, ping, (std::string))
 *                               (int, add_one, (int))
 *                               (int, add, (int)(int))
 *                             )
 * \endcode
 * will create a base class called object_base with 3 functions, ping, 
 * add_one, and add, with the appropriate registration functions, as well as
 * a proxy object with the appropriate proxy forwarding calls.
 *
 * The above macro generates all of the following code:
 * \code
 * class proxy_object_type;
 * class object_base : public cppipc::ipc_object_base {
 *  public:
 *   typedef object_proxy proxy_object_type;
 *   virtual inline ~object_base() { }
 *   virtual std::string ping(std::string) = 0;
 *   virtual int add_one(int) = 0;
 *   virtual int add_one(int, int) = 0;
 * 
 *   virtual void save(turi::oarchive& oarc) const {}
 *   virtual void load(turi::iarchive& iarc) {}
 * 
 *   REGISTRATION_BEGIN(test_object_base)
 *   REGISTER(test_object_base::ping)
 *   REGISTER(test_object_base::add)
 *   REGISTER(test_object_base::add_one)
 *   REGISTRATION_END
 * };
 *
 * class object_proxy : public object_base { 
 *  public:
 *   cppipc::object_proxy<object_base> proxy;
 *
 *   inline test_object_proxy(cppipc::comm_client& comm, 
 *                            bool auto_create = true,
 *                            size_t object_id = (size_t)(-1)):
 *                                proxy(comm, auto_create, object_id){ } 
 *
 *   inline std::string ping(std::string arg_0) { 
 *     return proxy.call(&object_base::ping, arg_0) 
 *   } 
 *   inline int add_one(int arg_0) { 
 *     return proxy.call(&object_base::add_one, arg_0) 
 *   } 
 *   inline int add(int arg_0, int arg_1) { 
 *     return proxy.call(&object_base::add, arg_0 , arg_1);
 *   }
 *   inline void save(turi::oarchive& oarc) const {
 *     oarc << proxy.get_object_id();
 *   }
 *   inline void load(turi::iarchive& iarc) {
 *     size_t objid; iarc >> objid;
 *     proxy.set_object_id(objid);
 *   }
 * };
 * \endcode
 */

#ifdef DISABLE_TURI_CPPIPC_PROXY_GENERATION

#define GENERATE_INTERFACE_AND_PROXY(base_name, proxy_name, functions) \
    class proxy_name; \
    GENERATE_INTERFACE(base_name, proxy_name, functions)

#else

#define GENERATE_INTERFACE_AND_PROXY(base_name, proxy_name, functions) \
    class proxy_name; \
    GENERATE_INTERFACE(base_name, proxy_name, functions) \
    GENERATE_PROXY(base_name, proxy_name, functions) 

#endif

#endif
