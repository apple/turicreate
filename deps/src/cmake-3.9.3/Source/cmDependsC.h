/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmDependsC_h
#define cmDependsC_h

#include "cmConfigure.h"

#include "cmDepends.h"

#include "cmsys/RegularExpression.hxx"
#include <iosfwd>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

class cmLocalGenerator;

/** \class cmDependsC
 * \brief Dependency scanner for C and C++ object files.
 */
class cmDependsC : public cmDepends
{
  CM_DISABLE_COPY(cmDependsC)

public:
  /** Checking instances need to know the build directory name and the
      relative path from the build directory to the target file.  */
  cmDependsC();
  cmDependsC(cmLocalGenerator* lg, const char* targetDir,
             const std::string& lang,
             const std::map<std::string, DependencyVector>* validDeps);

  /** Virtual destructor to cleanup subclasses properly.  */
  ~cmDependsC() CM_OVERRIDE;

protected:
  // Implement writing/checking methods required by superclass.
  bool WriteDependencies(const std::set<std::string>& sources,
                         const std::string& obj, std::ostream& makeDepends,
                         std::ostream& internalDepends) CM_OVERRIDE;

  // Method to scan a single file.
  void Scan(std::istream& is, const char* directory,
            const std::string& fullName);

  // Regular expression to identify C preprocessor include directives.
  cmsys::RegularExpression IncludeRegexLine;

  // Regular expressions to choose which include files to scan
  // recursively and which to complain about not finding.
  cmsys::RegularExpression IncludeRegexScan;
  cmsys::RegularExpression IncludeRegexComplain;
  std::string IncludeRegexLineString;
  std::string IncludeRegexScanString;
  std::string IncludeRegexComplainString;

  // Regex to transform #include lines.
  std::string IncludeRegexTransformString;
  cmsys::RegularExpression IncludeRegexTransform;
  typedef std::map<std::string, std::string> TransformRulesType;
  TransformRulesType TransformRules;
  void SetupTransforms();
  void ParseTransform(std::string const& xform);
  void TransformLine(std::string& line);

public:
  // Data structures for dependency graph walk.
  struct UnscannedEntry
  {
    std::string FileName;
    std::string QuotedLocation;
  };

  struct cmIncludeLines
  {
    cmIncludeLines()
      : Used(false)
    {
    }
    std::vector<UnscannedEntry> UnscannedEntries;
    bool Used;
  };

protected:
  const std::map<std::string, DependencyVector>* ValidDeps;
  std::set<std::string> Encountered;
  std::queue<UnscannedEntry> Unscanned;

  std::map<std::string, cmIncludeLines*> FileCache;
  std::map<std::string, std::string> HeaderLocationCache;

  std::string CacheFileName;

  void WriteCacheFile() const;
  void ReadCacheFile();
};

#endif
