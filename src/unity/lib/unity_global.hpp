/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_GLOBAL_HPP
#define TURI_UNITY_GLOBAL_HPP

#include <unity/lib/unity_base_types.hpp>
#include <unity/lib/toolkit_function_specification.hpp>
#include <unity/lib/toolkit_function_registry.hpp>
#include <unity/lib/toolkit_class_registry.hpp>
#include <unity/lib/api/unity_global_interface.hpp>

namespace turi {

/**
 * \ingroup unity
 * The \ref turi::unity_global and \ref turi::unity_global_base classes
 * implement a singleton object on the server side which is exposed to the
 * client via the cppipc system. This singleton object provides other
 * miscellaneous uncategorized services (global functions) to the python layer.
 */
class unity_global: public unity_global_base {
 private:
  toolkit_function_registry* toolkit_functions;
  toolkit_class_registry* classes;

  struct so_registration_list {
    void* dl;
    std::string modulename; // base filename of the shared library excluding the extension
    std::string original_soname; // also the key in dynamic_loaded_toolkits
    std::string effective_soname;
    std::vector<std::string> functions;
    std::vector<std::string> classes;
  };
  // map of soname to registration
  std::map<std::string, so_registration_list> dynamic_loaded_toolkits;

  const char* OLD_CLASS_MAGIC_HEADER = "GLMODELX";
  const char* CLASS_MAGIC_HEADER = "TCMODEL0";

 public:
  /**
   * Constructor
   * \param reg Pointer to Toolkit registry. Since Unity Global manaages
   * toolkit execution
   * \param server Pointer to the comm server object. Toolkit execution status
   * will be emitted there.
   */
  unity_global(toolkit_function_registry* _toolkit_functions,
               toolkit_class_registry* _classes);

  virtual ~unity_global();

  /**
   * Get the version string
   */
  std::string get_version();

  /**
   * Constructs a graph from a binary file on disk, or HDFS
   */
  std::shared_ptr<unity_sgraph_base> load_graph(std::string fname);

  /**
   * Lists the names of all regsitered classes.
   */
  std::vector<std::string> list_toolkit_classes();

  /**
   * Load toolkit class from file.
   *
   * Returns variant_map varmap  
   *  - varmap['archive_version'] if 0, is the legacy version. 1 is the current version.
   * 
   * Archive version 1:
   *
   *  - varmap['model_name'] stores the C++ model name. Always available
   *  - varmap['model'] stores the unity toolkit class object pointer.
   *  May not be always available.
   *  - varmap['side_data'] stores a varmap of any additional side data 
   *    serialized along with the model. May not be always available.
   *
   *
   * Legacy:
   *  - varmap['model_wrapper'] stores the serialized lambda to construct the python class.
   *  - varmap['model_base'] stores the unity toolkit class object pointer.
   *
   * Throws an exception if there is an error reading the url or serializing to
   * a proper toolkit class object.
   */
  variant_map_type load_model(const std::string& url);

  /**
   * Save a toolkit class to file.
   *
   * \param tkclass The pointer to the actual toolkit class object.
   * \param sidedata Any additional side information
   * \param url The destination url to store the class.
   */
  void save_model(std::shared_ptr<model_base> tkclass,
                  const variant_map_type& sidedata, const std::string& url);

  /**
   * Saves a model using an alternative model saving method where a model class
   * is not available.
   *
   * \param model_name A unique string name of the model
   * \param sidedata Any additional side information
   * \param url The destination url to store the class.
   */
  void save_model2(const std::string& model_name,
                  const variant_map_type& sidedata, const std::string& url);

  /**
   * Lists the names of all registered toolkit functions.
   */
  std::vector<std::string> list_toolkit_functions();

  /**
   * Returns a dictionary describing the toolkit. It will return a dictionary
   * with 2 fields:
   *  - "name": The name of the toolkit
   *  - "arguments": The list of input parameters
   *  - "documentation"
   */
  std::map<std::string, flexible_type> describe_toolkit_function(std::string toolkitname);


  /**
   * Returns a dictionary describing the class. It will return a dictionary
   * with 2 fields:
   *  - "name": The name of the toolkit
   *  - "functions": A dictionary with key: function name, and value,
   *                     a list of input parameters.
   *  - "get_properties": The list of all readable properties of the class
   *  - "set_properties": The list of all writable properties of the class
   *  - "documentation"
   */
  std::map<std::string, flexible_type> describe_toolkit_class(std::string class_name);


  /**
   * Creates a class instance.
   */
  std::shared_ptr<model_base> create_toolkit_class(std::string class_name);

  /**
   * Runs a toolkit of the specified name, and with the specified arguments.
   * Returns a toolkit_function_response_type which contains the result of the toolkit
   * execution (success/failure) as well as any additional returned state
   * (graphs/classes/etc). Will throw an exception if the toolkit name was not
   * found.
   */
  toolkit_function_response_type run_toolkit(std::string toolkit_name,
                                    variant_map_type& arguments);

  /**
   * Internal utility function. Gets the structure of the lazy
   * evaluation dag for the graph operations.
   */
  std::string get_graph_dag();

