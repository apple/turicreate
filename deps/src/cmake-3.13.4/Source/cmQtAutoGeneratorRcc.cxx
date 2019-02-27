/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmQtAutoGeneratorRcc.h"
#include "cmQtAutoGen.h"

#include "cmAlgorithms.h"
#include "cmCryptoHash.h"
#include "cmFileLockResult.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cmUVHandlePtr.h"

#include <functional>

// -- Class methods

cmQtAutoGeneratorRcc::cmQtAutoGeneratorRcc()
  : MultiConfig_(false)
  , SettingsChanged_(false)
  , Stage_(StageT::SETTINGS_READ)
  , Error_(false)
  , Generate_(false)
  , BuildFileChanged_(false)
{
  // Initialize libuv asynchronous iteration request
  UVRequest().init(*UVLoop(), &cmQtAutoGeneratorRcc::UVPollStage, this);
}

cmQtAutoGeneratorRcc::~cmQtAutoGeneratorRcc()
{
}

bool cmQtAutoGeneratorRcc::Init(cmMakefile* makefile)
{
  // -- Utility lambdas
  auto InfoGet = [makefile](std::string const& key) {
    return makefile->GetSafeDefinition(key);
  };
  auto InfoGetList =
    [makefile](std::string const& key) -> std::vector<std::string> {
    std::vector<std::string> list;
    cmSystemTools::ExpandListArgument(makefile->GetSafeDefinition(key), list);
    return list;
  };
  auto InfoGetConfig = [makefile,
                        this](std::string const& key) -> std::string {
    const char* valueConf = nullptr;
    {
      std::string keyConf = key;
      keyConf += '_';
      keyConf += InfoConfig();
      valueConf = makefile->GetDefinition(keyConf);
    }
    if (valueConf == nullptr) {
      return makefile->GetSafeDefinition(key);
    }
    return std::string(valueConf);
  };
  auto InfoGetConfigList =
    [&InfoGetConfig](std::string const& key) -> std::vector<std::string> {
    std::vector<std::string> list;
    cmSystemTools::ExpandListArgument(InfoGetConfig(key), list);
    return list;
  };

  // -- Read info file
  if (!makefile->ReadListFile(InfoFile().c_str())) {
    Log().ErrorFile(GeneratorT::RCC, InfoFile(), "File processing failed");
    return false;
  }

  // - Configurations
  Log().RaiseVerbosity(InfoGet("ARCC_VERBOSITY"));
  MultiConfig_ = makefile->IsOn("ARCC_MULTI_CONFIG");

  // - Directories
  AutogenBuildDir_ = InfoGet("ARCC_BUILD_DIR");
  if (AutogenBuildDir_.empty()) {
    Log().ErrorFile(GeneratorT::RCC, InfoFile(), "Build directory empty");
    return false;
  }

  IncludeDir_ = InfoGetConfig("ARCC_INCLUDE_DIR");
  if (IncludeDir_.empty()) {
    Log().ErrorFile(GeneratorT::RCC, InfoFile(), "Include directory empty");
    return false;
  }

  // - Rcc executable
  RccExecutable_ = InfoGet("ARCC_RCC_EXECUTABLE");
  RccListOptions_ = InfoGetList("ARCC_RCC_LIST_OPTIONS");

  // - Job
  LockFile_ = InfoGet("ARCC_LOCK_FILE");
  QrcFile_ = InfoGet("ARCC_SOURCE");
  QrcFileName_ = cmSystemTools::GetFilenameName(QrcFile_);
  QrcFileDir_ = cmSystemTools::GetFilenamePath(QrcFile_);
  RccPathChecksum_ = InfoGet("ARCC_OUTPUT_CHECKSUM");
  RccFileName_ = InfoGet("ARCC_OUTPUT_NAME");
  Options_ = InfoGetConfigList("ARCC_OPTIONS");
  Inputs_ = InfoGetList("ARCC_INPUTS");

  // - Settings file
  SettingsFile_ = InfoGetConfig("ARCC_SETTINGS_FILE");

  // - Validity checks
  if (LockFile_.empty()) {
    Log().ErrorFile(GeneratorT::RCC, InfoFile(), "Lock file name missing");
    return false;
  }
  if (SettingsFile_.empty()) {
    Log().ErrorFile(GeneratorT::RCC, InfoFile(), "Settings file name missing");
    return false;
  }
  if (AutogenBuildDir_.empty()) {
    Log().ErrorFile(GeneratorT::RCC, InfoFile(),
                    "Autogen build directory missing");
    return false;
  }
  if (RccExecutable_.empty()) {
    Log().ErrorFile(GeneratorT::RCC, InfoFile(), "rcc executable missing");
    return false;
  }
  if (QrcFile_.empty()) {
    Log().ErrorFile(GeneratorT::RCC, InfoFile(), "rcc input file missing");
    return false;
  }
  if (RccFileName_.empty()) {
    Log().ErrorFile(GeneratorT::RCC, InfoFile(), "rcc output file missing");
    return false;
  }

  // Init derived information
  // ------------------------

  RccFilePublic_ = AutogenBuildDir_;
  RccFilePublic_ += '/';
  RccFilePublic_ += RccPathChecksum_;
  RccFilePublic_ += '/';
  RccFilePublic_ += RccFileName_;

  // Compute rcc output file name
  if (IsMultiConfig()) {
    RccFileOutput_ = IncludeDir_;
    RccFileOutput_ += '/';
    RccFileOutput_ += MultiConfigOutput();
  } else {
    RccFileOutput_ = RccFilePublic_;
  }

  return true;
}

