/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef _WIN32
#include <dlfcn.h>
#else
#include <cross_platform/windows_wrapper.hpp>
#endif

#include <core/util/syserr_reporting.hpp>
#include <core/data/image/io.hpp>
#include <core/logging/logger.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/searching/boyer_moore.hpp>
#include <boost/filesystem/path.hpp>
#include <core/parallel/lambda_omp.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#ifdef TC_HAS_PYTHON
#include <core/system/lambda/lambda_master.hpp>
#endif
#include <model_server/lib/version.hpp>
#include <model_server/lib/unity_global.hpp>
#include <perf/memory_info.hpp>
#include <core/globals/globals.hpp>
#include <core/storage/sframe_interface/unity_sgraph.hpp>
#include <core/storage/sframe_interface/unity_sarray.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <model_server/lib/sdk_registration_function_types.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <core/storage/lazy_eval/lazy_eval_operation_dag.hpp>

namespace turi {

  unity_global::unity_global(toolkit_function_registry* _toolkit_functions,
                             toolkit_class_registry* _classes)
      :toolkit_functions(_toolkit_functions), classes(_classes) {
    log_func_entry();
  }

  unity_global::~unity_global() {}

  std::string unity_global::get_version() {
    return UNITY_VERSION;
  }

  std::string unity_global::get_graph_dag() {
    std::stringstream strm;
    unity_sgraph::get_dag()->print(strm);
    return strm.str();
  }

  std::shared_ptr<unity_sgraph_base> unity_global::load_graph(std::string fname) {
    log_func_entry();
    std::shared_ptr<unity_sgraph> g(new unity_sgraph());
    try {
      g->load_graph(fname);
    } catch (...) {
      throw;
    }
    return g;
  }

  std::string unity_global::get_turicreate_object_type(const std::string& url) {
    logstream(LOG_INFO) << "Getting turicreate object type stored at: " << sanitize_url(url) << std::endl;

    // valid values are: model, graph. sframe, sarray
    return dir_archive::get_directory_metadata(url, "contents");
  }

  std::vector<std::string> unity_global::list_toolkit_classes() {
    return classes->available_toolkit_classes();
  }

  void unity_global::model_variant_deep_save(const variant_type& v, oarchive& oarc) {
    oarc << v.which();
    switch(v.which()) {
     case 0:
       oarc << boost::get<flexible_type>(v);
       break;
     case 1:
       {
         std::shared_ptr<unity_sgraph> g =
             std::static_pointer_cast<unity_sgraph>(variant_get_ref<std::shared_ptr<unity_sgraph_base>>(v));
         oarc << *g;
         break;
       }
     case 2:
       oarc << boost::get<dataframe_t>(v);
       break;
     case 3:
       {
         auto model_ptr = boost::get<std::shared_ptr<model_base>>(v);
         oarc << std::string(model_ptr->name());
         oarc << (*model_ptr);
       }
       break;
     case 4:
       {
         std::shared_ptr<unity_sframe> s =
             std::static_pointer_cast<unity_sframe>(variant_get_ref<std::shared_ptr<unity_sframe_base>>(v));
         oarc << *s;
         break;
       }
       break;
     case 5:
       {
         std::shared_ptr<unity_sarray> s =
             std::static_pointer_cast<unity_sarray>(variant_get_ref<std::shared_ptr<unity_sarray_base>>(v));
         oarc << *s;
         break;
       }
       break;
     case 6:
       {
         const variant_map_type& varmap = variant_get_ref<variant_map_type>(v);
         oarc << (size_t)varmap.size();
         for(const auto& elem : varmap) {
           oarc << elem.first;
           model_variant_deep_save(elem.second, oarc);
         }
         break;
       }
     case 7:
       {
         const variant_vector_type& varvec = variant_get_ref<variant_vector_type>(v);
         oarc << (size_t)varvec.size();
         for(const auto& elem : varvec) {
           model_variant_deep_save(elem, oarc);
         }
         break;
       }
     default:
       break;
    };
  }
  void unity_global::model_variant_deep_load(variant_type& v, iarchive& iarc) {
    int which;
    iarc >> which;
    switch(which) {
     case 0:
       {
         v = flexible_type();
         iarc >> boost::get<flexible_type>(v);
         break;
       }
     case 1:
       {
         std::shared_ptr<unity_sgraph> g(new unity_sgraph());
         iarc >> *g;
         variant_set_value<std::shared_ptr<unity_sgraph>>(v, g);
         break;
       }
     case 2:
       {
         v = dataframe_t();
         iarc >> boost::get<dataframe_t>(v);
         break;
       }
     case 3:
       {
         std::string model_name;
         iarc >> model_name;
         std::shared_ptr<model_base> model_ptr = classes->get_toolkit_class(model_name);
         iarc >> *model_ptr;
         v = model_ptr;
       }
       break;
     case 4:
       {
         std::shared_ptr<unity_sframe> s(new unity_sframe());
         iarc >> *s;
         variant_set_value<std::shared_ptr<unity_sframe>>(v, s);
         break;
       }
     case 5:
       {
         std::shared_ptr<unity_sarray> s(new unity_sarray());
         iarc >> *s;
         variant_set_value<std::shared_ptr<unity_sarray>>(v, s);
         break;
       }
     case 6:
       {
         size_t numvals;
         iarc >> numvals;
         variant_map_type varmap;
         for (size_t i = 0;i < numvals; ++i) {
           std::string key;
           variant_type value;
           iarc >> key;
           model_variant_deep_load(value, iarc);
           varmap[key] = std::move(value);
         }
         variant_set_value<variant_map_type>(v, varmap);
         break;
       }
     case 7:
       {
         size_t numvals;
         iarc >> numvals;
         variant_vector_type varvec;
         varvec.resize(numvals);
         for (size_t i = 0;i < numvals; ++i) {
           variant_type value;
           model_variant_deep_load(value, iarc);
           varvec[i] = std::move(value);
         }
         variant_set_value<variant_vector_type>(v, varvec);
         break;
       }
     default:
       break;
    }
  }

