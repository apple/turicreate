/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmUVHandlePtr.h"
#include <string>

#include "cmConnection.h"
#include "cm_uv.h"

class cmPipeConnection : public cmEventBasedConnection
{
public:
  cmPipeConnection(const std::string& name,
                   cmConnectionBufferStrategy* bufferStrategy = nullptr);

  bool OnServeStart(std::string* pString) override;

  bool OnConnectionShuttingDown() override;

  void Connect(uv_stream_t* server) override;

private:
  const std::string PipeName;
  cm::uv_pipe_ptr ServerPipe;
};
