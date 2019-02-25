/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmQtAutoGenerator.h"
#include "cmQtAutoGen.h"

#include "cmsys/FStream.hxx"

#include "cmAlgorithms.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmStateDirectory.h"
#include "cmStateSnapshot.h"
#include "cmSystemTools.h"
#include "cmake.h"

#include <algorithm>

// -- Class methods

void cmQtAutoGenerator::Logger::RaiseVerbosity(std::string const& value)
{
  unsigned long verbosity = 0;
  if (cmSystemTools::StringToULong(value.c_str(), &verbosity)) {
    if (this->Verbosity_ < verbosity) {
      this->Verbosity_ = static_cast<unsigned int>(verbosity);
    }
  }
}

void cmQtAutoGenerator::Logger::SetColorOutput(bool value)
{
  ColorOutput_ = value;
}

std::string cmQtAutoGenerator::Logger::HeadLine(std::string const& title)
{
  std::string head = title;
  head += '\n';
  head.append(head.size() - 1, '-');
  head += '\n';
  return head;
}

void cmQtAutoGenerator::Logger::Info(GeneratorT genType,
                                     std::string const& message)
{
  std::string msg = GeneratorName(genType);
  msg += ": ";
  msg += message;
  if (msg.back() != '\n') {
    msg.push_back('\n');
  }
  {
    std::lock_guard<std::mutex> lock(Mutex_);
    cmSystemTools::Stdout(msg.c_str(), msg.size());
  }
}

void cmQtAutoGenerator::Logger::Warning(GeneratorT genType,
                                        std::string const& message)
{
  std::string msg;
  if (message.find('\n') == std::string::npos) {
    // Single line message
    msg += GeneratorName(genType);
    msg += " warning: ";
  } else {
    // Multi line message
    msg += HeadLine(GeneratorName(genType) + " warning");
  }
  // Message
  msg += message;
  if (msg.back() != '\n') {
    msg.push_back('\n');
  }
  msg.push_back('\n');
  {
    std::lock_guard<std::mutex> lock(Mutex_);
    cmSystemTools::Stdout(msg.c_str(), msg.size());
  }
}

void cmQtAutoGenerator::Logger::WarningFile(GeneratorT genType,
                                            std::string const& filename,
                                            std::string const& message)
{
  std::string msg = "  ";
  msg += Quoted(filename);
  msg.push_back('\n');
  // Message
  msg += message;
  Warning(genType, msg);
}

void cmQtAutoGenerator::Logger::Error(GeneratorT genType,
                                      std::string const& message)
{
  std::string msg;
  msg += HeadLine(GeneratorName(genType) + " error");
  // Message
  msg += message;
  if (msg.back() != '\n') {
    msg.push_back('\n');
  }
  msg.push_back('\n');
  {
    std::lock_guard<std::mutex> lock(Mutex_);
    cmSystemTools::Stderr(msg.c_str(), msg.size());
  }
}

void cmQtAutoGenerator::Logger::ErrorFile(GeneratorT genType,
                                          std::string const& filename,
                                          std::string const& message)
{
  std::string emsg = "  ";
  emsg += Quoted(filename);
  emsg += '\n';
  // Message
  emsg += message;
  Error(genType, emsg);
}

void cmQtAutoGenerator::Logger::ErrorCommand(
  GeneratorT genType, std::string const& message,
  std::vector<std::string> const& command, std::string const& output)
{
  std::string msg;
  msg.push_back('\n');
  msg += HeadLine(GeneratorName(genType) + " subprocess error");
  msg += message;
  if (msg.back() != '\n') {
    msg.push_back('\n');
  }
  msg.push_back('\n');
  msg += HeadLine("Command");
  msg += QuotedCommand(command);
  if (msg.back() != '\n') {
    msg.push_back('\n');
  }
  msg.push_back('\n');
  msg += HeadLine("Output");
  msg += output;
  if (msg.back() != '\n') {
    msg.push_back('\n');
  }
  msg.push_back('\n');
  {
    std::lock_guard<std::mutex> lock(Mutex_);
    cmSystemTools::Stderr(msg.c_str(), msg.size());
  }
}

std::string cmQtAutoGenerator::FileSystem::GetRealPath(
  std::string const& filename)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return cmSystemTools::GetRealPath(filename);
}

std::string cmQtAutoGenerator::FileSystem::CollapseCombinedPath(
  std::string const& dir, std::string const& file)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return cmSystemTools::CollapseCombinedPath(dir, file);
}

