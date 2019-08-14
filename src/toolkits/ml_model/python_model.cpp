/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <iostream>
#include <fstream>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/extensions/model_base.hpp>
#include <core/storage/fileio/temp_files.hpp>


namespace turi {
namespace python_model {

const size_t PICKLER_READ_WRITE_BUFFER_SIZE = 65536;

class python_model : public model_base {

  static constexpr size_t PYTHON_MODEL_VERSION = 0;

  protected:

  std::string temp_file = "";

  public:

  virtual void save_impl(oarchive& oarc) const override {

    // Read from pickle file (path: temp_file)
    std::ifstream in_file(temp_file, std::ios::binary);

    // Get size of the file.
    in_file.seekg(0, in_file.end);
    unsigned int file_size = in_file.tellg();

    // Write the language binding and size.
    std::string language = "python";
    oarc << language << file_size;

    // Setup file read buffers.
    in_file.seekg(0, in_file.beg);
    std::istreambuf_iterator<char> eos;
    std::istreambuf_iterator<char> file_it(in_file.rdbuf());
    while (file_it != eos){
        oarc << *file_it++;
    }
    in_file.close();
  }


  /**
   * Get a version for the object.
   */
  size_t get_version() const override {
    return PYTHON_MODEL_VERSION;
  }
  /**
   * Load the object using Turi's iarc.
   */
  virtual void load_version(iarchive& iarc, size_t version) override {
    std::string language;
    unsigned int file_size = 0;

    // Read out the language and file-size.
    iarc >> language >> file_size;

    // If language is python.
    if (language == "python") {
      temp_file = get_temp_name();
      std::ofstream out_file(temp_file, std::ios::binary);
      char* buffer = new char[PICKLER_READ_WRITE_BUFFER_SIZE];
      size_t n_blocks = file_size / PICKLER_READ_WRITE_BUFFER_SIZE;
      size_t bytes = 0;

      // Write to a temp_file using a buffer.
      for (size_t i = 0; i < n_blocks; i++){
        iarc.read(buffer, PICKLER_READ_WRITE_BUFFER_SIZE);
        out_file.write(buffer, PICKLER_READ_WRITE_BUFFER_SIZE);
      }
      bytes = file_size - n_blocks * PICKLER_READ_WRITE_BUFFER_SIZE;
      iarc.read(buffer, bytes);
      out_file.write(buffer, bytes);

      // Clean up.
      delete [] buffer;
      out_file.close();

    } else {
      log_and_throw("Internal Error: Unable to read file. Invalid language binding.");
    }
  }

  std::string temp_file_getter() const {
    return temp_file;
  }
  void temp_file_setter(std::string _temp_file) {
    temp_file = _temp_file;
  }

  BEGIN_CLASS_MEMBER_REGISTRATION("_PythonModel")
  REGISTER_PROPERTY(temp_file);
  REGISTER_GETTER("temp_file", python_model::temp_file_getter);
  REGISTER_SETTER("temp_file", python_model::temp_file_setter);
  END_CLASS_MEMBER_REGISTRATION

};

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(python_model)
END_CLASS_REGISTRATION

} // python_model
} // demo turicreate
