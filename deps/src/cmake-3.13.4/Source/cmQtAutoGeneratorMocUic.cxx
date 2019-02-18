/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmQtAutoGeneratorMocUic.h"
#include "cmQtAutoGen.h"

#include <algorithm>
#include <array>
#include <functional>
#include <list>
#include <memory>
#include <sstream>
#include <utility>

#include "cmAlgorithms.h"
#include "cmCryptoHash.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cmake.h"

#if defined(__APPLE__)
#  include <unistd.h>
#endif

// -- Class methods

std::string cmQtAutoGeneratorMocUic::BaseSettingsT::AbsoluteBuildPath(
  std::string const& relativePath) const
{
  return FileSys->CollapseCombinedPath(AutogenBuildDir, relativePath);
}

/**
 * @brief Tries to find the header file to the given file base path by
 * appending different header extensions
 * @return True on success
 */
bool cmQtAutoGeneratorMocUic::BaseSettingsT::FindHeader(
  std::string& header, std::string const& testBasePath) const
{
  for (std::string const& ext : HeaderExtensions) {
    std::string testFilePath(testBasePath);
    testFilePath.push_back('.');
    testFilePath += ext;
    if (FileSys->FileExists(testFilePath)) {
      header = testFilePath;
      return true;
    }
  }
  return false;
}

bool cmQtAutoGeneratorMocUic::MocSettingsT::skipped(
  std::string const& fileName) const
{
  return (!Enabled || (SkipList.find(fileName) != SkipList.end()));
}

/**
 * @brief Returns the first relevant Qt macro name found in the given C++ code
 * @return The name of the Qt macro or an empty string
 */
std::string cmQtAutoGeneratorMocUic::MocSettingsT::FindMacro(
  std::string const& content) const
{
  for (KeyExpT const& filter : MacroFilters) {
    // Run a simple find string operation before the expensive
    // regular expression check
    if (content.find(filter.Key) != std::string::npos) {
      cmsys::RegularExpressionMatch match;
      if (filter.Exp.find(content.c_str(), match)) {
        // Return macro name on demand
        return filter.Key;
      }
    }
  }
  return std::string();
}

std::string cmQtAutoGeneratorMocUic::MocSettingsT::MacrosString() const
{
  std::string res;
  const auto itB = MacroFilters.cbegin();
  const auto itE = MacroFilters.cend();
  const auto itL = itE - 1;
  auto itC = itB;
  for (; itC != itE; ++itC) {
    // Separator
    if (itC != itB) {
      if (itC != itL) {
        res += ", ";
      } else {
        res += " or ";
      }
    }
    // Key
    res += itC->Key;
  }
  return res;
}

std::string cmQtAutoGeneratorMocUic::MocSettingsT::FindIncludedFile(
  std::string const& sourcePath, std::string const& includeString) const
{
  // Search in vicinity of the source
  {
    std::string testPath = sourcePath;
    testPath += includeString;
    if (FileSys->FileExists(testPath)) {
      return FileSys->GetRealPath(testPath);
    }
  }
  // Search in include directories
  for (std::string const& path : IncludePaths) {
    std::string fullPath = path;
    fullPath.push_back('/');
    fullPath += includeString;
    if (FileSys->FileExists(fullPath)) {
      return FileSys->GetRealPath(fullPath);
    }
  }
  // Return empty string
  return std::string();
}

void cmQtAutoGeneratorMocUic::MocSettingsT::FindDependencies(
  std::string const& content, std::set<std::string>& depends) const
{
  if (!DependFilters.empty() && !content.empty()) {
    for (KeyExpT const& filter : DependFilters) {
      // Run a simple find string check
      if (content.find(filter.Key) != std::string::npos) {
        // Run the expensive regular expression check loop
        const char* contentChars = content.c_str();
        cmsys::RegularExpressionMatch match;
        while (filter.Exp.find(contentChars, match)) {
          {
            std::string dep = match.match(1);
            if (!dep.empty()) {
              depends.emplace(std::move(dep));
            }
          }
          contentChars += match.end();
        }
      }
    }
  }
}

bool cmQtAutoGeneratorMocUic::UicSettingsT::skipped(
  std::string const& fileName) const
{
  return (!Enabled || (SkipList.find(fileName) != SkipList.end()));
}

void cmQtAutoGeneratorMocUic::JobParseT::Process(WorkerT& wrk)
{
  if (AutoMoc && Header) {
    // Don't parse header for moc if the file is included by a source already
    if (wrk.Gen().ParallelMocIncluded(FileName)) {
      AutoMoc = false;
    }
  }

  if (AutoMoc || AutoUic) {
    std::string error;
    MetaT meta;
    if (wrk.FileSys().FileRead(meta.Content, FileName, &error)) {
      if (!meta.Content.empty()) {
        meta.FileDir = wrk.FileSys().SubDirPrefix(FileName);
        meta.FileBase =
          wrk.FileSys().GetFilenameWithoutLastExtension(FileName);

        bool success = true;
        if (AutoMoc) {
          if (Header) {
            success = ParseMocHeader(wrk, meta);
          } else {
            success = ParseMocSource(wrk, meta);
          }
        }
        if (AutoUic && success) {
          ParseUic(wrk, meta);
        }
      } else {
        wrk.LogFileWarning(GeneratorT::GEN, FileName,
                           "The source file is empty");
      }
    } else {
      wrk.LogFileError(GeneratorT::GEN, FileName,
                       "Could not read the file: " + error);
    }
  }
}