bool cmQtAutoGeneratorRcc::Process()
{
  // Run libuv event loop
  UVRequest().send();
  if (uv_run(UVLoop(), UV_RUN_DEFAULT) == 0) {
    if (Error_) {
      return false;
    }
  } else {
    return false;
  }
  return true;
}

void cmQtAutoGeneratorRcc::UVPollStage(uv_async_t* handle)
{
  reinterpret_cast<cmQtAutoGeneratorRcc*>(handle->data)->PollStage();
}

void cmQtAutoGeneratorRcc::PollStage()
{
  switch (Stage_) {
    // -- Initialize
    case StageT::SETTINGS_READ:
      if (SettingsFileRead()) {
        SetStage(StageT::TEST_QRC_RCC_FILES);
      } else {
        SetStage(StageT::FINISH);
      }
      break;

    // -- Change detection
    case StageT::TEST_QRC_RCC_FILES:
      if (TestQrcRccFiles()) {
        SetStage(StageT::GENERATE);
      } else {
        SetStage(StageT::TEST_RESOURCES_READ);
      }
      break;
    case StageT::TEST_RESOURCES_READ:
      if (TestResourcesRead()) {
        SetStage(StageT::TEST_RESOURCES);
      }
      break;
    case StageT::TEST_RESOURCES:
      if (TestResources()) {
        SetStage(StageT::GENERATE);
      } else {
        SetStage(StageT::TEST_INFO_FILE);
      }
      break;
    case StageT::TEST_INFO_FILE:
      TestInfoFile();
      SetStage(StageT::GENERATE_WRAPPER);
      break;

    // -- Generation
    case StageT::GENERATE:
      GenerateParentDir();
      SetStage(StageT::GENERATE_RCC);
      break;
    case StageT::GENERATE_RCC:
      if (GenerateRcc()) {
        SetStage(StageT::GENERATE_WRAPPER);
      }
      break;
    case StageT::GENERATE_WRAPPER:
      GenerateWrapper();
      SetStage(StageT::SETTINGS_WRITE);
      break;

    // -- Finalize
    case StageT::SETTINGS_WRITE:
      SettingsFileWrite();
      SetStage(StageT::FINISH);
      break;
    case StageT::FINISH:
      // Clear all libuv handles
      UVRequest().reset();
      // Set highest END stage manually
      Stage_ = StageT::END;
      break;
    case StageT::END:
      break;
  }
}

void cmQtAutoGeneratorRcc::SetStage(StageT stage)
{
  if (Error_) {
    stage = StageT::FINISH;
  }
  // Only allow to increase the stage
  if (Stage_ < stage) {
    Stage_ = stage;
    UVRequest().send();
  }
}

std::string cmQtAutoGeneratorRcc::MultiConfigOutput() const
{
  static std::string const suffix = "_CMAKE_";
  std::string res;
  res += RccPathChecksum_;
  res += '/';
  res += AppendFilenameSuffix(RccFileName_, suffix);
  return res;
}

