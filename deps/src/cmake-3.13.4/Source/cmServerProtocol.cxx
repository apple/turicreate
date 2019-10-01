/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmServerProtocol.h"

#include "cmAlgorithms.h"
#include "cmExternalMakefileProjectGenerator.h"
#include "cmFileMonitor.h"
#include "cmGlobalGenerator.h"
#include "cmJsonObjectDictionary.h"
#include "cmJsonObjects.h"
#include "cmServer.h"
#include "cmServerDictionary.h"
#include "cmState.h"
#include "cmSystemTools.h"
#include "cm_uv.h"
#include "cmake.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Get rid of some windows macros:
#undef max

namespace {

std::vector<std::string> toStringList(const Json::Value& in)
{
  std::vector<std::string> result;
  for (auto const& it : in) {
    result.push_back(it.asString());
  }
  return result;
}

} // namespace

cmServerRequest::cmServerRequest(cmServer* server, cmConnection* connection,
                                 const std::string& t, const std::string& c,
                                 const Json::Value& d)
  : Type(t)
  , Cookie(c)
  , Data(d)
  , Connection(connection)
  , m_Server(server)
{
}

void cmServerRequest::ReportProgress(int min, int current, int max,
                                     const std::string& message) const
{
  this->m_Server->WriteProgress(*this, min, current, max, message);
}

void cmServerRequest::ReportMessage(const std::string& message,
                                    const std::string& title) const
{
  m_Server->WriteMessage(*this, message, title);
}

cmServerResponse cmServerRequest::Reply(const Json::Value& data) const
{
  cmServerResponse response(*this);
  response.SetData(data);
  return response;
}

cmServerResponse cmServerRequest::ReportError(const std::string& message) const
{
  cmServerResponse response(*this);
  response.SetError(message);
  return response;
}

cmServerResponse::cmServerResponse(const cmServerRequest& request)
  : Type(request.Type)
  , Cookie(request.Cookie)
{
}

void cmServerResponse::SetData(const Json::Value& data)
{
  assert(this->m_Payload == PAYLOAD_UNKNOWN);
  if (!data[kCOOKIE_KEY].isNull() || !data[kTYPE_KEY].isNull()) {
    this->SetError("Response contains cookie or type field.");
    return;
  }
  this->m_Payload = PAYLOAD_DATA;
  this->m_Data = data;
}

void cmServerResponse::SetError(const std::string& message)
{
  assert(this->m_Payload == PAYLOAD_UNKNOWN);
  this->m_Payload = PAYLOAD_ERROR;
  this->m_ErrorMessage = message;
}

bool cmServerResponse::IsComplete() const
{
  return this->m_Payload != PAYLOAD_UNKNOWN;
}

bool cmServerResponse::IsError() const
{
  assert(this->m_Payload != PAYLOAD_UNKNOWN);
  return this->m_Payload == PAYLOAD_ERROR;
}

std::string cmServerResponse::ErrorMessage() const
{
  if (this->m_Payload == PAYLOAD_ERROR) {
    return this->m_ErrorMessage;
  }
  return std::string();
}

Json::Value cmServerResponse::Data() const
{
  assert(this->m_Payload != PAYLOAD_UNKNOWN);
  return this->m_Data;
}

bool cmServerProtocol::Activate(cmServer* server,
                                const cmServerRequest& request,
                                std::string* errorMessage)
{
  assert(server);
  this->m_Server = server;
  this->m_CMakeInstance = cm::make_unique<cmake>(cmake::RoleProject);
  const bool result = this->DoActivate(request, errorMessage);
  if (!result) {
    this->m_CMakeInstance = nullptr;
  }
  return result;
}

cmFileMonitor* cmServerProtocol::FileMonitor() const
{
  return this->m_Server ? this->m_Server->FileMonitor() : nullptr;
}

void cmServerProtocol::SendSignal(const std::string& name,
                                  const Json::Value& data) const
{
  if (this->m_Server) {
    this->m_Server->WriteSignal(name, data);
  }
}

cmake* cmServerProtocol::CMakeInstance() const
{
  return this->m_CMakeInstance.get();
}