bool cmQtAutoGeneratorMocUic::JobParseT::ParseMocSource(WorkerT& wrk,
                                                        MetaT const& meta)
{
  struct JobPre
  {
    bool self;       // source file is self
    bool underscore; // "moc_" style include
    std::string SourceFile;
    std::string IncludeString;
  };

  struct MocInclude
  {
    std::string Inc;  // full include string
    std::string Dir;  // include string directory
    std::string Base; // include string file base
  };

  // Check if this source file contains a relevant macro
  std::string const ownMacro = wrk.Moc().FindMacro(meta.Content);

  // Extract moc includes from file
  std::deque<MocInclude> mocIncsUsc;
  std::deque<MocInclude> mocIncsDot;
  {
    if (meta.Content.find("moc") != std::string::npos) {
      const char* contentChars = meta.Content.c_str();
      cmsys::RegularExpressionMatch match;
      while (wrk.Moc().RegExpInclude.find(contentChars, match)) {
        std::string incString = match.match(2);
        std::string incDir(wrk.FileSys().SubDirPrefix(incString));
        std::string incBase =
          wrk.FileSys().GetFilenameWithoutLastExtension(incString);
        if (cmHasLiteralPrefix(incBase, "moc_")) {
          // moc_<BASE>.cxx
          // Remove the moc_ part from the base name
          mocIncsUsc.emplace_back(MocInclude{
            std::move(incString), std::move(incDir), incBase.substr(4) });
        } else {
          // <BASE>.moc
          mocIncsDot.emplace_back(MocInclude{
            std::move(incString), std::move(incDir), std::move(incBase) });
        }
        // Forward content pointer
        contentChars += match.end();
      }
    }
  }

  // Check if there is anything to do
  if (ownMacro.empty() && mocIncsUsc.empty() && mocIncsDot.empty()) {
    return true;
  }

  bool ownDotMocIncluded = false;
  bool ownMocUscIncluded = false;
  std::deque<JobPre> jobs;

  // Process moc_<BASE>.cxx includes
  for (const MocInclude& mocInc : mocIncsUsc) {
    std::string const header =
      MocFindIncludedHeader(wrk, meta.FileDir, mocInc.Dir + mocInc.Base);
    if (!header.empty()) {
      // Check if header is skipped
      if (wrk.Moc().skipped(header)) {
        continue;
      }
      // Register moc job
      const bool ownMoc = (mocInc.Base == meta.FileBase);
      jobs.emplace_back(JobPre{ ownMoc, true, header, mocInc.Inc });
      // Store meta information for relaxed mode
      if (ownMoc) {
        ownMocUscIncluded = true;
      }
    } else {
      {
        std::string emsg = "The file includes the moc file ";
        emsg += Quoted(mocInc.Inc);
        emsg += ", but the header ";
        emsg += Quoted(MocStringHeaders(wrk, mocInc.Base));
        emsg += " could not be found.";
        wrk.LogFileError(GeneratorT::MOC, FileName, emsg);
      }
      return false;
    }
  }

  // Process <BASE>.moc includes
  for (const MocInclude& mocInc : mocIncsDot) {
    const bool ownMoc = (mocInc.Base == meta.FileBase);
    if (wrk.Moc().RelaxedMode) {
      // Relaxed mode
      if (!ownMacro.empty() && ownMoc) {
        // Add self
        jobs.emplace_back(JobPre{ ownMoc, false, FileName, mocInc.Inc });
        ownDotMocIncluded = true;
      } else {
        // In relaxed mode try to find a header instead but issue a warning.
        // This is for KDE4 compatibility
        std::string const header =
          MocFindIncludedHeader(wrk, meta.FileDir, mocInc.Dir + mocInc.Base);
        if (!header.empty()) {
          // Check if header is skipped
          if (wrk.Moc().skipped(header)) {
            continue;
          }
          // Register moc job
          jobs.emplace_back(JobPre{ ownMoc, false, header, mocInc.Inc });
          if (ownMacro.empty()) {
            if (ownMoc) {
              std::string emsg = "The file includes the moc file ";
              emsg += Quoted(mocInc.Inc);
              emsg += ", but does not contain a ";
              emsg += wrk.Moc().MacrosString();
              emsg += " macro.\nRunning moc on\n  ";
              emsg += Quoted(header);
              emsg += "!\nBetter include ";
              emsg += Quoted("moc_" + mocInc.Base + ".cpp");
              emsg += " for a compatibility with strict mode.\n"
                      "(CMAKE_AUTOMOC_RELAXED_MODE warning)\n";
              wrk.LogFileWarning(GeneratorT::MOC, FileName, emsg);
            } else {
              std::string emsg = "The file includes the moc file ";
              emsg += Quoted(mocInc.Inc);
              emsg += " instead of ";
              emsg += Quoted("moc_" + mocInc.Base + ".cpp");
              emsg += ".\nRunning moc on\n  ";
              emsg += Quoted(header);
              emsg += "!\nBetter include ";
              emsg += Quoted("moc_" + mocInc.Base + ".cpp");
              emsg += " for compatibility with strict mode.\n"
                      "(CMAKE_AUTOMOC_RELAXED_MODE warning)\n";
              wrk.LogFileWarning(GeneratorT::MOC, FileName, emsg);
            }
          }
        } else {
          {
            std::string emsg = "The file includes the moc file ";
            emsg += Quoted(mocInc.Inc);
            emsg += ", which seems to be the moc file from a different "
                    "source file.\nCMAKE_AUTOMOC_RELAXED_MODE: Also a "
                    "matching header ";
            emsg += Quoted(MocStringHeaders(wrk, mocInc.Base));
            emsg += " could not be found.";
            wrk.LogFileError(GeneratorT::MOC, FileName, emsg);
          }
          return false;
        }
      }
    } else {
      // Strict mode
      if (ownMoc) {
        // Include self
        jobs.emplace_back(JobPre{ ownMoc, false, FileName, mocInc.Inc });
        ownDotMocIncluded = true;
        // Accept but issue a warning if moc isn't required
        if (ownMacro.empty()) {
          std::string emsg = "The file includes the moc file ";
          emsg += Quoted(mocInc.Inc);
          emsg += ", but does not contain a ";
          emsg += wrk.Moc().MacrosString();
          emsg += " macro.";
          wrk.LogFileWarning(GeneratorT::MOC, FileName, emsg);
        }
      } else {
        // Don't allow <BASE>.moc include other than self in strict mode
        {
          std::string emsg = "The file includes the moc file ";
          emsg += Quoted(mocInc.Inc);
          emsg += ", which seems to be the moc file from a different "
                  "source file.\nThis is not supported. Include ";
          emsg += Quoted(meta.FileBase + ".moc");
          emsg += " to run moc on this source file.";
          wrk.LogFileError(GeneratorT::MOC, FileName, emsg);
        }
        return false;
      }
    }
  }

  if (!ownMacro.empty() && !ownDotMocIncluded) {
    // In this case, check whether the scanned file itself contains a
    // Q_OBJECT.
    // If this is the case, the moc_foo.cpp should probably be generated from
    // foo.cpp instead of foo.h, because otherwise it won't build.
    // But warn, since this is not how it is supposed to be used.
    // This is for KDE4 compatibility.
    if (wrk.Moc().RelaxedMode && ownMocUscIncluded) {
      JobPre uscJobPre;
      // Remove underscore job request
      {
        auto itC = jobs.begin();
        auto itE = jobs.end();
        for (; itC != itE; ++itC) {
          JobPre& job(*itC);
          if (job.self && job.underscore) {
            uscJobPre = std::move(job);
            jobs.erase(itC);
            break;
          }
        }
      }
      // Issue a warning
      {
        std::string emsg = "The file contains a ";
        emsg += ownMacro;
        emsg += " macro, but does not include ";
        emsg += Quoted(meta.FileBase + ".moc");
        emsg += ". Instead it includes ";
        emsg += Quoted(uscJobPre.IncludeString);
        emsg += ".\nRunning moc on\n  ";
        emsg += Quoted(FileName);
        emsg += "!\nBetter include ";
        emsg += Quoted(meta.FileBase + ".moc");
        emsg += " for compatibility with strict mode.\n"
                "(CMAKE_AUTOMOC_RELAXED_MODE warning)";
        wrk.LogFileWarning(GeneratorT::MOC, FileName, emsg);
      }
      // Add own source job
      jobs.emplace_back(
        JobPre{ true, false, FileName, uscJobPre.IncludeString });
    } else {
      // Otherwise always error out since it will not compile.
      {
        std::string emsg = "The file contains a ";
        emsg += ownMacro;
        emsg += " macro, but does not include ";
        emsg += Quoted(meta.FileBase + ".moc");
        emsg += "!\nConsider to\n - add #include \"";
        emsg += meta.FileBase;
        emsg += ".moc\"\n - enable SKIP_AUTOMOC for this file";
        wrk.LogFileError(GeneratorT::MOC, FileName, emsg);
      }
      return false;
    }
  }

  // Convert pre jobs to actual jobs
  for (JobPre& jobPre : jobs) {
    JobHandleT jobHandle(new JobMocT(std::move(jobPre.SourceFile), FileName,
                                     std::move(jobPre.IncludeString)));
    if (jobPre.self) {
      // Read dependencies from this source
      static_cast<JobMocT&>(*jobHandle).FindDependencies(wrk, meta.Content);
    }
    if (!wrk.Gen().ParallelJobPushMoc(jobHandle)) {
      return false;
    }
  }
  return true;
}

bool cmQtAutoGeneratorMocUic::JobParseT::ParseMocHeader(WorkerT& wrk,
                                                        MetaT const& meta)
{
  bool success = true;
  std::string const macroName = wrk.Moc().FindMacro(meta.Content);
  if (!macroName.empty()) {
    JobHandleT jobHandle(
      new JobMocT(std::string(FileName), std::string(), std::string()));
    // Read dependencies from this source
    static_cast<JobMocT&>(*jobHandle).FindDependencies(wrk, meta.Content);
    success = wrk.Gen().ParallelJobPushMoc(jobHandle);
  }
  return success;
}

std::string cmQtAutoGeneratorMocUic::JobParseT::MocStringHeaders(
  WorkerT& wrk, std::string const& fileBase) const
{
  std::string res = fileBase;
  res += ".{";
  res += cmJoin(wrk.Base().HeaderExtensions, ",");
  res += "}";
  return res;
}

std::string cmQtAutoGeneratorMocUic::JobParseT::MocFindIncludedHeader(
  WorkerT& wrk, std::string const& includerDir, std::string const& includeBase)
{
  std::string header;
  // Search in vicinity of the source
  if (!wrk.Base().FindHeader(header, includerDir + includeBase)) {
    // Search in include directories
    for (std::string const& path : wrk.Moc().IncludePaths) {
      std::string fullPath = path;
      fullPath.push_back('/');
      fullPath += includeBase;
      if (wrk.Base().FindHeader(header, fullPath)) {
        break;
      }
    }
  }
  // Sanitize
  if (!header.empty()) {
    header = wrk.FileSys().GetRealPath(header);
  }
  return header;
}

bool cmQtAutoGeneratorMocUic::JobParseT::ParseUic(WorkerT& wrk,
                                                  MetaT const& meta)
{
  bool success = true;
  if (meta.Content.find("ui_") != std::string::npos) {
    const char* contentChars = meta.Content.c_str();
    cmsys::RegularExpressionMatch match;
    while (wrk.Uic().RegExpInclude.find(contentChars, match)) {
      if (!ParseUicInclude(wrk, meta, match.match(2))) {
        success = false;
        break;
      }
      contentChars += match.end();
    }
  }
  return success;
}

