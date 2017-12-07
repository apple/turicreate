/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTest.h"

#include "cm_curl.h"
#include "cm_zlib.h"
#include "cmsys/Base64.h"
#include "cmsys/Directory.hxx"
#include "cmsys/FStream.hxx"
#include "cmsys/Glob.hxx"
#include "cmsys/Process.h"
#include "cmsys/String.hxx"
#include "cmsys/SystemInformation.hxx"
#include <ctype.h>
#include <iostream>
#include <map>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include <utility>
#include <vector>

#include "cmAlgorithms.h"
#include "cmCTestBuildAndTestHandler.h"
#include "cmCTestBuildHandler.h"
#include "cmCTestConfigureHandler.h"
#include "cmCTestCoverageHandler.h"
#include "cmCTestGenericHandler.h"
#include "cmCTestMemCheckHandler.h"
#include "cmCTestScriptHandler.h"
#include "cmCTestStartCommand.h"
#include "cmCTestSubmitHandler.h"
#include "cmCTestTestHandler.h"
#include "cmCTestUpdateHandler.h"
#include "cmCTestUploadHandler.h"
#include "cmCurl.h"
#include "cmDynamicLoader.h"
#include "cmGeneratedFileStream.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmProcessOutput.h"
#include "cmState.h"
#include "cmStateSnapshot.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmVersion.h"
#include "cmVersionConfig.h"
#include "cmXMLWriter.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

#if defined(__BEOS__) || defined(__HAIKU__)
#include <be/kernel/OS.h> /* disable_debugger() API. */
#endif

#define DEBUGOUT                                                              \
  std::cout << __LINE__ << " ";                                               \
  std::cout
#define DEBUGERR                                                              \
  std::cerr << __LINE__ << " ";                                               \
  std::cerr

struct tm* cmCTest::GetNightlyTime(std::string const& str, bool tomorrowtag)
{
  struct tm* lctime;
  time_t tctime = time(CM_NULLPTR);
  lctime = gmtime(&tctime);
  char buf[1024];
  // add todays year day and month to the time in str because
  // curl_getdate no longer assumes the day is today
  sprintf(buf, "%d%02d%02d %s", lctime->tm_year + 1900, lctime->tm_mon + 1,
          lctime->tm_mday, str.c_str());
  cmCTestLog(this, OUTPUT, "Determine Nightly Start Time"
               << std::endl
               << "   Specified time: " << str << std::endl);
  // Convert the nightly start time to seconds. Since we are
  // providing only a time and a timezone, the current date of
  // the local machine is assumed. Consequently, nightlySeconds
  // is the time at which the nightly dashboard was opened or
  // will be opened on the date of the current client machine.
  // As such, this time may be in the past or in the future.
  time_t ntime = curl_getdate(buf, &tctime);
  cmCTestLog(this, DEBUG, "   Get curl time: " << ntime << std::endl);
  tctime = time(CM_NULLPTR);
  cmCTestLog(this, DEBUG, "   Get the current time: " << tctime << std::endl);

  const int dayLength = 24 * 60 * 60;
  cmCTestLog(this, DEBUG, "Seconds: " << tctime << std::endl);
  while (ntime > tctime) {
    // If nightlySeconds is in the past, this is the current
    // open dashboard, then return nightlySeconds.  If
    // nightlySeconds is in the future, this is the next
    // dashboard to be opened, so subtract 24 hours to get the
    // time of the current open dashboard
    ntime -= dayLength;
    cmCTestLog(this, DEBUG, "Pick yesterday" << std::endl);
    cmCTestLog(this, DEBUG, "   Future time, subtract day: " << ntime
                                                             << std::endl);
  }
  while (tctime > (ntime + dayLength)) {
    ntime += dayLength;
    cmCTestLog(this, DEBUG, "   Past time, add day: " << ntime << std::endl);
  }
  cmCTestLog(this, DEBUG, "nightlySeconds: " << ntime << std::endl);
  cmCTestLog(this, DEBUG, "   Current time: " << tctime << " Nightly time: "
                                              << ntime << std::endl);
  if (tomorrowtag) {
    cmCTestLog(this, OUTPUT, "   Use future tag, Add a day" << std::endl);
    ntime += dayLength;
  }
  lctime = gmtime(&ntime);
  return lctime;
}

std::string cmCTest::CleanString(const std::string& str)
{
  std::string::size_type spos = str.find_first_not_of(" \n\t\r\f\v");
  std::string::size_type epos = str.find_last_not_of(" \n\t\r\f\v");
  if (spos == std::string::npos) {
    return std::string();
  }
  if (epos != std::string::npos) {
    epos = epos - spos + 1;
  }
  return str.substr(spos, epos);
}

std::string cmCTest::CurrentTime()
{
  time_t currenttime = time(CM_NULLPTR);
  struct tm* t = localtime(&currenttime);
  // return ::CleanString(ctime(&currenttime));
  char current_time[1024];
  if (this->ShortDateFormat) {
    strftime(current_time, 1000, "%b %d %H:%M %Z", t);
  } else {
    strftime(current_time, 1000, "%a %b %d %H:%M:%S %Z %Y", t);
  }
  cmCTestLog(this, DEBUG, "   Current_Time: " << current_time << std::endl);
  return cmCTest::CleanString(current_time);
}

std::string cmCTest::GetCostDataFile()
{
  std::string fname = this->GetCTestConfiguration("CostDataFile");
  if (fname == "") {
    fname = this->GetBinaryDir() + "/Testing/Temporary/CTestCostData.txt";
  }
  return fname;
}

#ifdef CMAKE_BUILD_WITH_CMAKE
static size_t HTTPResponseCallback(void* ptr, size_t size, size_t nmemb,
                                   void* data)
{
  int realsize = (int)(size * nmemb);

  std::string* response = static_cast<std::string*>(data);
  const char* chPtr = static_cast<char*>(ptr);
  *response += chPtr;

  return realsize;
}

int cmCTest::HTTPRequest(std::string url, HTTPMethod method,
                         std::string& response, std::string const& fields,
                         std::string const& putFile, int timeout)
{
  CURL* curl;
  FILE* file;
  ::curl_global_init(CURL_GLOBAL_ALL);
  curl = ::curl_easy_init();
  cmCurlSetCAInfo(curl);

  // set request options based on method
  switch (method) {
    case cmCTest::HTTP_POST:
      ::curl_easy_setopt(curl, CURLOPT_POST, 1);
      ::curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields.c_str());
      break;
    case cmCTest::HTTP_PUT:
      if (!cmSystemTools::FileExists(putFile.c_str())) {
        response = "Error: File ";
        response += putFile + " does not exist.\n";
        return -1;
      }
      ::curl_easy_setopt(curl, CURLOPT_PUT, 1);
      file = cmsys::SystemTools::Fopen(putFile, "rb");
      ::curl_easy_setopt(curl, CURLOPT_INFILE, file);
    // fall through to append GET fields
    case cmCTest::HTTP_GET:
      if (!fields.empty()) {
        url += "?" + fields;
      }
      break;
  }

  ::curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  ::curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
  ::curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

  // set response options
  ::curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HTTPResponseCallback);
  ::curl_easy_setopt(curl, CURLOPT_FILE, (void*)&response);
  ::curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);

  CURLcode res = ::curl_easy_perform(curl);

  ::curl_easy_cleanup(curl);
  ::curl_global_cleanup();

  return static_cast<int>(res);
}
#endif

std::string cmCTest::MakeURLSafe(const std::string& str)
{
  std::ostringstream ost;
  char buffer[10];
  for (std::string::size_type pos = 0; pos < str.size(); pos++) {
    unsigned char ch = str[pos];
    if ((ch > 126 || ch < 32 || ch == '&' || ch == '%' || ch == '+' ||
         ch == '=' || ch == '@') &&
        ch != 9) {
      sprintf(buffer, "%02x;", (unsigned int)ch);
      ost << buffer;
    } else {
      ost << ch;
    }
  }
  return ost.str();
}

std::string cmCTest::DecodeURL(const std::string& in)
{
  std::string out;
  for (const char* c = in.c_str(); *c; ++c) {
    if (*c == '%' && isxdigit(*(c + 1)) && isxdigit(*(c + 2))) {
      char buf[3] = { *(c + 1), *(c + 2), 0 };
      out.append(1, char(strtoul(buf, CM_NULLPTR, 16)));
      c += 2;
    } else {
      out.append(1, *c);
    }
  }
  return out;
}

cmCTest::cmCTest()
{
  this->LabelSummary = true;
  this->ParallelLevel = 1;
  this->ParallelLevelSetInCli = false;
  this->TestLoad = 0;
  this->SubmitIndex = 0;
  this->Failover = false;
  this->BatchJobs = false;
  this->ForceNewCTestProcess = false;
  this->TomorrowTag = false;
  this->Verbose = false;

  this->Debug = false;
  this->ShowLineNumbers = false;
  this->Quiet = false;
  this->ExtraVerbose = false;
  this->ProduceXML = false;
  this->ShowOnly = false;
  this->RunConfigurationScript = false;
  this->UseHTTP10 = false;
  this->PrintLabels = false;
  this->CompressTestOutput = true;
  this->TestModel = cmCTest::EXPERIMENTAL;
  this->MaxTestNameWidth = 30;
  this->InteractiveDebugMode = true;
  this->TimeOut = 0;
  this->GlobalTimeout = 0;
  this->LastStopTimeout = 24 * 60 * 60;
  this->CompressXMLFiles = false;
  this->CTestConfigFile = "";
  this->ScheduleType = "";
  this->StopTime = "";
  this->NextDayStopTime = false;
  this->OutputLogFile = CM_NULLPTR;
  this->OutputLogFileLastTag = -1;
  this->SuppressUpdatingCTestConfiguration = false;
  this->DartVersion = 1;
  this->DropSiteCDash = false;
  this->OutputTestOutputOnTestFailure = false;
  this->RepeatTests = 1; // default to run each test once
  this->RepeatUntilFail = false;
  std::string outOnFail;
  if (cmSystemTools::GetEnv("CTEST_OUTPUT_ON_FAILURE", outOnFail)) {
    this->OutputTestOutputOnTestFailure =
      !cmSystemTools::IsOff(outOnFail.c_str());
  }
  this->InitStreams();

  this->Parts[PartStart].SetName("Start");
  this->Parts[PartUpdate].SetName("Update");
  this->Parts[PartConfigure].SetName("Configure");
  this->Parts[PartBuild].SetName("Build");
  this->Parts[PartTest].SetName("Test");
  this->Parts[PartCoverage].SetName("Coverage");
  this->Parts[PartMemCheck].SetName("MemCheck");
  this->Parts[PartSubmit].SetName("Submit");
  this->Parts[PartNotes].SetName("Notes");
  this->Parts[PartExtraFiles].SetName("ExtraFiles");
  this->Parts[PartUpload].SetName("Upload");

  // Fill the part name-to-id map.
  for (Part p = PartStart; p != PartCount; p = Part(p + 1)) {
    this->PartMap[cmSystemTools::LowerCase(this->Parts[p].GetName())] = p;
  }

  this->ShortDateFormat = true;

  this->TestingHandlers["build"] = new cmCTestBuildHandler;
  this->TestingHandlers["buildtest"] = new cmCTestBuildAndTestHandler;
  this->TestingHandlers["coverage"] = new cmCTestCoverageHandler;
  this->TestingHandlers["script"] = new cmCTestScriptHandler;
  this->TestingHandlers["test"] = new cmCTestTestHandler;
  this->TestingHandlers["update"] = new cmCTestUpdateHandler;
  this->TestingHandlers["configure"] = new cmCTestConfigureHandler;
  this->TestingHandlers["memcheck"] = new cmCTestMemCheckHandler;
  this->TestingHandlers["submit"] = new cmCTestSubmitHandler;
  this->TestingHandlers["upload"] = new cmCTestUploadHandler;

  cmCTest::t_TestingHandlers::iterator it;
  for (it = this->TestingHandlers.begin(); it != this->TestingHandlers.end();
       ++it) {
    it->second->SetCTestInstance(this);
  }

  // Make sure we can capture the build tool output.
  cmSystemTools::EnableVSConsoleOutput();
}

cmCTest::~cmCTest()
{
  cmDeleteAll(this->TestingHandlers);
  this->SetOutputLogFileName(CM_NULLPTR);
}

void cmCTest::SetParallelLevel(int level)
{
  this->ParallelLevel = level < 1 ? 1 : level;
}

void cmCTest::SetTestLoad(unsigned long load)
{
  this->TestLoad = load;
}