  variant_map_type unity_global::load_model_impl(turi::iarchive& iarc, bool include_data) {
    char buf[256] = "";
    size_t magic_header_size = strlen(CLASS_MAGIC_HEADER);
    iarc.read(buf, magic_header_size);
    if (strcmp(buf, OLD_CLASS_MAGIC_HEADER) == 0) {
      // legacy loader
      std::string model_name;
      std::string model_wrapper;
      iarc >> model_name;
      logstream(LOG_INFO) << "Model name: " << model_name << std::endl;
      iarc >> model_wrapper;
      std::shared_ptr<model_base> model_ptr = classes->get_toolkit_class(model_name);
      iarc  >> *(model_ptr);
      if (iarc.fail()) {
        std::string message = "Fail to read.";
        log_and_throw_io_failure(message);
      }

      // fill the return values
      variant_map_type ret;
      variant_set_value<flexible_type>(ret["archive_version"], 0);
      variant_set_value<std::shared_ptr<model_base>>(ret["model_base"], model_ptr);
      flexible_type flex_model_wrapper = (flexible_type)model_wrapper;
      variant_set_value<flexible_type>(ret["model_wrapper"], flex_model_wrapper);
      variant_set_value<flexible_type>(ret["model_name"], flexible_type(model_name));
      return ret;
    } else if (strcmp(buf, CLASS_MAGIC_HEADER) == 0) {
      // new loader
      std::string model_name;
      iarc >> model_name;
      variant_type var;
      model_variant_deep_load(var, iarc);
      variant_map_type ret;
      if (include_data) {
        DASSERT_TRUE(variant_is<variant_map_type>(var));
        ret = variant_get_value<variant_map_type>(var);
      } else {
        DASSERT_TRUE(variant_is<std::shared_ptr<model_base> >(var));
        ret["model"] = var;
      }
      variant_set_value<flexible_type>(ret["archive_version"], 1);
      variant_set_value<flexible_type>(ret["model_name"], flexible_type(model_name));
      if (iarc.fail()) {
        std::string message = "Fail to read.";
        log_and_throw_io_failure(message);
      }
      return ret;
    } else {
      log_and_throw(std::string("Invalid model file."));
    }

  }