bool cmServerProtocol::DoActivate(const cmServerRequest& /*request*/,
                                  std::string* /*errorMessage*/)
{
  return true;
}

std::pair<int, int> cmServerProtocol1::ProtocolVersion() const
{
  return std::make_pair(1, 2);
}

static void setErrorMessage(std::string* errorMessage, const std::string& text)
{
  if (errorMessage) {
    *errorMessage = text;
  }
}

static bool getOrTestHomeDirectory(cmState* state, std::string& value,
                                   std::string* errorMessage)
{
  const std::string cachedValue =
    std::string(state->GetCacheEntryValue("CMAKE_HOME_DIRECTORY"));
  if (value.empty()) {
    value = cachedValue;
    return true;
  }
  const std::string suffix = "/CMakeLists.txt";
  const std::string cachedValueCML = cachedValue + suffix;
  const std::string valueCML = value + suffix;
  if (!cmSystemTools::SameFile(valueCML, cachedValueCML)) {
    setErrorMessage(errorMessage,
                    std::string("\"CMAKE_HOME_DIRECTORY\" is set but "
                                "incompatible with configured "
                                "source directory value."));
    return false;
  }
  return true;
}

static bool getOrTestValue(cmState* state, const std::string& key,
                           std::string& value,
                           const std::string& keyDescription,
                           std::string* errorMessage)
{
  const char* entry = state->GetCacheEntryValue(key);
  const std::string cachedValue =
    entry == nullptr ? std::string() : std::string(entry);
  if (value.empty()) {
    value = cachedValue;
  }
  if (!cachedValue.empty() && cachedValue != value) {
    setErrorMessage(errorMessage,
                    std::string("\"") + key +
                      "\" is set but incompatible with configured " +
                      keyDescription + " value.");
    return false;
  }
  return true;
}

bool cmServerProtocol1::DoActivate(const cmServerRequest& request,
                                   std::string* errorMessage)
{
  std::string sourceDirectory = request.Data[kSOURCE_DIRECTORY_KEY].asString();
  const std::string buildDirectory =
    request.Data[kBUILD_DIRECTORY_KEY].asString();
  std::string generator = request.Data[kGENERATOR_KEY].asString();
  std::string extraGenerator = request.Data[kEXTRA_GENERATOR_KEY].asString();
  std::string toolset = request.Data[kTOOLSET_KEY].asString();
  std::string platform = request.Data[kPLATFORM_KEY].asString();

  if (buildDirectory.empty()) {
    setErrorMessage(errorMessage,
                    std::string("\"") + kBUILD_DIRECTORY_KEY +
                      "\" is missing.");
    return false;
  }

  cmake* cm = CMakeInstance();
  if (cmSystemTools::PathExists(buildDirectory)) {
    if (!cmSystemTools::FileIsDirectory(buildDirectory)) {
      setErrorMessage(errorMessage,
                      std::string("\"") + kBUILD_DIRECTORY_KEY +
                        "\" exists but is not a directory.");
      return false;
    }

    const std::string cachePath = cm->FindCacheFile(buildDirectory);
    if (cm->LoadCache(cachePath)) {
      cmState* state = cm->GetState();

      // Check generator:
      if (!getOrTestValue(state, "CMAKE_GENERATOR", generator, "generator",
                          errorMessage)) {
        return false;
      }

      // check extra generator:
      if (!getOrTestValue(state, "CMAKE_EXTRA_GENERATOR", extraGenerator,
                          "extra generator", errorMessage)) {
        return false;
      }

      // check sourcedir:
      if (!getOrTestHomeDirectory(state, sourceDirectory, errorMessage)) {
        return false;
      }

      // check toolset:
      if (!getOrTestValue(state, "CMAKE_GENERATOR_TOOLSET", toolset, "toolset",
                          errorMessage)) {
        return false;
      }

      // check platform:
      if (!getOrTestValue(state, "CMAKE_GENERATOR_PLATFORM", platform,
                          "platform", errorMessage)) {
        return false;
      }
    }
  }

  if (sourceDirectory.empty()) {
    setErrorMessage(errorMessage,
                    std::string("\"") + kSOURCE_DIRECTORY_KEY +
                      "\" is unset but required.");
    return false;
  }
  if (!cmSystemTools::FileIsDirectory(sourceDirectory)) {
    setErrorMessage(errorMessage,
                    std::string("\"") + kSOURCE_DIRECTORY_KEY +
                      "\" is not a directory.");
    return false;
  }
  if (generator.empty()) {
    setErrorMessage(errorMessage,
                    std::string("\"") + kGENERATOR_KEY +
                      "\" is unset but required.");
    return false;
  }

  std::vector<cmake::GeneratorInfo> generators;
  cm->GetRegisteredGenerators(generators);
  auto baseIt = std::find_if(generators.begin(), generators.end(),
                             [&generator](const cmake::GeneratorInfo& info) {
                               return info.name == generator;
                             });
  if (baseIt == generators.end()) {
    setErrorMessage(errorMessage,
                    std::string("Generator \"") + generator +
                      "\" not supported.");
    return false;
  }
  auto extraIt = std::find_if(
    generators.begin(), generators.end(),
    [&generator, &extraGenerator](const cmake::GeneratorInfo& info) {
      return info.baseName == generator && info.extraName == extraGenerator;
    });
  if (extraIt == generators.end()) {
    setErrorMessage(errorMessage,
                    std::string("The combination of generator \"" + generator +
                                "\" and extra generator \"" + extraGenerator +
                                "\" is not supported."));
    return false;
  }
  if (!extraIt->supportsToolset && !toolset.empty()) {
    setErrorMessage(errorMessage,
                    std::string("Toolset was provided but is not supported by "
                                "the requested generator."));
    return false;
  }
  if (!extraIt->supportsPlatform && !platform.empty()) {
    setErrorMessage(errorMessage,
                    std::string("Platform was provided but is not supported "
                                "by the requested generator."));
    return false;
  }

  this->GeneratorInfo =
    GeneratorInformation(generator, extraGenerator, toolset, platform,
                         sourceDirectory, buildDirectory);

  this->m_State = STATE_ACTIVE;
  return true;
}