void cmQtAutoGenerator::FileSystem::SplitPath(
  const std::string& p, std::vector<std::string>& components,
  bool expand_home_dir)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  cmSystemTools::SplitPath(p, components, expand_home_dir);
}

std::string cmQtAutoGenerator::FileSystem::JoinPath(
  const std::vector<std::string>& components)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return cmSystemTools::JoinPath(components);
}

std::string cmQtAutoGenerator::FileSystem::JoinPath(
  std::vector<std::string>::const_iterator first,
  std::vector<std::string>::const_iterator last)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return cmSystemTools::JoinPath(first, last);
}

std::string cmQtAutoGenerator::FileSystem::GetFilenameWithoutLastExtension(
  const std::string& filename)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return cmSystemTools::GetFilenameWithoutLastExtension(filename);
}

std::string cmQtAutoGenerator::FileSystem::SubDirPrefix(
  std::string const& filename)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return cmQtAutoGen::SubDirPrefix(filename);
}

void cmQtAutoGenerator::FileSystem::setupFilePathChecksum(
  std::string const& currentSrcDir, std::string const& currentBinDir,
  std::string const& projectSrcDir, std::string const& projectBinDir)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  FilePathChecksum_.setupParentDirs(currentSrcDir, currentBinDir,
                                    projectSrcDir, projectBinDir);
}

std::string cmQtAutoGenerator::FileSystem::GetFilePathChecksum(
  std::string const& filename)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return FilePathChecksum_.getPart(filename);
}

bool cmQtAutoGenerator::FileSystem::FileExists(std::string const& filename)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return cmSystemTools::FileExists(filename);
}

bool cmQtAutoGenerator::FileSystem::FileExists(std::string const& filename,
                                               bool isFile)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return cmSystemTools::FileExists(filename, isFile);
}

unsigned long cmQtAutoGenerator::FileSystem::FileLength(
  std::string const& filename)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return cmSystemTools::FileLength(filename);
}

bool cmQtAutoGenerator::FileSystem::FileIsOlderThan(
  std::string const& buildFile, std::string const& sourceFile,
  std::string* error)
{
  bool res(false);
  int result = 0;
  {
    std::lock_guard<std::mutex> lock(Mutex_);
    res = cmSystemTools::FileTimeCompare(buildFile, sourceFile, &result);
  }
  if (res) {
    res = (result < 0);
  } else {
    if (error != nullptr) {
      error->append(
        "File modification time comparison failed for the files\n  ");
      error->append(Quoted(buildFile));
      error->append("\nand\n  ");
      error->append(Quoted(sourceFile));
    }
  }
  return res;
}

bool cmQtAutoGenerator::FileSystem::FileRead(std::string& content,
                                             std::string const& filename,
                                             std::string* error)
{
  bool success = false;
  if (FileExists(filename, true)) {
    unsigned long const length = FileLength(filename);
    {
      std::lock_guard<std::mutex> lock(Mutex_);
      cmsys::ifstream ifs(filename.c_str(), (std::ios::in | std::ios::binary));
      if (ifs) {
        content.reserve(length);
        content.assign(std::istreambuf_iterator<char>{ ifs },
                       std::istreambuf_iterator<char>{});
        if (ifs) {
          success = true;
        } else {
          content.clear();
          if (error != nullptr) {
            error->append("Reading from the file failed.");
          }
        }
      } else if (error != nullptr) {
        error->append("Opening the file for reading failed.");
      }
    }
  } else if (error != nullptr) {
    error->append(
      "The file does not exist, is not readable or is a directory.");
  }
  return success;
}

bool cmQtAutoGenerator::FileSystem::FileRead(GeneratorT genType,
                                             std::string& content,
                                             std::string const& filename)
{
  std::string error;
  if (!FileRead(content, filename, &error)) {
    Log()->ErrorFile(genType, filename, error);
    return false;
  }
  return true;
}

bool cmQtAutoGenerator::FileSystem::FileWrite(std::string const& filename,
                                              std::string const& content,
                                              std::string* error)
{
  bool success = false;
  // Make sure the parent directory exists
  if (MakeParentDirectory(filename)) {
    std::lock_guard<std::mutex> lock(Mutex_);
    cmsys::ofstream outfile;
    outfile.open(filename.c_str(),
                 (std::ios::out | std::ios::binary | std::ios::trunc));
    if (outfile) {
      outfile << content;
      // Check for write errors
      if (outfile.good()) {
        success = true;
      } else {
        if (error != nullptr) {
          error->assign("File writing failed");
        }
      }
    } else {
      if (error != nullptr) {
        error->assign("Opening file for writing failed");
      }
    }
  } else {
    if (error != nullptr) {
      error->assign("Could not create parent directory");
    }
  }
  return success;
}

