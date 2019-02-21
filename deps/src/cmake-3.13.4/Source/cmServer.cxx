/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmServer.h"

#include "cmAlgorithms.h"
#include "cmConnection.h"
#include "cmFileMonitor.h"
#include "cmJsonObjectDictionary.h"
#include "cmServerDictionary.h"
#include "cmServerProtocol.h"
#include "cmSystemTools.h"
#include "cm_jsoncpp_reader.h"
#include "cm_jsoncpp_writer.h"
#include "cmake.h"
#include "cmsys/FStream.hxx"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>

void on_signal(uv_signal_t* signal, int signum)
{
  auto conn = static_cast<cmServerBase*>(signal->data);
  conn->OnSignal(signum);
}

static void on_walk_to_shutdown(uv_handle_t* handle, void* arg)
{
  (void)arg;
  assert(uv_is_closing(handle));
  if (!uv_is_closing(handle)) {
    uv_close(handle, &cmEventBasedConnection::on_close);
  }
}

class cmServer::DebugInfo
{
public:
  DebugInfo()
    : StartTime(uv_hrtime())
  {
  }

  bool PrintStatistics = false;

  std::string OutputFile;
  uint64_t StartTime;
};

cmServer::cmServer(cmConnection* conn, bool supportExperimental)
  : cmServerBase(conn)
  , SupportExperimental(supportExperimental)
{
  // Register supported protocols:
  this->RegisterProtocol(new cmServerProtocol1);
}

cmServer::~cmServer()
{
  Close();

  for (cmServerProtocol* p : this->SupportedProtocols) {
    delete p;
  }
}

void cmServer::ProcessRequest(cmConnection* connection,
                              const std::string& input)
{
  Json::Reader reader;
  Json::Value value;
  if (!reader.parse(input, value)) {
    this->WriteParseError(connection, "Failed to parse JSON input.");
    return;
  }

  std::unique_ptr<DebugInfo> debug;
  Json::Value debugValue = value["debug"];
  if (!debugValue.isNull()) {
    debug = cm::make_unique<DebugInfo>();
    debug->OutputFile = debugValue["dumpToFile"].asString();
    debug->PrintStatistics = debugValue["showStats"].asBool();
  }

  const cmServerRequest request(this, connection, value[kTYPE_KEY].asString(),
                                value[kCOOKIE_KEY].asString(), value);

  if (request.Type.empty()) {
    cmServerResponse response(request);
    response.SetError("No type given in request.");
    this->WriteResponse(connection, response, nullptr);
    return;
  }

  cmSystemTools::SetMessageCallback(reportMessage,
                                    const_cast<cmServerRequest*>(&request));
  if (this->Protocol) {
    this->Protocol->CMakeInstance()->SetProgressCallback(
      reportProgress, const_cast<cmServerRequest*>(&request));
    this->WriteResponse(connection, this->Protocol->Process(request),
                        debug.get());
  } else {
    this->WriteResponse(connection, this->SetProtocolVersion(request),
                        debug.get());
  }
}

void cmServer::RegisterProtocol(cmServerProtocol* protocol)
{
  if (protocol->IsExperimental() && !this->SupportExperimental) {
    delete protocol;
    return;
  }
  auto version = protocol->ProtocolVersion();
  assert(version.first >= 0);
  assert(version.second >= 0);
  auto it = std::find_if(this->SupportedProtocols.begin(),
                         this->SupportedProtocols.end(),
                         [version](cmServerProtocol* p) {
                           return p->ProtocolVersion() == version;
                         });
  if (it == this->SupportedProtocols.end()) {
    this->SupportedProtocols.push_back(protocol);
  }
}

void cmServer::PrintHello(cmConnection* connection) const
{
  Json::Value hello = Json::objectValue;
  hello[kTYPE_KEY] = "hello";

  Json::Value& protocolVersions = hello[kSUPPORTED_PROTOCOL_VERSIONS] =
    Json::arrayValue;

  for (auto const& proto : this->SupportedProtocols) {
    auto version = proto->ProtocolVersion();
    Json::Value tmp = Json::objectValue;
    tmp[kMAJOR_KEY] = version.first;
    tmp[kMINOR_KEY] = version.second;
    if (proto->IsExperimental()) {
      tmp[kIS_EXPERIMENTAL_KEY] = true;
    }
    protocolVersions.append(tmp);
  }

  this->WriteJsonObject(connection, hello, nullptr);
}

