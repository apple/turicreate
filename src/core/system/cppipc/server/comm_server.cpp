/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <random>
#include <boost/bind.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/algorithm/string.hpp>
#include <core/logging/logger.hpp>
#include <core/system/cppipc/server/comm_server.hpp>
#include <core/system/cppipc/server/dispatch.hpp>
#include <core/system/cppipc/common/status_types.hpp>
#include <core/system/cppipc/common/message_types.hpp>
#include <core/system/cppipc/common/object_factory_impl.hpp>
#include <core/system/nanosockets/socket_errors.hpp>
#include <core/system/nanosockets/async_reply_socket.hpp>
#include <core/system/nanosockets/publish_socket.hpp>


namespace cppipc {

/**
 * Generates a publish address based on an address pattern.
 * Where addr is a ZeroMQ endpoint,
 * if addr is of the form ipc://[filename], this will return
 * ipc://[filename]_status.  if addr is of the form tcp://address:port,
 * this will return tcp://address:*
 *
 * This is used to generate a publish address when none is provided
 */
std::string generate_aux_address(std::string addr, std::string addon) {
  // generate a address where the status will be published
  std::string status_address;
  if (boost::starts_with(addr, "ipc://")) {
    status_address = addr + addon;
  } else if (boost::starts_with(addr, "tcp://")) {
    // not supported by nanomsg
    status_address = "";
  } else if (boost::starts_with(addr, "inproc://")){
    status_address = addr + addon;
  } else {
    status_address = "";
  }
  return status_address;
}

comm_server::comm_server(std::vector<std::string> zkhosts,
                         std::string name,
                         std::string alternate_bind_address,
                         std::string alternate_control_address,
                         std::string alternate_publish_address,
                         std::string secret_key):