bool cmQtAutoGenerator::FileSystem::FileWrite(GeneratorT genType,
                                              std::string const& filename,
                                              std::string const& content)
{
  std::string error;
  if (!FileWrite(filename, content, &error)) {
    Log()->ErrorFile(genType, filename, error);
    return false;
  }
  return true;
}

bool cmQtAutoGenerator::FileSystem::FileDiffers(std::string const& filename,
                                                std::string const& content)
{
  bool differs = true;
  {
    std::string oldContents;
    if (FileRead(oldContents, filename)) {
      differs = (oldContents != content);
    }
  }
  return differs;
}

bool cmQtAutoGenerator::FileSystem::FileRemove(std::string const& filename)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return cmSystemTools::RemoveFile(filename);
}

bool cmQtAutoGenerator::FileSystem::Touch(std::string const& filename,
                                          bool create)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return cmSystemTools::Touch(filename, create);
}

bool cmQtAutoGenerator::FileSystem::MakeDirectory(std::string const& dirname)
{
  std::lock_guard<std::mutex> lock(Mutex_);
  return cmSystemTools::MakeDirectory(dirname);
}

bool cmQtAutoGenerator::FileSystem::MakeDirectory(GeneratorT genType,
                                                  std::string const& dirname)
{
  if (!MakeDirectory(dirname)) {
    Log()->ErrorFile(genType, dirname, "Could not create directory");
    return false;
  }
  return true;
}

bool cmQtAutoGenerator::FileSystem::MakeParentDirectory(
  std::string const& filename)
{
  bool success = true;
  std::string const dirName = cmSystemTools::GetFilenamePath(filename);
  if (!dirName.empty()) {
    success = MakeDirectory(dirName);
  }
  return success;
}

bool cmQtAutoGenerator::FileSystem::MakeParentDirectory(
  GeneratorT genType, std::string const& filename)
{
  if (!MakeParentDirectory(filename)) {
    Log()->ErrorFile(genType, filename, "Could not create parent directory");
    return false;
  }
  return true;
}

int cmQtAutoGenerator::ReadOnlyProcessT::PipeT::init(uv_loop_t* uv_loop,
                                                     ReadOnlyProcessT* process)
{
  Process_ = process;
  Target_ = nullptr;
  return UVPipe_.init(*uv_loop, 0, this);
}

int cmQtAutoGenerator::ReadOnlyProcessT::PipeT::startRead(std::string* target)
{
  Target_ = target;
  return uv_read_start(uv_stream(), &PipeT::UVAlloc, &PipeT::UVData);
}

void cmQtAutoGenerator::ReadOnlyProcessT::PipeT::reset()
{
  Process_ = nullptr;
  Target_ = nullptr;
  UVPipe_.reset();
  Buffer_.clear();
  Buffer_.shrink_to_fit();
}

void cmQtAutoGenerator::ReadOnlyProcessT::PipeT::UVAlloc(uv_handle_t* handle,
                                                         size_t suggestedSize,
                                                         uv_buf_t* buf)
{
  auto& pipe = *reinterpret_cast<PipeT*>(handle->data);
  pipe.Buffer_.resize(suggestedSize);
  buf->base = &pipe.Buffer_.front();
  buf->len = pipe.Buffer_.size();
}

void cmQtAutoGenerator::ReadOnlyProcessT::PipeT::UVData(uv_stream_t* stream,
                                                        ssize_t nread,
                                                        const uv_buf_t* buf)
{
  auto& pipe = *reinterpret_cast<PipeT*>(stream->data);
  if (nread > 0) {
    // Append data to merged output
    if ((buf->base != nullptr) && (pipe.Target_ != nullptr)) {
      pipe.Target_->append(buf->base, nread);
    }
  } else if (nread < 0) {
    // EOF or error
    auto* proc = pipe.Process_;
    // Check it this an unusual error
    if (nread != UV_EOF) {
      if (!proc->Result()->error()) {
        proc->Result()->ErrorMessage =
          "libuv reading from pipe failed with error code ";
        proc->Result()->ErrorMessage += std::to_string(nread);
      }
    }
    // Clear libuv pipe handle and try to finish
    pipe.reset();
    proc->UVTryFinish();
  }
}

