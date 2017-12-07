/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmOutputRequiredFilesCommand.h"

#include "cmsys/FStream.hxx"
#include "cmsys/RegularExpression.hxx"
#include <map>
#include <utility>

#include "cmAlgorithms.h"
#include "cmGeneratorExpression.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cm_unordered_map.hxx"

class cmExecutionStatus;

/** \class cmDependInformation
 * \brief Store dependency information for a single source file.
 *
 * This structure stores the depend information for a single source file.
 */
class cmDependInformation
{
public:
  /**
   * Construct with dependency generation marked not done; instance
   * not placed in cmMakefile's list.
   */
  cmDependInformation()
    : DependDone(false)
    , SourceFile(CM_NULLPTR)
  {
  }

  /**
   * The set of files on which this one depends.
   */
  typedef std::set<cmDependInformation*> DependencySetType;
  DependencySetType DependencySet;

  /**
   * This flag indicates whether dependency checking has been
   * performed for this file.
   */
  bool DependDone;

  /**
   * If this object corresponds to a cmSourceFile instance, this points
   * to it.
   */
  const cmSourceFile* SourceFile;

  /**
   * Full path to this file.
   */
  std::string FullPath;

  /**
   * Full path not including file name.
   */
  std::string PathOnly;

  /**
   * Name used to #include this file.
   */
  std::string IncludeName;

  /**
   * This method adds the dependencies of another file to this one.
   */
  void AddDependencies(cmDependInformation* info)
  {
    if (this != info) {
      this->DependencySet.insert(info);
    }
  }
};

class cmLBDepend
{
public:
  /**
   * Construct the object with verbose turned off.
   */
  cmLBDepend()
  {
    this->Verbose = false;
    this->IncludeFileRegularExpression.compile("^.*$");
    this->ComplainFileRegularExpression.compile("^$");
  }

  /**
   * Destructor.
   */
  ~cmLBDepend() { cmDeleteAll(this->DependInformationMap); }

  /**
   * Set the makefile that is used as a source of classes.
   */
  void SetMakefile(cmMakefile* makefile)
  {
    this->Makefile = makefile;

    // Now extract the include file regular expression from the makefile.
    this->IncludeFileRegularExpression.compile(
      this->Makefile->GetIncludeRegularExpression());
    this->ComplainFileRegularExpression.compile(
      this->Makefile->GetComplainRegularExpression());

    // Now extract any include paths from the targets
    std::set<std::string> uniqueIncludes;
    std::vector<std::string> orderedAndUniqueIncludes;
    cmTargets& targets = this->Makefile->GetTargets();
    for (cmTargets::iterator l = targets.begin(); l != targets.end(); ++l) {
      const char* incDirProp = l->second.GetProperty("INCLUDE_DIRECTORIES");
      if (!incDirProp) {
        continue;
      }

      std::string incDirs = cmGeneratorExpression::Preprocess(
        incDirProp, cmGeneratorExpression::StripAllGeneratorExpressions);

      std::vector<std::string> includes;
      cmSystemTools::ExpandListArgument(incDirs, includes);

      for (std::vector<std::string>::const_iterator j = includes.begin();
           j != includes.end(); ++j) {
        std::string path = *j;
        this->Makefile->ExpandVariablesInString(path);
        if (uniqueIncludes.insert(path).second) {
          orderedAndUniqueIncludes.push_back(path);
        }
      }
    }

    for (std::vector<std::string>::const_iterator it =
           orderedAndUniqueIncludes.begin();
         it != orderedAndUniqueIncludes.end(); ++it) {
      this->AddSearchPath(*it);
    }
  }

  /**
   * Add a directory to the search path for include files.
   */
  void AddSearchPath(const std::string& path)
  {
    this->IncludeDirectories.push_back(path);
  }

  /**
   * Generate dependencies for the file given.  Returns a pointer to
   * the cmDependInformation object for the file.
   */
  const cmDependInformation* FindDependencies(const char* file)
  {
    cmDependInformation* info = this->GetDependInformation(file, CM_NULLPTR);
    this->GenerateDependInformation(info);
    return info;
  }

protected:
  /**
   * Compute the depend information for this class.
   */