bool cmQtAutoGeneratorMocUic::JobParseT::ParseUicInclude(
  WorkerT& wrk, MetaT const& meta, std::string&& includeString)
{
  bool success = false;
  std::string uiInputFile = UicFindIncludedFile(wrk, meta, includeString);
  if (!uiInputFile.empty()) {
    if (!wrk.Uic().skipped(uiInputFile)) {
      JobHandleT jobHandle(new JobUicT(std::move(uiInputFile), FileName,
                                       std::move(includeString)));
      success = wrk.Gen().ParallelJobPushUic(jobHandle);
    } else {
      // A skipped file is successful
      success = true;
    }
  }
  return success;
}

std::string cmQtAutoGeneratorMocUic::JobParseT::UicFindIncludedFile(
  WorkerT& wrk, MetaT const& meta, std::string const& includeString)
{
  std::string res;
  std::string searchFile =
    wrk.FileSys().GetFilenameWithoutLastExtension(includeString).substr(3);
  searchFile += ".ui";
  // Collect search paths list
  std::deque<std::string> testFiles;
  {
    std::string const searchPath = wrk.FileSys().SubDirPrefix(includeString);

    std::string searchFileFull;
    if (!searchPath.empty()) {
      searchFileFull = searchPath;
      searchFileFull += searchFile;
    }
    // Vicinity of the source
    {
      std::string const sourcePath = meta.FileDir;
      testFiles.push_back(sourcePath + searchFile);
      if (!searchPath.empty()) {
        testFiles.push_back(sourcePath + searchFileFull);
      }
    }
    // AUTOUIC search paths
    if (!wrk.Uic().SearchPaths.empty()) {
      for (std::string const& sPath : wrk.Uic().SearchPaths) {
        testFiles.push_back((sPath + "/").append(searchFile));
      }
      if (!searchPath.empty()) {
        for (std::string const& sPath : wrk.Uic().SearchPaths) {
          testFiles.push_back((sPath + "/").append(searchFileFull));
        }
      }
    }
  }

  // Search for the .ui file!
  for (std::string const& testFile : testFiles) {
    if (wrk.FileSys().FileExists(testFile)) {
      res = wrk.FileSys().GetRealPath(testFile);
      break;
    }
  }

  // Log error
  if (res.empty()) {
    std::string emsg = "Could not find ";
    emsg += Quoted(searchFile);
    emsg += " in\n";
    for (std::string const& testFile : testFiles) {
      emsg += "  ";
      emsg += Quoted(testFile);
      emsg += "\n";
    }
    wrk.LogFileError(GeneratorT::UIC, FileName, emsg);
  }

  return res;
}

void cmQtAutoGeneratorMocUic::JobMocPredefsT::Process(WorkerT& wrk)
{
  // (Re)generate moc_predefs.h on demand
  bool generate(false);
  bool fileExists(wrk.FileSys().FileExists(wrk.Moc().PredefsFileAbs));
  if (!fileExists) {
    if (wrk.Log().Verbose()) {
      std::string reason = "Generating ";
      reason += Quoted(wrk.Moc().PredefsFileRel);
      reason += " because it doesn't exist";
      wrk.LogInfo(GeneratorT::MOC, reason);
    }
    generate = true;
  } else if (wrk.Moc().SettingsChanged) {
    if (wrk.Log().Verbose()) {
      std::string reason = "Generating ";
      reason += Quoted(wrk.Moc().PredefsFileRel);
      reason += " because the settings changed.";
      wrk.LogInfo(GeneratorT::MOC, reason);
    }
    generate = true;
  }
  if (generate) {
    ProcessResultT result;
    {
      // Compose command
      std::vector<std::string> cmd = wrk.Moc().PredefsCmd;
      // Add includes
      cmd.insert(cmd.end(), wrk.Moc().Includes.begin(),
                 wrk.Moc().Includes.end());
      // Add definitions
      for (std::string const& def : wrk.Moc().Definitions) {
        cmd.push_back("-D" + def);
      }
      // Execute command
      if (!wrk.RunProcess(GeneratorT::MOC, result, cmd)) {
        std::string emsg = "The content generation command for ";
        emsg += Quoted(wrk.Moc().PredefsFileRel);
        emsg += " failed.\n";
        emsg += result.ErrorMessage;
        wrk.LogCommandError(GeneratorT::MOC, emsg, cmd, result.StdOut);
      }
    }

    // (Re)write predefs file only on demand
    if (!result.error()) {
      if (!fileExists ||
          wrk.FileSys().FileDiffers(wrk.Moc().PredefsFileAbs, result.StdOut)) {
        if (wrk.FileSys().FileWrite(GeneratorT::MOC, wrk.Moc().PredefsFileAbs,
                                    result.StdOut)) {
          // Success
        } else {
          std::string emsg = "Writing ";
          emsg += Quoted(wrk.Moc().PredefsFileRel);
          emsg += " failed.";
          wrk.LogFileError(GeneratorT::MOC, wrk.Moc().PredefsFileAbs, emsg);
        }
      } else {
        // Touch to update the time stamp
        if (wrk.Log().Verbose()) {
          std::string msg = "Touching ";
          msg += Quoted(wrk.Moc().PredefsFileRel);
          msg += ".";
          wrk.LogInfo(GeneratorT::MOC, msg);
        }
        wrk.FileSys().Touch(wrk.Moc().PredefsFileAbs);
      }
    }
  }
}

void cmQtAutoGeneratorMocUic::JobMocT::FindDependencies(
  WorkerT& wrk, std::string const& content)
{
  wrk.Moc().FindDependencies(content, Depends);
  DependsValid = true;
}

void cmQtAutoGeneratorMocUic::JobMocT::Process(WorkerT& wrk)
{
  // Compute build file name
  if (!IncludeString.empty()) {
    BuildFile = wrk.Base().AutogenIncludeDir;
    BuildFile += '/';
    BuildFile += IncludeString;
  } else {
    std::string rel = wrk.FileSys().GetFilePathChecksum(SourceFile);
    rel += "/moc_";
    rel += wrk.FileSys().GetFilenameWithoutLastExtension(SourceFile);
    rel += ".cpp";
    // Register relative file path
    wrk.Gen().ParallelMocAutoRegister(rel);
    // Absolute build path
    if (wrk.Base().MultiConfig) {
      BuildFile = wrk.Base().AutogenIncludeDir;
      BuildFile += '/';
      BuildFile += rel;
    } else {
      BuildFile = wrk.Base().AbsoluteBuildPath(rel);
    }
  }

  if (UpdateRequired(wrk)) {
    GenerateMoc(wrk);
  }
}

bool cmQtAutoGeneratorMocUic::JobMocT::UpdateRequired(WorkerT& wrk)
{
  bool const verbose = wrk.Gen().Log().Verbose();

  // Test if the build file exists
  if (!wrk.FileSys().FileExists(BuildFile)) {
    if (verbose) {
      std::string reason = "Generating ";
      reason += Quoted(BuildFile);
      reason += " from its source file ";
      reason += Quoted(SourceFile);
      reason += " because it doesn't exist";
      wrk.LogInfo(GeneratorT::MOC, reason);
    }
    return true;
  }

  // Test if any setting changed
  if (wrk.Moc().SettingsChanged) {
    if (verbose) {
      std::string reason = "Generating ";
      reason += Quoted(BuildFile);
      reason += " from ";
      reason += Quoted(SourceFile);
      reason += " because the MOC settings changed";
      wrk.LogInfo(GeneratorT::MOC, reason);
    }
    return true;
  }

  // Test if the moc_predefs file is newer
  if (!wrk.Moc().PredefsFileAbs.empty()) {
    bool isOlder = false;
    {
      std::string error;
      isOlder = wrk.FileSys().FileIsOlderThan(
        BuildFile, wrk.Moc().PredefsFileAbs, &error);
      if (!isOlder && !error.empty()) {
        wrk.LogError(GeneratorT::MOC, error);
        return false;
      }
    }
    if (isOlder) {
      if (verbose) {
        std::string reason = "Generating ";
        reason += Quoted(BuildFile);
        reason += " because it's older than: ";
        reason += Quoted(wrk.Moc().PredefsFileAbs);
        wrk.LogInfo(GeneratorT::MOC, reason);
      }
      return true;
    }
  }

  // Test if the source file is newer
  {
    bool isOlder = false;
    {
      std::string error;
      isOlder = wrk.FileSys().FileIsOlderThan(BuildFile, SourceFile, &error);
      if (!isOlder && !error.empty()) {
        wrk.LogError(GeneratorT::MOC, error);
        return false;
      }
    }
    if (isOlder) {
      if (verbose) {
        std::string reason = "Generating ";
        reason += Quoted(BuildFile);
        reason += " because it's older than its source file ";
        reason += Quoted(SourceFile);
        wrk.LogInfo(GeneratorT::MOC, reason);
      }
      return true;
    }
  }

  // Test if a dependency file is newer
  {
    // Read dependencies on demand
    if (!DependsValid) {
      std::string content;
      {
        std::string error;
        if (!wrk.FileSys().FileRead(content, SourceFile, &error)) {
          std::string emsg = "Could not read file\n  ";
          emsg += Quoted(SourceFile);
          emsg += "\nrequired by moc include ";
          emsg += Quoted(IncludeString);
          emsg += " in\n  ";
          emsg += Quoted(IncluderFile);
          emsg += ".\n";
          emsg += error;
          wrk.LogError(GeneratorT::MOC, emsg);
          return false;
        }
      }
      FindDependencies(wrk, content);
    }
    // Check dependency timestamps
    std::string error;
    std::string sourceDir = wrk.FileSys().SubDirPrefix(SourceFile);
    for (std::string const& depFileRel : Depends) {
      std::string depFileAbs =
        wrk.Moc().FindIncludedFile(sourceDir, depFileRel);
      if (!depFileAbs.empty()) {
        if (wrk.FileSys().FileIsOlderThan(BuildFile, depFileAbs, &error)) {
          if (verbose) {
            std::string reason = "Generating ";
            reason += Quoted(BuildFile);
            reason += " from ";
            reason += Quoted(SourceFile);
            reason += " because it is older than it's dependency file ";
            reason += Quoted(depFileAbs);
            wrk.LogInfo(GeneratorT::MOC, reason);
          }
          return true;
        }
        if (!error.empty()) {
          wrk.LogError(GeneratorT::MOC, error);
          return false;
        }
      } else {
        std::string message = "Could not find dependency file ";
        message += Quoted(depFileRel);
        wrk.LogFileWarning(GeneratorT::MOC, SourceFile, message);
      }
    }
  }

  return false;
}