void cmQtAutoGenerator::ProcessResultT::reset()
{
  ExitStatus = 0;
  TermSignal = 0;
  if (!StdOut.empty()) {
    StdOut.clear();
    StdOut.shrink_to_fit();
  }
  if (!StdErr.empty()) {
    StdErr.clear();
    StdErr.shrink_to_fit();
  }
  if (!ErrorMessage.empty()) {
    ErrorMessage.clear();
    ErrorMessage.shrink_to_fit();
  }
}

void cmQtAutoGenerator::ReadOnlyProcessT::setup(
  ProcessResultT* result, bool mergedOutput,
  std::vector<std::string> const& command, std::string const& workingDirectory)
{
  Setup_.WorkingDirectory = workingDirectory;
  Setup_.Command = command;
  Setup_.Result = result;
  Setup_.MergedOutput = mergedOutput;
}

bool cmQtAutoGenerator::ReadOnlyProcessT::start(
  uv_loop_t* uv_loop, std::function<void()>&& finishedCallback)
{
  if (IsStarted() || (Result() == nullptr)) {
    return false;
  }

  // Reset result before the start
  Result()->reset();

  // Fill command string pointers
  if (!Setup().Command.empty()) {
    CommandPtr_.reserve(Setup().Command.size() + 1);
    for (std::string const& arg : Setup().Command) {
      CommandPtr_.push_back(arg.c_str());
    }
    CommandPtr_.push_back(nullptr);
  } else {
    Result()->ErrorMessage = "Empty command";
  }

  if (!Result()->error()) {
    if (UVPipeOut_.init(uv_loop, this) != 0) {
      Result()->ErrorMessage = "libuv stdout pipe initialization failed";
    }
  }
  if (!Result()->error()) {
    if (UVPipeErr_.init(uv_loop, this) != 0) {
      Result()->ErrorMessage = "libuv stderr pipe initialization failed";
    }
  }
  if (!Result()->error()) {
    // -- Setup process stdio options
    // stdin
    UVOptionsStdIO_[0].flags = UV_IGNORE;
    UVOptionsStdIO_[0].data.stream = nullptr;
    // stdout
    UVOptionsStdIO_[1].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
    UVOptionsStdIO_[1].data.stream = UVPipeOut_.uv_stream();
    // stderr
    UVOptionsStdIO_[2].flags =
      static_cast<uv_stdio_flags>(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
    UVOptionsStdIO_[2].data.stream = UVPipeErr_.uv_stream();

    // -- Setup process options
    std::fill_n(reinterpret_cast<char*>(&UVOptions_), sizeof(UVOptions_), 0);
    UVOptions_.exit_cb = &ReadOnlyProcessT::UVExit;
    UVOptions_.file = CommandPtr_[0];
    UVOptions_.args = const_cast<char**>(&CommandPtr_.front());
    UVOptions_.cwd = Setup_.WorkingDirectory.c_str();
    UVOptions_.flags = UV_PROCESS_WINDOWS_HIDE;
    UVOptions_.stdio_count = static_cast<int>(UVOptionsStdIO_.size());
    UVOptions_.stdio = &UVOptionsStdIO_.front();

    // -- Spawn process
    if (UVProcess_.spawn(*uv_loop, UVOptions_, this) != 0) {
      Result()->ErrorMessage = "libuv process spawn failed";
    }
  }
  // -- Start reading from stdio streams
  if (!Result()->error()) {
    if (UVPipeOut_.startRead(&Result()->StdOut) != 0) {
      Result()->ErrorMessage = "libuv start reading from stdout pipe failed";
    }
  }
  if (!Result()->error()) {
    if (UVPipeErr_.startRead(Setup_.MergedOutput ? &Result()->StdOut
                                                 : &Result()->StdErr) != 0) {
      Result()->ErrorMessage = "libuv start reading from stderr pipe failed";
    }
  }

  if (!Result()->error()) {
    IsStarted_ = true;
    FinishedCallback_ = std::move(finishedCallback);
  } else {
    // Clear libuv handles and finish
    UVProcess_.reset();
    UVPipeOut_.reset();
    UVPipeErr_.reset();
    CommandPtr_.clear();
  }

  return IsStarted();
}

void cmQtAutoGenerator::ReadOnlyProcessT::UVExit(uv_process_t* handle,
                                                 int64_t exitStatus,
                                                 int termSignal)
{
  auto& proc = *reinterpret_cast<ReadOnlyProcessT*>(handle->data);
  if (proc.IsStarted() && !proc.IsFinished()) {
    // Set error message on demand
    proc.Result()->ExitStatus = exitStatus;
    proc.Result()->TermSignal = termSignal;
    if (!proc.Result()->error()) {
      if (termSignal != 0) {
        proc.Result()->ErrorMessage = "Process was terminated by signal ";
        proc.Result()->ErrorMessage +=
          std::to_string(proc.Result()->TermSignal);
      } else if (exitStatus != 0) {
        proc.Result()->ErrorMessage = "Process failed with return value ";
        proc.Result()->ErrorMessage +=
          std::to_string(proc.Result()->ExitStatus);
      }
    }

    // Reset process handle and try to finish
    proc.UVProcess_.reset();
    proc.UVTryFinish();
  }
}

void cmQtAutoGenerator::ReadOnlyProcessT::UVTryFinish()
{
  // There still might be data in the pipes after the process has finished.
  // Therefore check if the process is finished AND all pipes are closed
  // before signaling the worker thread to continue.
  if (UVProcess_.get() == nullptr) {
    if (UVPipeOut_.uv_pipe() == nullptr) {
      if (UVPipeErr_.uv_pipe() == nullptr) {
        IsFinished_ = true;
        FinishedCallback_();
      }
    }
  }
}

cmQtAutoGenerator::cmQtAutoGenerator()
  : FileSys_(&Logger_)
{
  // Initialize logger
  {
    std::string verbose;
    if (cmSystemTools::GetEnv("VERBOSE", verbose) && !verbose.empty()) {
      unsigned long iVerbose = 0;
      if (cmSystemTools::StringToULong(verbose.c_str(), &iVerbose)) {
        Logger_.SetVerbosity(static_cast<unsigned int>(iVerbose));
      } else {
        // Non numeric verbosity
        Logger_.SetVerbose(cmSystemTools::IsOn(verbose));
      }
    }
  }
  {
    std::string colorEnv;
    cmSystemTools::GetEnv("COLOR", colorEnv);
    if (!colorEnv.empty()) {
      Logger_.SetColorOutput(cmSystemTools::IsOn(colorEnv));
    } else {
      Logger_.SetColorOutput(true);
    }
  }

  // Initialize libuv loop
  uv_disable_stdio_inheritance();
#ifdef CMAKE_UV_SIGNAL_HACK
  UVHackRAII_ = cm::make_unique<cmUVSignalHackRAII>();
#endif
  UVLoop_ = cm::make_unique<uv_loop_t>();
  uv_loop_init(UVLoop());
}

cmQtAutoGenerator::~cmQtAutoGenerator()
{
  // Close libuv loop
  uv_loop_close(UVLoop());
}

bool cmQtAutoGenerator::Run(std::string const& infoFile,
                            std::string const& config)
{
  // Info settings
  InfoFile_ = infoFile;
  cmSystemTools::ConvertToUnixSlashes(InfoFile_);
  InfoDir_ = cmSystemTools::GetFilenamePath(infoFile);
  InfoConfig_ = config;

  bool success = false;
  {
    cmake cm(cmake::RoleScript);
    cm.SetHomeOutputDirectory(InfoDir());
    cm.SetHomeDirectory(InfoDir());
    cm.GetCurrentSnapshot().SetDefaultDefinitions();
    cmGlobalGenerator gg(&cm);

    cmStateSnapshot snapshot = cm.GetCurrentSnapshot();
    snapshot.GetDirectory().SetCurrentBinary(InfoDir());
    snapshot.GetDirectory().SetCurrentSource(InfoDir());

    auto makefile = cm::make_unique<cmMakefile>(&gg, snapshot);
    // The OLD/WARN behavior for policy CMP0053 caused a speed regression.
    // https://gitlab.kitware.com/cmake/cmake/issues/17570
    makefile->SetPolicyVersion("3.9", std::string());
    gg.SetCurrentMakefile(makefile.get());
    success = this->Init(makefile.get());
  }
  if (success) {
    success = this->Process();
  }
  return success;
}

std::string cmQtAutoGenerator::SettingsFind(std::string const& content,
                                            const char* key)
{
  std::string prefix(key);
  prefix += ':';
  std::string::size_type pos = content.find(prefix);
  if (pos != std::string::npos) {
    pos += prefix.size();
    if (pos < content.size()) {
      std::string::size_type posE = content.find('\n', pos);
      if ((posE != std::string::npos) && (posE != pos)) {
        return content.substr(pos, posE - pos);
      }
    }
  }
  return std::string();
}
