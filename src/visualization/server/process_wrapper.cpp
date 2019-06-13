/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "process_wrapper.hpp"

#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>

#include <cassert>
#include <chrono>
#include <stdexcept>
#include <sstream>
#include <string>

#include <errno.h>
#include <unistd.h>

using namespace turi::visualization;

process_wrapper::process_wrapper(const std::string& path_to_client) : m_alive(true) {
  // constructor
  // instantiate visualization client process
  m_client_process.popen(path_to_client.c_str(),
                  std::vector<std::string>(),
                  STDOUT_FILENO,
                  true /* open_write_pipe */);
  if (!m_client_process.exists()) {
    throw std::runtime_error("Turi Create visualization process was unable to launch.");
  }
  m_client_process.set_nonblocking(true);
  m_client_process.autoreap();

  // start the background threads to pull and push data over the pipe
  m_inputThread = std::thread([this]() {
    std::string previousInputRemaining;
    while (good()) {
      std::string input = previousInputRemaining + m_client_process.read_from_child();
      if (input.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }

      // split on newline - each message is newline separated
      std::stringstream ss;
      for (char c : input) {
        if (c == '\n') {
          std::string msg = ss.str();
          if (!msg.empty()) {
            m_inputBuffer.write(msg);
          }
          ss.str(std::string()); // clear the stream
        } else {
          ss << c;
        }
      }

      // whatever is left over in the stream, store in previousInputRemaining
      previousInputRemaining = ss.str();
    }
  });
  m_outputThread = std::thread([this]() {
    std::unique_lock<mutex> guard(m_mutex);
    while (true) {
      while (m_client_process.exists() && m_outputBuffer.size() > 0) {
        std::string output = m_outputBuffer.read();
        DASSERT_FALSE(output.empty());
        //fprintf(stderr, "TC sending data: %s\n", output.c_str());
        DASSERT_TRUE(strlen(output.c_str()) == output.size());
        m_client_process.write_to_child(output.c_str(), output.size());
      }
      if (!good()) {
        break;
      }
      m_cond.wait(guard);
    }
  });

  // workaround to pop-under GUI app window from popen:
  // https://stackoverflow.com/a/13553471

  #ifdef __APPLE__
  ::turi::process osascript;
  osascript.popen("/usr/bin/osascript",
      std::vector<std::string>({
        "-e",
        "delay .5",
        "-e",
        "tell application \"Turi Create Visualization\" to activate"
      }),
      0, false);
  #endif
}

process_wrapper::~process_wrapper() {
  // clean up member threads
  {
    std::lock_guard<mutex> guard(m_mutex);
    m_alive = false;
    m_cond.signal();
  }
  m_inputThread.join();
  m_outputThread.join();
}

process_wrapper& process_wrapper::operator<<(const std::string& to_client) {
  // TODO - error handling?
  if (good()) {
    std::lock_guard<mutex> guard(m_mutex);
    m_outputBuffer.write(to_client);
    m_cond.signal();
  }
  return *this;
}

process_wrapper& process_wrapper::operator>>(std::string& from_client) {
  // TODO - error handling?
  if (good()) {
    from_client = m_inputBuffer.read();
  }
  return *this;
}

bool process_wrapper::good() {
  return m_alive && m_client_process.exists();
}
