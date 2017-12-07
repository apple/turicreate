/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once

#include "cmConfigure.h"

#include "cm_uv.h"

#include <string>

class cmFileMonitor;
class cmServer;

class cmServerConnection
{
  CM_DISABLE_COPY(cmServerConnection)

public:
  cmServerConnection();
  virtual ~cmServerConnection();

  void SetServer(cmServer* s);

  bool ProcessEvents(std::string* errorMessage);

  void ReadData(const std::string& data);
  void TriggerShutdown();
  void WriteData(const std::string& data);
  void ProcessNextRequest();

  virtual void Connect(uv_stream_t* server) { (void)(server); }

  cmFileMonitor* FileMonitor() const { return this->mFileMonitor; }

protected:
  virtual bool DoSetup(std::string* errorMessage) = 0;
  virtual void TearDown() = 0;

  void SendGreetings();

  uv_loop_t* Loop() const { return mLoop; }

protected:
  std::string RawReadBuffer;
  std::string RequestBuffer;

  uv_stream_t* ReadStream = nullptr;
  uv_stream_t* WriteStream = nullptr;

private:
  uv_loop_t* mLoop = nullptr;
  cmFileMonitor* mFileMonitor = nullptr;
  cmServer* Server = nullptr;
  uv_signal_t* SIGINTHandler = nullptr;
  uv_signal_t* SIGHUPHandler = nullptr;

  friend class LoopGuard;
};

class cmServerStdIoConnection : public cmServerConnection
{
public:
  cmServerStdIoConnection();
  bool DoSetup(std::string* errorMessage) override;

  void TearDown() override;

private:
  typedef union
  {
    uv_tty_t* tty;
    uv_pipe_t* pipe;
  } InOutUnion;

  bool usesTty = false;

  InOutUnion Input;
  InOutUnion Output;
};

class cmServerPipeConnection : public cmServerConnection
{
public:
  cmServerPipeConnection(const std::string& name);
  bool DoSetup(std::string* errorMessage) override;

  void TearDown() override;

  void Connect(uv_stream_t* server) override;

private:
  const std::string PipeName;
  uv_pipe_t* ServerPipe = nullptr;
  uv_pipe_t* ClientPipe = nullptr;
};