bool cmCTest::ShouldCompressTestOutput()
{
  return this->CompressTestOutput;
}

cmCTest::Part cmCTest::GetPartFromName(const char* name)
{
  // Look up by lower-case to make names case-insensitive.
  std::string lower_name = cmSystemTools::LowerCase(name);
  PartMapType::const_iterator i = this->PartMap.find(lower_name);
  if (i != this->PartMap.end()) {
    return i->second;
  }

  // The string does not name a valid part.
  return PartCount;
}

int cmCTest::Initialize(const char* binary_dir, cmCTestStartCommand* command)
{
  bool quiet = false;
  if (command && command->ShouldBeQuiet()) {
    quiet = true;
  }

  cmCTestOptionalLog(this, DEBUG, "Here: " << __LINE__ << std::endl, quiet);
  if (!this->InteractiveDebugMode) {
    this->BlockTestErrorDiagnostics();
  } else {
    cmSystemTools::PutEnv("CTEST_INTERACTIVE_DEBUG_MODE=1");
  }

  this->BinaryDir = binary_dir;
  cmSystemTools::ConvertToUnixSlashes(this->BinaryDir);

  this->UpdateCTestConfiguration();

  cmCTestOptionalLog(this, DEBUG, "Here: " << __LINE__ << std::endl, quiet);
  if (this->ProduceXML) {
    cmCTestOptionalLog(this, DEBUG, "Here: " << __LINE__ << std::endl, quiet);
    cmCTestOptionalLog(
      this, OUTPUT, "   Site: "
        << this->GetCTestConfiguration("Site") << std::endl
        << "   Build name: "
        << cmCTest::SafeBuildIdField(this->GetCTestConfiguration("BuildName"))
        << std::endl,
      quiet);
    cmCTestOptionalLog(this, DEBUG, "Produce XML is on" << std::endl, quiet);
    if (this->TestModel == cmCTest::NIGHTLY &&
        this->GetCTestConfiguration("NightlyStartTime").empty()) {
      cmCTestOptionalLog(
        this, WARNING,
        "WARNING: No nightly start time found please set in CTestConfig.cmake"
        " or DartConfig.cmake"
          << std::endl,
        quiet);
      cmCTestOptionalLog(this, DEBUG, "Here: " << __LINE__ << std::endl,
                         quiet);
      return 0;
    }
  }

  cmake cm(cmake::RoleScript);
  cm.SetHomeDirectory("");
  cm.SetHomeOutputDirectory("");
  cm.GetCurrentSnapshot().SetDefaultDefinitions();
  cmGlobalGenerator gg(&cm);
  CM_AUTO_PTR<cmMakefile> mf(new cmMakefile(&gg, cm.GetCurrentSnapshot()));
  if (!this->ReadCustomConfigurationFileTree(this->BinaryDir.c_str(),
                                             mf.get())) {
    cmCTestOptionalLog(
      this, DEBUG, "Cannot find custom configuration file tree" << std::endl,
      quiet);
    return 0;
  }

  if (this->ProduceXML) {
    // Verify "Testing" directory exists:
    //
    std::string testingDir = this->BinaryDir + "/Testing";
    if (cmSystemTools::FileExists(testingDir.c_str())) {
      if (!cmSystemTools::FileIsDirectory(testingDir)) {
        cmCTestLog(this, ERROR_MESSAGE, "File "
                     << testingDir
                     << " is in the place of the testing directory"
                     << std::endl);
        return 0;
      }
    } else {
      if (!cmSystemTools::MakeDirectory(testingDir.c_str())) {
        cmCTestLog(this, ERROR_MESSAGE, "Cannot create directory "
                     << testingDir << std::endl);
        return 0;
      }
    }

    // Create new "TAG" file or read existing one:
    //
    bool createNewTag = true;
    if (command) {
      createNewTag = command->ShouldCreateNewTag();
    }

    std::string tagfile = testingDir + "/TAG";
    cmsys::ifstream tfin(tagfile.c_str());
    std::string tag;

    if (createNewTag) {
      time_t tctime = time(CM_NULLPTR);
      if (this->TomorrowTag) {
        tctime += (24 * 60 * 60);
      }
      struct tm* lctime = gmtime(&tctime);
      if (tfin && cmSystemTools::GetLineFromStream(tfin, tag)) {
        int year = 0;
        int mon = 0;
        int day = 0;
        int hour = 0;
        int min = 0;
        sscanf(tag.c_str(), "%04d%02d%02d-%02d%02d", &year, &mon, &day, &hour,
               &min);
        if (year != lctime->tm_year + 1900 || mon != lctime->tm_mon + 1 ||
            day != lctime->tm_mday) {
          tag = "";
        }
        std::string tagmode;
        if (cmSystemTools::GetLineFromStream(tfin, tagmode)) {
          if (tagmode.size() > 4 && !this->Parts[PartStart]) {
            this->TestModel = cmCTest::GetTestModelFromString(tagmode.c_str());
          }
        }
        tfin.close();
      }
      if (tag.empty() || (CM_NULLPTR != command) || this->Parts[PartStart]) {
        cmCTestOptionalLog(
          this, DEBUG,
          "TestModel: " << this->GetTestModelString() << std::endl, quiet);
        cmCTestOptionalLog(
          this, DEBUG, "TestModel: " << this->TestModel << std::endl, quiet);
        if (this->TestModel == cmCTest::NIGHTLY) {
          lctime = this->GetNightlyTime(
            this->GetCTestConfiguration("NightlyStartTime"),
            this->TomorrowTag);
        }
        char datestring[100];
        sprintf(datestring, "%04d%02d%02d-%02d%02d", lctime->tm_year + 1900,
                lctime->tm_mon + 1, lctime->tm_mday, lctime->tm_hour,
                lctime->tm_min);
        tag = datestring;
        cmsys::ofstream ofs(tagfile.c_str());
        if (ofs) {
          ofs << tag << std::endl;
          ofs << this->GetTestModelString() << std::endl;
        }
        ofs.close();
        if (CM_NULLPTR == command) {
          cmCTestOptionalLog(this, OUTPUT, "Create new tag: "
                               << tag << " - " << this->GetTestModelString()
                               << std::endl,
                             quiet);
        }
      }
    } else {
      if (tfin) {
        cmSystemTools::GetLineFromStream(tfin, tag);
        tfin.close();
      }

      if (tag.empty()) {
        cmCTestLog(this, ERROR_MESSAGE, "Cannot read existing TAG file in "
                     << testingDir << std::endl);
        return 0;
      }

      cmCTestOptionalLog(this, OUTPUT, "  Use existing tag: "
                           << tag << " - " << this->GetTestModelString()
                           << std::endl,
                         quiet);
    }

    this->CurrentTag = tag;
  }

  return 1;
}

bool cmCTest::InitializeFromCommand(cmCTestStartCommand* command)
{
  std::string src_dir = this->GetCTestConfiguration("SourceDirectory");
  std::string bld_dir = this->GetCTestConfiguration("BuildDirectory");
  this->DartVersion = 1;
  this->DropSiteCDash = false;
  for (Part p = PartStart; p != PartCount; p = Part(p + 1)) {
    this->Parts[p].SubmitFiles.clear();
  }

  cmMakefile* mf = command->GetMakefile();
  std::string fname;

  std::string src_dir_fname = src_dir;
  src_dir_fname += "/CTestConfig.cmake";
  cmSystemTools::ConvertToUnixSlashes(src_dir_fname);

  std::string bld_dir_fname = bld_dir;
  bld_dir_fname += "/CTestConfig.cmake";
  cmSystemTools::ConvertToUnixSlashes(bld_dir_fname);

  if (cmSystemTools::FileExists(bld_dir_fname.c_str())) {
    fname = bld_dir_fname;
  } else if (cmSystemTools::FileExists(src_dir_fname.c_str())) {
    fname = src_dir_fname;
  }

  if (!fname.empty()) {
    cmCTestOptionalLog(this, OUTPUT, "   Reading ctest configuration file: "
                         << fname << std::endl,
                       command->ShouldBeQuiet());
    bool readit = mf->ReadDependentFile(fname.c_str());
    if (!readit) {
      std::string m = "Could not find include file: ";
      m += fname;
      command->SetError(m);
      return false;
    }
  } else {
    cmCTestOptionalLog(this, WARNING,
                       "Cannot locate CTest configuration: in BuildDirectory: "
                         << bld_dir_fname << std::endl,
                       command->ShouldBeQuiet());
    cmCTestOptionalLog(
      this, WARNING, "Cannot locate CTest configuration: in SourceDirectory: "
        << src_dir_fname << std::endl,
      command->ShouldBeQuiet());
  }

  this->SetCTestConfigurationFromCMakeVariable(mf, "NightlyStartTime",
                                               "CTEST_NIGHTLY_START_TIME",
                                               command->ShouldBeQuiet());
  this->SetCTestConfigurationFromCMakeVariable(mf, "Site", "CTEST_SITE",
                                               command->ShouldBeQuiet());
  this->SetCTestConfigurationFromCMakeVariable(
    mf, "BuildName", "CTEST_BUILD_NAME", command->ShouldBeQuiet());
  const char* dartVersion = mf->GetDefinition("CTEST_DART_SERVER_VERSION");
  if (dartVersion) {
    this->DartVersion = atoi(dartVersion);
    if (this->DartVersion < 0) {
      cmCTestLog(this, ERROR_MESSAGE, "Invalid Dart server version: "
                   << dartVersion << ". Please specify the version number."
                   << std::endl);
      return false;
    }
  }
  this->DropSiteCDash = mf->IsOn("CTEST_DROP_SITE_CDASH");

  if (!this->Initialize(bld_dir.c_str(), command)) {
    return false;
  }
  cmCTestOptionalLog(this, OUTPUT, "   Use "
                       << this->GetTestModelString()
                       << " tag: " << this->GetCurrentTag() << std::endl,
                     command->ShouldBeQuiet());
  return true;
}

bool cmCTest::UpdateCTestConfiguration()
{
  if (this->SuppressUpdatingCTestConfiguration) {
    return true;
  }
  std::string fileName = this->CTestConfigFile;
  if (fileName.empty()) {
    fileName = this->BinaryDir + "/CTestConfiguration.ini";
    if (!cmSystemTools::FileExists(fileName.c_str())) {
      fileName = this->BinaryDir + "/DartConfiguration.tcl";
    }
  }
  cmCTestLog(this, HANDLER_VERBOSE_OUTPUT,
             "UpdateCTestConfiguration  from :" << fileName << "\n");
  if (!cmSystemTools::FileExists(fileName.c_str())) {
    // No need to exit if we are not producing XML
    if (this->ProduceXML) {
      cmCTestLog(this, ERROR_MESSAGE, "Cannot find file: " << fileName
                                                           << std::endl);
      return false;
    }
  } else {
    cmCTestLog(this, HANDLER_VERBOSE_OUTPUT, "Parse Config file:" << fileName
                                                                  << "\n");
    // parse the dart test file
    cmsys::ifstream fin(fileName.c_str());

    if (!fin) {
      return false;
    }

    char buffer[1024];
    while (fin) {
      buffer[0] = 0;
      fin.getline(buffer, 1023);
      buffer[1023] = 0;
      std::string line = cmCTest::CleanString(buffer);
      if (line.empty()) {
        continue;
      }
      while (fin && (line[line.size() - 1] == '\\')) {
        line = line.substr(0, line.size() - 1);
        buffer[0] = 0;
        fin.getline(buffer, 1023);
        buffer[1023] = 0;
        line += cmCTest::CleanString(buffer);
      }
      if (line[0] == '#') {
        continue;
      }
      std::string::size_type cpos = line.find_first_of(':');
      if (cpos == std::string::npos) {
        continue;
      }
      std::string key = line.substr(0, cpos);
      std::string value = cmCTest::CleanString(line.substr(cpos + 1));
      this->CTestConfiguration[key] = value;
    }
    fin.close();
  }
  if (!this->GetCTestConfiguration("BuildDirectory").empty()) {
    this->BinaryDir = this->GetCTestConfiguration("BuildDirectory");
    cmSystemTools::ChangeDirectory(this->BinaryDir);
  }
  this->TimeOut = atoi(this->GetCTestConfiguration("TimeOut").c_str());
  std::string const& testLoad = this->GetCTestConfiguration("TestLoad");
  if (!testLoad.empty()) {
    unsigned long load;
    if (cmSystemTools::StringToULong(testLoad.c_str(), &load)) {
      this->SetTestLoad(load);
    } else {
      cmCTestLog(this, WARNING,
                 "Invalid value for 'Test Load' : " << testLoad << std::endl);
    }
  }
  if (this->ProduceXML) {
    this->CompressXMLFiles = cmSystemTools::IsOn(
      this->GetCTestConfiguration("CompressSubmission").c_str());
  }
  return true;
}

