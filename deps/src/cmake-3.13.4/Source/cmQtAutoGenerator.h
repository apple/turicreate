/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmQtAutoGenerator_h
#define cmQtAutoGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmFilePathChecksum.h"
#include "cmQtAutoGen.h"
#include "cmUVHandlePtr.h"
#include "cmUVSignalHackRAII.h" // IWYU pragma: keep
#include "cm_uv.h"

#include <array>
#include <functional>
#include <mutex>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

class cmMakefile;

/// @brief Base class for QtAutoGen gernerators
class cmQtAutoGenerator : public cmQtAutoGen
{
  CM_DISABLE_COPY(cmQtAutoGenerator)
public:
  // -- Types

  /// @brief Thread safe logging
  class Logger
  {
  public:
    // -- Verbosity
    unsigned int Verbosity() const { return this->Verbosity_; }
    void SetVerbosity(unsigned int value) { this->Verbosity_ = value; }
    void RaiseVerbosity(std::string const& value);
    bool Verbose() const { return (this->Verbosity_ != 0); }
    void SetVerbose(bool value) { this->Verbosity_ = value ? 1 : 0; }
    bool ColorOutput() const { return this->ColorOutput_; }
    void SetColorOutput(bool value);
    // -- Log info
    void Info(GeneratorT genType, std::string const& message);
    // -- Log warning
    void Warning(GeneratorT genType, std::string const& message);
    void WarningFile(GeneratorT genType, std::string const& filename,
                     std::string const& message);
    // -- Log error
    void Error(GeneratorT genType, std::string const& message);
    void ErrorFile(GeneratorT genType, std::string const& filename,
                   std::string const& message);
    void ErrorCommand(GeneratorT genType, std::string const& message,
                      std::vector<std::string> const& command,
                      std::string const& output);

  private:
    static std::string HeadLine(std::string const& title);

  private:
    std::mutex Mutex_;
    unsigned int Verbosity_ = 0;
    bool ColorOutput_ = false;
  };

  /// @brief Thread safe file system interface
  class FileSystem
  {
  public:
    FileSystem(Logger* log)
      : Log_(log)
    {
    }

    /// @brief Logger
    Logger* Log() const { return Log_; }

    // -- Paths
    /// @brief Wrapper for cmSystemTools::GetRealPath
    std::string GetRealPath(std::string const& filename);
    /// @brief Wrapper for cmSystemTools::CollapseCombinedPath
    std::string CollapseCombinedPath(std::string const& dir,
                                     std::string const& file);
    /// @brief Wrapper for cmSystemTools::SplitPath
    void SplitPath(const std::string& p, std::vector<std::string>& components,
                   bool expand_home_dir = true);
    /// @brief Wrapper for cmSystemTools::JoinPath
    std::string JoinPath(const std::vector<std::string>& components);
    /// @brief Wrapper for cmSystemTools::JoinPath
    std::string JoinPath(std::vector<std::string>::const_iterator first,
                         std::vector<std::string>::const_iterator last);
    /// @brief Wrapper for cmSystemTools::GetFilenameWithoutLastExtension
    std::string GetFilenameWithoutLastExtension(const std::string& filename);
    /// @brief Wrapper for cmQtAutoGen::SubDirPrefix
    std::string SubDirPrefix(std::string const& filename);
    /// @brief Wrapper for cmFilePathChecksum::setupParentDirs
    void setupFilePathChecksum(std::string const& currentSrcDir,
                               std::string const& currentBinDir,
                               std::string const& projectSrcDir,
                               std::string const& projectBinDir);
    /// @brief Wrapper for cmFilePathChecksum::getPart
    std::string GetFilePathChecksum(std::string const& filename);

    // -- File access
    /// @brief Wrapper for cmSystemTools::FileExists
    bool FileExists(std::string const& filename);
    /// @brief Wrapper for cmSystemTools::FileExists
    bool FileExists(std::string const& filename, bool isFile);
    /// @brief Wrapper for cmSystemTools::FileLength
    unsigned long FileLength(std::string const& filename);
    bool FileIsOlderThan(std::string const& buildFile,
                         std::string const& sourceFile,
                         std::string* error = nullptr);

    bool FileRead(std::string& content, std::string const& filename,
                  std::string* error = nullptr);
    /// @brief Error logging version
    bool FileRead(GeneratorT genType, std::string& content,
                  std::string const& filename);

    bool FileWrite(std::string const& filename, std::string const& content,
                   std::string* error = nullptr);
    /// @brief Error logging version
    bool FileWrite(GeneratorT genType, std::string const& filename,
                   std::string const& content);

    bool FileDiffers(std::string const& filename, std::string const& content);

    bool FileRemove(std::string const& filename);
    bool Touch(std::string const& filename, bool create = false);