void cmServerProtocol1::HandleCMakeFileChanges(const std::string& path,
                                               int event, int status)
{
  assert(status == 0);
  static_cast<void>(status);

  if (!m_isDirty) {
    m_isDirty = true;
    SendSignal(kDIRTY_SIGNAL, Json::objectValue);
  }
  Json::Value obj = Json::objectValue;
  obj[kPATH_KEY] = path;
  Json::Value properties = Json::arrayValue;
  if (event & UV_RENAME) {
    properties.append(kRENAME_PROPERTY_VALUE);
  }
  if (event & UV_CHANGE) {
    properties.append(kCHANGE_PROPERTY_VALUE);
  }

  obj[kPROPERTIES_KEY] = properties;
  SendSignal(kFILE_CHANGE_SIGNAL, obj);
}

const cmServerResponse cmServerProtocol1::Process(
  const cmServerRequest& request)
{
  assert(this->m_State >= STATE_ACTIVE);

  if (request.Type == kCACHE_TYPE) {
    return this->ProcessCache(request);
  }
  if (request.Type == kCMAKE_INPUTS_TYPE) {
    return this->ProcessCMakeInputs(request);
  }
  if (request.Type == kCODE_MODEL_TYPE) {
    return this->ProcessCodeModel(request);
  }
  if (request.Type == kCOMPUTE_TYPE) {
    return this->ProcessCompute(request);
  }
  if (request.Type == kCONFIGURE_TYPE) {
    return this->ProcessConfigure(request);
  }
  if (request.Type == kFILESYSTEM_WATCHERS_TYPE) {
    return this->ProcessFileSystemWatchers(request);
  }
  if (request.Type == kGLOBAL_SETTINGS_TYPE) {
    return this->ProcessGlobalSettings(request);
  }
  if (request.Type == kSET_GLOBAL_SETTINGS_TYPE) {
    return this->ProcessSetGlobalSettings(request);
  }
  if (request.Type == kCTEST_INFO_TYPE) {
    return this->ProcessCTests(request);
  }

  return request.ReportError("Unknown command!");
}

bool cmServerProtocol1::IsExperimental() const
{
  return true;
}