bool cmQtAutoGeneratorRcc::SettingsFileRead()
{
  // Compose current settings strings
  {
    cmCryptoHash crypt(cmCryptoHash::AlgoSHA256);
    std::string const sep(" ~~~ ");
    {
      std::string str;
      str += RccExecutable_;
      str += sep;
      str += cmJoin(RccListOptions_, ";");
      str += sep;
      str += QrcFile_;
      str += sep;
      str += RccPathChecksum_;
      str += sep;
      str += RccFileName_;
      str += sep;
      str += cmJoin(Options_, ";");
      str += sep;
      str += cmJoin(Inputs_, ";");
      str += sep;
      SettingsString_ = crypt.HashString(str);
    }
  }

  // Make sure the settings file exists
  if (!FileSys().FileExists(SettingsFile_, true)) {
    // Touch the settings file to make sure it exists
    FileSys().Touch(SettingsFile_, true);
  }

  // Lock the lock file
  {
    // Make sure the lock file exists
    if (!FileSys().FileExists(LockFile_, true)) {
      if (!FileSys().Touch(LockFile_, true)) {
        Log().ErrorFile(GeneratorT::RCC, LockFile_,
                        "Lock file creation failed");
        Error_ = true;
        return false;
      }
    }
    // Lock the lock file
    cmFileLockResult lockResult =
      LockFileLock_.Lock(LockFile_, static_cast<unsigned long>(-1));
    if (!lockResult.IsOk()) {
      Log().ErrorFile(GeneratorT::RCC, LockFile_,
                      "File lock failed: " + lockResult.GetOutputMessage());
      Error_ = true;
      return false;
    }
  }

  // Read old settings
  {
    std::string content;
    if (FileSys().FileRead(content, SettingsFile_)) {
      SettingsChanged_ = (SettingsString_ != SettingsFind(content, "rcc"));
      // In case any setting changed clear the old settings file.
      // This triggers a full rebuild on the next run if the current
      // build is aborted before writing the current settings in the end.
      if (SettingsChanged_) {
        FileSys().FileWrite(GeneratorT::RCC, SettingsFile_, "");
      }
    } else {
      SettingsChanged_ = true;
    }
  }

  return true;
}

void cmQtAutoGeneratorRcc::SettingsFileWrite()
{
  // Only write if any setting changed
  if (SettingsChanged_) {
    if (Log().Verbose()) {
      Log().Info(GeneratorT::RCC,
                 "Writing settings file " + Quoted(SettingsFile_));
    }
    // Write settings file
    std::string content = "rcc:";
    content += SettingsString_;
    content += '\n';
    if (!FileSys().FileWrite(GeneratorT::RCC, SettingsFile_, content)) {
      Log().ErrorFile(GeneratorT::RCC, SettingsFile_,
                      "Settings file writing failed");
      // Remove old settings file to trigger a full rebuild on the next run
      FileSys().FileRemove(SettingsFile_);
      Error_ = true;
    }
  }

  // Unlock the lock file
  LockFileLock_.Release();
}

bool cmQtAutoGeneratorRcc::TestQrcRccFiles()
{
  // Do basic checks if rcc generation is required

  // Test if the rcc output file exists
  if (!FileSys().FileExists(RccFileOutput_)) {
    if (Log().Verbose()) {
      std::string reason = "Generating ";
      reason += Quoted(RccFileOutput_);
      reason += " from its source file ";
      reason += Quoted(QrcFile_);
      reason += " because it doesn't exist";
      Log().Info(GeneratorT::RCC, reason);
    }
    Generate_ = true;
    return Generate_;
  }

  // Test if the settings changed
  if (SettingsChanged_) {
    if (Log().Verbose()) {
      std::string reason = "Generating ";
      reason += Quoted(RccFileOutput_);
      reason += " from ";
      reason += Quoted(QrcFile_);
      reason += " because the RCC settings changed";
      Log().Info(GeneratorT::RCC, reason);
    }
    Generate_ = true;
    return Generate_;
  }

  // Test if the rcc output file is older than the .qrc file
  {
    bool isOlder = false;
    {
      std::string error;
      isOlder = FileSys().FileIsOlderThan(RccFileOutput_, QrcFile_, &error);
      if (!error.empty()) {
        Log().ErrorFile(GeneratorT::RCC, QrcFile_, error);
        Error_ = true;
      }
    }
    if (isOlder) {
      if (Log().Verbose()) {
        std::string reason = "Generating ";
        reason += Quoted(RccFileOutput_);
        reason += " because it is older than ";
        reason += Quoted(QrcFile_);
        Log().Info(GeneratorT::RCC, reason);
      }
      Generate_ = true;
    }
  }

  return Generate_;
}