void cmCTest::BlockTestErrorDiagnostics()
{
  cmSystemTools::PutEnv("DART_TEST_FROM_DART=1");
  cmSystemTools::PutEnv("DASHBOARD_TEST_FROM_CTEST=" CMake_VERSION);
#if defined(_WIN32)
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#elif defined(__BEOS__) || defined(__HAIKU__)
  disable_debugger(1);
#endif
}

void cmCTest::SetTestModel(int mode)
{
  this->InteractiveDebugMode = false;
  this->TestModel = mode;
}

bool cmCTest::SetTest(const char* ttype, bool report)
{
  if (cmSystemTools::LowerCase(ttype) == "all") {
    for (Part p = PartStart; p != PartCount; p = Part(p + 1)) {
      this->Parts[p].Enable();
    }
    return true;
  }
  Part p = this->GetPartFromName(ttype);
  if (p != PartCount) {
    this->Parts[p].Enable();
    return true;
  }
  if (report) {
    cmCTestLog(this, ERROR_MESSAGE, "Don't know about test \""
                 << ttype << "\" yet..." << std::endl);
  }
  return false;
}

void cmCTest::Finalize()
{
}

bool cmCTest::OpenOutputFile(const std::string& path, const std::string& name,
                             cmGeneratedFileStream& stream, bool compress)
{
  std::string testingDir = this->BinaryDir + "/Testing";
  if (!path.empty()) {
    testingDir += "/" + path;
  }
  if (cmSystemTools::FileExists(testingDir.c_str())) {
    if (!cmSystemTools::FileIsDirectory(testingDir)) {
      cmCTestLog(this, ERROR_MESSAGE, "File "
                   << testingDir << " is in the place of the testing directory"
                   << std::endl);
      return false;
    }
  } else {
    if (!cmSystemTools::MakeDirectory(testingDir.c_str())) {
      cmCTestLog(this, ERROR_MESSAGE, "Cannot create directory " << testingDir
                                                                 << std::endl);
      return false;
    }
  }
  std::string filename = testingDir + "/" + name;
  stream.Open(filename.c_str());
  if (!stream) {
    cmCTestLog(this, ERROR_MESSAGE, "Problem opening file: " << filename
                                                             << std::endl);
    return false;
  }
  if (compress) {
    if (this->CompressXMLFiles) {
      stream.SetCompression(true);
    }
  }
  return true;
}

bool cmCTest::AddIfExists(Part part, const char* file)
{
  if (this->CTestFileExists(file)) {
    this->AddSubmitFile(part, file);
  } else {
    std::string name = file;
    name += ".gz";
    if (this->CTestFileExists(name)) {
      this->AddSubmitFile(part, file);
    } else {
      return false;
    }
  }
  return true;
}

bool cmCTest::CTestFileExists(const std::string& filename)
{
  std::string testingDir =
    this->BinaryDir + "/Testing/" + this->CurrentTag + "/" + filename;
  return cmSystemTools::FileExists(testingDir.c_str());
}

cmCTestGenericHandler* cmCTest::GetInitializedHandler(const char* handler)
{
  cmCTest::t_TestingHandlers::iterator it =
    this->TestingHandlers.find(handler);
  if (it == this->TestingHandlers.end()) {
    return CM_NULLPTR;
  }
  it->second->Initialize();
  return it->second;
}

cmCTestGenericHandler* cmCTest::GetHandler(const char* handler)
{
  cmCTest::t_TestingHandlers::iterator it =
    this->TestingHandlers.find(handler);
  if (it == this->TestingHandlers.end()) {
    return CM_NULLPTR;
  }
  return it->second;
}

int cmCTest::ExecuteHandler(const char* shandler)
{
  cmCTestGenericHandler* handler = this->GetHandler(shandler);
  if (!handler) {
    return -1;
  }
  handler->Initialize();
  return handler->ProcessHandler();
}

int cmCTest::ProcessSteps()
{
  int res = 0;
  bool notest = true;
  int update_count = 0;

  for (Part p = PartStart; notest && p != PartCount; p = Part(p + 1)) {
    notest = !this->Parts[p];
  }
  if (this->Parts[PartUpdate] && (this->GetRemainingTimeAllowed() - 120 > 0)) {
    cmCTestGenericHandler* uphandler = this->GetHandler("update");
    uphandler->SetPersistentOption(
      "SourceDirectory",
      this->GetCTestConfiguration("SourceDirectory").c_str());
    update_count = uphandler->ProcessHandler();
    if (update_count < 0) {
      res |= cmCTest::UPDATE_ERRORS;
    }
  }
  if (this->TestModel == cmCTest::CONTINUOUS && !update_count) {
    return 0;
  }
  if (this->Parts[PartConfigure] &&
      (this->GetRemainingTimeAllowed() - 120 > 0)) {
    if (this->GetHandler("configure")->ProcessHandler() < 0) {
      res |= cmCTest::CONFIGURE_ERRORS;
    }
  }
  if (this->Parts[PartBuild] && (this->GetRemainingTimeAllowed() - 120 > 0)) {
    this->UpdateCTestConfiguration();
    if (this->GetHandler("build")->ProcessHandler() < 0) {
      res |= cmCTest::BUILD_ERRORS;
    }
  }
  if ((this->Parts[PartTest] || notest) &&
      (this->GetRemainingTimeAllowed() - 120 > 0)) {
    this->UpdateCTestConfiguration();
    if (this->GetHandler("test")->ProcessHandler() < 0) {
      res |= cmCTest::TEST_ERRORS;
    }
  }
  if (this->Parts[PartCoverage] &&
      (this->GetRemainingTimeAllowed() - 120 > 0)) {
    this->UpdateCTestConfiguration();
    if (this->GetHandler("coverage")->ProcessHandler() < 0) {
      res |= cmCTest::COVERAGE_ERRORS;
    }
  }
  if (this->Parts[PartMemCheck] &&
      (this->GetRemainingTimeAllowed() - 120 > 0)) {
    this->UpdateCTestConfiguration();
    if (this->GetHandler("memcheck")->ProcessHandler() < 0) {
      res |= cmCTest::MEMORY_ERRORS;
    }
  }
  if (!notest) {
    std::string notes_dir = this->BinaryDir + "/Testing/Notes";
    if (cmSystemTools::FileIsDirectory(notes_dir)) {
      cmsys::Directory d;
      d.Load(notes_dir);
      unsigned long kk;
      for (kk = 0; kk < d.GetNumberOfFiles(); kk++) {
        const char* file = d.GetFile(kk);
        std::string fullname = notes_dir + "/" + file;
        if (cmSystemTools::FileExists(fullname.c_str()) &&
            !cmSystemTools::FileIsDirectory(fullname)) {
          if (!this->NotesFiles.empty()) {
            this->NotesFiles += ";";
          }
          this->NotesFiles += fullname;
          this->Parts[PartNotes].Enable();
        }
      }
    }
  }
  if (this->Parts[PartNotes]) {
    this->UpdateCTestConfiguration();
    if (!this->NotesFiles.empty()) {
      this->GenerateNotesFile(this->NotesFiles.c_str());
    }
  }
  if (this->Parts[PartSubmit]) {
    this->UpdateCTestConfiguration();
    if (this->GetHandler("submit")->ProcessHandler() < 0) {
      res |= cmCTest::SUBMIT_ERRORS;
    }
  }
  if (res != 0) {
    cmCTestLog(this, ERROR_MESSAGE, "Errors while running CTest" << std::endl);
  }
  return res;
}

std::string cmCTest::GetTestModelString()
{
  if (!this->SpecificTrack.empty()) {
    return this->SpecificTrack;
  }
  switch (this->TestModel) {
    case cmCTest::NIGHTLY:
      return "Nightly";
    case cmCTest::CONTINUOUS:
      return "Continuous";
  }
  return "Experimental";
}

int cmCTest::GetTestModelFromString(const char* str)
{
  if (!str) {
    return cmCTest::EXPERIMENTAL;
  }
  std::string rstr = cmSystemTools::LowerCase(str);
  if (cmHasLiteralPrefix(rstr.c_str(), "cont")) {
    return cmCTest::CONTINUOUS;
  }
  if (cmHasLiteralPrefix(rstr.c_str(), "nigh")) {
    return cmCTest::NIGHTLY;
  }
  return cmCTest::EXPERIMENTAL;
}

//######################################################################
//######################################################################
//######################################################################
//######################################################################

int cmCTest::RunMakeCommand(const char* command, std::string& output,
                            int* retVal, const char* dir, int timeout,
                            std::ostream& ofs, Encoding encoding)
{
  // First generate the command and arguments
  std::vector<std::string> args = cmSystemTools::ParseArguments(command);

  if (args.empty()) {
    return false;
  }

  std::vector<const char*> argv;
  for (std::vector<std::string>::const_iterator a = args.begin();
       a != args.end(); ++a) {
    argv.push_back(a->c_str());
  }
  argv.push_back(CM_NULLPTR);

  output = "";
  cmCTestLog(this, HANDLER_VERBOSE_OUTPUT, "Run command:");
  std::vector<const char*>::iterator ait;
  for (ait = argv.begin(); ait != argv.end() && *ait; ++ait) {
    cmCTestLog(this, HANDLER_VERBOSE_OUTPUT, " \"" << *ait << "\"");
  }
  cmCTestLog(this, HANDLER_VERBOSE_OUTPUT, std::endl);

  // Now create process object
  cmsysProcess* cp = cmsysProcess_New();
  cmsysProcess_SetCommand(cp, &*argv.begin());
  cmsysProcess_SetWorkingDirectory(cp, dir);
  cmsysProcess_SetOption(cp, cmsysProcess_Option_HideWindow, 1);
  cmsysProcess_SetTimeout(cp, timeout);
  cmsysProcess_Execute(cp);

  // Initialize tick's
  std::string::size_type tick = 0;
  std::string::size_type tick_len = 1024;
  std::string::size_type tick_line_len = 50;

  char* data;
  int length;
  cmProcessOutput processOutput(encoding);
  std::string strdata;
  cmCTestLog(this, HANDLER_PROGRESS_OUTPUT, "   Each . represents "
               << tick_len << " bytes of output" << std::endl
               << "    " << std::flush);
  while (cmsysProcess_WaitForData(cp, &data, &length, CM_NULLPTR)) {
    processOutput.DecodeText(data, length, strdata);
    for (size_t cc = 0; cc < strdata.size(); ++cc) {
      if (strdata[cc] == 0) {
        strdata[cc] = '\n';
      }
    }
    output.append(strdata);
    while (output.size() > (tick * tick_len)) {
      tick++;
      cmCTestLog(this, HANDLER_PROGRESS_OUTPUT, "." << std::flush);
      if (tick % tick_line_len == 0 && tick > 0) {
        cmCTestLog(this, HANDLER_PROGRESS_OUTPUT,
                   "  Size: " << int((double(output.size()) / 1024.0) + 1)
                              << "K" << std::endl
                              << "    " << std::flush);
      }
    }
    cmCTestLog(this, HANDLER_VERBOSE_OUTPUT,
               cmCTestLogWrite(strdata.c_str(), strdata.size()));
    if (ofs) {
      ofs << cmCTestLogWrite(strdata.c_str(), strdata.size());
    }
  }
  processOutput.DecodeText(std::string(), strdata);
  if (!strdata.empty()) {
    output.append(strdata);
    cmCTestLog(this, HANDLER_VERBOSE_OUTPUT,
               cmCTestLogWrite(strdata.c_str(), strdata.size()));
    if (ofs) {
      ofs << cmCTestLogWrite(strdata.c_str(), strdata.size());
    }
  }
  cmCTestLog(this, HANDLER_PROGRESS_OUTPUT, " Size of output: "
               << int(double(output.size()) / 1024.0) << "K" << std::endl);

  cmsysProcess_WaitForExit(cp, CM_NULLPTR);

  int result = cmsysProcess_GetState(cp);

  if (result == cmsysProcess_State_Exited) {
    *retVal = cmsysProcess_GetExitValue(cp);
    cmCTestLog(this, HANDLER_VERBOSE_OUTPUT,
               "Command exited with the value: " << *retVal << std::endl);
  } else if (result == cmsysProcess_State_Exception) {
    *retVal = cmsysProcess_GetExitException(cp);
    cmCTestLog(this, WARNING, "There was an exception: " << *retVal
                                                         << std::endl);
  } else if (result == cmsysProcess_State_Expired) {
    cmCTestLog(this, WARNING, "There was a timeout" << std::endl);
  } else if (result == cmsysProcess_State_Error) {
    output += "\n*** ERROR executing: ";
    output += cmsysProcess_GetErrorString(cp);
    output += "\n***The build process failed.";
    cmCTestLog(this, ERROR_MESSAGE, "There was an error: "
                 << cmsysProcess_GetErrorString(cp) << std::endl);
  }

  cmsysProcess_Delete(cp);

  return result;
}