  /**
   * Evaluate a pickled python lambda with the given argument.
   */
  flexible_type eval_lambda(const std::string& pylambda_string, const flexible_type& arg);

  /**
   * Evaluate a pickled python lambda with dictionary argument.
   */
  flexible_type eval_dict_lambda(const std::string& pylambda_string,
                            const std::vector<std::string>& keys,
                            const std::vector<flexible_type>& args);

  /**
   * Evaluate a pickled python lambda on a list of argument in parallel.
   */
  std::vector<flexible_type> parallel_eval_lambda(const std::string& pylambda_string,
                                                  const std::vector<flexible_type>& arg);

  /**
   * \internal
   * Reads the content of the given url.
   *
   * Return a string containing the content of the given url.
   *
   * Throws exception if IO error occurs.
   *
   * \note This function should only be used for internal testing.
   */
  std::string __read__(const std::string& url);

  /**
   * \internal
   * Writes the content of to the given url.
   *
   * Throws exception if IO error occurs.
   *
   * \note This function should only be used for internal testing.
   */
  void __write__(const std::string& url, const std::string& content);

  /**
   * \internal
   * Creates a directory that will have the given url.
   *
   * Throws exception if directory already exists.
   */
  bool __mkdir__(const std::string& url);

  /**
   * \internal
   * Changes permissions of the given url.
   */
  bool __chmod__(const std::string& url, short mode);

  /**
   * \internal
   * Returns the size of the process heap.
   * May not be available. Returns 0 if unavailable.
   */
  size_t __get_heap_size__();


  /**
   * \internal
   * Returns the amount of memory used inside the heap.
   * May not be available. Returns 0 if unavailable.
   */
  size_t __get_allocated_size__();

  /**
   * \internal
   * Sets the logging level
   */
  void set_log_level(size_t);

  /**
   * \internal
   * Lists all the global configuration values. If runtime_modifiable == true,
   * lists all global values which can be modified at runtime. If runtime ==
   * false, lists all global values which can only be modified by environment
   * variables.
   */
  std::map<std::string, flexible_type> list_globals(bool runtime_modifiable);


  /**
   * \internal
   * Sets a modifiable global configuration value. Returns an empty string
   * on success and an error string on failure.
   */
  std::string set_global(std::string key, flexible_type value);

  /**
   * \internal
   * Create a sequentially increasing (or decreasing) SArray.
   *
   * If 'reverse' is true, counts down instead of up.
   */
  std::shared_ptr<unity_sarray_base> create_sequential_sarray(ssize_t size, ssize_t start, bool reverse);

  /**
   * Attempts to load a toolkit from a shared library.
   * Returns an empty string on success.
   * An string describing the error on failure.
   *
   * The so will be loaded with prefix [module_subpath].[filename].[...]
   * For instance: if the so is called "example.so" containing a function
   * square_root.
   * \code
   *   load_toolkit("example.so", "")
   * \endcode
   * will load the toolkit function into "example.square_root".
   * (it will also appear in tc.extensions.example.square_root)
   *
   * \code
   *   load_toolkit("example.so", "pika")
   * \endcode
   * will load the toolkit function "pika.example.square_root".
   * (it will also appear in tc.extensions.pika.example.square_root)
   *
   * module_subpath can also be ".."
   * \code
   *   load_toolkit("example.so", "..")
   * \endcode
   * In which case it will appear anywhere except in the top level of
   * tc.extensions as tc.extensions.square_root.
   */
  std::string load_toolkit(std::string soname,
                           std::string module_subpath);

  /**
   * Lists all the functions in a toolkit. Raises an exception if the toolkit
   * was not previously loaded by load_toolkit.
   */
  std::vector<std::string> list_toolkit_functions_in_dynamic_module(std::string soname);

  /**
   * Lists all the classes in a toolkit. Raises an exception if the toolkit
   * was not previously loaded by load_toolkit.
   */
  std::vector<std::string> list_toolkit_classes_in_dynamic_module(std::string soname);
  /**
   * \internal
   * Retrieve the folder that is currently being used to hold temp files.
   *
   * This is where all the SFrame files and such are located.
   */
  std::string get_current_cache_file_location();

  /**
   * Returns a pointer to the toolkit function registry
   */
  toolkit_function_registry* get_toolkit_function_registry();

  /**
   * Returns a pointer to the toolkit class registry
   */
  toolkit_class_registry* get_toolkit_class_registry();


  /**
   * Given a url, returns the type of the Turi object, return value could be:
   * model, graph, sframe, sarray
   */
  std::string get_turicreate_object_type(const std::string& url);


  /**
   * A alternate implementation of var which knows how to 
   * save models. Models are special because they rely on the unity_global
   * registry. Fully compatible with variant_deep_save otherwise.
   * \see variant_deep_save
   */
  void model_variant_deep_save(const variant_type& var, oarchive& oarc);

  /**
   * A alternate implementation of variant_deep_load which knows how to 
   * save models. Models are special because they rely on the unity_global
   * registry. Fully compatible with variant_deep_load otherwise.

   * \see variant_deep_load
   */
  void model_variant_deep_load(variant_type& var, iarchive& iarc);
};
}
#endif