    // -- Directory access
    bool MakeDirectory(std::string const& dirname);
    /// @brief Error logging version
    bool MakeDirectory(GeneratorT genType, std::string const& dirname);

    bool MakeParentDirectory(std::string const& filename);
    /// @brief Error logging version
    bool MakeParentDirectory(GeneratorT genType, std::string const& filename);

  private:
    std::mutex Mutex_;
    cmFilePathChecksum FilePathChecksum_;
    Logger* Log_;
  };

  /// @brief Return value and output of an external process
  struct ProcessResultT
  {
    void reset();
    bool error() const
    {
      return (ExitStatus != 0) || (TermSignal != 0) || !ErrorMessage.empty();
    }

    std::int64_t ExitStatus = 0;
    int TermSignal = 0;
    std::string StdOut;
    std::string StdErr;
    std::string ErrorMessage;
  };

  /// @brief External process management class
  struct ReadOnlyProcessT
  {
    // -- Types

    /// @brief libuv pipe buffer class
    class PipeT
    {
    public:
      int init(uv_loop_t* uv_loop, ReadOnlyProcessT* process);
      int startRead(std::string* target);
      void reset();

      // -- Libuv casts
      uv_pipe_t* uv_pipe() { return UVPipe_.get(); }
      uv_stream_t* uv_stream()
      {
        return reinterpret_cast<uv_stream_t*>(uv_pipe());
      }
      uv_handle_t* uv_handle()
      {
        return reinterpret_cast<uv_handle_t*>(uv_pipe());
      }

      // -- Libuv callbacks
      static void UVAlloc(uv_handle_t* handle, size_t suggestedSize,
                          uv_buf_t* buf);
      static void UVData(uv_stream_t* stream, ssize_t nread,
                         const uv_buf_t* buf);

    private:
      ReadOnlyProcessT* Process_ = nullptr;
      std::string* Target_ = nullptr;
      std::vector<char> Buffer_;
      cm::uv_pipe_ptr UVPipe_;
    };

    /// @brief Process settings
    struct SetupT
    {
      std::string WorkingDirectory;
      std::vector<std::string> Command;
      ProcessResultT* Result = nullptr;
      bool MergedOutput = false;
    };

    // -- Constructor
    ReadOnlyProcessT() = default;

    // -- Const accessors
    const SetupT& Setup() const { return Setup_; }
    ProcessResultT* Result() const { return Setup_.Result; }
    bool IsStarted() const { return IsStarted_; }
    bool IsFinished() const { return IsFinished_; }

    // -- Runtime
    void setup(ProcessResultT* result, bool mergedOutput,
               std::vector<std::string> const& command,
               std::string const& workingDirectory = std::string());
    bool start(uv_loop_t* uv_loop, std::function<void()>&& finishedCallback);

  private:
    // -- Friends
    friend class PipeT;
    // -- Libuv callbacks
    static void UVExit(uv_process_t* handle, int64_t exitStatus,
                       int termSignal);
    void UVTryFinish();

    // -- Setup
    SetupT Setup_;
    // -- Runtime
    bool IsStarted_ = false;
    bool IsFinished_ = false;
    std::function<void()> FinishedCallback_;
    std::vector<const char*> CommandPtr_;
    std::array<uv_stdio_container_t, 3> UVOptionsStdIO_;
    uv_process_options_t UVOptions_;
    cm::uv_process_ptr UVProcess_;
    PipeT UVPipeOut_;
    PipeT UVPipeErr_;
  };

public:
  // -- Constructors
  cmQtAutoGenerator();
  virtual ~cmQtAutoGenerator();

  // -- Run
  bool Run(std::string const& infoFile, std::string const& config);

  // -- Accessors
  // Logging
  Logger& Log() { return Logger_; }
  // File System
  FileSystem& FileSys() { return FileSys_; }
  // InfoFile
  std::string const& InfoFile() const { return InfoFile_; }
  std::string const& InfoDir() const { return InfoDir_; }
  std::string const& InfoConfig() const { return InfoConfig_; }
  // libuv loop
  uv_loop_t* UVLoop() { return UVLoop_.get(); }
  cm::uv_async_ptr& UVRequest() { return UVRequest_; }

  // -- Utility
  static std::string SettingsFind(std::string const& content, const char* key);

protected:
  // -- Abstract processing interface
  virtual bool Init(cmMakefile* makefile) = 0;
  virtual bool Process() = 0;

private:
  // -- Logging
  Logger Logger_;
  FileSystem FileSys_;
  // -- Info settings
  std::string InfoFile_;
  std::string InfoDir_;
  std::string InfoConfig_;
// -- libuv loop
#ifdef CMAKE_UV_SIGNAL_HACK
  std::unique_ptr<cmUVSignalHackRAII> UVHackRAII_;
#endif
  std::unique_ptr<uv_loop_t> UVLoop_;
  cm::uv_async_ptr UVRequest_;
};

#endif