  void DependWalk(cmDependInformation* info)
  {
    cmsys::ifstream fin(info->FullPath.c_str());
    if (!fin) {
      cmSystemTools::Error("error can not open ", info->FullPath.c_str());
      return;
    }

    std::string line;
    while (cmSystemTools::GetLineFromStream(fin, line)) {
      if (cmHasLiteralPrefix(line.c_str(), "#include")) {
        // if it is an include line then create a string class
        size_t qstart = line.find('\"', 8);
        size_t qend;
        // if a quote is not found look for a <
        if (qstart == std::string::npos) {
          qstart = line.find('<', 8);
          // if a < is not found then move on
          if (qstart == std::string::npos) {
            cmSystemTools::Error("unknown include directive ", line.c_str());
            continue;
          }
          qend = line.find('>', qstart + 1);
        } else {
          qend = line.find('\"', qstart + 1);
        }
        // extract the file being included
        std::string includeFile = line.substr(qstart + 1, qend - qstart - 1);
        // see if the include matches the regular expression
        if (!this->IncludeFileRegularExpression.find(includeFile)) {
          if (this->Verbose) {
            std::string message = "Skipping ";
            message += includeFile;
            message += " for file ";
            message += info->FullPath;
            cmSystemTools::Error(message.c_str(), CM_NULLPTR);
          }
          continue;
        }

        // Add this file and all its dependencies.
        this->AddDependency(info, includeFile.c_str());
        /// add the cxx file if it exists
        std::string cxxFile = includeFile;
        std::string::size_type pos = cxxFile.rfind('.');
        if (pos != std::string::npos) {
          std::string root = cxxFile.substr(0, pos);
          cxxFile = root + ".cxx";
          bool found = false;
          // try jumping to .cxx .cpp and .c in order
          if (cmSystemTools::FileExists(cxxFile.c_str())) {
            found = true;
          }
          for (std::vector<std::string>::iterator i =
                 this->IncludeDirectories.begin();
               i != this->IncludeDirectories.end(); ++i) {
            std::string path = *i;
            path = path + "/";
            path = path + cxxFile;
            if (cmSystemTools::FileExists(path.c_str())) {
              found = true;
            }
          }
          if (!found) {
            cxxFile = root + ".cpp";
            if (cmSystemTools::FileExists(cxxFile.c_str())) {
              found = true;
            }
            for (std::vector<std::string>::iterator i =
                   this->IncludeDirectories.begin();
                 i != this->IncludeDirectories.end(); ++i) {
              std::string path = *i;
              path = path + "/";
              path = path + cxxFile;
              if (cmSystemTools::FileExists(path.c_str())) {
                found = true;
              }
            }
          }
          if (!found) {
            cxxFile = root + ".c";
            if (cmSystemTools::FileExists(cxxFile.c_str())) {
              found = true;
            }
            for (std::vector<std::string>::iterator i =
                   this->IncludeDirectories.begin();
                 i != this->IncludeDirectories.end(); ++i) {
              std::string path = *i;
              path = path + "/";
              path = path + cxxFile;
              if (cmSystemTools::FileExists(path.c_str())) {
                found = true;
              }
            }
          }
          if (!found) {
            cxxFile = root + ".txx";
            if (cmSystemTools::FileExists(cxxFile.c_str())) {
              found = true;
            }
            for (std::vector<std::string>::iterator i =
                   this->IncludeDirectories.begin();
                 i != this->IncludeDirectories.end(); ++i) {
              std::string path = *i;
              path = path + "/";
              path = path + cxxFile;
              if (cmSystemTools::FileExists(path.c_str())) {
                found = true;
              }
            }
          }
          if (found) {
            this->AddDependency(info, cxxFile.c_str());
          }
        }
      }
    }
  }

  /**
   * Add a dependency.  Possibly walk it for more dependencies.
   */
  void AddDependency(cmDependInformation* info, const char* file)
  {
    cmDependInformation* dependInfo =
      this->GetDependInformation(file, info->PathOnly.c_str());
    this->GenerateDependInformation(dependInfo);
    info->AddDependencies(dependInfo);
  }

