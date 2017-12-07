/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmDepends_h
#define cmDepends_h

#include "cmConfigure.h"

#include <iosfwd>
#include <map>
#include <set>
#include <stddef.h>
#include <string>
#include <vector>

class cmFileTimeComparison;
class cmLocalGenerator;

/** \class cmDepends
 * \brief Dependency scanner superclass.
 *
 * This class is responsible for maintaining a .depends.make file in
 * the build tree corresponding to an object file.  Subclasses help it
 * maintain dependencies for particular languages.
 */
class cmDepends
{
  CM_DISABLE_COPY(cmDepends)

public:
  /** Instances need to know the build directory name and the relative
      path from the build directory to the target file.  */
  cmDepends(cmLocalGenerator* lg = CM_NULLPTR, const char* targetDir = "");

  /** at what level will the compile be done from */
  void SetCompileDirectory(const char* dir) { this->CompileDirectory = dir; }

  /** Set the local generator for the directory in which we are
      scanning dependencies.  This is not a full local generator; it
      has been setup to do relative path conversions for the current
      directory.  */
  void SetLocalGenerator(cmLocalGenerator* lg) { this->LocalGenerator = lg; }

  /** Set the specific language to be scanned.  */
  void SetLanguage(const std::string& lang) { this->Language = lang; }

  /** Set the target build directory.  */
  void SetTargetDirectory(const char* dir) { this->TargetDirectory = dir; }

  /** should this be verbose in its output */
  void SetVerbose(bool verb) { this->Verbose = verb; }

  /** Virtual destructor to cleanup subclasses properly.  */
  virtual ~cmDepends();

  /** Write dependencies for the target file.  */
  bool Write(std::ostream& makeDepends, std::ostream& internalDepends);

  class DependencyVector : public std::vector<std::string>
  {
  };

  /** Check dependencies for the target file.  Returns true if
      dependencies are okay and false if they must be generated.  If
      they must be generated Clear has already been called to wipe out
      the old dependencies.
      Dependencies which are still valid will be stored in validDeps. */
  bool Check(const char* makeFile, const char* internalFile,
             std::map<std::string, DependencyVector>& validDeps);

  /** Clear dependencies for the target file so they will be regenerated.  */
  void Clear(const char* file);

  /** Set the file comparison object */
  void SetFileComparison(cmFileTimeComparison* fc)
  {
    this->FileComparison = fc;
  }

protected:
  // Write dependencies for the target file to the given stream.
  // Return true for success and false for failure.
  virtual bool WriteDependencies(const std::set<std::string>& sources,
                                 const std::string& obj,
                                 std::ostream& makeDepends,
                                 std::ostream& internalDepends);

  // Check dependencies for the target file in the given stream.
  // Return false if dependencies must be regenerated and true
  // otherwise.
  virtual bool CheckDependencies(
    std::istream& internalDepends, const char* internalDependsFileName,
    std::map<std::string, DependencyVector>& validDeps);

  // Finalize the dependency information for the target.
  virtual bool Finalize(std::ostream& makeDepends,
                        std::ostream& internalDepends);

  // The directory in which the build rule for the target file is executed.
  std::string CompileDirectory;

  // The local generator.
  cmLocalGenerator* LocalGenerator;

  // Flag for verbose output.
  bool Verbose;
  cmFileTimeComparison* FileComparison;

  std::string Language;

  // The full path to the target's build directory.
  std::string TargetDirectory;

  size_t MaxPath;
  char* Dependee;
  char* Depender;

  // The include file search path.
  std::vector<std::string> IncludePath;

  void SetIncludePathFromLanguage(const std::string& lang);
};

#endif