void cmQtAutoGeneratorMocUic::JobMocT::GenerateMoc(WorkerT& wrk)
{
  // Make sure the parent directory exists
  if (wrk.FileSys().MakeParentDirectory(GeneratorT::MOC, BuildFile)) {
    // Compose moc command
    std::vector<std::string> cmd;
    cmd.push_back(wrk.Moc().Executable);
    // Add options
    cmd.insert(cmd.end(), wrk.Moc().AllOptions.begin(),
               wrk.Moc().AllOptions.end());
    // Add predefs include
    if (!wrk.Moc().PredefsFileAbs.empty()) {
      cmd.push_back("--include");
      cmd.push_back(wrk.Moc().PredefsFileAbs);
    }
    cmd.push_back("-o");
    cmd.push_back(BuildFile);
    cmd.push_back(SourceFile);

    // Execute moc command
    ProcessResultT result;
    if (wrk.RunProcess(GeneratorT::MOC, result, cmd)) {
      // Moc command success
      // Print moc output
      if (!result.StdOut.empty()) {
        wrk.LogInfo(GeneratorT::MOC, result.StdOut);
      }
      // Notify the generator that a not included file changed (on demand)
      if (IncludeString.empty()) {
        wrk.Gen().ParallelMocAutoUpdated();
      }
    } else {
      // Moc command failed
      {
        std::string emsg = "The moc process failed to compile\n  ";
        emsg += Quoted(SourceFile);
        emsg += "\ninto\n  ";
        emsg += Quoted(BuildFile);
        emsg += ".\n";
        emsg += result.ErrorMessage;
        wrk.LogCommandError(GeneratorT::MOC, emsg, cmd, result.StdOut);
      }
      wrk.FileSys().FileRemove(BuildFile);
    }
  }
}

void cmQtAutoGeneratorMocUic::JobUicT::Process(WorkerT& wrk)
{
  // Compute build file name
  BuildFile = wrk.Base().AutogenIncludeDir;
  BuildFile += '/';
  BuildFile += IncludeString;

  if (UpdateRequired(wrk)) {
    GenerateUic(wrk);
  }
}

bool cmQtAutoGeneratorMocUic::JobUicT::UpdateRequired(WorkerT& wrk)
{
  bool const verbose = wrk.Gen().Log().Verbose();

  // Test if the build file exists
  if (!wrk.FileSys().FileExists(BuildFile)) {
    if (verbose) {
      std::string reason = "Generating ";
      reason += Quoted(BuildFile);
      reason += " from its source file ";
      reason += Quoted(SourceFile);
      reason += " because it doesn't exist";
      wrk.LogInfo(GeneratorT::UIC, reason);
    }
    return true;
  }

  // Test if the uic settings changed
  if (wrk.Uic().SettingsChanged) {
    if (verbose) {
      std::string reason = "Generating ";
      reason += Quoted(BuildFile);
      reason += " from ";
      reason += Quoted(SourceFile);
      reason += " because the UIC settings changed";
      wrk.LogInfo(GeneratorT::UIC, reason);
    }
    return true;
  }

  // Test if the source file is newer
  {
    bool isOlder = false;
    {
      std::string error;
      isOlder = wrk.FileSys().FileIsOlderThan(BuildFile, SourceFile, &error);
      if (!isOlder && !error.empty()) {
        wrk.LogError(GeneratorT::UIC, error);
        return false;
      }
    }
    if (isOlder) {
      if (verbose) {
        std::string reason = "Generating ";
        reason += Quoted(BuildFile);
        reason += " because it's older than its source file ";
        reason += Quoted(SourceFile);
        wrk.LogInfo(GeneratorT::UIC, reason);
      }
      return true;
    }
  }

  return false;
}

void cmQtAutoGeneratorMocUic::JobUicT::GenerateUic(WorkerT& wrk)
{
  // Make sure the parent directory exists
  if (wrk.FileSys().MakeParentDirectory(GeneratorT::UIC, BuildFile)) {
    // Compose uic command
    std::vector<std::string> cmd;
    cmd.push_back(wrk.Uic().Executable);
    {
      std::vector<std::string> allOpts = wrk.Uic().TargetOptions;
      auto optionIt = wrk.Uic().Options.find(SourceFile);
      if (optionIt != wrk.Uic().Options.end()) {
        UicMergeOptions(allOpts, optionIt->second,
                        (wrk.Base().QtVersionMajor == 5));
      }
      cmd.insert(cmd.end(), allOpts.begin(), allOpts.end());
    }
    cmd.push_back("-o");
    cmd.push_back(BuildFile);
    cmd.push_back(SourceFile);

    ProcessResultT result;
    if (wrk.RunProcess(GeneratorT::UIC, result, cmd)) {
      // Uic command success
      // Print uic output
      if (!result.StdOut.empty()) {
        wrk.LogInfo(GeneratorT::UIC, result.StdOut);
      }
    } else {
      // Uic command failed
      {
        std::string emsg = "The uic process failed to compile\n  ";
        emsg += Quoted(SourceFile);
        emsg += "\ninto\n  ";
        emsg += Quoted(BuildFile);
        emsg += "\nincluded by\n  ";
        emsg += Quoted(IncluderFile);
        emsg += ".\n";
        emsg += result.ErrorMessage;
        wrk.LogCommandError(GeneratorT::UIC, emsg, cmd, result.StdOut);
      }
      wrk.FileSys().FileRemove(BuildFile);
    }
  }
}

void cmQtAutoGeneratorMocUic::JobDeleterT::operator()(JobT* job)
{
  delete job;
}

cmQtAutoGeneratorMocUic::WorkerT::WorkerT(cmQtAutoGeneratorMocUic* gen,
                                          uv_loop_t* uvLoop)
  : Gen_(gen)
{
  // Initialize uv asynchronous callback for process starting
  ProcessRequest_.init(*uvLoop, &WorkerT::UVProcessStart, this);
  // Start thread
  Thread_ = std::thread(&WorkerT::Loop, this);
}

cmQtAutoGeneratorMocUic::WorkerT::~WorkerT()
{
  // Join thread
  if (Thread_.joinable()) {
    Thread_.join();
  }
}

void cmQtAutoGeneratorMocUic::WorkerT::LogInfo(
  GeneratorT genType, std::string const& message) const
{
  return Log().Info(genType, message);
}

void cmQtAutoGeneratorMocUic::WorkerT::LogWarning(
  GeneratorT genType, std::string const& message) const
{
  return Log().Warning(genType, message);
}

void cmQtAutoGeneratorMocUic::WorkerT::LogFileWarning(
  GeneratorT genType, std::string const& filename,
  std::string const& message) const
{
  return Log().WarningFile(genType, filename, message);
}

