/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once

#include "cmConfigure.h" // IWYU pragma: keep

#include <functional>
#include <string>
#include <vector>

#include "cm_uv.h"

class cmRootWatcher;

class cmFileMonitor
{
  CM_DISABLE_COPY(cmFileMonitor)

public:
  cmFileMonitor(uv_loop_t* l);
  ~cmFileMonitor();

  using Callback = std::function<void(const std::string&, int, int)>;
  void MonitorPaths(const std::vector<std::string>& paths, Callback const& cb);
  void StopMonitoring();

  std::vector<std::string> WatchedFiles() const;
  std::vector<std::string> WatchedDirectories() const;

private:
  cmRootWatcher* Root;
};
