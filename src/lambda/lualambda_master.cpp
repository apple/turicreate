/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <lambda/lualambda_master.hpp>
#include <parallel/lambda_omp.hpp>
#include <luastate/LuaState.h>
#include <utility>
#include <algorithm>
#include <boost/algorithm/string.hpp>
extern "C" {
#include <lua/luajit.h>
}
namespace turi {
namespace lambda {

  // we need to limit the maximum number of lambda workers. 
  // There are issues if there are too many.
  const size_t MAX_LUALAMBDA_WORKERS = 16;

  lualambda_master& lualambda_master::get_instance() {
    static lualambda_master instance(std::min<size_t>(MAX_LUALAMBDA_WORKERS, 
                                                      std::max<size_t>(thread::cpu_count(), 1)));
      return instance;
  };

  lualambda_master::lualambda_master(size_t nworkers) {
    start(nworkers);
  }

  void lualambda_master::start(size_t nworkers) {
    clients.resize(nworkers);
    for (size_t i = 0;i < clients.size(); ++i) {
      auto luastate = new lua::State(true);
      clients[i].reset(luastate);
      luaJIT_setmode(luastate->getState().get(), 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_ON);
      worker_queue.push(i);
    }
  }

  void lualambda_master::shutdown() {
    clients.clear();
  }

  size_t lualambda_master::pop_worker() {
    size_t worker_id;
    std::unique_lock<turi::mutex> lck(mtx);
    while (worker_queue.empty()) {
      cv.wait(lck);
    }
    worker_id = worker_queue.front();
    worker_queue.pop();
    return worker_id;
  }

  void lualambda_master::push_worker(size_t workerid) {
    std::unique_lock<turi::mutex> lck(mtx);
    worker_queue.push(workerid);
    lck.unlock();
    cv.notify_one();
  }

  size_t lualambda_master::make_lambda(const std::string& lambda_str) {
    size_t lambda_hash = std::hash<std::string>()(lambda_str);
    std::string newstr = lambda_str;
    if (boost::starts_with(newstr,"LUA")) {
      newstr = newstr.substr(3);
    }
    parallel_for (0, num_workers(), [&](size_t i) {
      clients[i]->doString(newstr);
      clients[i]->doString("lambda" + std::to_string(lambda_hash) + " = __lambda__transfer__");
    });
    return lambda_hash;
  }

  void lualambda_master::release_lambda(size_t lambda_hash) {
    parallel_for (0, num_workers(), [&](size_t i) {
      clients[i]->doString("lambda" + std::to_string(lambda_hash) + " = {}");
    });
  }

  static void call_lua_function(lua::Value& function, const flexible_type& arg, flexible_type& ret) {
    lua::Value valret;
    switch(arg.get_type()) {
     case flex_type_enum::INTEGER:
       valret = function(arg.get<flex_int>());
       break;
     case flex_type_enum::FLOAT:
       valret = function(arg.get<flex_float>());
       break;
     case flex_type_enum::STRING:
       valret = function(arg.get<flex_string>().c_str());
       break;
     default:
       log_and_throw("Not Supported at the moment");
    };
    if (valret.is<lua::Integer>()) {
      lua::Integer val = 0;
      valret.get<lua::Integer>(val);
      ret = val;
    } else if (valret.is<lua::Number>()) {
      lua::Number val = 0;
      valret.get<lua::Number>(val);
      ret = val;
    } else if (valret.is<lua::String>()) {
      std::string val;
      valret.get<std::string>(val);
      ret = std::move(val);
    } else {
      ret = FLEX_UNDEFINED;
    }
  }

  /// single argument version 
  std::vector<flexible_type> lualambda_master::bulk_eval(size_t lambda_hash, const std::vector<flexible_type>& args, bool skip_undefined, int seed) {
    size_t worker_id = pop_worker();
    std::vector<flexible_type> ret;
    std::string lambdaname = "lambda" + std::to_string(lambda_hash);
    auto function = (*clients[worker_id])[lambdaname.c_str()];
    try {
      ret.resize(args.size());
      for (size_t i = 0;i < args.size(); ++i) {
        if (skip_undefined && args[i].get_type() == flex_type_enum::UNDEFINED) {
          ret[i] = FLEX_UNDEFINED;
        } else {
          call_lua_function(function, args[i], ret[i]);
        }
      }
    } catch (...) {
      push_worker(worker_id);
      throw;
    }
    push_worker(worker_id);
    return ret;
  }

  /// dictionary argument version
  std::vector<flexible_type> lualambda_master::bulk_eval(size_t lambda_hash,
      const std::vector<std::string>& keys,
      const std::vector<std::vector<flexible_type>>& values,
      bool skip_undefined, int seed) {
    log_and_throw("Not implemented");
  }
} // end of lambda
} // end of turicreate
