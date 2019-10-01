/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmQtAutoGeneratorMocUic_h
#define cmQtAutoGeneratorMocUic_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmQtAutoGen.h"
#include "cmQtAutoGenerator.h"
#include "cmUVHandlePtr.h"
#include "cm_uv.h"
#include "cmsys/RegularExpression.hxx"

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <map>
#include <memory> // IWYU pragma: keep
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

class cmMakefile;

// @brief AUTOMOC and AUTOUIC generator
class cmQtAutoGeneratorMocUic : public cmQtAutoGenerator
{
  CM_DISABLE_COPY(cmQtAutoGeneratorMocUic)
public:
  cmQtAutoGeneratorMocUic();
  ~cmQtAutoGeneratorMocUic() override;

public:
  // -- Types
  class WorkerT;

  /// @brief Search key plus regular expression pair
  ///
  struct KeyExpT
  {
    KeyExpT() = default;

    KeyExpT(const char* key, const char* exp)
      : Key(key)
      , Exp(exp)
    {
    }

    KeyExpT(std::string const& key, std::string const& exp)
      : Key(key)
      , Exp(exp)
    {
    }

    std::string Key;
    cmsys::RegularExpression Exp;
  };

  /// @brief Common settings
  ///
  class BaseSettingsT
  {
    CM_DISABLE_COPY(BaseSettingsT)
  public:
    // -- Volatile methods
    BaseSettingsT(FileSystem* fileSystem)
      : MultiConfig(false)
      , IncludeProjectDirsBefore(false)
      , QtVersionMajor(4)
      , NumThreads(1)
      , FileSys(fileSystem)
    {
    }

    // -- Const methods
    std::string AbsoluteBuildPath(std::string const& relativePath) const;
    bool FindHeader(std::string& header,
                    std::string const& testBasePath) const;

    // -- Attributes
    // - Config
    bool MultiConfig;
    bool IncludeProjectDirsBefore;
    unsigned int QtVersionMajor;
    unsigned int NumThreads;
    // - Directories
    std::string ProjectSourceDir;
    std::string ProjectBinaryDir;
    std::string CurrentSourceDir;
    std::string CurrentBinaryDir;
    std::string AutogenBuildDir;
    std::string AutogenIncludeDir;
    // - Files
    std::vector<std::string> HeaderExtensions;
    // - File system
    FileSystem* FileSys;
  };

  /// @brief Moc settings
  ///
  class MocSettingsT
  {
    CM_DISABLE_COPY(MocSettingsT)
  public:
    MocSettingsT(FileSystem* fileSys)
      : FileSys(fileSys)
    {
    }

    // -- Const methods
    bool skipped(std::string const& fileName) const;
    std::string FindMacro(std::string const& content) const;
    std::string MacrosString() const;
    std::string FindIncludedFile(std::string const& sourcePath,
                                 std::string const& includeString) const;
    void FindDependencies(std::string const& content,
                          std::set<std::string>& depends) const;

    // -- Attributes
    bool Enabled = false;
    bool SettingsChanged = false;
    bool RelaxedMode = false;
    std::string Executable;
    std::string CompFileAbs;
    std::string PredefsFileRel;
    std::string PredefsFileAbs;
    std::set<std::string> SkipList;
    std::vector<std::string> IncludePaths;
    std::vector<std::string> Includes;
    std::vector<std::string> Definitions;
    std::vector<std::string> Options;
    std::vector<std::string> AllOptions;
    std::vector<std::string> PredefsCmd;
    std::vector<KeyExpT> DependFilters;
    std::vector<KeyExpT> MacroFilters;
    cmsys::RegularExpression RegExpInclude;
    // - File system
    FileSystem* FileSys;
  };

  /// @brief Uic settings
  ///
  class UicSettingsT
  {
    CM_DISABLE_COPY(UicSettingsT)
  public:
    UicSettingsT() = default;
    // -- Const methods
    bool skipped(std::string const& fileName) const;

    // -- Attributes
    bool Enabled = false;
    bool SettingsChanged = false;
    std::string Executable;
    std::set<std::string> SkipList;
    std::vector<std::string> TargetOptions;
    std::map<std::string, std::vector<std::string>> Options;
    std::vector<std::string> SearchPaths;
    cmsys::RegularExpression RegExpInclude;
  };