void cmServer::reportProgress(const char* msg, float progress, void* data)
{
  const cmServerRequest* request = static_cast<const cmServerRequest*>(data);
  assert(request);
  if (progress < 0.0f || progress > 1.0f) {
    request->ReportMessage(msg, "");
  } else {
    request->ReportProgress(0, static_cast<int>(progress * 1000), 1000, msg);
  }
}

void cmServer::reportMessage(const char* msg, const char* title,
                             bool& /* cancel */, void* data)
{
  const cmServerRequest* request = static_cast<const cmServerRequest*>(data);
  assert(request);
  assert(msg);
  std::string titleString;
  if (title) {
    titleString = title;
  }
  request->ReportMessage(std::string(msg), titleString);
}

cmServerResponse cmServer::SetProtocolVersion(const cmServerRequest& request)
{
  if (request.Type != kHANDSHAKE_TYPE) {
    return request.ReportError("Waiting for type \"" + kHANDSHAKE_TYPE +
                               "\".");
  }

  Json::Value requestedProtocolVersion = request.Data[kPROTOCOL_VERSION_KEY];
  if (requestedProtocolVersion.isNull()) {
    return request.ReportError("\"" + kPROTOCOL_VERSION_KEY +
                               "\" is required for \"" + kHANDSHAKE_TYPE +
                               "\".");
  }

  if (!requestedProtocolVersion.isObject()) {
    return request.ReportError("\"" + kPROTOCOL_VERSION_KEY +
                               "\" must be a JSON object.");
  }

  Json::Value majorValue = requestedProtocolVersion[kMAJOR_KEY];
  if (!majorValue.isInt()) {
    return request.ReportError("\"" + kMAJOR_KEY +
                               "\" must be set and an integer.");
  }

  Json::Value minorValue = requestedProtocolVersion[kMINOR_KEY];
  if (!minorValue.isNull() && !minorValue.isInt()) {
    return request.ReportError("\"" + kMINOR_KEY +
                               "\" must be unset or an integer.");
  }

  const int major = majorValue.asInt();
  const int minor = minorValue.isNull() ? -1 : minorValue.asInt();
  if (major < 0) {
    return request.ReportError("\"" + kMAJOR_KEY + "\" must be >= 0.");
  }
  if (!minorValue.isNull() && minor < 0) {
    return request.ReportError("\"" + kMINOR_KEY +
                               "\" must be >= 0 when set.");
  }

  this->Protocol =
    this->FindMatchingProtocol(this->SupportedProtocols, major, minor);
  if (!this->Protocol) {
    return request.ReportError("Protocol version not supported.");
  }

  std::string errorMessage;
  if (!this->Protocol->Activate(this, request, &errorMessage)) {
    this->Protocol = nullptr;
    return request.ReportError("Failed to activate protocol version: " +
                               errorMessage);
  }
  return request.Reply(Json::objectValue);
}

bool cmServer::Serve(std::string* errorMessage)
{
  if (this->SupportedProtocols.empty()) {
    *errorMessage =
      "No protocol versions defined. Maybe you need --experimental?";
    return false;
  }
  assert(!this->Protocol);

  return cmServerBase::Serve(errorMessage);
}

cmFileMonitor* cmServer::FileMonitor() const
{
  return fileMonitor.get();
}

void cmServer::WriteJsonObject(const Json::Value& jsonValue,
                               const DebugInfo* debug) const
{
  cm::shared_lock<cm::shared_mutex> lock(ConnectionsMutex);
  for (auto& connection : this->Connections) {
    WriteJsonObject(connection.get(), jsonValue, debug);
  }
}

