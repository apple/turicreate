/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once

#include "cmConfigure.h" // IWYU pragma: keep

#include "cm_jsoncpp_value.h"
#include "cm_thread.hxx"
#include "cm_uv.h"

#include "cmUVHandlePtr.h"

#include <memory> // IWYU pragma: keep
#include <string>
#include <vector>

class cmConnection;
class cmFileMonitor;
class cmServerProtocol;
class cmServerRequest;
class cmServerResponse;

/***
 * This essentially hold and manages a libuv event queue and responds to
 * messages
 * on any of its connections.
 */
class cmServerBase
{
public:
  cmServerBase(cmConnection* connection);
  virtual ~cmServerBase();

  virtual void AddNewConnection(cmConnection* ownedConnection);

  /***
   * The main override responsible for tailoring behavior towards
   * whatever the given server is supposed to do
   *
   * This should almost always be called by the given connections
   * directly.
   *
   * @param connection The connection the request was received on
   * @param request The actual request
   */
  virtual void ProcessRequest(cmConnection* connection,
                              const std::string& request) = 0;
  virtual void OnConnected(cmConnection* connection);

  /***
   * Start a dedicated thread. If this is used to start the server, it will
   * join on the
   * servers dtor.
   */
  virtual bool StartServeThread();
  virtual bool Serve(std::string* errorMessage);

  virtual void OnServeStart();
  virtual void StartShutDown();

  virtual bool OnSignal(int signum);
  uv_loop_t* GetLoop();
  void Close();
  void OnDisconnect(cmConnection* pConnection);

protected:
  mutable cm::shared_mutex ConnectionsMutex;
  std::vector<std::unique_ptr<cmConnection>> Connections;

  bool ServeThreadRunning = false;
  uv_thread_t ServeThread;
  cm::uv_async_ptr ShutdownSignal;
#ifndef NDEBUG
public:
  // When the server starts it will mark down it's current thread ID,
  // which is useful in other contexts to just assert that operations
  // are performed on that same thread.
  uv_thread_t ServeThreadId = {};

protected:
#endif

  uv_loop_t Loop;

  cm::uv_signal_ptr SIGINTHandler;
  cm::uv_signal_ptr SIGHUPHandler;
};

class cmServer : public cmServerBase
{
  CM_DISABLE_COPY(cmServer)

public:
  class DebugInfo;

  cmServer(cmConnection* conn, bool supportExperimental);
  ~cmServer() override;

  bool Serve(std::string* errorMessage) override;

  cmFileMonitor* FileMonitor() const;

private:
  void RegisterProtocol(cmServerProtocol* protocol);

  // Callbacks from cmServerConnection:

  void ProcessRequest(cmConnection* connection,
                      const std::string& request) override;
  std::shared_ptr<cmFileMonitor> fileMonitor;

public:
  void OnServeStart() override;

  void StartShutDown() override;

public:
  void OnConnected(cmConnection* connection) override;

private:
  static void reportProgress(const char* msg, float progress, void* data);
  static void reportMessage(const char* msg, const char* title, bool& cancel,
                            void* data);

  // Handle requests:
  cmServerResponse SetProtocolVersion(const cmServerRequest& request);

  void PrintHello(cmConnection* connection) const;

  // Write responses:
  void WriteProgress(const cmServerRequest& request, int min, int current,
                     int max, const std::string& message) const;
  void WriteMessage(const cmServerRequest& request, const std::string& message,
                    const std::string& title) const;
  void WriteResponse(cmConnection* connection,
                     const cmServerResponse& response,
                     const DebugInfo* debug) const;
  void WriteParseError(cmConnection* connection,
                       const std::string& message) const;
  void WriteSignal(const std::string& name, const Json::Value& obj) const;

  void WriteJsonObject(Json::Value const& jsonValue,
                       const DebugInfo* debug) const;

  void WriteJsonObject(cmConnection* connection, Json::Value const& jsonValue,
                       const DebugInfo* debug) const;

  static cmServerProtocol* FindMatchingProtocol(
    const std::vector<cmServerProtocol*>& protocols, int major, int minor);

  const bool SupportExperimental;

  cmServerProtocol* Protocol = nullptr;
  std::vector<cmServerProtocol*> SupportedProtocols;

  friend class cmServerProtocol;
  friend class cmServerRequest;
};