//######################################################################
//######################################################################
//######################################################################
//######################################################################

int cmCTest::RunTest(std::vector<const char*> argv, std::string* output,
                     int* retVal, std::ostream* log, double testTimeOut,
                     std::vector<std::string>* environment, Encoding encoding)
{
  bool modifyEnv = (environment && !environment->empty());

  // determine how much time we have
  double timeout = this->GetRemainingTimeAllowed() - 120;
  if (this->TimeOut > 0 && this->TimeOut < timeout) {
    timeout = this->TimeOut;
  }
  if (testTimeOut > 0 && testTimeOut < this->GetRemainingTimeAllowed()) {
    timeout = testTimeOut;
  }

  // always have at least 1 second if we got to here
  if (timeout <= 0) {
    timeout = 1;
  }
  cmCTestLog(this, HANDLER_VERBOSE_OUTPUT,
             "Test timeout computed to be: " << timeout << "\n");
  if (cmSystemTools::SameFile(argv[0], cmSystemTools::GetCTestCommand()) &&
      !this->ForceNewCTestProcess) {
    cmCTest inst;
    inst.ConfigType = this->ConfigType;
    inst.TimeOut = timeout;

    // Capture output of the child ctest.
    std::ostringstream oss;
    inst.SetStreams(&oss, &oss);

    std::vector<std::string> args;
    for (unsigned int i = 0; i < argv.size(); ++i) {
      if (argv[i]) {
        // make sure we pass the timeout in for any build and test
        // invocations. Since --build-generator is required this is a
        // good place to check for it, and to add the arguments in
        if (strcmp(argv[i], "--build-generator") == 0 && timeout > 0) {
          args.push_back("--test-timeout");
          std::ostringstream msg;
          msg << timeout;
          args.push_back(msg.str());
        }
        args.push_back(argv[i]);
      }
    }
    if (log) {
      *log << "* Run internal CTest" << std::endl;
    }

    CM_AUTO_PTR<cmSystemTools::SaveRestoreEnvironment> saveEnv;
    if (modifyEnv) {
      saveEnv.reset(new cmSystemTools::SaveRestoreEnvironment);
      cmSystemTools::AppendEnv(*environment);
    }

    *retVal = inst.Run(args, output);
    if (output) {
      *output += oss.str();
    }
    if (log && output) {
      *log << *output;
    }
    if (output) {
      cmCTestLog(this, HANDLER_VERBOSE_OUTPUT,
                 "Internal cmCTest object used to run test." << std::endl
                                                             << *output
                                                             << std::endl);
    }

    return cmsysProcess_State_Exited;
  }
  std::vector<char> tempOutput;
  if (output) {
    *output = "";
  }

  CM_AUTO_PTR<cmSystemTools::SaveRestoreEnvironment> saveEnv;
  if (modifyEnv) {
    saveEnv.reset(new cmSystemTools::SaveRestoreEnvironment);
    cmSystemTools::AppendEnv(*environment);
  }

  cmsysProcess* cp = cmsysProcess_New();
  cmsysProcess_SetCommand(cp, &*argv.begin());
  cmCTestLog(this, DEBUG, "Command is: " << argv[0] << std::endl);
  if (cmSystemTools::GetRunCommandHideConsole()) {
    cmsysProcess_SetOption(cp, cmsysProcess_Option_HideWindow, 1);
  }

  cmsysProcess_SetTimeout(cp, timeout);
  cmsysProcess_Execute(cp);

  char* data;
  int length;
  cmProcessOutput processOutput(encoding);
  std::string strdata;
  while (cmsysProcess_WaitForData(cp, &data, &length, CM_NULLPTR)) {
    processOutput.DecodeText(data, length, strdata);
    if (output) {
      tempOutput.insert(tempOutput.end(), data, data + length);
    }
    cmCTestLog(this, HANDLER_VERBOSE_OUTPUT,
               cmCTestLogWrite(strdata.c_str(), strdata.size()));
    if (log) {
      log->write(strdata.c_str(), strdata.size());
    }
  }
  processOutput.DecodeText(std::string(), strdata);
  if (!strdata.empty()) {
    cmCTestLog(this, HANDLER_VERBOSE_OUTPUT,
               cmCTestLogWrite(strdata.c_str(), strdata.size()));
    if (log) {
      log->write(strdata.c_str(), strdata.size());
    }
  }

  cmsysProcess_WaitForExit(cp, CM_NULLPTR);
  processOutput.DecodeText(tempOutput, tempOutput);
  if (output && tempOutput.begin() != tempOutput.end()) {
    output->append(&*tempOutput.begin(), tempOutput.size());
  }
  cmCTestLog(this, HANDLER_VERBOSE_OUTPUT, "-- Process completed"
               << std::endl);

  int result = cmsysProcess_GetState(cp);

  if (result == cmsysProcess_State_Exited) {
    *retVal = cmsysProcess_GetExitValue(cp);
    if (*retVal != 0 && this->OutputTestOutputOnTestFailure) {
      OutputTestErrors(tempOutput);
    }
  } else if (result == cmsysProcess_State_Exception) {
    if (this->OutputTestOutputOnTestFailure) {
      OutputTestErrors(tempOutput);
    }
    *retVal = cmsysProcess_GetExitException(cp);
    std::string outerr = "\n*** Exception executing: ";
    outerr += cmsysProcess_GetExceptionString(cp);
    if (output) {
      *output += outerr;
    }
    cmCTestLog(this, HANDLER_VERBOSE_OUTPUT, outerr << std::endl
                                                    << std::flush);
  } else if (result == cmsysProcess_State_Error) {
    std::string outerr = "\n*** ERROR executing: ";
    outerr += cmsysProcess_GetErrorString(cp);
    if (output) {
      *output += outerr;
    }
    cmCTestLog(this, HANDLER_VERBOSE_OUTPUT, outerr << std::endl
                                                    << std::flush);
  }
  cmsysProcess_Delete(cp);

  return result;
}

std::string cmCTest::SafeBuildIdField(const std::string& value)
{
  std::string safevalue(value);

  if (safevalue != "") {
    // Disallow non-filename and non-space whitespace characters.
    // If they occur, replace them with ""
    //
    const char* disallowed = "\\:*?\"<>|\n\r\t\f\v";

    if (safevalue.find_first_of(disallowed) != std::string::npos) {
      std::string::size_type i = 0;
      std::string::size_type n = strlen(disallowed);
      char replace[2];
      replace[1] = 0;

      for (i = 0; i < n; ++i) {
        replace[0] = disallowed[i];
        cmSystemTools::ReplaceString(safevalue, replace, "");
      }
    }
  }

  if (safevalue == "") {
    safevalue = "(empty)";
  }

  return safevalue;
}

void cmCTest::StartXML(cmXMLWriter& xml, bool append)
{
  if (this->CurrentTag.empty()) {
    cmCTestLog(this, ERROR_MESSAGE, "Current Tag empty, this may mean"
                                    " NightlStartTime was not set correctly."
                 << std::endl);
    cmSystemTools::SetFatalErrorOccured();
  }

  // find out about the system
  cmsys::SystemInformation info;
  info.RunCPUCheck();
  info.RunOSCheck();
  info.RunMemoryCheck();

  std::string buildname =
    cmCTest::SafeBuildIdField(this->GetCTestConfiguration("BuildName"));
  std::string stamp = cmCTest::SafeBuildIdField(this->CurrentTag + "-" +
                                                this->GetTestModelString());
  std::string site =
    cmCTest::SafeBuildIdField(this->GetCTestConfiguration("Site"));

  xml.StartDocument();
  xml.StartElement("Site");
  xml.Attribute("BuildName", buildname);
  xml.BreakAttributes();
  xml.Attribute("BuildStamp", stamp);
  xml.Attribute("Name", site);
  xml.Attribute("Generator",
                std::string("ctest-") + cmVersion::GetCMakeVersion());
  if (append) {
    xml.Attribute("Append", "true");
  }
  xml.Attribute("CompilerName", this->GetCTestConfiguration("Compiler"));
  xml.Attribute("CompilerVersion",
                this->GetCTestConfiguration("CompilerVersion"));
  xml.Attribute("OSName", info.GetOSName());
  xml.Attribute("Hostname", info.GetHostname());
  xml.Attribute("OSRelease", info.GetOSRelease());
  xml.Attribute("OSVersion", info.GetOSVersion());
  xml.Attribute("OSPlatform", info.GetOSPlatform());
  xml.Attribute("Is64Bits", info.Is64Bits());
  xml.Attribute("VendorString", info.GetVendorString());
  xml.Attribute("VendorID", info.GetVendorID());
  xml.Attribute("FamilyID", info.GetFamilyID());
  xml.Attribute("ModelID", info.GetModelID());
  xml.Attribute("ProcessorCacheSize", info.GetProcessorCacheSize());
  xml.Attribute("NumberOfLogicalCPU", info.GetNumberOfLogicalCPU());
  xml.Attribute("NumberOfPhysicalCPU", info.GetNumberOfPhysicalCPU());
  xml.Attribute("TotalVirtualMemory", info.GetTotalVirtualMemory());
  xml.Attribute("TotalPhysicalMemory", info.GetTotalPhysicalMemory());
  xml.Attribute("LogicalProcessorsPerPhysical",
                info.GetLogicalProcessorsPerPhysical());
  xml.Attribute("ProcessorClockFrequency", info.GetProcessorClockFrequency());

  std::string changeId = this->GetCTestConfiguration("ChangeId");
  if (!changeId.empty()) {
    xml.Attribute("ChangeId", changeId);
  }

  this->AddSiteProperties(xml);
}

void cmCTest::AddSiteProperties(cmXMLWriter& xml)
{
  cmCTestScriptHandler* ch =
    static_cast<cmCTestScriptHandler*>(this->GetHandler("script"));
  cmake* cm = ch->GetCMake();
  // if no CMake then this is the old style script and props like
  // this will not work anyway.
  if (!cm) {
    return;
  }
  // This code should go when cdash is changed to use labels only
  const char* subproject = cm->GetState()->GetGlobalProperty("SubProject");
  if (subproject) {
    xml.StartElement("Subproject");
    xml.Attribute("name", subproject);
    const char* labels =
      ch->GetCMake()->GetState()->GetGlobalProperty("SubProjectLabels");
    if (labels) {
      xml.StartElement("Labels");
      std::string l = labels;
      std::vector<std::string> args;
      cmSystemTools::ExpandListArgument(l, args);
      for (std::vector<std::string>::iterator i = args.begin();
           i != args.end(); ++i) {
        xml.Element("Label", *i);
      }
      xml.EndElement();
    }
    xml.EndElement();
  }

  // This code should stay when cdash only does label based sub-projects
  const char* label = cm->GetState()->GetGlobalProperty("Label");
  if (label) {
    xml.StartElement("Labels");
    xml.Element("Label", label);
    xml.EndElement();
  }
}

void cmCTest::EndXML(cmXMLWriter& xml)
{
  xml.EndElement(); // Site
  xml.EndDocument();
}

int cmCTest::GenerateCTestNotesOutput(cmXMLWriter& xml,
                                      const cmCTest::VectorOfStrings& files)
{
  std::string buildname =
    cmCTest::SafeBuildIdField(this->GetCTestConfiguration("BuildName"));
  cmCTest::VectorOfStrings::const_iterator it;
  xml.StartDocument();
  xml.ProcessingInstruction("xml-stylesheet",
                            "type=\"text/xsl\" "
                            "href=\"Dart/Source/Server/XSL/Build.xsl "
                            "<file:///Dart/Source/Server/XSL/Build.xsl> \"");
  xml.StartElement("Site");
  xml.Attribute("BuildName", buildname);
  xml.Attribute("BuildStamp",
                this->CurrentTag + "-" + this->GetTestModelString());
  xml.Attribute("Name", this->GetCTestConfiguration("Site"));
  xml.Attribute("Generator",
                std::string("ctest") + cmVersion::GetCMakeVersion());
  this->AddSiteProperties(xml);
  xml.StartElement("Notes");

  for (it = files.begin(); it != files.end(); it++) {
    cmCTestLog(this, OUTPUT, "\tAdd file: " << *it << std::endl);
    std::string note_time = this->CurrentTime();
    xml.StartElement("Note");
    xml.Attribute("Name", *it);
    xml.Element("Time", cmSystemTools::GetTime());
    xml.Element("DateTime", note_time);
    xml.StartElement("Text");
    cmsys::ifstream ifs(it->c_str());
    if (ifs) {
      std::string line;
      while (cmSystemTools::GetLineFromStream(ifs, line)) {
        xml.Content(line);
        xml.Content("\n");
      }
      ifs.close();
    } else {
      xml.Content("Problem reading file: " + *it + "\n");
      cmCTestLog(this, ERROR_MESSAGE, "Problem reading file: "
                   << *it << " while creating notes" << std::endl);
    }
    xml.EndElement(); // Text
    xml.EndElement(); // Note
  }
  xml.EndElement(); // Notes
  xml.EndElement(); // Site
  xml.EndDocument();
  return 1;
}