void cmQtAutoGeneratorMocUic::WorkerT::LogError(
  GeneratorT genType, std::string const& message) const
{
  Gen().ParallelRegisterJobError();
  Log().Error(genType, message);
}

void cmQtAutoGeneratorMocUic::WorkerT::LogFileError(
  GeneratorT genType, std::string const& filename,
  std::string const& message) const
{
  Gen().ParallelRegisterJobError();
  Log().ErrorFile(genType, filename, message);
}

void cmQtAutoGeneratorMocUic::WorkerT::LogCommandError(
  GeneratorT genType, std::string const& message,
  std::vector<std::string> const& command, std::string const& output) const
{
  Gen().ParallelRegisterJobError();
  Log().ErrorCommand(genType, message, command, output);
}

bool cmQtAutoGeneratorMocUic::WorkerT::RunProcess(
  GeneratorT genType, ProcessResultT& result,
  std::vector<std::string> const& command)
{
  if (command.empty()) {
    return false;
  }

  // Create process instance
  {
    std::lock_guard<std::mutex> lock(ProcessMutex_);
    Process_ = cm::make_unique<ReadOnlyProcessT>();
    Process_->setup(&result, true, command, Gen().Base().AutogenBuildDir);
  }

  // Send asynchronous process start request to libuv loop
  ProcessRequest_.send();

  // Log command
  if (this->Log().Verbose()) {
    std::string msg = "Running command:\n";
    msg += QuotedCommand(command);
    msg += '\n';
    this->LogInfo(genType, msg);
  }

  // Wait until the process has been finished and destroyed
  {
    std::unique_lock<std::mutex> ulock(ProcessMutex_);
    while (Process_) {
      ProcessCondition_.wait(ulock);
    }
  }
  return !result.error();
}

void cmQtAutoGeneratorMocUic::WorkerT::Loop()
{
  while (true) {
    Gen().WorkerSwapJob(JobHandle_);
    if (JobHandle_) {
      JobHandle_->Process(*this);
    } else {
      break;
    }
  }
}

void cmQtAutoGeneratorMocUic::WorkerT::UVProcessStart(uv_async_t* handle)
{
  auto& wrk = *reinterpret_cast<WorkerT*>(handle->data);
  {
    std::lock_guard<std::mutex> lock(wrk.ProcessMutex_);
    if (wrk.Process_ && !wrk.Process_->IsStarted()) {
      wrk.Process_->start(handle->loop,
                          std::bind(&WorkerT::UVProcessFinished, &wrk));
    }
  }
}

void cmQtAutoGeneratorMocUic::WorkerT::UVProcessFinished()
{
  {
    std::lock_guard<std::mutex> lock(ProcessMutex_);
    if (Process_ && Process_->IsFinished()) {
      Process_.reset();
    }
  }
  // Notify idling thread
  ProcessCondition_.notify_one();
}

cmQtAutoGeneratorMocUic::cmQtAutoGeneratorMocUic()
  : Base_(&FileSys())
  , Moc_(&FileSys())
  , Stage_(StageT::SETTINGS_READ)
  , JobsRemain_(0)
  , JobError_(false)
  , JobThreadsAbort_(false)
  , MocAutoFileUpdated_(false)
{
  // Precompile regular expressions
  Moc_.RegExpInclude.compile(
    "(^|\n)[ \t]*#[ \t]*include[ \t]+"
    "[\"<](([^ \">]+/)?moc_[^ \">/]+\\.cpp|[^ \">]+\\.moc)[\">]");
  Uic_.RegExpInclude.compile("(^|\n)[ \t]*#[ \t]*include[ \t]+"
                             "[\"<](([^ \">]+/)?ui_[^ \">/]+\\.h)[\">]");

  // Initialize libuv asynchronous iteration request
  UVRequest().init(*UVLoop(), &cmQtAutoGeneratorMocUic::UVPollStage, this);
}

cmQtAutoGeneratorMocUic::~cmQtAutoGeneratorMocUic()
{
}