bool cmQtAutoGeneratorRcc::TestResourcesRead()
{
  if (!Inputs_.empty()) {
    // Inputs are known already
    return true;
  }

  if (!RccListOptions_.empty()) {
    // Start a rcc list process and parse the output
    if (Process_) {
      // Process is running already
      if (Process_->IsFinished()) {
        // Process is finished
        if (!ProcessResult_.error()) {
          // Process success
          std::string parseError;
          if (!RccListParseOutput(ProcessResult_.StdOut, ProcessResult_.StdErr,
                                  Inputs_, parseError)) {
            Log().ErrorFile(GeneratorT::RCC, QrcFile_, parseError);
            Error_ = true;
          }
        } else {
          Log().ErrorFile(GeneratorT::RCC, QrcFile_,
                          ProcessResult_.ErrorMessage);
          Error_ = true;
        }
        // Clean up
        Process_.reset();
        ProcessResult_.reset();
      } else {
        // Process is not finished, yet.
        return false;
      }
    } else {
      // Start a new process
      // rcc prints relative entry paths when started in the directory of the
      // qrc file with a pathless qrc file name argument.
      // This is important because on Windows absolute paths returned by rcc
      // might contain bad multibyte characters when the qrc file path
      // contains non-ASCII pcharacters.
      std::vector<std::string> cmd;
      cmd.push_back(RccExecutable_);
      cmd.insert(cmd.end(), RccListOptions_.begin(), RccListOptions_.end());
      cmd.push_back(QrcFileName_);
      // We're done here if the process fails to start
      return !StartProcess(QrcFileDir_, cmd, false);
    }
  } else {
    // rcc does not support the --list command.
    // Read the qrc file content and parse it.
    std::string qrcContent;
    if (FileSys().FileRead(GeneratorT::RCC, qrcContent, QrcFile_)) {
      RccListParseContent(qrcContent, Inputs_);
    }
  }

  if (!Inputs_.empty()) {
    // Convert relative paths to absolute paths
    RccListConvertFullPath(QrcFileDir_, Inputs_);
  }

  return true;
}

bool cmQtAutoGeneratorRcc::TestResources()
{
  if (Inputs_.empty()) {
    return true;
  }
  {
    std::string error;
    for (std::string const& resFile : Inputs_) {
      // Check if the resource file exists
      if (!FileSys().FileExists(resFile)) {
        error = "Could not find the resource file\n  ";
        error += Quoted(resFile);
        error += '\n';
        Log().ErrorFile(GeneratorT::RCC, QrcFile_, error);
        Error_ = true;
        break;
      }
      // Check if the resource file is newer than the build file
      if (FileSys().FileIsOlderThan(RccFileOutput_, resFile, &error)) {
        if (Log().Verbose()) {
          std::string reason = "Generating ";
          reason += Quoted(RccFileOutput_);
          reason += " from ";
          reason += Quoted(QrcFile_);
          reason += " because it is older than ";
          reason += Quoted(resFile);
          Log().Info(GeneratorT::RCC, reason);
        }
        Generate_ = true;
        break;
      }
      // Print error and break on demand
      if (!error.empty()) {
        Log().ErrorFile(GeneratorT::RCC, QrcFile_, error);
        Error_ = true;
        break;
      }
    }
  }

  return Generate_;
}

void cmQtAutoGeneratorRcc::TestInfoFile()
{
  // Test if the rcc output file is older than the info file
  {
    bool isOlder = false;
    {
      std::string error;
      isOlder = FileSys().FileIsOlderThan(RccFileOutput_, InfoFile(), &error);
      if (!error.empty()) {
        Log().ErrorFile(GeneratorT::RCC, QrcFile_, error);
        Error_ = true;
      }
    }
    if (isOlder) {
      if (Log().Verbose()) {
        std::string reason = "Touching ";
        reason += Quoted(RccFileOutput_);
        reason += " because it is older than ";
        reason += Quoted(InfoFile());
        Log().Info(GeneratorT::RCC, reason);
      }
      // Touch build file
      FileSys().Touch(RccFileOutput_);
      BuildFileChanged_ = true;
    }
  }
}