int cmCTest::GenerateNotesFile(const VectorOfStrings& files)
{
  cmGeneratedFileStream ofs;
  if (!this->OpenOutputFile(this->CurrentTag, "Notes.xml", ofs)) {
    cmCTestLog(this, ERROR_MESSAGE, "Cannot open notes file" << std::endl);
    return 1;
  }
  cmXMLWriter xml(ofs);
  this->GenerateCTestNotesOutput(xml, files);
  return 0;
}

int cmCTest::GenerateNotesFile(const char* cfiles)
{
  if (!cfiles) {
    return 1;
  }

  VectorOfStrings files;

  cmCTestLog(this, OUTPUT, "Create notes file" << std::endl);

  files = cmSystemTools::SplitString(cfiles, ';');
  if (files.empty()) {
    return 1;
  }

  return this->GenerateNotesFile(files);
}

std::string cmCTest::Base64GzipEncodeFile(std::string const& file)
{
  std::string tarFile = file + "_temp.tar.gz";
  std::vector<std::string> files;
  files.push_back(file);

  if (!cmSystemTools::CreateTar(tarFile.c_str(), files,
                                cmSystemTools::TarCompressGZip, false)) {
    cmCTestLog(this, ERROR_MESSAGE, "Error creating tar while "
                                    "encoding file: "
                 << file << std::endl);
    return "";
  }
  std::string base64 = this->Base64EncodeFile(tarFile);
  cmSystemTools::RemoveFile(tarFile);
  return base64;
}

std::string cmCTest::Base64EncodeFile(std::string const& file)
{
  size_t const len = cmSystemTools::FileLength(file);
  cmsys::ifstream ifs(file.c_str(), std::ios::in
#ifdef _WIN32
                        | std::ios::binary
#endif
                      );
  std::vector<char> file_buffer(len + 1);
  ifs.read(&file_buffer[0], len);
  ifs.close();

  std::vector<char> encoded_buffer((len * 3) / 2 + 5);

  size_t const rlen = cmsysBase64_Encode(
    reinterpret_cast<unsigned char*>(&file_buffer[0]), len,
    reinterpret_cast<unsigned char*>(&encoded_buffer[0]), 1);

  return std::string(&encoded_buffer[0], rlen);
}

bool cmCTest::SubmitExtraFiles(const VectorOfStrings& files)
{
  VectorOfStrings::const_iterator it;
  for (it = files.begin(); it != files.end(); ++it) {
    if (!cmSystemTools::FileExists(it->c_str())) {
      cmCTestLog(this, ERROR_MESSAGE, "Cannot find extra file: "
                   << *it << " to submit." << std::endl;);
      return false;
    }
    this->AddSubmitFile(PartExtraFiles, it->c_str());
  }
  return true;
}

bool cmCTest::SubmitExtraFiles(const char* cfiles)
{
  if (!cfiles) {
    return true;
  }

  VectorOfStrings files;

  cmCTestLog(this, OUTPUT, "Submit extra files" << std::endl);

  files = cmSystemTools::SplitString(cfiles, ';');
  if (files.empty()) {
    return true;
  }

  return this->SubmitExtraFiles(files);
}

// for a -D argument convert the next argument into
// the proper list of dashboard steps via SetTest
bool cmCTest::AddTestsForDashboardType(std::string& targ)
{
  if (targ == "Experimental") {
    this->SetTestModel(cmCTest::EXPERIMENTAL);
    this->SetTest("Start");
    this->SetTest("Configure");
    this->SetTest("Build");
    this->SetTest("Test");
    this->SetTest("Coverage");
    this->SetTest("Submit");
  } else if (targ == "ExperimentalStart") {
    this->SetTestModel(cmCTest::EXPERIMENTAL);
    this->SetTest("Start");
  } else if (targ == "ExperimentalUpdate") {
    this->SetTestModel(cmCTest::EXPERIMENTAL);
    this->SetTest("Update");
  } else if (targ == "ExperimentalConfigure") {
    this->SetTestModel(cmCTest::EXPERIMENTAL);
    this->SetTest("Configure");
  } else if (targ == "ExperimentalBuild") {
    this->SetTestModel(cmCTest::EXPERIMENTAL);
    this->SetTest("Build");
  } else if (targ == "ExperimentalTest") {
    this->SetTestModel(cmCTest::EXPERIMENTAL);
    this->SetTest("Test");
  } else if (targ == "ExperimentalMemCheck" || targ == "ExperimentalPurify") {
    this->SetTestModel(cmCTest::EXPERIMENTAL);
    this->SetTest("MemCheck");
  } else if (targ == "ExperimentalCoverage") {
    this->SetTestModel(cmCTest::EXPERIMENTAL);
    this->SetTest("Coverage");
  } else if (targ == "ExperimentalSubmit") {
    this->SetTestModel(cmCTest::EXPERIMENTAL);
    this->SetTest("Submit");
  } else if (targ == "Continuous") {
    this->SetTestModel(cmCTest::CONTINUOUS);
    this->SetTest("Start");
    this->SetTest("Update");
    this->SetTest("Configure");
    this->SetTest("Build");
    this->SetTest("Test");
    this->SetTest("Coverage");
    this->SetTest("Submit");
  } else if (targ == "ContinuousStart") {
    this->SetTestModel(cmCTest::CONTINUOUS);
    this->SetTest("Start");
  } else if (targ == "ContinuousUpdate") {
    this->SetTestModel(cmCTest::CONTINUOUS);
    this->SetTest("Update");
  } else if (targ == "ContinuousConfigure") {
    this->SetTestModel(cmCTest::CONTINUOUS);
    this->SetTest("Configure");
  } else if (targ == "ContinuousBuild") {
    this->SetTestModel(cmCTest::CONTINUOUS);
    this->SetTest("Build");
  } else if (targ == "ContinuousTest") {
    this->SetTestModel(cmCTest::CONTINUOUS);
    this->SetTest("Test");
  } else if (targ == "ContinuousMemCheck" || targ == "ContinuousPurify") {
    this->SetTestModel(cmCTest::CONTINUOUS);
    this->SetTest("MemCheck");
  } else if (targ == "ContinuousCoverage") {
    this->SetTestModel(cmCTest::CONTINUOUS);
    this->SetTest("Coverage");
  } else if (targ == "ContinuousSubmit") {
    this->SetTestModel(cmCTest::CONTINUOUS);
    this->SetTest("Submit");
  } else if (targ == "Nightly") {
    this->SetTestModel(cmCTest::NIGHTLY);
    this->SetTest("Start");
    this->SetTest("Update");
    this->SetTest("Configure");
    this->SetTest("Build");
    this->SetTest("Test");
    this->SetTest("Coverage");
    this->SetTest("Submit");
  } else if (targ == "NightlyStart") {
    this->SetTestModel(cmCTest::NIGHTLY);
    this->SetTest("Start");
  } else if (targ == "NightlyUpdate") {
    this->SetTestModel(cmCTest::NIGHTLY);
    this->SetTest("Update");
  } else if (targ == "NightlyConfigure") {
    this->SetTestModel(cmCTest::NIGHTLY);
    this->SetTest("Configure");
  } else if (targ == "NightlyBuild") {
    this->SetTestModel(cmCTest::NIGHTLY);
    this->SetTest("Build");
  } else if (targ == "NightlyTest") {
    this->SetTestModel(cmCTest::NIGHTLY);
    this->SetTest("Test");
  } else if (targ == "NightlyMemCheck" || targ == "NightlyPurify") {
    this->SetTestModel(cmCTest::NIGHTLY);
    this->SetTest("MemCheck");
  } else if (targ == "NightlyCoverage") {
    this->SetTestModel(cmCTest::NIGHTLY);
    this->SetTest("Coverage");
  } else if (targ == "NightlySubmit") {
    this->SetTestModel(cmCTest::NIGHTLY);
    this->SetTest("Submit");
  } else if (targ == "MemoryCheck") {
    this->SetTestModel(cmCTest::EXPERIMENTAL);
    this->SetTest("Start");
    this->SetTest("Configure");
    this->SetTest("Build");
    this->SetTest("MemCheck");
    this->SetTest("Coverage");
    this->SetTest("Submit");
  } else if (targ == "NightlyMemoryCheck") {
    this->SetTestModel(cmCTest::NIGHTLY);
    this->SetTest("Start");
    this->SetTest("Update");
    this->SetTest("Configure");
    this->SetTest("Build");
    this->SetTest("MemCheck");
    this->SetTest("Coverage");
    this->SetTest("Submit");
  } else {
    return false;
  }
  return true;
}

void cmCTest::ErrorMessageUnknownDashDValue(std::string& val)
{
  cmCTestLog(this, ERROR_MESSAGE,
             "CTest -D called with incorrect option: " << val << std::endl);

  cmCTestLog(
    this, ERROR_MESSAGE, "Available options are:"
      << std::endl
      << "  ctest -D Continuous" << std::endl
      << "  ctest -D Continuous(Start|Update|Configure|Build)" << std::endl
      << "  ctest -D Continuous(Test|Coverage|MemCheck|Submit)" << std::endl
      << "  ctest -D Experimental" << std::endl
      << "  ctest -D Experimental(Start|Update|Configure|Build)" << std::endl
      << "  ctest -D Experimental(Test|Coverage|MemCheck|Submit)" << std::endl
      << "  ctest -D Nightly" << std::endl
      << "  ctest -D Nightly(Start|Update|Configure|Build)" << std::endl
      << "  ctest -D Nightly(Test|Coverage|MemCheck|Submit)" << std::endl
      << "  ctest -D NightlyMemoryCheck" << std::endl);
}

bool cmCTest::CheckArgument(const std::string& arg, const char* varg1,
                            const char* varg2)
{
  return (varg1 && arg == varg1) || (varg2 && arg == varg2);
}

