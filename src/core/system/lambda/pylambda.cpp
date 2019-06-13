/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/lambda/pylambda.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_rows.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/util/cityhash_tc.hpp>
#include <shmipc/shmipc.hpp>

namespace turi { namespace lambda {

pylambda_evaluation_functions evaluation_functions;

/** This is called through ctypes to set up the evaluation function interface.
 */
void EXPORT set_pylambda_evaluation_functions(pylambda_evaluation_functions* eval_function_struct) {
  evaluation_functions = *eval_function_struct;
}

/**  Creates the current lambda interface.
 *
 */
size_t make_lambda(const std::string& pylambda_str) {
  DASSERT_TRUE(evaluation_functions.init_lambda != NULL);

  size_t lambda_id = evaluation_functions.init_lambda(pylambda_str);
  python::check_for_python_exception();

  logstream(LOG_DEBUG) << "Created lambda id=" << lambda_id << std::endl;

  return lambda_id;
}

void release_lambda(size_t lambda_id) {

  logstream(LOG_DEBUG) << "release lambda id=" << lambda_id << std::endl;

  DASSERT_TRUE(evaluation_functions.release_lambda != NULL);

  evaluation_functions.release_lambda(lambda_id);
  python::check_for_python_exception();
}

// First, the lower-level functions that wrap each of the cython
// functions.  These don't do much more than set up the calling
// argmuments.
pylambda_evaluator::~pylambda_evaluator() {
  if (m_shared_memory_listener.active()) {
    m_shared_memory_thread_terminating = true;
    m_shared_memory_listener.join();
  }
}

size_t pylambda_evaluator::make_lambda(const std::string& pylambda_str) {
  return lambda::make_lambda(pylambda_str);
}

void pylambda_evaluator::release_lambda(size_t lambda_id) {
  return lambda::release_lambda(lambda_id);
}

flexible_type pylambda_evaluator::eval(size_t lambda_id, const flexible_type& arg) {

  flexible_type ret;

  lambda_call_data lcd;

  lcd.output_enum_type = flex_type_enum::UNDEFINED;
  lcd.skip_undefined = false;
  lcd.input_values = &arg;
  lcd.output_values = &ret;
  lcd.n_inputs = 1;

  evaluation_functions.eval_lambda(lambda_id, &lcd);
  python::check_for_python_exception();

  return ret;
}

/**
 * Bulk version of eval.
 */
std::vector<flexible_type> pylambda_evaluator::bulk_eval(
    size_t lambda_id,
    const std::vector<flexible_type>& args,
    bool skip_undefined,
    int seed) {

  evaluation_functions.set_random_seed(seed);

  std::vector<flexible_type> ret(args.size());

  lambda_call_data lcd;
  lcd.output_enum_type = flex_type_enum::UNDEFINED;
  lcd.skip_undefined = skip_undefined;
  lcd.input_values = args.data();
  lcd.output_values = ret.data();
  lcd.n_inputs = args.size();

  evaluation_functions.eval_lambda(lambda_id, &lcd);
  python::check_for_python_exception();

  return ret;
}

std::vector<flexible_type> pylambda_evaluator::bulk_eval_rows(
    size_t lambda_id,
    const sframe_rows& rows,
    bool skip_undefined,
    int seed) {

  evaluation_functions.set_random_seed(seed);

  std::vector<flexible_type> ret(rows.num_rows());

  size_t i = 0;
  for (const auto& x : rows) {
    if (skip_undefined && x[0] == FLEX_UNDEFINED) {
      ret[i++] = FLEX_UNDEFINED;
    } else {
      ret[i++] = eval(lambda_id, x[0]);
    }
  }
  return ret;
}


/**
 * Bulk version of eval_dict.
 */
std::vector<flexible_type> pylambda_evaluator::bulk_eval_dict(
    size_t lambda_id,
    const std::vector<std::string>& keys,
    const std::vector<std::vector<flexible_type>>& values,
    bool skip_undefined,
    int seed) {

  evaluation_functions.set_random_seed(seed);

  std::vector<flexible_type> ret(values.size());

  lambda_call_by_dict_data lcd;
  lcd.output_enum_type = flex_type_enum::UNDEFINED;
  lcd.skip_undefined = skip_undefined;
  lcd.input_keys = &keys;
  lcd.input_rows = &values;
  lcd.output_values = ret.data();

  evaluation_functions.eval_lambda_by_dict(lambda_id, &lcd);
  python::check_for_python_exception();

  return ret;
}

std::vector<flexible_type> pylambda_evaluator::bulk_eval_dict_rows(
    size_t lambda_id,
    const std::vector<std::string>& keys,
    const sframe_rows& rows,
    bool skip_undefined,
    int seed) {

  evaluation_functions.set_random_seed(seed);

  std::vector<flexible_type> ret(rows.num_rows());

  lambda_call_by_sframe_rows_data lcd;
  lcd.output_enum_type = flex_type_enum::UNDEFINED;
  lcd.skip_undefined = skip_undefined;
  lcd.input_keys = &keys;
  lcd.input_rows = &rows;
  lcd.output_values = ret.data();

  evaluation_functions.eval_lambda_by_sframe_rows(lambda_id, &lcd);
  python::check_for_python_exception();

  return ret;
}

std::vector<flexible_type>
pylambda_evaluator::bulk_eval_rows_serialized(const char* ptr, size_t len) {
  iarchive iarc(ptr, len);
  char c;
  iarc >> c;
  if (c == (char)bulk_eval_serialized_tag::BULK_EVAL_ROWS) {
    size_t lambda_id;
    sframe_rows rows;
    bool skip_undefined;
    int seed;
    iarc >> lambda_id >> rows >> skip_undefined >> seed;
    return bulk_eval_rows(lambda_id, rows, skip_undefined, seed);
  } else if (c == (char)bulk_eval_serialized_tag::BULK_EVAL_DICT_ROWS) {
    size_t lambda_id;
    std::vector<std::string> keys;
    sframe_rows values;
    bool skip_undefined;
    int seed;
    iarc >> lambda_id >> keys >> values >> skip_undefined >> seed;
    return bulk_eval_dict_rows(lambda_id, keys, values, skip_undefined, seed);
  } else {
    logstream(LOG_FATAL) << "Invalid serialized result" << std::endl;
    return std::vector<flexible_type>();
  }
}

std::string pylambda_evaluator::initialize_shared_memory_comm() {
  if (m_shared_memory_server) {
    if (!m_shared_memory_listener.active()) {
      m_shared_memory_listener.launch(
          [=]() {
            while(!m_shared_memory_server->wait_for_connect(3)) {
              if (m_shared_memory_thread_terminating) return;
            }
            char* receive_buffer = nullptr;
            size_t receive_buffer_length = 0;
            size_t message_length = 0;
            char* send_buffer = nullptr;
            size_t send_buffer_length= 0 ;
            while(1) {
              bool has_data =
                  shmipc::large_receive(*m_shared_memory_server,
                                        &receive_buffer,
                                        &receive_buffer_length,
                                        message_length,
                                        3 /* timeout */);
              if (!has_data) {
                if (m_shared_memory_thread_terminating) break;
                else continue;
              } else {
                oarchive oarc;
                oarc.buf = send_buffer;
                oarc.len = send_buffer_length;
                try {
                  auto ret = bulk_eval_rows_serialized(receive_buffer, message_length);
                  oarc << (char)(1) << ret;
                } catch (std::string& s) {
                  oarc << (char)(0) << s;
                } catch (const char* s) {
                  oarc << (char)(0) << std::string(s);
                } catch (...) {
                  oarc << (char)(0) << std::string("Unknown Runtime Exception");
                }
                shmipc::large_send(*m_shared_memory_server,
                                   oarc.buf,
                                   oarc.off);
                send_buffer = oarc.buf;
                send_buffer_length = oarc.len;
              }
            }
            if (receive_buffer) free(receive_buffer);
            if (send_buffer) free(send_buffer);
          });
    }
    return m_shared_memory_server->get_shared_memory_name();
  } else {
    return "";
  }
}
} // end of namespace lambda
} // end of namespace turi