cmServerResponse cmServerProtocol1::ProcessCache(
  const cmServerRequest& request)
{
  cmState* state = this->CMakeInstance()->GetState();

  Json::Value result = Json::objectValue;

  std::vector<std::string> allKeys = state->GetCacheEntryKeys();

  Json::Value list = Json::arrayValue;
  std::vector<std::string> keys = toStringList(request.Data[kKEYS_KEY]);
  if (keys.empty()) {
    keys = allKeys;
  } else {
    for (auto const& i : keys) {
      if (std::find(allKeys.begin(), allKeys.end(), i) == allKeys.end()) {
        return request.ReportError("Key \"" + i + "\" not found in cache.");
      }
    }
  }
  std::sort(keys.begin(), keys.end());
  for (auto const& key : keys) {
    Json::Value entry = Json::objectValue;
    entry[kKEY_KEY] = key;
    entry[kTYPE_KEY] =
      cmState::CacheEntryTypeToString(state->GetCacheEntryType(key));
    entry[kVALUE_KEY] = state->GetCacheEntryValue(key);

    Json::Value props = Json::objectValue;
    bool haveProperties = false;
    for (auto const& prop : state->GetCacheEntryPropertyList(key)) {
      haveProperties = true;
      props[prop] = state->GetCacheEntryProperty(key, prop);
    }
    if (haveProperties) {
      entry[kPROPERTIES_KEY] = props;
    }

    list.append(entry);
  }

  result[kCACHE_KEY] = list;
  return request.Reply(result);
}

cmServerResponse cmServerProtocol1::ProcessCMakeInputs(
  const cmServerRequest& request)
{
  if (this->m_State < STATE_CONFIGURED) {
    return request.ReportError("This instance was not yet configured.");
  }

  const cmake* cm = this->CMakeInstance();
  const std::string cmakeRootDir = cmSystemTools::GetCMakeRoot();
  const std::string& sourceDir = cm->GetHomeDirectory();

  Json::Value result = Json::objectValue;
  result[kSOURCE_DIRECTORY_KEY] = sourceDir;
  result[kCMAKE_ROOT_DIRECTORY_KEY] = cmakeRootDir;
  result[kBUILD_FILES_KEY] = cmDumpCMakeInputs(cm);
  return request.Reply(result);
}

cmServerResponse cmServerProtocol1::ProcessCodeModel(
  const cmServerRequest& request)
{
  if (this->m_State != STATE_COMPUTED) {
    return request.ReportError("No build system was generated yet.");
  }

  return request.Reply(cmDumpCodeModel(this->CMakeInstance()));
}

cmServerResponse cmServerProtocol1::ProcessCompute(
  const cmServerRequest& request)
{
  if (this->m_State > STATE_CONFIGURED) {
    return request.ReportError("This build system was already generated.");
  }
  if (this->m_State < STATE_CONFIGURED) {
    return request.ReportError("This project was not configured yet.");
  }

  cmake* cm = this->CMakeInstance();
  int ret = cm->Generate();

  if (ret < 0) {
    return request.ReportError("Failed to compute build system.");
  }
  m_State = STATE_COMPUTED;
  return request.Reply(Json::Value());
}