// Processes one command line argument (and its arguments if any)
// for many simple options and then returns
bool cmCTest::HandleCommandLineArguments(size_t& i,
                                         std::vector<std::string>& args,
                                         std::string& errormsg)
{
  std::string arg = args[i];
  if (this->CheckArgument(arg, "-F")) {
    this->Failover = true;
  }
  if (this->CheckArgument(arg, "-j", "--parallel") && i < args.size() - 1) {
    i++;
    int plevel = atoi(args[i].c_str());
    this->SetParallelLevel(plevel);
    this->ParallelLevelSetInCli = true;
  } else if (arg.find("-j") == 0) {
    int plevel = atoi(arg.substr(2).c_str());
    this->SetParallelLevel(plevel);
    this->ParallelLevelSetInCli = true;
  }
  if (this->CheckArgument(arg, "--repeat-until-fail")) {
    if (i >= args.size() - 1) {
      errormsg = "'--repeat-until-fail' requires an argument";
      return false;
    }
    i++;
    long repeat = 1;
    if (!cmSystemTools::StringToLong(args[i].c_str(), &repeat)) {
      errormsg =
        "'--repeat-until-fail' given non-integer value '" + args[i] + "'";
      return false;
    }
    this->RepeatTests = static_cast<int>(repeat);
    if (repeat > 1) {
      this->RepeatUntilFail = true;
    }
  }

  if (this->CheckArgument(arg, "--test-load") && i < args.size() - 1) {
    i++;
    unsigned long load;
    if (cmSystemTools::StringToULong(args[i].c_str(), &load)) {
      this->SetTestLoad(load);
    } else {
      cmCTestLog(this, WARNING,
                 "Invalid value for 'Test Load' : " << args[i] << std::endl);
    }
  }

  if (this->CheckArgument(arg, "--no-compress-output")) {
    this->CompressTestOutput = false;
  }

  if (this->CheckArgument(arg, "--print-labels")) {
    this->PrintLabels = true;
  }

  if (this->CheckArgument(arg, "--http1.0")) {
    this->UseHTTP10 = true;
  }

  if (this->CheckArgument(arg, "--timeout") && i < args.size() - 1) {
    i++;
    double timeout = (double)atof(args[i].c_str());
    this->GlobalTimeout = timeout;
  }

  if (this->CheckArgument(arg, "--stop-time") && i < args.size() - 1) {
    i++;
    this->SetStopTime(args[i]);
  }

  if (this->CheckArgument(arg, "-C", "--build-config") &&
      i < args.size() - 1) {
    i++;
    this->SetConfigType(args[i].c_str());
  }

  if (this->CheckArgument(arg, "--debug")) {
    this->Debug = true;
    this->ShowLineNumbers = true;
  }
  if (this->CheckArgument(arg, "--track") && i < args.size() - 1) {
    i++;
    this->SpecificTrack = args[i];
  }
  if (this->CheckArgument(arg, "--show-line-numbers")) {
    this->ShowLineNumbers = true;
  }
  if (this->CheckArgument(arg, "--no-label-summary")) {
    this->LabelSummary = false;
  }
  if (this->CheckArgument(arg, "-Q", "--quiet")) {
    this->Quiet = true;
  }
  if (this->CheckArgument(arg, "-V", "--verbose")) {
    this->Verbose = true;
  }
  if (this->CheckArgument(arg, "-B")) {
    this->BatchJobs = true;
  }
  if (this->CheckArgument(arg, "-VV", "--extra-verbose")) {
    this->ExtraVerbose = true;
    this->Verbose = true;
  }
  if (this->CheckArgument(arg, "--output-on-failure")) {
    this->OutputTestOutputOnTestFailure = true;
  }
  if (this->CheckArgument(arg, "--test-output-size-passed") &&
      i < args.size() - 1) {
    i++;
    long outputSize;
    if (cmSystemTools::StringToLong(args[i].c_str(), &outputSize)) {
      if (cmCTestTestHandler* pCTestTestHandler =
            static_cast<cmCTestTestHandler*>(this->TestingHandlers["test"])) {
        pCTestTestHandler->SetTestOutputSizePassed(int(outputSize));
      }
    } else {
      cmCTestLog(this, WARNING,
                 "Invalid value for '--test-output-size-passed': " << args[i]
                                                                   << "\n");
    }
  }
  if (this->CheckArgument(arg, "--test-output-size-failed") &&
      i < args.size() - 1) {
    i++;
    long outputSize;
    if (cmSystemTools::StringToLong(args[i].c_str(), &outputSize)) {
      if (cmCTestTestHandler* pCTestTestHandler =
            static_cast<cmCTestTestHandler*>(this->TestingHandlers["test"])) {
        pCTestTestHandler->SetTestOutputSizeFailed(int(outputSize));
      }
    } else {
      cmCTestLog(this, WARNING,
                 "Invalid value for '--test-output-size-failed': " << args[i]
                                                                   << "\n");
    }
  }
  if (this->CheckArgument(arg, "-N", "--show-only")) {
    this->ShowOnly = true;
  }

  if (this->CheckArgument(arg, "-O", "--output-log") && i < args.size() - 1) {
    i++;
    this->SetOutputLogFileName(args[i].c_str());
  }

  if (this->CheckArgument(arg, "--tomorrow-tag")) {
    this->TomorrowTag = true;
  }
  if (this->CheckArgument(arg, "--force-new-ctest-process")) {
    this->ForceNewCTestProcess = true;
  }
  if (this->CheckArgument(arg, "-W", "--max-width") && i < args.size() - 1) {
    i++;
    this->MaxTestNameWidth = atoi(args[i].c_str());
  }
  if (this->CheckArgument(arg, "--interactive-debug-mode") &&
      i < args.size() - 1) {
    i++;
    this->InteractiveDebugMode = cmSystemTools::IsOn(args[i].c_str());
  }
  if (this->CheckArgument(arg, "--submit-index") && i < args.size() - 1) {
    i++;
    this->SubmitIndex = atoi(args[i].c_str());
    if (this->SubmitIndex < 0) {
      this->SubmitIndex = 0;
    }
  }

  if (this->CheckArgument(arg, "--overwrite") && i < args.size() - 1) {
    i++;
    this->AddCTestConfigurationOverwrite(args[i]);
  }
  if (this->CheckArgument(arg, "-A", "--add-notes") && i < args.size() - 1) {
    this->ProduceXML = true;
    this->SetTest("Notes");
    i++;
    this->SetNotesFiles(args[i].c_str());
  }

  // options that control what tests are run
  if (this->CheckArgument(arg, "-I", "--tests-information") &&
      i < args.size() - 1) {
    i++;
    this->GetHandler("test")->SetPersistentOption("TestsToRunInformation",
                                                  args[i].c_str());
    this->GetHandler("memcheck")
      ->SetPersistentOption("TestsToRunInformation", args[i].c_str());
  }
  if (this->CheckArgument(arg, "-U", "--union")) {
    this->GetHandler("test")->SetPersistentOption("UseUnion", "true");
    this->GetHandler("memcheck")->SetPersistentOption("UseUnion", "true");
  }
  if (this->CheckArgument(arg, "-R", "--tests-regex") && i < args.size() - 1) {
    i++;
    this->GetHandler("test")->SetPersistentOption("IncludeRegularExpression",
                                                  args[i].c_str());
    this->GetHandler("memcheck")
      ->SetPersistentOption("IncludeRegularExpression", args[i].c_str());
  }
  if (this->CheckArgument(arg, "-L", "--label-regex") && i < args.size() - 1) {
    i++;
    this->GetHandler("test")->SetPersistentOption("LabelRegularExpression",
                                                  args[i].c_str());
    this->GetHandler("memcheck")
      ->SetPersistentOption("LabelRegularExpression", args[i].c_str());
  }
  if (this->CheckArgument(arg, "-LE", "--label-exclude") &&
      i < args.size() - 1) {
    i++;
    this->GetHandler("test")->SetPersistentOption(
      "ExcludeLabelRegularExpression", args[i].c_str());
    this->GetHandler("memcheck")
      ->SetPersistentOption("ExcludeLabelRegularExpression", args[i].c_str());
  }

  if (this->CheckArgument(arg, "-E", "--exclude-regex") &&
      i < args.size() - 1) {
    i++;
    this->GetHandler("test")->SetPersistentOption("ExcludeRegularExpression",
                                                  args[i].c_str());
    this->GetHandler("memcheck")
      ->SetPersistentOption("ExcludeRegularExpression", args[i].c_str());
  }

  if (this->CheckArgument(arg, "-FA", "--fixture-exclude-any") &&
      i < args.size() - 1) {
    i++;
    this->GetHandler("test")->SetPersistentOption(
      "ExcludeFixtureRegularExpression", args[i].c_str());
    this->GetHandler("memcheck")
      ->SetPersistentOption("ExcludeFixtureRegularExpression",
                            args[i].c_str());
  }
  if (this->CheckArgument(arg, "-FS", "--fixture-exclude-setup") &&
      i < args.size() - 1) {
    i++;
    this->GetHandler("test")->SetPersistentOption(
      "ExcludeFixtureSetupRegularExpression", args[i].c_str());
    this->GetHandler("memcheck")
      ->SetPersistentOption("ExcludeFixtureSetupRegularExpression",
                            args[i].c_str());
  }
  if (this->CheckArgument(arg, "-FC", "--fixture-exclude-cleanup") &&
      i < args.size() - 1) {
    i++;
    this->GetHandler("test")->SetPersistentOption(
      "ExcludeFixtureCleanupRegularExpression", args[i].c_str());
    this->GetHandler("memcheck")
      ->SetPersistentOption("ExcludeFixtureCleanupRegularExpression",
                            args[i].c_str());
  }

  if (this->CheckArgument(arg, "--rerun-failed")) {
    this->GetHandler("test")->SetPersistentOption("RerunFailed", "true");
    this->GetHandler("memcheck")->SetPersistentOption("RerunFailed", "true");
  }
  return true;
}

// handle the -S -SR and -SP arguments
void cmCTest::HandleScriptArguments(size_t& i, std::vector<std::string>& args,
                                    bool& SRArgumentSpecified)
{
  std::string arg = args[i];
  if (this->CheckArgument(arg, "-SP", "--script-new-process") &&
      i < args.size() - 1) {
    this->RunConfigurationScript = true;
    i++;
    cmCTestScriptHandler* ch =
      static_cast<cmCTestScriptHandler*>(this->GetHandler("script"));
    // -SR is an internal argument, -SP should be ignored when it is passed
    if (!SRArgumentSpecified) {
      ch->AddConfigurationScript(args[i].c_str(), false);
    }
  }

  if (this->CheckArgument(arg, "-SR", "--script-run") && i < args.size() - 1) {
    SRArgumentSpecified = true;
    this->RunConfigurationScript = true;
    i++;
    cmCTestScriptHandler* ch =
      static_cast<cmCTestScriptHandler*>(this->GetHandler("script"));
    ch->AddConfigurationScript(args[i].c_str(), true);
  }

  if (this->CheckArgument(arg, "-S", "--script") && i < args.size() - 1) {
    this->RunConfigurationScript = true;
    i++;
    cmCTestScriptHandler* ch =
      static_cast<cmCTestScriptHandler*>(this->GetHandler("script"));
    // -SR is an internal argument, -S should be ignored when it is passed
    if (!SRArgumentSpecified) {
      ch->AddConfigurationScript(args[i].c_str(), true);
    }
  }
}

bool cmCTest::AddVariableDefinition(const std::string& arg)
{
  std::string name;
  std::string value;
  cmStateEnums::CacheEntryType type = cmStateEnums::UNINITIALIZED;

  if (cmake::ParseCacheEntry(arg, name, value, type)) {
    this->Definitions[name] = value;
    return true;
  }

  return false;
}

// the main entry point of ctest, called from main
int cmCTest::Run(std::vector<std::string>& args, std::string* output)
{
  const char* ctestExec = "ctest";
  bool cmakeAndTest = false;
  bool executeTests = true;
  bool SRArgumentSpecified = false;

  // copy the command line
  this->InitialCommandLineArguments.insert(
    this->InitialCommandLineArguments.end(), args.begin(), args.end());

  // process the command line arguments
  for (size_t i = 1; i < args.size(); ++i) {
    // handle the simple commandline arguments
    std::string errormsg;
    if (!this->HandleCommandLineArguments(i, args, errormsg)) {
      cmSystemTools::Error(errormsg.c_str());
      return 1;
    }

    // handle the script arguments -S -SR -SP
    this->HandleScriptArguments(i, args, SRArgumentSpecified);

    // --dashboard: handle a request for a dashboard
    std::string arg = args[i];
    if (this->CheckArgument(arg, "-D", "--dashboard") && i < args.size() - 1) {
      this->ProduceXML = true;
      i++;
      std::string targ = args[i];
      // AddTestsForDashboard parses the dashboard type and converts it
      // into the separate stages
      if (!this->AddTestsForDashboardType(targ)) {
        if (!this->AddVariableDefinition(targ)) {
          this->ErrorMessageUnknownDashDValue(targ);
          executeTests = false;
        }
      }
    }

    // If it's not exactly -D, but it starts with -D, then try to parse out
    // a variable definition from it, same as CMake does. Unsuccessful
    // attempts are simply ignored since previous ctest versions ignore
    // this too. (As well as many other unknown command line args.)
    //
    if (arg != "-D" && cmSystemTools::StringStartsWith(arg.c_str(), "-D")) {
      std::string input = arg.substr(2);
      this->AddVariableDefinition(input);
    }

    // --test-action: calls SetTest(<stage>, /*report=*/ false) to enable
    // the corresponding stage
    if (!this->HandleTestActionArgument(ctestExec, i, args)) {
      executeTests = false;
    }

    // --test-model: what type of test model
    if (!this->HandleTestModelArgument(ctestExec, i, args)) {
      executeTests = false;
    }

    // --extra-submit
    if (this->CheckArgument(arg, "--extra-submit") && i < args.size() - 1) {
      this->ProduceXML = true;
      this->SetTest("Submit");
      i++;
      if (!this->SubmitExtraFiles(args[i].c_str())) {
        return 0;
      }
    }

    // --build-and-test options
    if (this->CheckArgument(arg, "--build-and-test") && i < args.size() - 1) {
      cmakeAndTest = true;
    }

    // --schedule-random
    if (this->CheckArgument(arg, "--schedule-random")) {
      this->ScheduleType = "Random";
    }

    // pass the argument to all the handlers as well, but i may no longer be
    // set to what it was originally so I'm not sure this is working as
    // intended
    cmCTest::t_TestingHandlers::iterator it;
    for (it = this->TestingHandlers.begin(); it != this->TestingHandlers.end();
         ++it) {
      if (!it->second->ProcessCommandLineArguments(arg, i, args)) {
        cmCTestLog(this, ERROR_MESSAGE,
                   "Problem parsing command line arguments within a handler");
        return 0;
      }
    }
  } // the close of the for argument loop

  // handle CTEST_PARALLEL_LEVEL environment variable
  if (!this->ParallelLevelSetInCli) {
    std::string parallel;
    if (cmSystemTools::GetEnv("CTEST_PARALLEL_LEVEL", parallel)) {
      int plevel = atoi(parallel.c_str());
      this->SetParallelLevel(plevel);
    }
  }

  // now what should cmake do? if --build-and-test was specified then
  // we run the build and test handler and return
  if (cmakeAndTest) {
    return this->RunCMakeAndTest(output);
  }

  if (executeTests) {
    return this->ExecuteTests();
  }

  return 1;
}