  /**
   * Fill in the given object with dependency information.  If the
   * information is already complete, nothing is done.
   */
  void GenerateDependInformation(cmDependInformation* info)
  {
    // If dependencies are already done, stop now.
    if (info->DependDone) {
      return;
    }
    // Make sure we don't visit the same file more than once.
    info->DependDone = true;

    const char* path = info->FullPath.c_str();
    if (!path) {
      cmSystemTools::Error(
        "Attempt to find dependencies for file without path!");
      return;
    }

    bool found = false;

    // If the file exists, use it to find dependency information.
    if (cmSystemTools::FileExists(path, true)) {
      // Use the real file to find its dependencies.
      this->DependWalk(info);
      found = true;
    }

    // See if the cmSourceFile for it has any files specified as
    // dependency hints.
    if (info->SourceFile != CM_NULLPTR) {

      // Get the cmSourceFile corresponding to this.
      const cmSourceFile& cFile = *(info->SourceFile);
      // See if there are any hints for finding dependencies for the missing
      // file.
      if (!cFile.GetDepends().empty()) {
        // Dependency hints have been given.  Use them to begin the
        // recursion.
        for (std::vector<std::string>::const_iterator file =
               cFile.GetDepends().begin();
             file != cFile.GetDepends().end(); ++file) {
          this->AddDependency(info, file->c_str());
        }

        // Found dependency information.  We are done.
        found = true;
      }
    }

    if (!found) {
      // Try to find the file amongst the sources
      cmSourceFile* srcFile = this->Makefile->GetSource(
        cmSystemTools::GetFilenameWithoutExtension(path));
      if (srcFile) {
        if (srcFile->GetFullPath() == path) {
          found = true;
        } else {
          // try to guess which include path to use
          for (std::vector<std::string>::iterator t =
                 this->IncludeDirectories.begin();
               t != this->IncludeDirectories.end(); ++t) {
            std::string incpath = *t;
            if (!incpath.empty() && incpath[incpath.size() - 1] != '/') {
              incpath = incpath + "/";
            }
            incpath = incpath + path;
            if (srcFile->GetFullPath() == incpath) {
              // set the path to the guessed path
              info->FullPath = incpath;
              found = true;
            }
          }
        }
      }
    }

    if (!found) {
      // Couldn't find any dependency information.
      if (this->ComplainFileRegularExpression.find(
            info->IncludeName.c_str())) {
        cmSystemTools::Error("error cannot find dependencies for ", path);
      } else {
        // Destroy the name of the file so that it won't be output as a
        // dependency.
        info->FullPath = "";
      }
    }
  }

  /**
   * Get an instance of cmDependInformation corresponding to the given file
   * name.
   */
  cmDependInformation* GetDependInformation(const char* file,
                                            const char* extraPath)
  {
    // Get the full path for the file so that lookup is unambiguous.
    std::string fullPath = this->FullPath(file, extraPath);

    // Try to find the file's instance of cmDependInformation.
    DependInformationMapType::const_iterator result =
      this->DependInformationMap.find(fullPath);
    if (result != this->DependInformationMap.end()) {
      // Found an instance, return it.
      return result->second;
    }
    // Didn't find an instance.  Create a new one and save it.
    cmDependInformation* info = new cmDependInformation;
    info->FullPath = fullPath;
    info->PathOnly = cmSystemTools::GetFilenamePath(fullPath);
    info->IncludeName = file;
    this->DependInformationMap[fullPath] = info;
    return info;
  }