void cmQtAutoGeneratorRcc::GenerateParentDir()
{
  // Make sure the parent directory exists
  if (!FileSys().MakeParentDirectory(GeneratorT::RCC, RccFileOutput_)) {
    Error_ = true;
  }
}

/**
 * @return True when finished
 */
bool cmQtAutoGeneratorRcc::GenerateRcc()
{
  if (!Generate_) {
    // Nothing to do
    return true;
  }

  if (Process_) {
    // Process is running already
    if (Process_->IsFinished()) {
      // Process is finished
      if (!ProcessResult_.error()) {
        // Rcc process success
        // Print rcc output
        if (!ProcessResult_.StdOut.empty()) {
          Log().Info(GeneratorT::RCC, ProcessResult_.StdOut);
        }
        BuildFileChanged_ = true;
      } else {
        // Rcc process failed
        {
          std::string emsg = "The rcc process failed to compile\n  ";
          emsg += Quoted(QrcFile_);
          emsg += "\ninto\n  ";
          emsg += Quoted(RccFileOutput_);
          if (ProcessResult_.error()) {
            emsg += "\n";
            emsg += ProcessResult_.ErrorMessage;
          }
          Log().ErrorCommand(GeneratorT::RCC, emsg, Process_->Setup().Command,
                             ProcessResult_.StdOut);
        }
        FileSys().FileRemove(RccFileOutput_);
        Error_ = true;
      }
      // Clean up
      Process_.reset();
      ProcessResult_.reset();
    } else {
      // Process is not finished, yet.
      return false;
    }
  } else {
    // Start a rcc process
    std::vector<std::string> cmd;
    cmd.push_back(RccExecutable_);
    cmd.insert(cmd.end(), Options_.begin(), Options_.end());
    cmd.push_back("-o");
    cmd.push_back(RccFileOutput_);
    cmd.push_back(QrcFile_);
    // We're done here if the process fails to start
    return !StartProcess(AutogenBuildDir_, cmd, true);
  }

  return true;
}

void cmQtAutoGeneratorRcc::GenerateWrapper()
{
  // Generate a wrapper source file on demand
  if (IsMultiConfig()) {
    // Wrapper file content
    std::string content;
    content += "// This is an autogenerated configuration wrapper file.\n";
    content += "// Changes will be overwritten.\n";
    content += "#include <";
    content += MultiConfigOutput();
    content += ">\n";

    // Write content to file
    if (FileSys().FileDiffers(RccFilePublic_, content)) {
      // Write new wrapper file
      if (Log().Verbose()) {
        Log().Info(GeneratorT::RCC,
                   "Generating RCC wrapper file " + RccFilePublic_);
      }
      if (!FileSys().FileWrite(GeneratorT::RCC, RccFilePublic_, content)) {
        Log().ErrorFile(GeneratorT::RCC, RccFilePublic_,
                        "RCC wrapper file writing failed");
        Error_ = true;
      }
    } else if (BuildFileChanged_) {
      // Just touch the wrapper file
      if (Log().Verbose()) {
        Log().Info(GeneratorT::RCC,
                   "Touching RCC wrapper file " + RccFilePublic_);
      }
      FileSys().Touch(RccFilePublic_);
    }
  }
}

bool cmQtAutoGeneratorRcc::StartProcess(
  std::string const& workingDirectory, std::vector<std::string> const& command,
  bool mergedOutput)
{
  // Log command
  if (Log().Verbose()) {
    std::string msg = "Running command:\n";
    msg += QuotedCommand(command);
    msg += '\n';
    Log().Info(GeneratorT::RCC, msg);
  }

  // Create process handler
  Process_ = cm::make_unique<ReadOnlyProcessT>();
  Process_->setup(&ProcessResult_, mergedOutput, command, workingDirectory);
  // Start process
  if (!Process_->start(UVLoop(),
                       std::bind(&cm::uv_async_ptr::send, &UVRequest()))) {
    Log().ErrorFile(GeneratorT::RCC, QrcFile_, ProcessResult_.ErrorMessage);
    Error_ = true;
    // Clean up
    Process_.reset();
    ProcessResult_.reset();
    return false;
  }
  return true;
}