bool cmCTest::HandleTestActionArgument(const char* ctestExec, size_t& i,
                                       const std::vector<std::string>& args)
{
  bool success = true;
  std::string arg = args[i];
  if (this->CheckArgument(arg, "-T", "--test-action") &&
      (i < args.size() - 1)) {
    this->ProduceXML = true;
    i++;
    if (!this->SetTest(args[i].c_str(), false)) {
      success = false;
      cmCTestLog(this, ERROR_MESSAGE, "CTest -T called with incorrect option: "
                   << args[i] << std::endl);
      cmCTestLog(this, ERROR_MESSAGE, "Available options are:"
                   << std::endl
                   << "  " << ctestExec << " -T all" << std::endl
                   << "  " << ctestExec << " -T start" << std::endl
                   << "  " << ctestExec << " -T update" << std::endl
                   << "  " << ctestExec << " -T configure" << std::endl
                   << "  " << ctestExec << " -T build" << std::endl
                   << "  " << ctestExec << " -T test" << std::endl
                   << "  " << ctestExec << " -T coverage" << std::endl
                   << "  " << ctestExec << " -T memcheck" << std::endl
                   << "  " << ctestExec << " -T notes" << std::endl
                   << "  " << ctestExec << " -T submit" << std::endl);
    }
  }
  return success;
}

bool cmCTest::HandleTestModelArgument(const char* ctestExec, size_t& i,
                                      const std::vector<std::string>& args)
{
  bool success = true;
  std::string arg = args[i];
  if (this->CheckArgument(arg, "-M", "--test-model") &&
      (i < args.size() - 1)) {
    i++;
    std::string const& str = args[i];
    if (cmSystemTools::LowerCase(str) == "nightly") {
      this->SetTestModel(cmCTest::NIGHTLY);
    } else if (cmSystemTools::LowerCase(str) == "continuous") {
      this->SetTestModel(cmCTest::CONTINUOUS);
    } else if (cmSystemTools::LowerCase(str) == "experimental") {
      this->SetTestModel(cmCTest::EXPERIMENTAL);
    } else {
      success = false;
      cmCTestLog(this, ERROR_MESSAGE, "CTest -M called with incorrect option: "
                   << str << std::endl);
      cmCTestLog(this, ERROR_MESSAGE, "Available options are:"
                   << std::endl
                   << "  " << ctestExec << " -M Continuous" << std::endl
                   << "  " << ctestExec << " -M Experimental" << std::endl
                   << "  " << ctestExec << " -M Nightly" << std::endl);
    }
  }
  return success;
}

int cmCTest::ExecuteTests()
{
  int res;
  // call process directory
  if (this->RunConfigurationScript) {
    if (this->ExtraVerbose) {
      cmCTestLog(this, OUTPUT, "* Extra verbosity turned on" << std::endl);
    }
    cmCTest::t_TestingHandlers::iterator it;
    for (it = this->TestingHandlers.begin(); it != this->TestingHandlers.end();
         ++it) {
      it->second->SetVerbose(this->ExtraVerbose);
      it->second->SetSubmitIndex(this->SubmitIndex);
    }
    this->GetHandler("script")->SetVerbose(this->Verbose);
    res = this->GetHandler("script")->ProcessHandler();
    if (res != 0) {
      cmCTestLog(this, DEBUG,
                 "running script failing returning: " << res << std::endl);
    }

  } else {
    // What is this?  -V seems to be the same as -VV,
    // and Verbose is always on in this case
    this->ExtraVerbose = this->Verbose;
    this->Verbose = true;
    cmCTest::t_TestingHandlers::iterator it;
    for (it = this->TestingHandlers.begin(); it != this->TestingHandlers.end();
         ++it) {
      it->second->SetVerbose(this->Verbose);
      it->second->SetSubmitIndex(this->SubmitIndex);
    }
    std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();
    if (!this->Initialize(cwd.c_str(), CM_NULLPTR)) {
      res = 12;
      cmCTestLog(this, ERROR_MESSAGE, "Problem initializing the dashboard."
                   << std::endl);
    } else {
      res = this->ProcessSteps();
    }
    this->Finalize();
  }
  if (res != 0) {
    cmCTestLog(this, DEBUG,
               "Running a test(s) failed returning : " << res << std::endl);
  }
  return res;
}

int cmCTest::RunCMakeAndTest(std::string* output)
{
  this->Verbose = true;
  cmCTestBuildAndTestHandler* handler =
    static_cast<cmCTestBuildAndTestHandler*>(this->GetHandler("buildtest"));
  int retv = handler->ProcessHandler();
  *output = handler->GetOutput();
#ifdef CMAKE_BUILD_WITH_CMAKE
  cmDynamicLoader::FlushCache();
#endif
  if (retv != 0) {
    cmCTestLog(this, DEBUG, "build and test failing returning: " << retv
                                                                 << std::endl);
  }
  return retv;
}

void cmCTest::SetNotesFiles(const char* notes)
{
  if (!notes) {
    return;
  }
  this->NotesFiles = notes;
}

void cmCTest::SetStopTime(std::string const& time)
{
  this->StopTime = time;
  this->DetermineNextDayStop();
}

int cmCTest::ReadCustomConfigurationFileTree(const char* dir, cmMakefile* mf)
{
  bool found = false;
  VectorOfStrings dirs;
  VectorOfStrings ndirs;
  cmCTestLog(this, DEBUG, "* Read custom CTest configuration directory: "
               << dir << std::endl);

  std::string fname = dir;
  fname += "/CTestCustom.cmake";
  cmCTestLog(this, DEBUG, "* Check for file: " << fname << std::endl);
  if (cmSystemTools::FileExists(fname.c_str())) {
    cmCTestLog(this, DEBUG, "* Read custom CTest configuration file: "
                 << fname << std::endl);
    bool erroroc = cmSystemTools::GetErrorOccuredFlag();
    cmSystemTools::ResetErrorOccuredFlag();

    if (!mf->ReadListFile(fname.c_str()) ||
        cmSystemTools::GetErrorOccuredFlag()) {
      cmCTestLog(this, ERROR_MESSAGE, "Problem reading custom configuration: "
                   << fname << std::endl);
    }
    found = true;
    if (erroroc) {
      cmSystemTools::SetErrorOccured();
    }
  }

  std::string rexpr = dir;
  rexpr += "/CTestCustom.ctest";
  cmCTestLog(this, DEBUG, "* Check for file: " << rexpr << std::endl);
  if (!found && cmSystemTools::FileExists(rexpr.c_str())) {
    cmsys::Glob gl;
    gl.RecurseOn();
    gl.FindFiles(rexpr);
    std::vector<std::string>& files = gl.GetFiles();
    std::vector<std::string>::iterator fileIt;
    for (fileIt = files.begin(); fileIt != files.end(); ++fileIt) {
      cmCTestLog(this, DEBUG, "* Read custom CTest configuration file: "
                   << *fileIt << std::endl);
      if (!mf->ReadListFile(fileIt->c_str()) ||
          cmSystemTools::GetErrorOccuredFlag()) {
        cmCTestLog(this, ERROR_MESSAGE,
                   "Problem reading custom configuration: " << *fileIt
                                                            << std::endl);
      }
    }
    found = true;
  }

  if (found) {
    cmCTest::t_TestingHandlers::iterator it;
    for (it = this->TestingHandlers.begin(); it != this->TestingHandlers.end();
         ++it) {
      cmCTestLog(this, DEBUG,
                 "* Read custom CTest configuration vectors for handler: "
                   << it->first << " (" << it->second << ")" << std::endl);
      it->second->PopulateCustomVectors(mf);
    }
  }

  return 1;
}

void cmCTest::PopulateCustomVector(cmMakefile* mf, const std::string& def,
                                   std::vector<std::string>& vec)
{
  const char* dval = mf->GetDefinition(def);
  if (!dval) {
    return;
  }
  cmCTestLog(this, DEBUG, "PopulateCustomVector: " << def << std::endl);

  vec.clear();
  cmSystemTools::ExpandListArgument(dval, vec);

  for (std::vector<std::string>::const_iterator it = vec.begin();
       it != vec.end(); ++it) {
    cmCTestLog(this, DEBUG, "  -- " << *it << std::endl);
  }
}

void cmCTest::PopulateCustomInteger(cmMakefile* mf, const std::string& def,
                                    int& val)
{
  const char* dval = mf->GetDefinition(def);
  if (!dval) {
    return;
  }
  val = atoi(dval);
}

std::string cmCTest::GetShortPathToFile(const char* cfname)
{
  const std::string& sourceDir = cmSystemTools::CollapseFullPath(
    this->GetCTestConfiguration("SourceDirectory"));
  const std::string& buildDir = cmSystemTools::CollapseFullPath(
    this->GetCTestConfiguration("BuildDirectory"));
  std::string fname = cmSystemTools::CollapseFullPath(cfname);

  // Find relative paths to both directories
  std::string srcRelpath =
    cmSystemTools::RelativePath(sourceDir.c_str(), fname.c_str());
  std::string bldRelpath =
    cmSystemTools::RelativePath(buildDir.c_str(), fname.c_str());

  // If any contains "." it is not parent directory
  bool inSrc = srcRelpath.find("..") == std::string::npos;
  bool inBld = bldRelpath.find("..") == std::string::npos;
  // TODO: Handle files with .. in their name

  std::string* res = CM_NULLPTR;

  if (inSrc && inBld) {
    // If both have relative path with no dots, pick the shorter one
    if (srcRelpath.size() < bldRelpath.size()) {
      res = &srcRelpath;
    } else {
      res = &bldRelpath;
    }
  } else if (inSrc) {
    res = &srcRelpath;
  } else if (inBld) {
    res = &bldRelpath;
  }

  std::string path;

  if (!res) {
    path = fname;
  } else {
    cmSystemTools::ConvertToUnixSlashes(*res);

    path = "./" + *res;
    if (path[path.size() - 1] == '/') {
      path = path.substr(0, path.size() - 1);
    }
  }

  cmsys::SystemTools::ReplaceString(path, ":", "_");
  cmsys::SystemTools::ReplaceString(path, " ", "_");
  return path;
}

std::string cmCTest::GetCTestConfiguration(const std::string& name)
{
  if (this->CTestConfigurationOverwrites.find(name) !=
      this->CTestConfigurationOverwrites.end()) {
    return this->CTestConfigurationOverwrites[name];
  }
  return this->CTestConfiguration[name];
}

void cmCTest::EmptyCTestConfiguration()
{
  this->CTestConfiguration.clear();
}

void cmCTest::DetermineNextDayStop()
{
  struct tm* lctime;
  time_t current_time = time(CM_NULLPTR);
  lctime = gmtime(&current_time);
  int gm_hour = lctime->tm_hour;
  time_t gm_time = mktime(lctime);
  lctime = localtime(&current_time);
  int local_hour = lctime->tm_hour;

  int tzone_offset = local_hour - gm_hour;
  if (gm_time > current_time && gm_hour < local_hour) {
    // this means gm_time is on the next day
    tzone_offset -= 24;
  } else if (gm_time < current_time && gm_hour > local_hour) {
    // this means gm_time is on the previous day
    tzone_offset += 24;
  }

  tzone_offset *= 100;
  char buf[1024];
  sprintf(buf, "%d%02d%02d %s %+05i", lctime->tm_year + 1900,
          lctime->tm_mon + 1, lctime->tm_mday, this->StopTime.c_str(),
          tzone_offset);

  time_t stop_time = curl_getdate(buf, &current_time);

  if (stop_time < current_time) {
    this->NextDayStopTime = true;
  }
}