  /**
   * Find the full path name for the given file name.
   * This uses the include directories.
   * TODO: Cache path conversions to reduce FileExists calls.
   */
  std::string FullPath(const char* fname, const char* extraPath)
  {
    DirectoryToFileToPathMapType::iterator m;
    if (extraPath) {
      m = this->DirectoryToFileToPathMap.find(extraPath);
    } else {
      m = this->DirectoryToFileToPathMap.find("");
    }

    if (m != this->DirectoryToFileToPathMap.end()) {
      FileToPathMapType& map = m->second;
      FileToPathMapType::iterator p = map.find(fname);
      if (p != map.end()) {
        return p->second;
      }
    }

    if (cmSystemTools::FileExists(fname, true)) {
      std::string fp = cmSystemTools::CollapseFullPath(fname);
      this->DirectoryToFileToPathMap[extraPath ? extraPath : ""][fname] = fp;
      return fp;
    }

    for (std::vector<std::string>::iterator i =
           this->IncludeDirectories.begin();
         i != this->IncludeDirectories.end(); ++i) {
      std::string path = *i;
      if (!path.empty() && path[path.size() - 1] != '/') {
        path = path + "/";
      }
      path = path + fname;
      if (cmSystemTools::FileExists(path.c_str(), true) &&
          !cmSystemTools::FileIsDirectory(path)) {
        std::string fp = cmSystemTools::CollapseFullPath(path);
        this->DirectoryToFileToPathMap[extraPath ? extraPath : ""][fname] = fp;
        return fp;
      }
    }

    if (extraPath) {
      std::string path = extraPath;
      if (!path.empty() && path[path.size() - 1] != '/') {
        path = path + "/";
      }
      path = path + fname;
      if (cmSystemTools::FileExists(path.c_str(), true) &&
          !cmSystemTools::FileIsDirectory(path)) {
        std::string fp = cmSystemTools::CollapseFullPath(path);
        this->DirectoryToFileToPathMap[extraPath][fname] = fp;
        return fp;
      }
    }

    // Couldn't find the file.
    return std::string(fname);
  }

  cmMakefile* Makefile;
  bool Verbose;
  cmsys::RegularExpression IncludeFileRegularExpression;
  cmsys::RegularExpression ComplainFileRegularExpression;
  std::vector<std::string> IncludeDirectories;
  typedef std::map<std::string, std::string> FileToPathMapType;
  typedef std::map<std::string, FileToPathMapType>
    DirectoryToFileToPathMapType;
  typedef std::map<std::string, cmDependInformation*> DependInformationMapType;
  DependInformationMapType DependInformationMap;
  DirectoryToFileToPathMapType DirectoryToFileToPathMap;
};

// cmOutputRequiredFilesCommand
bool cmOutputRequiredFilesCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.size() != 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // store the arg for final pass
  this->File = args[0];
  this->OutputFile = args[1];

  // compute the list of files
  cmLBDepend md;
  md.SetMakefile(this->Makefile);
  md.AddSearchPath(this->Makefile->GetCurrentSourceDirectory());
  // find the depends for a file
  const cmDependInformation* info = md.FindDependencies(this->File.c_str());
  if (info) {
    // write them out
    FILE* fout = cmsys::SystemTools::Fopen(this->OutputFile, "w");
    if (!fout) {
      std::string err = "Can not open output file: ";
      err += this->OutputFile;
      this->SetError(err);
      return false;
    }
    std::set<cmDependInformation const*> visited;
    this->ListDependencies(info, fout, &visited);
    fclose(fout);
  }

  return true;
}

void cmOutputRequiredFilesCommand::ListDependencies(
  cmDependInformation const* info, FILE* fout,
  std::set<cmDependInformation const*>* visited)
{
  // add info to the visited set
  visited->insert(info);
  // now recurse with info's dependencies
  for (cmDependInformation::DependencySetType::const_iterator d =
         info->DependencySet.begin();
       d != info->DependencySet.end(); ++d) {
    if (visited->find(*d) == visited->end()) {
      if (info->FullPath != "") {
        std::string tmp = (*d)->FullPath;
        std::string::size_type pos = tmp.rfind('.');
        if (pos != std::string::npos && (tmp.substr(pos) != ".h")) {
          tmp = tmp.substr(0, pos);
          fprintf(fout, "%s\n", (*d)->FullPath.c_str());
        }
      }
      this->ListDependencies(*d, fout, visited);
    }
  }
}