  variant_map_type unity_global::load_model(const std::string& url) {
    logstream(LOG_INFO) << "Load model from " << sanitize_url(url) << std::endl;
    try {
      dir_archive dir;
      dir.open_directory_for_read(url);
      std::string contents;
      if (dir.get_metadata("contents", contents) == false || contents != "model") {
        log_and_throw(std::string("Archive does not contain a model."));
      }
      iarchive iarc(dir);
      return load_model_impl(iarc, true /* include_data */);
    } catch (std::ios_base::failure& e) {
      std::string message = "Unable to load model from " + sanitize_url(url) + ": " + e.what();
      log_and_throw_io_failure(message);
    } catch (std::string& e) {
      log_and_throw(std::string("Unable to load model from ") + sanitize_url(url) + ": " + e);
    } catch (const std::exception& e) {
      log_and_throw(std::string("Unable to load model from ") + sanitize_url(url) + ": " + e.what());
    } catch (...) {
      log_and_throw(std::string("Unknown Error: Unable to load model from ") + sanitize_url(url));
    }
  }

  variant_map_type unity_global::load_model_from_data(std::istream& data) {
    logstream(LOG_INFO) << "Load model from data" << std::endl;
    try {
      iarchive iarc(data);
      // include_data is false, because data (SFrame/SArray) can't be serialized
      // as data (bytes). Requires a dir_archive with real filesystem.
      return load_model_impl(iarc, false /* include_data */);
    } catch (std::ios_base::failure& e) {
      std::string message = "Unable to load model from data: ";
      message += e.what();
      log_and_throw_io_failure(message);
    } catch (std::string& e) {
      log_and_throw(std::string("Unable to load model from data: ") + e);
    } catch (const std::exception& e) {
      log_and_throw(std::string("Unable to load model from data: ") + e.what());
    } catch (...) {
      log_and_throw(std::string("Unknown Error: Unable to load model from data."));
    }
  }

  void unity_global::save_model(std::shared_ptr<model_base> model,
                                const variant_map_type& side_data,
                                const std::string& url) {
    logstream(LOG_INFO) << "Save model to " << sanitize_url(url) << std::endl;
    logstream(LOG_INFO) << "Model name: " << model->name() << std::endl;
    try {
      dir_archive dir;
      dir.open_directory_for_write(url);
      dir.set_metadata("contents", "model");
      // prepare the set of fields to store
      variant_map_type stored_map;
      variant_set_value<variant_map_type>(stored_map["side_data"], side_data);
      variant_set_value<std::shared_ptr<model_base> >(stored_map["model"], model);
      // write to the archive. write a header, then the model name, then the map
      oarchive oarc(dir);
      oarc.write(CLASS_MAGIC_HEADER, strlen(CLASS_MAGIC_HEADER));
      oarc << std::string(model->name());
      model_variant_deep_save(to_variant(stored_map), oarc);

      if (dir.get_output_stream()->fail()) {
        std::string message = "Fail to write.";
        log_and_throw_io_failure(message);
      }
      dir.close();
    } catch (turi::error::io_error&) {
      // already in a turi error message; just re-throw it
      throw;
    } catch (std::ios_base::failure& e) {
      std::string message = "Unable to save model to " + sanitize_url(url) + ": " + e.what();
      log_and_throw_io_failure(message);
    } catch (std::string& e) {
      log_and_throw(std::string("Unable to save model to ") + sanitize_url(url) + ": " + e);
    } catch (...) {
      log_and_throw(std::string("Unknown Error: Unable to save model to ") + sanitize_url(url));
    }
  }

  void unity_global::save_model_to_data(std::shared_ptr<model_base> model, std::ostream& out) {
    logstream(LOG_INFO) << "Save model to data" << std::endl;
    logstream(LOG_INFO) << "Model name: " << model->name() << std::endl;
    try {
      oarchive oarc(out);
      // write to the archive. write a header, then the model name, then the map
      oarc.write(CLASS_MAGIC_HEADER, strlen(CLASS_MAGIC_HEADER));
      oarc << std::string(model->name());
      model_variant_deep_save(to_variant(model), oarc);

      if (oarc.fail()) {
        std::string message = "Fail to write.";
        log_and_throw_io_failure(message);
      }
    } catch (turi::error::io_error&) {
      // already in a turi error message; just re-throw it
      throw;
    } catch (std::ios_base::failure& e) {
      std::string message = std::string("Unable to save model to data: ") + e.what();
      log_and_throw_io_failure(message);
    } catch (std::string& e) {
      log_and_throw(std::string("Unable to save model to data: ") + e);
    } catch (...) {
      log_and_throw(std::string("Unknown Error: Unable to save model to data"));
    }
  }