bool cmQtAutoGeneratorMocUic::Init(cmMakefile* makefile)
{
  // -- Meta
  Base_.HeaderExtensions = makefile->GetCMakeInstance()->GetHeaderExtensions();

  // Utility lambdas
  auto InfoGet = [makefile](const char* key) {
    return makefile->GetSafeDefinition(key);
  };
  auto InfoGetBool = [makefile](const char* key) {
    return makefile->IsOn(key);
  };
  auto InfoGetList = [makefile](const char* key) -> std::vector<std::string> {
    std::vector<std::string> list;
    cmSystemTools::ExpandListArgument(makefile->GetSafeDefinition(key), list);
    return list;
  };
  auto InfoGetLists =
    [makefile](const char* key) -> std::vector<std::vector<std::string>> {
    std::vector<std::vector<std::string>> lists;
    {
      std::string const value = makefile->GetSafeDefinition(key);
      std::string::size_type pos = 0;
      while (pos < value.size()) {
        std::string::size_type next = value.find(ListSep, pos);
        std::string::size_type length =
          (next != std::string::npos) ? next - pos : value.size() - pos;
        // Remove enclosing braces
        if (length >= 2) {
          std::string::const_iterator itBeg = value.begin() + (pos + 1);
          std::string::const_iterator itEnd = itBeg + (length - 2);
          {
            std::string subValue(itBeg, itEnd);
            std::vector<std::string> list;
            cmSystemTools::ExpandListArgument(subValue, list);
            lists.push_back(std::move(list));
          }
        }
        pos += length;
        pos += ListSep.size();
      }
    }
    return lists;
  };
  auto InfoGetConfig = [makefile, this](const char* key) -> std::string {
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
    [&InfoGetConfig](const char* key) -> std::vector<std::string> {
    std::vector<std::string> list;
    cmSystemTools::ExpandListArgument(InfoGetConfig(key), list);
    return list;
  };

  // -- Read info file
  if (!makefile->ReadListFile(InfoFile().c_str())) {
    Log().ErrorFile(GeneratorT::GEN, InfoFile(), "File processing failed");
    return false;
  }

  // -- Meta
  Log().RaiseVerbosity(InfoGet("AM_VERBOSITY"));
  Base_.MultiConfig = InfoGetBool("AM_MULTI_CONFIG");
  {
    unsigned long num = Base_.NumThreads;
    if (cmSystemTools::StringToULong(InfoGet("AM_PARALLEL").c_str(), &num)) {
      num = std::max<unsigned long>(num, 1);
      num = std::min<unsigned long>(num, ParallelMax);
      Base_.NumThreads = static_cast<unsigned int>(num);
    }
  }

  // - Files and directories
  Base_.ProjectSourceDir = InfoGet("AM_CMAKE_SOURCE_DIR");
  Base_.ProjectBinaryDir = InfoGet("AM_CMAKE_BINARY_DIR");
  Base_.CurrentSourceDir = InfoGet("AM_CMAKE_CURRENT_SOURCE_DIR");
  Base_.CurrentBinaryDir = InfoGet("AM_CMAKE_CURRENT_BINARY_DIR");
  Base_.IncludeProjectDirsBefore =
    InfoGetBool("AM_CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE");
  Base_.AutogenBuildDir = InfoGet("AM_BUILD_DIR");
  if (Base_.AutogenBuildDir.empty()) {
    Log().ErrorFile(GeneratorT::GEN, InfoFile(),
                    "Autogen build directory missing");
    return false;
  }
  // include directory
  Base_.AutogenIncludeDir = InfoGetConfig("AM_INCLUDE_DIR");
  if (Base_.AutogenIncludeDir.empty()) {
    Log().ErrorFile(GeneratorT::GEN, InfoFile(),
                    "Autogen include directory missing");
    return false;
  }

  // - Files
  SettingsFile_ = InfoGetConfig("AM_SETTINGS_FILE");
  if (SettingsFile_.empty()) {
    Log().ErrorFile(GeneratorT::GEN, InfoFile(), "Settings file name missing");
    return false;
  }

  // - Qt environment
  {
    unsigned long qtv = Base_.QtVersionMajor;
    if (cmSystemTools::StringToULong(InfoGet("AM_QT_VERSION_MAJOR").c_str(),
                                     &qtv)) {
      Base_.QtVersionMajor = static_cast<unsigned int>(qtv);
    }
  }

  // - Moc
  Moc_.Executable = InfoGet("AM_QT_MOC_EXECUTABLE");
  Moc_.Enabled = !Moc().Executable.empty();
  if (Moc().Enabled) {
    {
      auto lst = InfoGetList("AM_MOC_SKIP");
      Moc_.SkipList.insert(lst.begin(), lst.end());
    }
    Moc_.Definitions = InfoGetConfigList("AM_MOC_DEFINITIONS");
#ifdef _WIN32
    {
      std::string win32("WIN32");
      auto itB = Moc().Definitions.cbegin();
      auto itE = Moc().Definitions.cend();
      if (std::find(itB, itE, win32) == itE) {
        Moc_.Definitions.emplace_back(std::move(win32));
      }
    }
#endif
    Moc_.IncludePaths = InfoGetConfigList("AM_MOC_INCLUDES");
    Moc_.Options = InfoGetList("AM_MOC_OPTIONS");
    Moc_.RelaxedMode = InfoGetBool("AM_MOC_RELAXED_MODE");
    for (std::string const& item : InfoGetList("AM_MOC_MACRO_NAMES")) {
      Moc_.MacroFilters.emplace_back(
        item, ("[\n][ \t]*{?[ \t]*" + item).append("[^a-zA-Z0-9_]"));
    }
    {
      auto pushFilter = [this](std::string const& key, std::string const& exp,
                               std::string& error) {
        if (!key.empty()) {
          if (!exp.empty()) {
            Moc_.DependFilters.push_back(KeyExpT());
            KeyExpT& filter(Moc_.DependFilters.back());
            if (filter.Exp.compile(exp)) {
              filter.Key = key;
            } else {
              error = "Regular expression compiling failed";
            }
          } else {
            error = "Regular expression is empty";
          }
        } else {
          error = "Key is empty";
        }
        if (!error.empty()) {
          error = ("AUTOMOC_DEPEND_FILTERS: " + error);
          error += "\n";
          error += "  Key: ";
          error += Quoted(key);
          error += "\n";
          error += "  Exp: ";
          error += Quoted(exp);
          error += "\n";
        }
      };

      std::string error;
      // Insert default filter for Q_PLUGIN_METADATA
      if (Base().QtVersionMajor != 4) {
        pushFilter("Q_PLUGIN_METADATA",
                   "[\n][ \t]*Q_PLUGIN_METADATA[ \t]*\\("
                   "[^\\)]*FILE[ \t]*\"([^\"]+)\"",
                   error);
      }
      // Insert user defined dependency filters
      {
        std::vector<std::string> flts = InfoGetList("AM_MOC_DEPEND_FILTERS");
        if ((flts.size() % 2) == 0) {
          for (std::vector<std::string>::iterator itC = flts.begin(),
                                                  itE = flts.end();
               itC != itE; itC += 2) {
            pushFilter(*itC, *(itC + 1), error);
            if (!error.empty()) {
              break;
            }
          }
        } else {
          Log().ErrorFile(
            GeneratorT::MOC, InfoFile(),
            "AUTOMOC_DEPEND_FILTERS list size is not a multiple of 2");
          return false;
        }
      }
      if (!error.empty()) {
        Log().ErrorFile(GeneratorT::MOC, InfoFile(), error);
        return false;
      }
    }
    Moc_.PredefsCmd = InfoGetList("AM_MOC_PREDEFS_CMD");
    // Install moc predefs job
    if (!Moc().PredefsCmd.empty()) {
      JobQueues_.MocPredefs.emplace_back(new JobMocPredefsT());
    }
  }

  // - Uic
  Uic_.Executable = InfoGet("AM_QT_UIC_EXECUTABLE");
  Uic_.Enabled = !Uic().Executable.empty();
  if (Uic().Enabled) {
    {
      auto lst = InfoGetList("AM_UIC_SKIP");
      Uic_.SkipList.insert(lst.begin(), lst.end());
    }
    Uic_.SearchPaths = InfoGetList("AM_UIC_SEARCH_PATHS");
    Uic_.TargetOptions = InfoGetConfigList("AM_UIC_TARGET_OPTIONS");
    {
      auto sources = InfoGetList("AM_UIC_OPTIONS_FILES");
      auto options = InfoGetLists("AM_UIC_OPTIONS_OPTIONS");
      // Compare list sizes
      if (sources.size() != options.size()) {
        std::ostringstream ost;
        ost << "files/options lists sizes mismatch (" << sources.size() << "/"
            << options.size() << ")";
        Log().ErrorFile(GeneratorT::UIC, InfoFile(), ost.str());
        return false;
      }
      auto fitEnd = sources.cend();
      auto fit = sources.begin();
      auto oit = options.begin();
      while (fit != fitEnd) {
        Uic_.Options[*fit] = std::move(*oit);
        ++fit;
        ++oit;
      }
    }
  }

  // Initialize source file jobs
  {
    std::hash<std::string> stringHash;
    std::set<std::size_t> uniqueHeaders;

    // Add header jobs
    for (std::string& hdr : InfoGetList("AM_HEADERS")) {
      const bool moc = !Moc().skipped(hdr);
      const bool uic = !Uic().skipped(hdr);
      if ((moc || uic) && uniqueHeaders.emplace(stringHash(hdr)).second) {
        JobQueues_.Headers.emplace_back(
          new JobParseT(std::move(hdr), moc, uic, true));
      }
    }
    // Add source jobs
    {
      std::vector<std::string> sources = InfoGetList("AM_SOURCES");
      // Add header(s) for the source file
      for (std::string& src : sources) {
        const bool srcMoc = !Moc().skipped(src);
        const bool srcUic = !Uic().skipped(src);
        if (!srcMoc && !srcUic) {
          continue;
        }
        // Search for the default header file and a private header
        {
          std::array<std::string, 2> bases;
          bases[0] = FileSys().SubDirPrefix(src);
          bases[0] += FileSys().GetFilenameWithoutLastExtension(src);
          bases[1] = bases[0];
          bases[1] += "_p";
          for (std::string const& headerBase : bases) {
            std::string header;
            if (Base().FindHeader(header, headerBase)) {
              const bool moc = srcMoc && !Moc().skipped(header);
              const bool uic = srcUic && !Uic().skipped(header);
              if ((moc || uic) &&
                  uniqueHeaders.emplace(stringHash(header)).second) {
                JobQueues_.Headers.emplace_back(
                  new JobParseT(std::move(header), moc, uic, true));
              }
            }
          }
        }
        // Add source job
        JobQueues_.Sources.emplace_back(
          new JobParseT(std::move(src), srcMoc, srcUic));
      }
    }
  }

  // Init derived information
  // ------------------------

  // Init file path checksum generator
  FileSys().setupFilePathChecksum(
    Base().CurrentSourceDir, Base().CurrentBinaryDir, Base().ProjectSourceDir,
    Base().ProjectBinaryDir);

  // Moc variables
  if (Moc().Enabled) {
    // Mocs compilation file
    Moc_.CompFileAbs = Base().AbsoluteBuildPath("mocs_compilation.cpp");

    // Moc predefs file
    if (!Moc_.PredefsCmd.empty()) {
      Moc_.PredefsFileRel = "moc_predefs";
      if (Base_.MultiConfig) {
        Moc_.PredefsFileRel += '_';
        Moc_.PredefsFileRel += InfoConfig();
      }
      Moc_.PredefsFileRel += ".h";
      Moc_.PredefsFileAbs = Base_.AbsoluteBuildPath(Moc().PredefsFileRel);
    }

    // Sort include directories on demand
    if (Base().IncludeProjectDirsBefore) {
      // Move strings to temporary list
      std::list<std::string> includes;
      includes.insert(includes.end(), Moc().IncludePaths.begin(),
                      Moc().IncludePaths.end());
      Moc_.IncludePaths.clear();
      Moc_.IncludePaths.reserve(includes.size());
      // Append project directories only
      {
        std::array<std::string const*, 2> const movePaths = {
          { &Base().ProjectBinaryDir, &Base().ProjectSourceDir }
        };
        for (std::string const* ppath : movePaths) {
          std::list<std::string>::iterator it = includes.begin();
          while (it != includes.end()) {
            std::string const& path = *it;
            if (cmSystemTools::StringStartsWith(path, ppath->c_str())) {
              Moc_.IncludePaths.push_back(path);
              it = includes.erase(it);
            } else {
              ++it;
            }
          }
        }
      }
      // Append remaining directories
      Moc_.IncludePaths.insert(Moc_.IncludePaths.end(), includes.begin(),
                               includes.end());
    }
    // Compose moc includes list
    {
      std::set<std::string> frameworkPaths;
      for (std::string const& path : Moc().IncludePaths) {
        Moc_.Includes.push_back("-I" + path);
        // Extract framework path
        if (cmHasLiteralSuffix(path, ".framework/Headers")) {
          // Go up twice to get to the framework root
          std::vector<std::string> pathComponents;
          FileSys().SplitPath(path, pathComponents);
          std::string frameworkPath = FileSys().JoinPath(
            pathComponents.begin(), pathComponents.end() - 2);
          frameworkPaths.insert(frameworkPath);
        }
      }
      // Append framework includes
      for (std::string const& path : frameworkPaths) {
        Moc_.Includes.push_back("-F");
        Moc_.Includes.push_back(path);
      }
    }
    // Setup single list with all options
    {
      // Add includes
      Moc_.AllOptions.insert(Moc_.AllOptions.end(), Moc().Includes.begin(),
                             Moc().Includes.end());
      // Add definitions
      for (std::string const& def : Moc().Definitions) {
        Moc_.AllOptions.push_back("-D" + def);
      }
      // Add options
      Moc_.AllOptions.insert(Moc_.AllOptions.end(), Moc().Options.begin(),
                             Moc().Options.end());
    }
  }

  return true;
}

bool cmQtAutoGeneratorMocUic::Process()
{
  // Run libuv event loop
  UVRequest().send();
  if (uv_run(UVLoop(), UV_RUN_DEFAULT) == 0) {
    if (JobError_) {
      return false;
    }
  } else {
    return false;
  }
  return true;
}

void cmQtAutoGeneratorMocUic::UVPollStage(uv_async_t* handle)
{
  reinterpret_cast<cmQtAutoGeneratorMocUic*>(handle->data)->PollStage();
}

void cmQtAutoGeneratorMocUic::PollStage()
{
  switch (Stage_) {
    case StageT::SETTINGS_READ:
      SettingsFileRead();
      SetStage(StageT::CREATE_DIRECTORIES);
      break;
    case StageT::CREATE_DIRECTORIES:
      CreateDirectories();
      SetStage(StageT::PARSE_SOURCES);
      break;
    case StageT::PARSE_SOURCES:
      if (ThreadsStartJobs(JobQueues_.Sources)) {
        SetStage(StageT::PARSE_HEADERS);
      }
      break;
    case StageT::PARSE_HEADERS:
      if (ThreadsStartJobs(JobQueues_.Headers)) {
        SetStage(StageT::MOC_PREDEFS);
      }
      break;
    case StageT::MOC_PREDEFS:
      if (ThreadsStartJobs(JobQueues_.MocPredefs)) {
        SetStage(StageT::MOC_PROCESS);
      }
      break;
    case StageT::MOC_PROCESS:
      if (ThreadsStartJobs(JobQueues_.Moc)) {
        SetStage(StageT::MOCS_COMPILATION);
      }
      break;
    case StageT::MOCS_COMPILATION:
      if (ThreadsJobsDone()) {
        MocGenerateCompilation();
        SetStage(StageT::UIC_PROCESS);
      }
      break;
    case StageT::UIC_PROCESS:
      if (ThreadsStartJobs(JobQueues_.Uic)) {
        SetStage(StageT::SETTINGS_WRITE);
      }
      break;
    case StageT::SETTINGS_WRITE:
      SettingsFileWrite();
      SetStage(StageT::FINISH);
      break;
    case StageT::FINISH:
      if (ThreadsJobsDone()) {
        // Clear all libuv handles
        ThreadsStop();
        UVRequest().reset();
        // Set highest END stage manually
        Stage_ = StageT::END;
      }
      break;
    case StageT::END:
      break;
  }
}

void cmQtAutoGeneratorMocUic::SetStage(StageT stage)
{
  if (JobError_) {
    stage = StageT::FINISH;
  }
  // Only allow to increase the stage
  if (Stage_ < stage) {
    Stage_ = stage;
    UVRequest().send();
  }
}

void cmQtAutoGeneratorMocUic::SettingsFileRead()
{
  // Compose current settings strings
  {
    cmCryptoHash crypt(cmCryptoHash::AlgoSHA256);
    std::string const sep(" ~~~ ");
    if (Moc_.Enabled) {
      std::string str;
      str += Moc().Executable;
      str += sep;
      str += cmJoin(Moc().AllOptions, ";");
      str += sep;
      str += Base().IncludeProjectDirsBefore ? "TRUE" : "FALSE";
      str += sep;
      str += cmJoin(Moc().PredefsCmd, ";");
      str += sep;
      SettingsStringMoc_ = crypt.HashString(str);
    }
    if (Uic().Enabled) {
      std::string str;
      str += Uic().Executable;
      str += sep;
      str += cmJoin(Uic().TargetOptions, ";");
      for (const auto& item : Uic().Options) {
        str += sep;
        str += item.first;
        str += sep;
        str += cmJoin(item.second, ";");
      }
      str += sep;
      SettingsStringUic_ = crypt.HashString(str);
    }
  }

  // Read old settings and compare
  {
    std::string content;
    if (FileSys().FileRead(content, SettingsFile_)) {
      if (Moc().Enabled) {
        if (SettingsStringMoc_ != SettingsFind(content, "moc")) {
          Moc_.SettingsChanged = true;
        }
      }
      if (Uic().Enabled) {
        if (SettingsStringUic_ != SettingsFind(content, "uic")) {
          Uic_.SettingsChanged = true;
        }
      }
      // In case any setting changed remove the old settings file.
      // This triggers a full rebuild on the next run if the current
      // build is aborted before writing the current settings in the end.
      if (Moc().SettingsChanged || Uic().SettingsChanged) {
        FileSys().FileRemove(SettingsFile_);
      }
    } else {
      // Settings file read failed
      if (Moc().Enabled) {
        Moc_.SettingsChanged = true;
      }
      if (Uic().Enabled) {
        Uic_.SettingsChanged = true;
      }
    }
  }
}

void cmQtAutoGeneratorMocUic::SettingsFileWrite()
{
  std::lock_guard<std::mutex> jobsLock(JobsMutex_);
  // Only write if any setting changed
  if (!JobError_ && (Moc().SettingsChanged || Uic().SettingsChanged)) {
    if (Log().Verbose()) {
      Log().Info(GeneratorT::GEN,
                 "Writing settings file " + Quoted(SettingsFile_));
    }
    // Compose settings file content
    std::string content;
    {
      auto SettingAppend = [&content](const char* key,
                                      std::string const& value) {
        if (!value.empty()) {
          content += key;
          content += ':';
          content += value;
          content += '\n';
        }
      };
      SettingAppend("moc", SettingsStringMoc_);
      SettingAppend("uic", SettingsStringUic_);
    }
    // Write settings file
    if (!FileSys().FileWrite(GeneratorT::GEN, SettingsFile_, content)) {
      Log().ErrorFile(GeneratorT::GEN, SettingsFile_,
                      "Settings file writing failed");
      // Remove old settings file to trigger a full rebuild on the next run
      FileSys().FileRemove(SettingsFile_);
      RegisterJobError();
    }
  }
}

void cmQtAutoGeneratorMocUic::CreateDirectories()
{
  // Create AUTOGEN include directory
  if (!FileSys().MakeDirectory(GeneratorT::GEN, Base().AutogenIncludeDir)) {
    RegisterJobError();
  }
}

bool cmQtAutoGeneratorMocUic::ThreadsStartJobs(JobQueueT& queue)
{
  bool done = false;
  std::size_t queueSize = queue.size();

  // Change the active queue
  {
    std::lock_guard<std::mutex> jobsLock(JobsMutex_);
    // Check if there are still unfinished jobs from the previous queue
    if (JobsRemain_ == 0) {
      if (!JobThreadsAbort_) {
        JobQueue_.swap(queue);
        JobsRemain_ = queueSize;
      } else {
        // Abort requested
        queue.clear();
        queueSize = 0;
      }
      done = true;
    }
  }

  if (done && (queueSize != 0)) {
    // Start new threads on demand
    if (Workers_.empty()) {
      Workers_.resize(Base().NumThreads);
      for (auto& item : Workers_) {
        item = cm::make_unique<WorkerT>(this, UVLoop());
      }
    } else {
      // Notify threads
      if (queueSize == 1) {
        JobsConditionRead_.notify_one();
      } else {
        JobsConditionRead_.notify_all();
      }
    }
  }

  return done;
}

void cmQtAutoGeneratorMocUic::ThreadsStop()
{
  if (!Workers_.empty()) {
    // Clear all jobs
    {
      std::lock_guard<std::mutex> jobsLock(JobsMutex_);
      JobThreadsAbort_ = true;
      JobsRemain_ -= JobQueue_.size();
      JobQueue_.clear();

      JobQueues_.Sources.clear();
      JobQueues_.Headers.clear();
      JobQueues_.MocPredefs.clear();
      JobQueues_.Moc.clear();
      JobQueues_.Uic.clear();
    }
    // Wake threads
    JobsConditionRead_.notify_all();
    // Join and clear threads
    Workers_.clear();
  }
}

bool cmQtAutoGeneratorMocUic::ThreadsJobsDone()
{
  std::lock_guard<std::mutex> jobsLock(JobsMutex_);
  return (JobsRemain_ == 0);
}

void cmQtAutoGeneratorMocUic::WorkerSwapJob(JobHandleT& jobHandle)
{
  bool const jobProcessed(jobHandle);
  if (jobProcessed) {
    jobHandle.reset(nullptr);
  }
  {
    std::unique_lock<std::mutex> jobsLock(JobsMutex_);
    // Reduce the remaining job count and notify the libuv loop
    // when all jobs are done
    if (jobProcessed) {
      --JobsRemain_;
      if (JobsRemain_ == 0) {
        UVRequest().send();
      }
    }
    // Wait for new jobs
    while (!JobThreadsAbort_ && JobQueue_.empty()) {
      JobsConditionRead_.wait(jobsLock);
    }
    // Try to pick up a new job handle
    if (!JobThreadsAbort_ && !JobQueue_.empty()) {
      jobHandle = std::move(JobQueue_.front());
      JobQueue_.pop_front();
    }
  }
}

void cmQtAutoGeneratorMocUic::ParallelRegisterJobError()
{
  std::lock_guard<std::mutex> jobsLock(JobsMutex_);
  RegisterJobError();
}

// Private method that requires cmQtAutoGeneratorMocUic::JobsMutex_ to be
// locked
void cmQtAutoGeneratorMocUic::RegisterJobError()
{
  JobError_ = true;
  if (!JobThreadsAbort_) {
    JobThreadsAbort_ = true;
    // Clear remaining jobs
    if (JobsRemain_ != 0) {
      JobsRemain_ -= JobQueue_.size();
      JobQueue_.clear();
    }
  }
}

bool cmQtAutoGeneratorMocUic::ParallelJobPushMoc(JobHandleT& jobHandle)
{
  std::lock_guard<std::mutex> jobsLock(JobsMutex_);
  if (!JobThreadsAbort_) {
    bool pushJobHandle = true;
    // Do additional tests if this is an included moc job
    const JobMocT& mocJob(static_cast<JobMocT&>(*jobHandle));
    if (!mocJob.IncludeString.empty()) {
      // Register included moc file and look for collisions
      MocIncludedFiles_.emplace(mocJob.SourceFile);
      if (!MocIncludedStrings_.emplace(mocJob.IncludeString).second) {
        // Another source file includes the same moc file!
        for (const JobHandleT& otherHandle : JobQueues_.Moc) {
          const JobMocT& otherJob(static_cast<JobMocT&>(*otherHandle));
          if (otherJob.IncludeString == mocJob.IncludeString) {
            // Check if the same moc file would be generated from different
            // source files which is an error.
            if (otherJob.SourceFile != mocJob.SourceFile) {
              // Include string collision
              std::string error = "The two source files\n  ";
              error += Quoted(mocJob.IncluderFile);
              error += " and\n  ";
              error += Quoted(otherJob.IncluderFile);
              error += "\ncontain the same moc include string ";
              error += Quoted(mocJob.IncludeString);
              error += "\nbut the moc file would be generated from different "
                       "source files\n  ";
              error += Quoted(mocJob.SourceFile);
              error += " and\n  ";
              error += Quoted(otherJob.SourceFile);
              error += ".\nConsider to\n"
                       "- not include the \"moc_<NAME>.cpp\" file\n"
                       "- add a directory prefix to a \"<NAME>.moc\" include "
                       "(e.g \"sub/<NAME>.moc\")\n"
                       "- rename the source file(s)\n";
              Log().Error(GeneratorT::MOC, error);
              RegisterJobError();
            }
            // Do not push this job in since the included moc file already
            // gets generated by an other job.
            pushJobHandle = false;
            break;
          }
        }
      }
    }
    // Push job on demand
    if (pushJobHandle) {
      JobQueues_.Moc.emplace_back(std::move(jobHandle));
    }
  }
  return !JobError_;
}

bool cmQtAutoGeneratorMocUic::ParallelJobPushUic(JobHandleT& jobHandle)
{
  std::lock_guard<std::mutex> jobsLock(JobsMutex_);
  if (!JobThreadsAbort_) {
    bool pushJobHandle = true;
    // Look for include collisions.
    const JobUicT& uicJob(static_cast<JobUicT&>(*jobHandle));
    for (const JobHandleT& otherHandle : JobQueues_.Uic) {
      const JobUicT& otherJob(static_cast<JobUicT&>(*otherHandle));
      if (otherJob.IncludeString == uicJob.IncludeString) {
        // Check if the same uic file would be generated from different
        // source files which would be an error.
        if (otherJob.SourceFile != uicJob.SourceFile) {
          // Include string collision
          std::string error = "The two source files\n  ";
          error += Quoted(uicJob.IncluderFile);
          error += " and\n  ";
          error += Quoted(otherJob.IncluderFile);
          error += "\ncontain the same uic include string ";
          error += Quoted(uicJob.IncludeString);
          error += "\nbut the uic file would be generated from different "
                   "source files\n  ";
          error += Quoted(uicJob.SourceFile);
          error += " and\n  ";
          error += Quoted(otherJob.SourceFile);
          error +=
            ".\nConsider to\n"
            "- add a directory prefix to a \"ui_<NAME>.h\" include "
            "(e.g \"sub/ui_<NAME>.h\")\n"
            "- rename the <NAME>.ui file(s) and adjust the \"ui_<NAME>.h\" "
            "include(s)\n";
          Log().Error(GeneratorT::UIC, error);
          RegisterJobError();
        }
        // Do not push this job in since the uic file already
        // gets generated by an other job.
        pushJobHandle = false;
        break;
      }
    }
    if (pushJobHandle) {
      JobQueues_.Uic.emplace_back(std::move(jobHandle));
    }
  }
  return !JobError_;
}

bool cmQtAutoGeneratorMocUic::ParallelMocIncluded(
  std::string const& sourceFile)
{
  std::lock_guard<std::mutex> mocLock(JobsMutex_);
  return (MocIncludedFiles_.find(sourceFile) != MocIncludedFiles_.end());
}

void cmQtAutoGeneratorMocUic::ParallelMocAutoRegister(
  std::string const& mocFile)
{
  std::lock_guard<std::mutex> mocLock(JobsMutex_);
  MocAutoFiles_.emplace(mocFile);
}

void cmQtAutoGeneratorMocUic::ParallelMocAutoUpdated()
{
  std::lock_guard<std::mutex> mocLock(JobsMutex_);
  MocAutoFileUpdated_ = true;
}

void cmQtAutoGeneratorMocUic::MocGenerateCompilation()
{
  std::lock_guard<std::mutex> mocLock(JobsMutex_);
  if (!JobError_ && Moc().Enabled) {
    // Write mocs compilation build file
    {
      // Compose mocs compilation file content
      std::string content =
        "// This file is autogenerated. Changes will be overwritten.\n";
      if (MocAutoFiles_.empty()) {
        // Placeholder content
        content += "// No files found that require moc or the moc files are "
                   "included\n";
        content += "enum some_compilers { need_more_than_nothing };\n";
      } else {
        // Valid content
        char const sbeg = Base().MultiConfig ? '<' : '"';
        char const send = Base().MultiConfig ? '>' : '"';
        for (std::string const& mocfile : MocAutoFiles_) {
          content += "#include ";
          content += sbeg;
          content += mocfile;
          content += send;
          content += '\n';
        }
      }

      std::string const& compAbs = Moc().CompFileAbs;
      if (FileSys().FileDiffers(compAbs, content)) {
        // Actually write mocs compilation file
        if (Log().Verbose()) {
          Log().Info(GeneratorT::MOC, "Generating MOC compilation " + compAbs);
        }
        if (!FileSys().FileWrite(GeneratorT::MOC, compAbs, content)) {
          Log().ErrorFile(GeneratorT::MOC, compAbs,
                          "mocs compilation file writing failed");
          RegisterJobError();
          return;
        }
      } else if (MocAutoFileUpdated_) {
        // Only touch mocs compilation file
        if (Log().Verbose()) {
          Log().Info(GeneratorT::MOC, "Touching mocs compilation " + compAbs);
        }
        FileSys().Touch(compAbs);
      }
    }
    // Write mocs compilation wrapper file
    if (Base().MultiConfig) {
    }
  }
}