  /// @brief Abstract job class for threaded processing
  ///
  class JobT
  {
    CM_DISABLE_COPY(JobT)
  public:
    JobT() = default;
    virtual ~JobT() = default;
    // -- Abstract processing interface
    virtual void Process(WorkerT& wrk) = 0;
  };

  /// @brief Deleter for classes derived from Job
  ///
  struct JobDeleterT
  {
    void operator()(JobT* job);
  };

  // Job management types
  typedef std::unique_ptr<JobT, JobDeleterT> JobHandleT;
  typedef std::deque<JobHandleT> JobQueueT;

  /// @brief Parse source job
  ///
  class JobParseT : public JobT
  {
  public:
    JobParseT(std::string&& fileName, bool moc, bool uic, bool header = false)
      : FileName(std::move(fileName))
      , AutoMoc(moc)
      , AutoUic(uic)
      , Header(header)
    {
    }

  private:
    struct MetaT
    {
      std::string Content;
      std::string FileDir;
      std::string FileBase;
    };

    void Process(WorkerT& wrk) override;
    bool ParseMocSource(WorkerT& wrk, MetaT const& meta);
    bool ParseMocHeader(WorkerT& wrk, MetaT const& meta);
    std::string MocStringHeaders(WorkerT& wrk,
                                 std::string const& fileBase) const;
    std::string MocFindIncludedHeader(WorkerT& wrk,
                                      std::string const& includerDir,
                                      std::string const& includeBase);
    bool ParseUic(WorkerT& wrk, MetaT const& meta);
    bool ParseUicInclude(WorkerT& wrk, MetaT const& meta,
                         std::string&& includeString);
    std::string UicFindIncludedFile(WorkerT& wrk, MetaT const& meta,
                                    std::string const& includeString);

  private:
    std::string FileName;
    bool AutoMoc = false;
    bool AutoUic = false;
    bool Header = false;
  };

  /// @brief Generate moc_predefs
  ///
  class JobMocPredefsT : public JobT
  {
  private:
    void Process(WorkerT& wrk) override;
  };

  /// @brief Moc a file job
  ///
  class JobMocT : public JobT
  {
  public:
    JobMocT(std::string&& sourceFile, std::string const& includerFile,
            std::string&& includeString)
      : SourceFile(std::move(sourceFile))
      , IncluderFile(includerFile)
      , IncludeString(std::move(includeString))
    {
    }

    void FindDependencies(WorkerT& wrk, std::string const& content);

  private:
    void Process(WorkerT& wrk) override;
    bool UpdateRequired(WorkerT& wrk);
    void GenerateMoc(WorkerT& wrk);

  public:
    std::string SourceFile;
    std::string IncluderFile;
    std::string IncludeString;
    std::string BuildFile;
    bool DependsValid = false;
    std::set<std::string> Depends;
  };

  /// @brief Uic a file job
  ///
  class JobUicT : public JobT
  {
  public:
    JobUicT(std::string&& sourceFile, std::string const& includerFile,
            std::string&& includeString)
      : SourceFile(std::move(sourceFile))
      , IncluderFile(includerFile)
      , IncludeString(std::move(includeString))
    {
    }

  private:
    void Process(WorkerT& wrk) override;
    bool UpdateRequired(WorkerT& wrk);
    void GenerateUic(WorkerT& wrk);

  public:
    std::string SourceFile;
    std::string IncluderFile;
    std::string IncludeString;
    std::string BuildFile;
  };

  /// @brief Worker Thread
  ///
  class WorkerT
  {
    CM_DISABLE_COPY(WorkerT)
  public:
    WorkerT(cmQtAutoGeneratorMocUic* gen, uv_loop_t* uvLoop);
    ~WorkerT();

    // -- Const accessors
    cmQtAutoGeneratorMocUic& Gen() const { return *Gen_; }
    Logger& Log() const { return Gen_->Log(); }
    FileSystem& FileSys() const { return Gen_->FileSys(); }
    const BaseSettingsT& Base() const { return Gen_->Base(); }
    const MocSettingsT& Moc() const { return Gen_->Moc(); }
    const UicSettingsT& Uic() const { return Gen_->Uic(); }