  void unity_global::save_model2(const std::string& modelname,
                                const variant_map_type& side_data,
                                const std::string& url) {
    logstream(LOG_INFO) << "Save model to " << sanitize_url(url) << std::endl;
    logstream(LOG_INFO) << "Model name: " << modelname << std::endl;
    try {
      dir_archive dir;
      dir.open_directory_for_write(url);
      dir.set_metadata("contents", "model");
      // prepare the set of fields to store
      variant_map_type stored_map;
      variant_set_value<variant_map_type>(stored_map["side_data"], side_data);
      // write to the archive. write a header, then the model name, then the map
      oarchive oarc(dir);
      oarc.write(CLASS_MAGIC_HEADER, strlen(CLASS_MAGIC_HEADER));
      oarc << modelname;
      model_variant_deep_save(to_variant(stored_map), oarc);

      if (dir.get_output_stream()->fail()) {
        std::string message = "Fail to write.";
        log_and_throw_io_failure(message);
      }
      dir.close();
    } catch (turi::error::io_error& e) {
      // already in a turi error message; just re-throw it
      throw;
    } catch (std::ios_base::failure& e) {
      std::string message = "Unable to save model to " + sanitize_url(url) + ": " + e.what();
      log_and_throw_io_failure(message);
    } catch (std::string& e) {
      log_and_throw(std::string("Unable to save model to ") + sanitize_url(url) + ": " + e);
    } catch (...) {
      log_and_throw(std::string("Unknown Error: Unable to save model to ") + sanitize_url(url));
    }
  }

  std::vector<std::string> unity_global::list_toolkit_functions() {
    return toolkit_functions->available_toolkit_functions();
  }

  std::map<std::string, flexible_type> unity_global::describe_toolkit_function(std::string name) {
    auto spec = toolkit_functions->get_toolkit_function_info(name);
    if (spec == NULL) {
      log_and_throw(std::string("No such toolkit!"));
    } else {
      return spec->description;
    }
  }

  std::map<std::string, flexible_type> unity_global::describe_toolkit_class(std::string name) {
    return classes->get_toolkit_class_description(name);
  }

  std::shared_ptr<model_base> unity_global::create_toolkit_class(std::string name) {
    return classes->get_toolkit_class(name);
  }

  toolkit_function_response_type unity_global::run_toolkit (
      std::string toolkit_name,
      variant_map_type& variant_map){

    logstream(LOG_INFO) << "Running toolkit: " << toolkit_name << std::endl;

    const toolkit_function_specification* spec = toolkit_functions->get_toolkit_function_info(toolkit_name);

    if (spec == NULL) {
      throw std::string("Toolkit not found");
    }

    toolkit_function_invocation invocation;

    invocation.classes = classes;

    invocation.progress = [=](std::string s){
                            logger(LOG_WARNING, "Invoke.progress deprecated");
                          };
    invocation.params = std::move(variant_map);

    // set default options
    for(const auto& opt: spec->default_options) {
      if (invocation.params.count(opt.first) == 0) {
        invocation.params[opt.first] = opt.second;
      }
    }

    try {
      return spec->toolkit_execute_function(invocation);
    } catch (std::string s) {
      toolkit_function_response_type ret;
      ret.success = false;
      ret.message = s;
      return ret;
    } catch (const char* s) {
      toolkit_function_response_type ret;
      ret.success = false;
      ret.message = s;
      return ret;
    }
  }

