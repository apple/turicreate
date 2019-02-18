/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once

#include "cmConfigure.h" // IWYU pragma: keep

#include "cm_jsoncpp_value.h"
#include "cmake.h"

#include <memory>
#include <string>
#include <utility>

class cmConnection;
class cmFileMonitor;
class cmServer;
class cmServerRequest;

class cmServerResponse
{
public:
  explicit cmServerResponse(const cmServerRequest& request);

  void SetData(const Json::Value& data);
  void SetError(const std::string& message);

  bool IsComplete() const;
  bool IsError() const;
  std::string ErrorMessage() const;
  Json::Value Data() const;

  const std::string Type;
  const std::string Cookie;

private:
  enum PayLoad
  {
    PAYLOAD_UNKNOWN,
    PAYLOAD_ERROR,
    PAYLOAD_DATA
  };
  PayLoad m_Payload = PAYLOAD_UNKNOWN;
  std::string m_ErrorMessage;
  Json::Value m_Data;
};

class cmServerRequest
{
public:
  cmServerResponse Reply(const Json::Value& data) const;
  cmServerResponse ReportError(const std::string& message) const;

  const std::string Type;
  const std::string Cookie;
  const Json::Value Data;
  cmConnection* Connection;

private:
  cmServerRequest(cmServer* server, cmConnection* connection,
                  const std::string& t, const std::string& c,
                  const Json::Value& d);

  void ReportProgress(int min, int current, int max,
                      const std::string& message) const;
  void ReportMessage(const std::string& message,
                     const std::string& title) const;

  cmServer* m_Server;

  friend class cmServer;
};

class cmServerProtocol
{
  CM_DISABLE_COPY(cmServerProtocol)

public:
  cmServerProtocol() = default;
  virtual ~cmServerProtocol() = default;

  virtual std::pair<int, int> ProtocolVersion() const = 0;
  virtual bool IsExperimental() const = 0;
  virtual const cmServerResponse Process(const cmServerRequest& request) = 0;

  bool Activate(cmServer* server, const cmServerRequest& request,
                std::string* errorMessage);

  cmFileMonitor* FileMonitor() const;
  void SendSignal(const std::string& name, const Json::Value& data) const;

protected:
  cmake* CMakeInstance() const;
  // Implement protocol specific activation tasks here. Called from Activate().
  virtual bool DoActivate(const cmServerRequest& request,
                          std::string* errorMessage);

private:
  std::unique_ptr<cmake> m_CMakeInstance;
  cmServer* m_Server = nullptr; // not owned!

  friend class cmServer;
};

class cmServerProtocol1 : public cmServerProtocol
{
public:
  std::pair<int, int> ProtocolVersion() const override;
  bool IsExperimental() const override;
  const cmServerResponse Process(const cmServerRequest& request) override;

private:
  bool DoActivate(const cmServerRequest& request,
                  std::string* errorMessage) override;

  void HandleCMakeFileChanges(const std::string& path, int event, int status);

  // Handle requests:
  cmServerResponse ProcessCache(const cmServerRequest& request);
  cmServerResponse ProcessCMakeInputs(const cmServerRequest& request);
  cmServerResponse ProcessCodeModel(const cmServerRequest& request);
  cmServerResponse ProcessCompute(const cmServerRequest& request);
  cmServerResponse ProcessConfigure(const cmServerRequest& request);
  cmServerResponse ProcessGlobalSettings(const cmServerRequest& request);
  cmServerResponse ProcessSetGlobalSettings(const cmServerRequest& request);
  cmServerResponse ProcessFileSystemWatchers(const cmServerRequest& request);
  cmServerResponse ProcessCTests(const cmServerRequest& request);

  enum State
  {
    STATE_INACTIVE,
    STATE_ACTIVE,
    STATE_CONFIGURED,
    STATE_COMPUTED
  };
  State m_State = STATE_INACTIVE;

  bool m_isDirty = false;

  struct GeneratorInformation
  {
  public:
    GeneratorInformation() = default;
    GeneratorInformation(const std::string& generatorName,
                         const std::string& extraGeneratorName,
                         const std::string& toolset,
                         const std::string& platform,
                         const std::string& sourceDirectory,
                         const std::string& buildDirectory);

    void SetupGenerator(cmake* cm, std::string* errorMessage);

    std::string GeneratorName;
    std::string ExtraGeneratorName;
    std::string Toolset;
    std::string Platform;

    std::string SourceDirectory;
    std::string BuildDirectory;
  };

  GeneratorInformation GeneratorInfo;
};