void cmCTest::SetCTestConfiguration(const char* name, const char* value,
                                    bool suppress)
{
  cmCTestOptionalLog(this, HANDLER_VERBOSE_OUTPUT, "SetCTestConfiguration:"
                       << name << ":" << (value ? value : "(null)") << "\n",
                     suppress);

  if (!name) {
    return;
  }
  if (!value) {
    this->CTestConfiguration.erase(name);
    return;
  }
  this->CTestConfiguration[name] = value;
}

std::string cmCTest::GetCurrentTag()
{
  return this->CurrentTag;
}

std::string cmCTest::GetBinaryDir()
{
  return this->BinaryDir;
}

std::string const& cmCTest::GetConfigType()
{
  return this->ConfigType;
}

bool cmCTest::GetShowOnly()
{
  return this->ShowOnly;
}

int cmCTest::GetMaxTestNameWidth() const
{
  return this->MaxTestNameWidth;
}

void cmCTest::SetProduceXML(bool v)
{
  this->ProduceXML = v;
}

bool cmCTest::GetProduceXML()
{
  return this->ProduceXML;
}

const char* cmCTest::GetSpecificTrack()
{
  if (this->SpecificTrack.empty()) {
    return CM_NULLPTR;
  }
  return this->SpecificTrack.c_str();
}

void cmCTest::SetSpecificTrack(const char* track)
{
  if (!track) {
    this->SpecificTrack = "";
    return;
  }
  this->SpecificTrack = track;
}

void cmCTest::AddSubmitFile(Part part, const char* name)
{
  this->Parts[part].SubmitFiles.push_back(name);
}

void cmCTest::AddCTestConfigurationOverwrite(const std::string& overStr)
{
  size_t epos = overStr.find('=');
  if (epos == std::string::npos) {
    cmCTestLog(this, ERROR_MESSAGE,
               "CTest configuration overwrite specified in the wrong format."
                 << std::endl
                 << "Valid format is: --overwrite key=value" << std::endl
                 << "The specified was: --overwrite " << overStr << std::endl);
    return;
  }
  std::string key = overStr.substr(0, epos);
  std::string value = overStr.substr(epos + 1);
  this->CTestConfigurationOverwrites[key] = value;
}

void cmCTest::SetConfigType(const char* ct)
{
  this->ConfigType = ct ? ct : "";
  cmSystemTools::ReplaceString(this->ConfigType, ".\\", "");
  std::string confTypeEnv = "CMAKE_CONFIG_TYPE=" + this->ConfigType;
  cmSystemTools::PutEnv(confTypeEnv);
}

bool cmCTest::SetCTestConfigurationFromCMakeVariable(
  cmMakefile* mf, const char* dconfig, const std::string& cmake_var,
  bool suppress)
{
  const char* ctvar;
  ctvar = mf->GetDefinition(cmake_var);
  if (!ctvar) {
    return false;
  }
  cmCTestOptionalLog(this, HANDLER_VERBOSE_OUTPUT,
                     "SetCTestConfigurationFromCMakeVariable:"
                       << dconfig << ":" << cmake_var << std::endl,
                     suppress);
  this->SetCTestConfiguration(dconfig, ctvar, suppress);
  return true;
}

bool cmCTest::RunCommand(const char* command, std::string* stdOut,
                         std::string* stdErr, int* retVal, const char* dir,
                         double timeout, Encoding encoding)
{
  std::vector<std::string> args = cmSystemTools::ParseArguments(command);

  if (args.empty()) {
    return false;
  }

  std::vector<const char*> argv;
  for (std::vector<std::string>::const_iterator a = args.begin();
       a != args.end(); ++a) {
    argv.push_back(a->c_str());
  }
  argv.push_back(CM_NULLPTR);

  *stdOut = "";
  *stdErr = "";

  cmsysProcess* cp = cmsysProcess_New();
  cmsysProcess_SetCommand(cp, &*argv.begin());
  cmsysProcess_SetWorkingDirectory(cp, dir);
  if (cmSystemTools::GetRunCommandHideConsole()) {
    cmsysProcess_SetOption(cp, cmsysProcess_Option_HideWindow, 1);
  }
  cmsysProcess_SetTimeout(cp, timeout);
  cmsysProcess_Execute(cp);

  std::vector<char> tempOutput;
  std::vector<char> tempError;
  char* data;
  int length;
  cmProcessOutput processOutput(encoding);
  std::string strdata;
  int res;
  bool done = false;
  while (!done) {
    res = cmsysProcess_WaitForData(cp, &data, &length, CM_NULLPTR);
    switch (res) {
      case cmsysProcess_Pipe_STDOUT:
        tempOutput.insert(tempOutput.end(), data, data + length);
        break;
      case cmsysProcess_Pipe_STDERR:
        tempError.insert(tempError.end(), data, data + length);
        break;
      default:
        done = true;
    }
    if ((res == cmsysProcess_Pipe_STDOUT || res == cmsysProcess_Pipe_STDERR) &&
        this->ExtraVerbose) {
      processOutput.DecodeText(data, length, strdata);
      cmSystemTools::Stdout(strdata.c_str(), strdata.size());
    }
  }
  if (this->ExtraVerbose) {
    processOutput.DecodeText(std::string(), strdata);
    if (!strdata.empty()) {
      cmSystemTools::Stdout(strdata.c_str(), strdata.size());
    }
  }

  cmsysProcess_WaitForExit(cp, CM_NULLPTR);
  if (!tempOutput.empty()) {
    processOutput.DecodeText(tempOutput, tempOutput);
    stdOut->append(&*tempOutput.begin(), tempOutput.size());
  }
  if (!tempError.empty()) {
    processOutput.DecodeText(tempError, tempError);
    stdErr->append(&*tempError.begin(), tempError.size());
  }

  bool result = true;
  if (cmsysProcess_GetState(cp) == cmsysProcess_State_Exited) {
    if (retVal) {
      *retVal = cmsysProcess_GetExitValue(cp);
    } else {
      if (cmsysProcess_GetExitValue(cp) != 0) {
        result = false;
      }
    }
  } else if (cmsysProcess_GetState(cp) == cmsysProcess_State_Exception) {
    const char* exception_str = cmsysProcess_GetExceptionString(cp);
    cmCTestLog(this, ERROR_MESSAGE, exception_str << std::endl);
    stdErr->append(exception_str, strlen(exception_str));
    result = false;
  } else if (cmsysProcess_GetState(cp) == cmsysProcess_State_Error) {
    const char* error_str = cmsysProcess_GetErrorString(cp);
    cmCTestLog(this, ERROR_MESSAGE, error_str << std::endl);
    stdErr->append(error_str, strlen(error_str));
    result = false;
  } else if (cmsysProcess_GetState(cp) == cmsysProcess_State_Expired) {
    const char* error_str = "Process terminated due to timeout\n";
    cmCTestLog(this, ERROR_MESSAGE, error_str << std::endl);
    stdErr->append(error_str, strlen(error_str));
    result = false;
  }

  cmsysProcess_Delete(cp);
  return result;
}

void cmCTest::SetOutputLogFileName(const char* name)
{
  if (this->OutputLogFile) {
    delete this->OutputLogFile;
    this->OutputLogFile = CM_NULLPTR;
  }
  if (name) {
    this->OutputLogFile = new cmGeneratedFileStream(name);
  }
}

static const char* cmCTestStringLogType[] = { "DEBUG",
                                              "OUTPUT",
                                              "HANDLER_OUTPUT",
                                              "HANDLER_PROGRESS_OUTPUT",
                                              "HANDLER_VERBOSE_OUTPUT",
                                              "WARNING",
                                              "ERROR_MESSAGE",
                                              CM_NULLPTR };

#define cmCTestLogOutputFileLine(stream)                                      \
  if (this->ShowLineNumbers) {                                                \
    (stream) << std::endl << file << ":" << line << " ";                      \
  }

void cmCTest::InitStreams()
{
  // By default we write output to the process output streams.
  this->StreamOut = &std::cout;
  this->StreamErr = &std::cerr;
}

void cmCTest::Log(int logType, const char* file, int line, const char* msg,
                  bool suppress)
{
  if (!msg || !*msg) {
    return;
  }
  if (suppress && logType != cmCTest::ERROR_MESSAGE) {
    return;
  }
  if (logType == cmCTest::HANDLER_PROGRESS_OUTPUT &&
      (this->Debug || this->ExtraVerbose)) {
    return;
  }
  if (this->OutputLogFile) {
    bool display = true;
    if (logType == cmCTest::DEBUG && !this->Debug) {
      display = false;
    }
    if (logType == cmCTest::HANDLER_VERBOSE_OUTPUT && !this->Debug &&
        !this->ExtraVerbose) {
      display = false;
    }
    if (display) {
      cmCTestLogOutputFileLine(*this->OutputLogFile);
      if (logType != this->OutputLogFileLastTag) {
        *this->OutputLogFile << "[";
        if (logType >= OTHER || logType < 0) {
          *this->OutputLogFile << "OTHER";
        } else {
          *this->OutputLogFile << cmCTestStringLogType[logType];
        }
        *this->OutputLogFile << "] " << std::endl << std::flush;
      }
      *this->OutputLogFile << msg << std::flush;
      if (logType != this->OutputLogFileLastTag) {
        *this->OutputLogFile << std::endl << std::flush;
        this->OutputLogFileLastTag = logType;
      }
    }
  }
  if (!this->Quiet) {
    std::ostream& out = *this->StreamOut;
    std::ostream& err = *this->StreamErr;
    switch (logType) {
      case DEBUG:
        if (this->Debug) {
          cmCTestLogOutputFileLine(out);
          out << msg;
          out.flush();
        }
        break;
      case OUTPUT:
      case HANDLER_OUTPUT:
        if (this->Debug || this->Verbose) {
          cmCTestLogOutputFileLine(out);
          out << msg;
          out.flush();
        }
        break;
      case HANDLER_VERBOSE_OUTPUT:
        if (this->Debug || this->ExtraVerbose) {
          cmCTestLogOutputFileLine(out);
          out << msg;
          out.flush();
        }
        break;
      case WARNING:
        cmCTestLogOutputFileLine(err);
        err << msg;
        err.flush();
        break;
      case ERROR_MESSAGE:
        cmCTestLogOutputFileLine(err);
        err << msg;
        err.flush();
        cmSystemTools::SetErrorOccured();
        break;
      default:
        cmCTestLogOutputFileLine(out);
        out << msg;
        out.flush();
    }
  }
}

double cmCTest::GetRemainingTimeAllowed()
{
  if (!this->GetHandler("script")) {
    return 1.0e7;
  }

  cmCTestScriptHandler* ch =
    static_cast<cmCTestScriptHandler*>(this->GetHandler("script"));

  return ch->GetRemainingTimeAllowed();
}

void cmCTest::OutputTestErrors(std::vector<char> const& process_output)
{
  std::string test_outputs("\n*** Test Failed:\n");
  if (!process_output.empty()) {
    test_outputs.append(&*process_output.begin(), process_output.size());
  }
  cmCTestLog(this, HANDLER_OUTPUT, test_outputs << std::endl << std::flush);
}

bool cmCTest::CompressString(std::string& str)
{
  int ret;
  z_stream strm;

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, -1); // default compression level
  if (ret != Z_OK) {
    return false;
  }

  unsigned char* in =
    reinterpret_cast<unsigned char*>(const_cast<char*>(str.c_str()));
  // zlib makes the guarantee that this is the maximum output size
  int outSize =
    static_cast<int>(static_cast<double>(str.size()) * 1.001 + 13.0);
  std::vector<unsigned char> out(outSize);

  strm.avail_in = static_cast<uInt>(str.size());
  strm.next_in = in;
  strm.avail_out = outSize;
  strm.next_out = &out[0];
  ret = deflate(&strm, Z_FINISH);

  if (ret != Z_STREAM_END) {
    cmCTestLog(this, ERROR_MESSAGE, "Error during gzip compression."
                 << std::endl);
    return false;
  }

  (void)deflateEnd(&strm);

  // Now base64 encode the resulting binary string
  std::vector<unsigned char> base64EncodedBuffer((outSize * 3) / 2);

  size_t rlen =
    cmsysBase64_Encode(&out[0], strm.total_out, &base64EncodedBuffer[0], 1);

  str.assign(reinterpret_cast<char*>(&base64EncodedBuffer[0]), rlen);

  return true;
}