  flexible_type unity_global::eval_lambda(const std::string& string, const flexible_type& arg) {
    log_func_entry();
#ifdef TC_HAS_PYTHON
    lambda::lambda_master& evaluator = lambda::lambda_master::get_instance();
    auto lambda_hash = evaluator.make_lambda(string);
    std::vector<flexible_type> return_val;
    evaluator.bulk_eval(lambda_hash, {arg}, return_val, false, 0);
    evaluator.release_lambda(lambda_hash);
    return return_val[0];
#else
    log_and_throw("Python lambdas not supported");
#endif
  }

  flexible_type unity_global::eval_dict_lambda(const std::string& lambda_string,
                            const std::vector<std::string>& keys,
                            const std::vector<flexible_type>& values) {
    log_func_entry();
#ifdef TC_HAS_PYTHON
    lambda::lambda_master& evaluator = lambda::lambda_master::get_instance();
    auto lambda_hash = evaluator.make_lambda(lambda_string);
    std::vector<flexible_type> return_val;
    evaluator.bulk_eval(lambda_hash, keys, {values}, return_val, false, 0);
    evaluator.release_lambda(lambda_hash);
    return return_val[0];
#else
    log_and_throw("Python lambdas not supported");
#endif
  }

  std::vector<flexible_type> unity_global::parallel_eval_lambda(const std::string& string, const std::vector<flexible_type>& arg) {
    log_func_entry();
#ifdef TC_HAS_PYTHON
    lambda::lambda_master& evaluator = lambda::lambda_master::get_instance();
    auto lambda_hash = evaluator.make_lambda(string);

    std::vector<flexible_type> ret(arg.size());
    ret.reserve(arg.size());
    parallel_for (0, arg.size(), [&](size_t i) {
      std::vector<flexible_type> out;
      evaluator.bulk_eval(lambda_hash, {arg[i]}, out, false, 0);
      ret[i] = out[0];
    });

    evaluator.release_lambda(lambda_hash);
    return ret;
#else
    log_and_throw("Python lambdas not supported");
#endif
  }

  std::string unity_global::__read__(const std::string& url) {
    general_ifstream fin(url);
    if (!fin.good()) {
      fin.close();
      log_and_throw_io_failure(std::string("Cannot open " + sanitize_url(url)));
    }

    std::stringstream ss;
    char* buf = new char[4096];
    while(fin.good()) {
      fin.read(buf, 4096);
      size_t bytes_read = fin.gcount();
      ss.write(buf, bytes_read);
    }

    delete[] buf;

    if (!fin.eof()) {
      fin.close();
      log_and_throw_io_failure(std::string("Read fail " + sanitize_url(url)));
    }

    fin.close();
    return ss.str();
  }

  void unity_global::__write__(const std::string& url, const std::string& content) {
    general_ofstream fout(url);
    if (!fout.good()) {
      fout.close();
      log_and_throw_io_failure(std::string("Cannot open " + sanitize_url(url)));
    }
    fout << content;
    fout.close();
  }

  bool unity_global::__mkdir__(const std::string& url) {
    return fileio::create_directory(url);
  }

  bool unity_global::__chmod__(const std::string& url, short mode) {
    std::cerr << "The mode: " << mode << std::endl;
    return fileio::change_file_mode(url, mode);
  }

  size_t unity_global::__get_heap_size__() {
    return memory_info::heap_bytes();
  }


  size_t unity_global::__get_allocated_size__() {
    return memory_info::allocated_bytes();
  }

  void unity_global::set_log_level(size_t level) {
    if (level <= 8) global_logger().set_log_level(level);
  }

  std::map<std::string, flexible_type> unity_global::list_globals(bool runtime_modifiable) {
    auto ret = globals::list_globals(runtime_modifiable);
    return std::map<std::string, flexible_type>(ret.begin(), ret.end());
  }

  std::string unity_global::set_global(std::string key, flexible_type val) {
    auto error_code = globals::set_global(key, val);
    switch(error_code) {
     case globals::set_global_error_codes::SUCCESS:
       return "";
     case globals::set_global_error_codes::NO_NAME:
       return "No such configuration variable";
     case globals::set_global_error_codes::NOT_RUNTIME_MODIFIABLE:
       return "Configuration variable " + key + " is not modifiable at runtime."
           "It can only be modified at start up by an environment variable";
     case globals::set_global_error_codes::INVALID_VAL:
       return "Invalid value";
     default:
       return "Unexpected failure";
    }
  }

