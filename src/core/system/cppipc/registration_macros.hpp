/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_REGISTRATION_MACROS_HPP
#define CPPIPC_REGISTRATION_MACROS_HPP

/**
 * \ingroup cppipc
 * Macros which help member function registration for IPC objects.
 * For instance, given the following base class which we would like to export.
 * \code
 * class object_base: public cppipc::ipc_object_base {
 *  public:
 *   virtual ~object_base() { }
 *   virtual std::string ping(std::string) = 0;
 *   virtual int add_one(int) = 0;
 *   virtual int add_one(int, int) = 0;
 * };
 * \endcode
 *
 * We simply introduce a REGISTRATION_BEGIN inside the public section of the
 * object, and call REGISTER once for each member function to export.
 * The result object definition is as follows:
 * \code
 * class object_base {
 *  public:
 *   virtual ~object_base() { }
 *   virtual std::string ping(std::string) = 0;
 *   virtual int add_one(int) = 0;
 *   virtual int add_one(int, int) = 0;
 *
 *
 *   REGISTRATION_BEGIN(object_base) // argument is a name which must
 *                                   // uniquely identify the object
 *   REGISTER(object_base::ping)
 *   REGISTER(object_base::add)
 *   REGISTER(object_base::add_one)
 *   REGISTRATION_END
 * };
 * \endcode
 *
 * The registration macros adds 2 static functions. The first function is
 * \code
 *  static inline std::string __get_type_name__() {
 *    return name;
 *  }
 * \endcode
 * Where "name" is the argument provided to REGISTRATION_BEGIN macro. This
 * name is used for object creation and must uniquely identify the type of the
 * object.
 *
 * The second function is
 * \code
 * template <typename Registry>
 * static inline void __register__(Registry& reg) {
 *    reg.register_function(&object_base::ping, "object_base::ping");
 *    reg.register_function(&object_base::add, "object_base::add");
 *    reg.register_function(&object_base::add_one, "object_base::add_one");
 * }
 * \endcode
 * and is used by both the client side and the server side to identify and name
 * the member functions available.
 *
 * \see GENERATE_INTERFACE
 * \see GENERATE_INTERFACE_AND_PROXY
 */
#define REGISTRATION_BEGIN(name) \
    static inline std::string __get_type_name__() {  \
      return #name; \
    } \
    template <typename Registry> \
    static inline void __register__(Registry& reg) {

#define XSTRINGIFY(s) STRINGIFY(s)
#define STRINGIFY(s) #s

      /*
#define REGISTER(FN) { \
      int status; \
      char* demangled_name; \
      demangled_name = abi::__cxa_demangle(typeid(decltype(&FN)).name(), 0, 0, &status); \
      reg.register_function(&FN, std::string(XSTRINGIFY(FN)) + " " + demangled_name); \
      free(demangled_name); \
          }
*/
#define REGISTER(FN) { \
      reg.register_function(&FN, std::string(XSTRINGIFY(FN))); \
}



#define REGISTRATION_END }

#endif //CPPIPC_REGISTRATION_MACROS_HPP