cmServerResponse cmServerProtocol1::ProcessConfigure(
  const cmServerRequest& request)
{
  if (this->m_State == STATE_INACTIVE) {
    return request.ReportError("This instance is inactive.");
  }

  FileMonitor()->StopMonitoring();

  std::string errorMessage;
  cmake* cm = this->CMakeInstance();
  this->GeneratorInfo.SetupGenerator(cm, &errorMessage);
  if (!errorMessage.empty()) {
    return request.ReportError(errorMessage);
  }

  // Make sure the types of cacheArguments matches (if given):
  std::vector<std::string> cacheArgs = { "unused" };
  bool cacheArgumentsError = false;
  const Json::Value passedArgs = request.Data[kCACHE_ARGUMENTS_KEY];
  if (!passedArgs.isNull()) {
    if (passedArgs.isString()) {
      cacheArgs.push_back(passedArgs.asString());
    } else if (passedArgs.isArray()) {
      for (auto const& arg : passedArgs) {
        if (!arg.isString()) {
          cacheArgumentsError = true;
          break;
        }
        cacheArgs.push_back(arg.asString());
      }
    } else {
      cacheArgumentsError = true;
    }
  }
  if (cacheArgumentsError) {
    request.ReportError(
      "cacheArguments must be unset, a string or an array of strings.");
  }

  std::string sourceDir = cm->GetHomeDirectory();
  const std::string buildDir = cm->GetHomeOutputDirectory();

  cmGlobalGenerator* gg = cm->GetGlobalGenerator();

  if (buildDir.empty()) {
    return request.ReportError("No build directory set via Handshake.");
  }

  if (cm->LoadCache(buildDir)) {
    // build directory has been set up before
    const std::string* cachedSourceDir =
      cm->GetState()->GetInitializedCacheValue("CMAKE_HOME_DIRECTORY");
    if (!cachedSourceDir) {
      return request.ReportError("No CMAKE_HOME_DIRECTORY found in cache.");
    }
    if (sourceDir.empty()) {
      sourceDir = *cachedSourceDir;
      cm->SetHomeDirectory(sourceDir);
    }

    const std::string* cachedGenerator =
      cm->GetState()->GetInitializedCacheValue("CMAKE_GENERATOR");
    if (cachedGenerator) {
      if (gg && gg->GetName() != *cachedGenerator) {
        return request.ReportError("Configured generator does not match with "
                                   "CMAKE_GENERATOR found in cache.");
      }
    }
  } else {
    // build directory has not been set up before
    if (sourceDir.empty()) {
      return request.ReportError("No sourceDirectory set via "
                                 "setGlobalSettings and no cache found in "
                                 "buildDirectory.");
    }
  }

  cmSystemTools::ResetErrorOccuredFlag(); // Reset error state

  if (cm->AddCMakePaths() != 1) {
    return request.ReportError("Failed to set CMake paths.");
  }

  if (!cm->SetCacheArgs(cacheArgs)) {
    return request.ReportError("cacheArguments could not be set.");
  }

  int ret = cm->Configure();
  if (ret < 0) {
    return request.ReportError("Configuration failed.");
  }

  std::vector<std::string> toWatchList;
  cmGetCMakeInputs(gg, std::string(), buildDir, nullptr, &toWatchList,
                   nullptr);

  FileMonitor()->MonitorPaths(toWatchList,
                              [this](const std::string& p, int e, int s) {
                                this->HandleCMakeFileChanges(p, e, s);
                              });

  m_State = STATE_CONFIGURED;
  m_isDirty = false;
  return request.Reply(Json::Value());
}

cmServerResponse cmServerProtocol1::ProcessGlobalSettings(
  const cmServerRequest& request)
{
  cmake* cm = this->CMakeInstance();
  Json::Value obj = Json::objectValue;

  // Capabilities information:
  obj[kCAPABILITIES_KEY] = cm->ReportCapabilitiesJson(true);

  obj[kDEBUG_OUTPUT_KEY] = cm->GetDebugOutput();
  obj[kTRACE_KEY] = cm->GetTrace();
  obj[kTRACE_EXPAND_KEY] = cm->GetTraceExpand();
  obj[kWARN_UNINITIALIZED_KEY] = cm->GetWarnUninitialized();
  obj[kWARN_UNUSED_KEY] = cm->GetWarnUnused();
  obj[kWARN_UNUSED_CLI_KEY] = cm->GetWarnUnusedCli();
  obj[kCHECK_SYSTEM_VARS_KEY] = cm->GetCheckSystemVars();

  obj[kSOURCE_DIRECTORY_KEY] = this->GeneratorInfo.SourceDirectory;
  obj[kBUILD_DIRECTORY_KEY] = this->GeneratorInfo.BuildDirectory;

  // Currently used generator:
  obj[kGENERATOR_KEY] = this->GeneratorInfo.GeneratorName;
  obj[kEXTRA_GENERATOR_KEY] = this->GeneratorInfo.ExtraGeneratorName;

  return request.Reply(obj);
}

static void setBool(const cmServerRequest& request, const std::string& key,
                    std::function<void(bool)> const& setter)
{
  if (request.Data[key].isNull()) {
    return;
  }
  setter(request.Data[key].asBool());
}