  std::shared_ptr<unity_sarray_base> unity_global::create_sequential_sarray(ssize_t size, ssize_t start, bool reverse) {
    return unity_sarray::create_sequential_sarray(size, start, reverse);
  }

  static bool file_contains_substring(std::string file,
                                      std::string substring) {
    general_ifstream fin(file);
    if (fin.fail()) {
      log_and_throw("Cannot open " + file);
    }
    size_t fsize = fin.file_size();
    if (fsize == (size_t)(-1)) {
      log_and_throw("Cannot open " + file);
    }
    char* buf = new char[fsize];
    fin.read(buf, fsize);
    auto f = boost::algorithm::boyer_moore_search(buf, buf + fsize,
                                                  substring.begin(), substring.end());
    // return is true if found
#if BOOST_VERSION >= 106100
    bool ret = (f.first != buf + fsize);
#else
    bool ret = (f != buf + fsize);
#endif
    delete [] buf;
    return ret;
  }

  std::string unity_global::load_toolkit(std::string soname,
                                         std::string module_subpath) {
    // rewrite "local" protocol
    std::string protocol = fileio::get_protocol(soname);
    if (protocol == "local") {
      soname = fileio::remove_protocol(soname);
    }

    so_registration_list regentry;
    regentry.original_soname = soname;
    logstream(LOG_INFO) << "Attempt loading of " << sanitize_url(soname) << std::endl;

    // see if the file exists and whether we need to donwnload it
    if (fileio::try_to_open_file(soname) == false) {
      return "Unable to open file " + sanitize_url(soname);
    }

    if (protocol != "") {
      // there is a protocol associated. We need to copy this file to local
      // issue a copy to copy it to the local temp directory
      std::string tempname = get_temp_name();
      fileio::copy(soname, tempname);
      soname = tempname;
    }
    if (!file_contains_substring(soname, "get_toolkit_function_registration") &&
        !file_contains_substring(soname, "get_toolkit_class_registration")) {
      return soname + " is not a valid extension";
    }



    // get the base name of the shared library (without the .so)
    std::string modulename = fileio::get_filename(regentry.original_soname);
    std::vector<std::string> split_names;
    boost::algorithm::split(split_names, modulename, boost::is_any_of("."));
    if (split_names.size() == 0) return "Invalid filename";
    if (module_subpath.empty()) {
      regentry.modulename = split_names[0];
    } else if (module_subpath == "..") {
      regentry.modulename = "";
    } else {
      regentry.modulename = module_subpath + "." + split_names[0];
    }

    // goody. now for the dl loading
#ifndef _WIN32
    void* dl = dlopen(soname.c_str(), RTLD_NOW | RTLD_LOCAL);
#else
    void *dl = (void *)LoadLibrary(soname.c_str());
#endif
    logstream(LOG_INFO) << "Library load of " << sanitize_url(soname) << std::endl;
    regentry.effective_soname = soname;
    regentry.dl = dl;
    // check for failure
    if (dl == NULL) {
#ifndef _WIN32
      char* err = dlerror();
      // I think we need to copy this out early
      std::string ret = err;
      logstream(LOG_ERROR) << "Unable to load " << sanitize_url(soname) << ": " << ret << std::endl;
      if (err) return ret;
      else return "dlopen failed due to an unknown error";
#else
      std::string ret = get_last_err_str(GetLastError());
      logstream(LOG_ERROR) << "Unable to load " << sanitize_url(soname) << ": " << ret << std::endl;
      if (!ret.empty()) return ret;
      else return "LoadLibrary failed due to an unknown error";
#endif
    }

  /**************************************************************************/
  /*                                                                        */
  /*                         Function Registration                          */
  /*                                                                        */
  /**************************************************************************/
    // get the registration symbols
    std::vector<std::string> toolkit_function_reg_names
                {"get_toolkit_function_registration",
                  "_Z33get_toolkit_function_registrationv",
                  "__Z33get_toolkit_function_registrationv"};

    get_toolkit_function_registration_type get_toolkit_function_registration = nullptr;
    for (auto reg_name : toolkit_function_reg_names) {
      get_toolkit_function_registration =
          reinterpret_cast<get_toolkit_function_registration_type>
          (
#ifndef _WIN32
           dlsym(dl, reg_name.c_str())
#else
           (void *)GetProcAddress((HMODULE)dl, reg_name.c_str())
#endif
           );
      if (get_toolkit_function_registration != nullptr) break;
    }

    // register functions
    if (get_toolkit_function_registration) {
      auto functions = (*get_toolkit_function_registration)();
      for (auto& fn: functions) {
        if (!regentry.modulename.empty()) {
          fn.name = regentry.modulename + "." + fn.name;
        }
        fn.description["file"] = regentry.original_soname;
        logstream(LOG_INFO) << "Adding function: " << fn.name << std::endl;
        regentry.functions.push_back(fn.name);
      }
      toolkit_functions->register_toolkit_function(functions);
    }

/**************************************************************************/
/*                                                                        */
/*                           Class Registration                           */
/*                                                                        */
/**************************************************************************/

    std::vector<std::string> toolkit_class_reg_names
                {"get_toolkit_class_registration",
                 "_Z30get_toolkit_class_registrationv",
                 "__Z30get_toolkit_class_registrationv"};
    get_toolkit_class_registration_type get_toolkit_class_registration = nullptr;
    for (auto reg_name : toolkit_class_reg_names) {
      get_toolkit_class_registration =
          reinterpret_cast<get_toolkit_class_registration_type>
          (
#ifndef _WIN32
           dlsym(dl, reg_name.c_str())
#else
           (void *)GetProcAddress((HMODULE)dl, reg_name.c_str())
#endif
           );
      if (get_toolkit_class_registration != nullptr) break;
    }

    // register classes
    if (get_toolkit_class_registration) {
      auto class_reg = (*get_toolkit_class_registration)();
      for (auto& cl: class_reg) {
        if (!regentry.modulename.empty()) {
          cl.name = regentry.modulename + "." + cl.name;
        }
        cl.description["file"] = regentry.original_soname;
        logstream(LOG_INFO) << "Adding class : " << cl.name << std::endl;
        regentry.functions.push_back(cl.name);
      }
      classes->register_toolkit_class(class_reg);
    }


    if (regentry.functions.empty() && regentry.classes.empty()) {
      // nothing has been registered! unload the dl
#ifndef _WIN32
      dlclose(dl);
#else
      FreeLibrary((HMODULE)dl);
#endif
      return "No functions or classes registered by " + sanitize_url(soname);
    }
    // note that it is possible to load a toolkit multiple times.
    // It is not safe to unload previously loaded toolkits since I may have
    // a reference to it (for instance a class). We just keep loading over
    // and hope for the best.

    // store and remember the dlhandle and what was registered;
    dynamic_loaded_toolkits[regentry.original_soname] = regentry;
    return std::string();
  }


  std::vector<std::string> unity_global::list_toolkit_functions_in_dynamic_module(std::string soname) {
    auto iter = dynamic_loaded_toolkits.find(soname);
    if (iter == dynamic_loaded_toolkits.end()) {
      throw("Toolkit name " + sanitize_url(soname) + " not found");
    }
    else return iter->second.functions;
  }

  std::vector<std::string> unity_global::list_toolkit_classes_in_dynamic_module(std::string soname) {
    auto iter = dynamic_loaded_toolkits.find(soname);
    if (iter == dynamic_loaded_toolkits.end()) {
      throw("Toolkit name " + sanitize_url(soname) + " not found");
    }
    else return iter->second.classes;
  }

  std::string unity_global::get_current_cache_file_location() {
    auto the_file = get_temp_name();
    boost::filesystem::path p(the_file);
    if(!p.has_parent_path()) {
      return std::string("");
    }
    auto the_location = p.parent_path();

    delete_temp_file(the_file);

    return the_location.string();
  }

  toolkit_function_registry* unity_global::get_toolkit_function_registry() {
    return toolkit_functions;
  }
  toolkit_class_registry* unity_global::get_toolkit_class_registry() {
    return classes;
  }

} // namespace turi