void cmServer::WriteJsonObject(cmConnection* connection,
                               const Json::Value& jsonValue,
                               const DebugInfo* debug) const
{
  Json::FastWriter writer;

  auto beforeJson = uv_hrtime();
  std::string result = writer.write(jsonValue);

  if (debug) {
    Json::Value copy = jsonValue;
    if (debug->PrintStatistics) {
      Json::Value stats = Json::objectValue;
      auto endTime = uv_hrtime();

      stats["jsonSerialization"] = double(endTime - beforeJson) / 1000000.0;
      stats["totalTime"] = double(endTime - debug->StartTime) / 1000000.0;
      stats["size"] = static_cast<int>(result.size());
      if (!debug->OutputFile.empty()) {
        stats["dumpFile"] = debug->OutputFile;
      }

      copy["zzzDebug"] = stats;

      result = writer.write(copy); // Update result to include debug info
    }

    if (!debug->OutputFile.empty()) {
      cmsys::ofstream myfile(debug->OutputFile.c_str());
      myfile << result;
    }
  }

  connection->WriteData(result);
}

cmServerProtocol* cmServer::FindMatchingProtocol(
  const std::vector<cmServerProtocol*>& protocols, int major, int minor)
{
  cmServerProtocol* bestMatch = nullptr;
  for (auto protocol : protocols) {
    auto version = protocol->ProtocolVersion();
    if (major != version.first) {
      continue;
    }
    if (minor == version.second) {
      return protocol;
    }
    if (!bestMatch || bestMatch->ProtocolVersion().second < version.second) {
      bestMatch = protocol;
    }
  }
  return minor < 0 ? bestMatch : nullptr;
}

void cmServer::WriteProgress(const cmServerRequest& request, int min,
                             int current, int max,
                             const std::string& message) const
{
  assert(min <= current && current <= max);
  assert(message.length() != 0);

  Json::Value obj = Json::objectValue;
  obj[kTYPE_KEY] = kPROGRESS_TYPE;
  obj[kREPLY_TO_KEY] = request.Type;
  obj[kCOOKIE_KEY] = request.Cookie;
  obj[kPROGRESS_MESSAGE_KEY] = message;
  obj[kPROGRESS_MINIMUM_KEY] = min;
  obj[kPROGRESS_MAXIMUM_KEY] = max;
  obj[kPROGRESS_CURRENT_KEY] = current;

  this->WriteJsonObject(request.Connection, obj, nullptr);
}

void cmServer::WriteMessage(const cmServerRequest& request,
                            const std::string& message,
                            const std::string& title) const
{
  if (message.empty()) {
    return;
  }

  Json::Value obj = Json::objectValue;
  obj[kTYPE_KEY] = kMESSAGE_TYPE;
  obj[kREPLY_TO_KEY] = request.Type;
  obj[kCOOKIE_KEY] = request.Cookie;
  obj[kMESSAGE_KEY] = message;
  if (!title.empty()) {
    obj[kTITLE_KEY] = title;
  }

  WriteJsonObject(request.Connection, obj, nullptr);
}

void cmServer::WriteParseError(cmConnection* connection,
                               const std::string& message) const
{
  Json::Value obj = Json::objectValue;
  obj[kTYPE_KEY] = kERROR_TYPE;
  obj[kERROR_MESSAGE_KEY] = message;
  obj[kREPLY_TO_KEY] = "";
  obj[kCOOKIE_KEY] = "";

  this->WriteJsonObject(connection, obj, nullptr);
}

void cmServer::WriteSignal(const std::string& name,
                           const Json::Value& data) const
{
  assert(data.isObject());
  Json::Value obj = data;
  obj[kTYPE_KEY] = kSIGNAL_TYPE;
  obj[kREPLY_TO_KEY] = "";
  obj[kCOOKIE_KEY] = "";
  obj[kNAME_KEY] = name;

  WriteJsonObject(obj, nullptr);
}

void cmServer::WriteResponse(cmConnection* connection,
                             const cmServerResponse& response,
                             const DebugInfo* debug) const
{
  assert(response.IsComplete());

  Json::Value obj = response.Data();
  obj[kCOOKIE_KEY] = response.Cookie;
  obj[kTYPE_KEY] = response.IsError() ? kERROR_TYPE : kREPLY_TYPE;
  obj[kREPLY_TO_KEY] = response.Type;
  if (response.IsError()) {
    obj[kERROR_MESSAGE_KEY] = response.ErrorMessage();
  }

  this->WriteJsonObject(connection, obj, debug);
}

void cmServer::OnConnected(cmConnection* connection)
{
  PrintHello(connection);
}