cmServerResponse cmServerProtocol1::ProcessSetGlobalSettings(
  const cmServerRequest& request)
{
  const std::vector<std::string> boolValues = {
    kDEBUG_OUTPUT_KEY,       kTRACE_KEY,       kTRACE_EXPAND_KEY,
    kWARN_UNINITIALIZED_KEY, kWARN_UNUSED_KEY, kWARN_UNUSED_CLI_KEY,
    kCHECK_SYSTEM_VARS_KEY
  };
  for (std::string const& i : boolValues) {
    if (!request.Data[i].isNull() && !request.Data[i].isBool()) {
      return request.ReportError("\"" + i +
                                 "\" must be unset or a bool value.");
    }
  }

  cmake* cm = this->CMakeInstance();

  setBool(request, kDEBUG_OUTPUT_KEY,
          [cm](bool e) { cm->SetDebugOutputOn(e); });
  setBool(request, kTRACE_KEY, [cm](bool e) { cm->SetTrace(e); });
  setBool(request, kTRACE_EXPAND_KEY, [cm](bool e) { cm->SetTraceExpand(e); });
  setBool(request, kWARN_UNINITIALIZED_KEY,
          [cm](bool e) { cm->SetWarnUninitialized(e); });
  setBool(request, kWARN_UNUSED_KEY, [cm](bool e) { cm->SetWarnUnused(e); });
  setBool(request, kWARN_UNUSED_CLI_KEY,
          [cm](bool e) { cm->SetWarnUnusedCli(e); });
  setBool(request, kCHECK_SYSTEM_VARS_KEY,
          [cm](bool e) { cm->SetCheckSystemVars(e); });

  return request.Reply(Json::Value());
}

cmServerResponse cmServerProtocol1::ProcessFileSystemWatchers(
  const cmServerRequest& request)
{
  const cmFileMonitor* const fm = FileMonitor();
  Json::Value result = Json::objectValue;
  Json::Value files = Json::arrayValue;
  for (auto const& f : fm->WatchedFiles()) {
    files.append(f);
  }
  Json::Value directories = Json::arrayValue;
  for (auto const& d : fm->WatchedDirectories()) {
    directories.append(d);
  }
  result[kWATCHED_FILES_KEY] = files;
  result[kWATCHED_DIRECTORIES_KEY] = directories;

  return request.Reply(result);
}

cmServerResponse cmServerProtocol1::ProcessCTests(
  const cmServerRequest& request)
{
  if (this->m_State < STATE_COMPUTED) {
    return request.ReportError("This instance was not yet computed.");
  }

  return request.Reply(cmDumpCTestInfo(this->CMakeInstance()));
}

cmServerProtocol1::GeneratorInformation::GeneratorInformation(
  const std::string& generatorName, const std::string& extraGeneratorName,
  const std::string& toolset, const std::string& platform,
  const std::string& sourceDirectory, const std::string& buildDirectory)
  : GeneratorName(generatorName)
  , ExtraGeneratorName(extraGeneratorName)
  , Toolset(toolset)
  , Platform(platform)
  , SourceDirectory(sourceDirectory)
  , BuildDirectory(buildDirectory)
{
}

void cmServerProtocol1::GeneratorInformation::SetupGenerator(
  cmake* cm, std::string* errorMessage)
{
  const std::string fullGeneratorName =
    cmExternalMakefileProjectGenerator::CreateFullGeneratorName(
      GeneratorName, ExtraGeneratorName);

  cm->SetHomeDirectory(SourceDirectory);
  cm->SetHomeOutputDirectory(BuildDirectory);

  cmGlobalGenerator* gg = cm->CreateGlobalGenerator(fullGeneratorName);
  if (!gg) {
    setErrorMessage(
      errorMessage,
      std::string("Could not set up the requested combination of \"") +
        kGENERATOR_KEY + "\" and \"" + kEXTRA_GENERATOR_KEY + "\"");
    return;
  }

  cm->SetGlobalGenerator(gg);

  cm->SetGeneratorToolset(Toolset);
  cm->SetGeneratorPlatform(Platform);
}