    // -- Log info
    void LogInfo(GeneratorT genType, std::string const& message) const;
    // -- Log warning
    void LogWarning(GeneratorT genType, std::string const& message) const;
    void LogFileWarning(GeneratorT genType, std::string const& filename,
                        std::string const& message) const;
    // -- Log error
    void LogError(GeneratorT genType, std::string const& message) const;
    void LogFileError(GeneratorT genType, std::string const& filename,
                      std::string const& message) const;
    void LogCommandError(GeneratorT genType, std::string const& message,
                         std::vector<std::string> const& command,
                         std::string const& output) const;

    // -- External processes
    /// @brief Verbose logging version
    bool RunProcess(GeneratorT genType, ProcessResultT& result,
                    std::vector<std::string> const& command);

  private:
    /// @brief Thread main loop
    void Loop();

    // -- Libuv callbacks
    static void UVProcessStart(uv_async_t* handle);
    void UVProcessFinished();

  private:
    // -- Generator
    cmQtAutoGeneratorMocUic* Gen_;
    // -- Job handle
    JobHandleT JobHandle_;
    // -- Process management
    std::mutex ProcessMutex_;
    cm::uv_async_ptr ProcessRequest_;
    std::condition_variable ProcessCondition_;
    std::unique_ptr<ReadOnlyProcessT> Process_;
    // -- System thread
    std::thread Thread_;
  };

  /// @brief Processing stage
  enum class StageT
  {
    SETTINGS_READ,
    CREATE_DIRECTORIES,
    PARSE_SOURCES,
    PARSE_HEADERS,
    MOC_PREDEFS,
    MOC_PROCESS,
    MOCS_COMPILATION,
    UIC_PROCESS,
    SETTINGS_WRITE,
    FINISH,
    END
  };

  // -- Const settings interface
  const BaseSettingsT& Base() const { return this->Base_; }
  const MocSettingsT& Moc() const { return this->Moc_; }
  const UicSettingsT& Uic() const { return this->Uic_; }

  // -- Worker thread interface
  void WorkerSwapJob(JobHandleT& jobHandle);
  // -- Parallel job processing interface
  void ParallelRegisterJobError();
  bool ParallelJobPushMoc(JobHandleT& jobHandle);
  bool ParallelJobPushUic(JobHandleT& jobHandle);
  bool ParallelMocIncluded(std::string const& sourceFile);
  void ParallelMocAutoRegister(std::string const& mocFile);
  void ParallelMocAutoUpdated();

private:
  // -- Abstract processing interface
  bool Init(cmMakefile* makefile) override;
  bool Process() override;
  // -- Process stage
  static void UVPollStage(uv_async_t* handle);
  void PollStage();
  void SetStage(StageT stage);
  // -- Settings file
  void SettingsFileRead();
  void SettingsFileWrite();
  // -- Thread processing
  bool ThreadsStartJobs(JobQueueT& queue);
  bool ThreadsJobsDone();
  void ThreadsStop();
  void RegisterJobError();
  // -- Generation
  void CreateDirectories();
  void MocGenerateCompilation();

private:
  // -- Settings
  BaseSettingsT Base_;
  MocSettingsT Moc_;
  UicSettingsT Uic_;
  // -- Progress
  StageT Stage_;
  // -- Job queues
  std::mutex JobsMutex_;
  struct
  {
    JobQueueT Sources;
    JobQueueT Headers;
    JobQueueT MocPredefs;
    JobQueueT Moc;
    JobQueueT Uic;
  } JobQueues_;
  JobQueueT JobQueue_;
  std::size_t volatile JobsRemain_;
  bool volatile JobError_;
  bool volatile JobThreadsAbort_;
  std::condition_variable JobsConditionRead_;
  // -- Moc meta
  std::set<std::string> MocIncludedStrings_;
  std::set<std::string> MocIncludedFiles_;
  std::set<std::string> MocAutoFiles_;
  bool volatile MocAutoFileUpdated_;
  // -- Settings file
  std::string SettingsFile_;
  std::string SettingsStringMoc_;
  std::string SettingsStringUic_;
  // -- Threads and loops
  std::vector<std::unique_ptr<WorkerT>> Workers_;
};

#endif