    started(false),
    comm_server_debug_mode(std::getenv("TURI_COMM_SERVER_DEBUG_MODE") != NULL)
    {


  object_socket = new nanosockets::async_reply_socket(
          boost::bind(&comm_server::callback, this, _1, _2),
          1, // 2 threads. one to handle pings, one to handle real messages
          alternate_bind_address);

  logstream(LOG_INFO) << "my alt bind address: " << alternate_bind_address << std::endl;
  control_socket = new nanosockets::async_reply_socket(
        boost::bind(&comm_server::callback, this, _1, _2), 1,
        (alternate_control_address.length()==0) ?
            generate_aux_address(alternate_bind_address, "_control") :
            alternate_control_address);
  publishsock = new nanosockets::publish_socket(
        (alternate_publish_address.length()==0) ?
            generate_aux_address(alternate_bind_address, "_status") :
            alternate_publish_address);
  get_srv_running_command().store(0);
  get_cancel_bit_checked().store(false);


  logstream(LOG_EMPH) << "Server listening on: "
                      << object_socket->get_bound_address() << std::endl;
  logstream(LOG_INFO) << "Server Control listening on: "
                      << control_socket->get_bound_address() << std::endl;
  logstream(LOG_INFO) << "Server status published on: "
                      << publishsock->get_bound_address() << std::endl;

  // there is a chicken and egg problem here. We can't use the object
  // factory to create the object factory. So, here it is: manual construction
  // and registration of the object factory
  object_factory = new object_factory_impl(*this);
  register_type<object_factory_base>([&]() {
                                     return new object_factory_impl(*this); } );
  auto deleter = +[](void* v) {
                    if (v != NULL) {
                      delete reinterpret_cast<object_factory_impl*>(v);
                    }
                  };
  std::shared_ptr<void> object_ptr(object_factory, deleter);
  registered_objects.insert({0, object_ptr});

  std::random_device rd;
  lcg_seed = (size_t(rd()) << 32) + rd();
}


std::string comm_server::get_bound_address() {
  return object_socket->get_bound_address();
}

std::string comm_server::get_control_address() {
  return control_socket->get_bound_address();
}

std::string comm_server::get_status_address() {
  return publishsock->get_bound_address();
}

void* comm_server::get_zmq_context() {
  return nullptr;
}



size_t comm_server::get_next_object_id() {
  lcg_seed = lcg_seed * 6364136223846793005LL + 1442695040888963407LL;
  // skip 0. We keep object 0 special.
  if (lcg_seed == 0) lcg_seed = 1;
  return lcg_seed;
}


comm_server::~comm_server() {
  log_func_entry();
  stop();
  object_socket->close();
  control_socket->close();
  publishsock->close();
  registered_objects.clear();
  delete object_socket;
  delete control_socket;
  delete publishsock;
  for (auto& dispatcher: dispatch_map) {
    delete dispatcher.second;
  }
  registered_objects.clear();
}

void comm_server::start() {
  log_func_entry();
  if (!started) {
    control_socket->start_polling();
    object_socket->start_polling();
    started = true;
  }
}

void comm_server::stop() {
  log_func_entry();
  if (started) {
    control_socket->stop_polling();
    object_socket->stop_polling();
    started = false;
  }

  // Attempt to cancel any currently running command
  get_srv_running_command().store((unsigned long long)uint64_t(-1));
}


void comm_server::report_status(std::string status_type, std::string message) {
  std::string combined = status_type + ": " + message;
  publishsock->send(combined);
}

bool comm_server::callback(nanosockets::zmq_msg_vector& recv,
                           nanosockets::zmq_msg_vector& reply) {
  // construct a call message from the received block
  call_message call;
  reply_message rep;
  bool success = call.construct(recv);

  if (!success) {
    rep.copy_body_from("Invalid Message");
    rep.status = reply_status::BAD_MESSAGE;
    rep.emit(reply);
    return true;
  }

  // find the object ID
  {
    boost::lock_guard<boost::mutex> guard(registered_object_lock);
    if (registered_objects.count(call.objectid) == 0) {
      std::string ret = "No such object " + std::to_string(call.objectid);
      logstream(LOG_ERROR) << ret << std::endl;
      rep.copy_body_from(ret);
      rep.status = reply_status::NO_OBJECT;
      rep.emit(reply);
      return true;
    }
  }
  //
  // find the function
  if (dispatch_map.count(call.function_name) == 0) {
    std::string ret = "No such function " + call.function_name;
    logstream(LOG_ERROR) << ret << std::endl;
    rep.copy_body_from(ret);
    rep.status = reply_status::NO_FUNCTION;
    rep.emit(reply);
    return true;
  }

  std::string trimmed_function_name;
  // trim the function call printing to stop at the first space
  std::copy(call.function_name.begin(),
            std::find(call.function_name.begin(), call.function_name.end(), ' '),
            std::inserter(trimmed_function_name, trimmed_function_name.end()));

  std::string message = "Calling object " + std::to_string(call.objectid)
                        + " function: " + trimmed_function_name;

  if(comm_server_debug_mode) {
    logstream(LOG_DEBUG) << message << std::endl;
  }

  /*
   * if (trimmed_function_name == "object_factory_base::ping" || call.objectid == 0) {
   *   logstream(LOG_DEBUG) << message << "\n";
   * } else {
   *   logstream(LOG_INFO) << message << "\n";
   * }
   */

  report_status(STATUS_COMM_SERVER_INFO, message);

  // ok we are good to go
  // create the appropriate archives
  turi::iarchive iarc(call.body, call.bodylen);
  turi::oarchive oarc;

  // Now set the currently running command if this is a real command (not a ping)
  auto ret = call.properties.find(std::string("command_id"));
  bool real_command = false;
  if(ret != call.properties.end()) {
    unsigned long long ul = std::stoull(ret->second);
    get_srv_running_command().store(ul);
    real_command = true;
  }

  rep.status = reply_status::OK;
  try {
    dispatch_map[call.function_name]->execute(
                          registered_objects[call.objectid].get(),
                          this, iarc, oarc);
  } catch (const std::ios_base::failure& e) {
    // IO Exception
    rep.copy_body_from(e.what());
    report_status(STATUS_COMM_SERVER_ERROR, e.what());
    rep.status = reply_status::IO_ERROR;
  } catch (std::bad_alloc& e) {
    // MEMORY Exception
    rep.copy_body_from(e.what());
    report_status(STATUS_COMM_SERVER_ERROR, e.what());
    rep.status = reply_status::MEMORY_ERROR;
  } catch (std::out_of_range& e) {
    // INDEX Exception
    rep.copy_body_from(e.what());
    report_status(STATUS_COMM_SERVER_ERROR, e.what());
    rep.status = reply_status::INDEX_ERROR;
  } catch (std::bad_cast& e) {
    // TYPE Exception
    rep.copy_body_from(e.what());
    report_status(STATUS_COMM_SERVER_ERROR, e.what());
    rep.status = reply_status::TYPE_ERROR;
  } catch (std::string& s) {
    // General Exception
    rep.copy_body_from(s);
    report_status(STATUS_COMM_SERVER_ERROR, s);
    rep.status = reply_status::EXCEPTION;
  } catch (const char* s) {
    rep.copy_body_from(s);
    report_status(STATUS_COMM_SERVER_ERROR, s);
    rep.status = reply_status::EXCEPTION;
  } catch (std::exception& e) {
    rep.copy_body_from(e.what());
    report_status(STATUS_COMM_SERVER_ERROR, e.what());
    rep.status = reply_status::EXCEPTION;
  } catch (...) {
    rep.copy_body_from("Unknown Runtime Exception");
    report_status(STATUS_COMM_SERVER_ERROR, "Unknown Runtime Exception");
    rep.status = reply_status::EXCEPTION;
  }

  /*
   * Complete hack.
   * For whatever reason zeromq's zmq_msg_send and zmq_msg_recv function
   * return the size of the mesage sent in an int. Even though the message
   * size can be size_t.
   * Also, zmq_msg_send/zmq_msg_recv use "-1" return for failure, thus
   * bringing up the issue of integer overflow just "coincidentally" hitting
   * -1 and thus failing terribly terribly.
   *  Solution is simple. Pad the buffer to even.
   */
  if (oarc.off & 1) oarc.write(" ", 1);

  report_status(STATUS_COMM_SERVER_INFO, "Function Execution Success");
  if (rep.status == reply_status::OK) {
    rep.body = oarc.buf;
    rep.bodylen = oarc.off;
  }

  // Command is now over, so this is not the running command anymore
  if(real_command) {
    std::atomic<bool> &cancel_checked = get_cancel_bit_checked();

    if(cancel_checked.load()) {
      if(must_cancel()) {
        rep.properties.insert(
            std::make_pair(std::string("cancel"), std::string("true")));
      } else {
        rep.properties.insert(
            std::make_pair(std::string("cancel"), std::string("false")));
      }
    }

    get_srv_running_command().store(0);
    cancel_checked.store(false);
  }

  rep.emit(reply);
  return true;
}

/**
 * Puts an object constructor into the object factory
 */
void comm_server::register_constructor(std::string type_name,
                                       std::function<std::shared_ptr<void>()> constructor_call) {
 object_factory->add_constructor(type_name, constructor_call);
}

void comm_server::delete_unused_objects(std::vector<size_t> object_ids,
                                        bool active_list) {

  std::sort(object_ids.begin(), object_ids.end());
  if (active_list) {
    // Get vector of IDs that are not used anymore on the client
    std::vector<std::pair<size_t, std::shared_ptr<void>> > v;
    {
      boost::lock_guard<boost::mutex> guard(registered_object_lock);
      // delete everything not in the active list
      std::set_difference(registered_objects.cbegin(), registered_objects.cend(),
                          object_ids.cbegin(), object_ids.cend(),
                          std::inserter(v, v.begin()), object_map_key_cmp());
    }

    // Actually delete objects
    for(auto i : v) {
      if(i.first != 0) {
        delete_object(i.first);
      }
    }
  } else {
    // delete everything in the active list
    for(auto i : object_ids) {
      boost::unique_lock<boost::mutex> guard(registered_object_lock);
      if (registered_objects.count(i)) {
        // delete object has its own lock
        guard.unlock();
        delete_object(i);
      }
    }
  }
}

} // cppipc