void cmServer::OnServeStart()
{
  cmServerBase::OnServeStart();
  fileMonitor = std::make_shared<cmFileMonitor>(GetLoop());
}

void cmServer::StartShutDown()
{
  if (fileMonitor) {
    fileMonitor->StopMonitoring();
    fileMonitor.reset();
  }
  cmServerBase::StartShutDown();
}

static void __start_thread(void* arg)
{
  auto server = static_cast<cmServerBase*>(arg);
  std::string error;
  bool success = server->Serve(&error);
  if (!success || error.empty() == false) {
    std::cerr << "Error during serve: " << error << std::endl;
  }
}

bool cmServerBase::StartServeThread()
{
  ServeThreadRunning = true;
  uv_thread_create(&ServeThread, __start_thread, this);
  return true;
}

static void __shutdownThread(uv_async_t* arg)
{
  auto server = static_cast<cmServerBase*>(arg->data);
  server->StartShutDown();
}

bool cmServerBase::Serve(std::string* errorMessage)
{
#ifndef NDEBUG
  uv_thread_t blank_thread_t = {};
  assert(uv_thread_equal(&blank_thread_t, &ServeThreadId));
  ServeThreadId = uv_thread_self();
#endif

  errorMessage->clear();

  ShutdownSignal.init(Loop, __shutdownThread, this);

  SIGINTHandler.init(Loop, this);
  SIGHUPHandler.init(Loop, this);

  SIGINTHandler.start(&on_signal, SIGINT);
  SIGHUPHandler.start(&on_signal, SIGHUP);

  OnServeStart();

  {
    cm::shared_lock<cm::shared_mutex> lock(ConnectionsMutex);
    for (auto& connection : Connections) {
      if (!connection->OnServeStart(errorMessage)) {
        return false;
      }
    }
  }

  if (uv_run(&Loop, UV_RUN_DEFAULT) != 0) {
    // It is important we don't ever let the event loop exit with open handles
    // at best this is a memory leak, but it can also introduce race conditions
    // which can hang the program.
    assert(false && "Event loop stopped in unclean state.");

    *errorMessage = "Internal Error: Event loop stopped in unclean state.";
    return false;
  }

  return true;
}

void cmServerBase::OnConnected(cmConnection*)
{
}

void cmServerBase::OnServeStart()
{
}

void cmServerBase::StartShutDown()
{
  ShutdownSignal.reset();
  SIGINTHandler.reset();
  SIGHUPHandler.reset();

  {
    std::unique_lock<cm::shared_mutex> lock(ConnectionsMutex);
    for (auto& connection : Connections) {
      connection->OnConnectionShuttingDown();
    }
    Connections.clear();
  }

  uv_walk(&Loop, on_walk_to_shutdown, nullptr);
}

bool cmServerBase::OnSignal(int signum)
{
  (void)signum;
  StartShutDown();
  return true;
}

cmServerBase::cmServerBase(cmConnection* connection)
{
  auto err = uv_loop_init(&Loop);
  (void)err;
  Loop.data = this;
  assert(err == 0);

  AddNewConnection(connection);
}

void cmServerBase::Close()
{
  if (Loop.data) {
    if (ServeThreadRunning) {
      this->ShutdownSignal.send();
      uv_thread_join(&ServeThread);
    }

    uv_loop_close(&Loop);
    Loop.data = nullptr;
  }
}
cmServerBase::~cmServerBase()
{
  Close();
}

void cmServerBase::AddNewConnection(cmConnection* ownedConnection)
{
  {
    std::unique_lock<cm::shared_mutex> lock(ConnectionsMutex);
    Connections.emplace_back(ownedConnection);
  }
  ownedConnection->SetServer(this);
}

uv_loop_t* cmServerBase::GetLoop()
{
  return &Loop;
}

void cmServerBase::OnDisconnect(cmConnection* pConnection)
{
  auto pred = [pConnection](const std::unique_ptr<cmConnection>& m) {
    return m.get() == pConnection;
  };
  {
    std::unique_lock<cm::shared_mutex> lock(ConnectionsMutex);
    Connections.erase(
      std::remove_if(Connections.begin(), Connections.end(), pred),
      Connections.end());
  }

  if (Connections.empty()) {
    this->ShutdownSignal.send();
  }
}
