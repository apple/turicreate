/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmOrderDirectories_h
#define cmOrderDirectories_h

#include "cmConfigure.h"

#include "cmsys/RegularExpression.hxx"
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

class cmGeneratorTarget;
class cmGlobalGenerator;
class cmOrderDirectoriesConstraint;

/** \class cmOrderDirectories
 * \brief Compute a safe runtime path order for a set of shared libraries.
 */
class cmOrderDirectories
{
public:
  cmOrderDirectories(cmGlobalGenerator* gg, cmGeneratorTarget const* target,
                     const char* purpose);
  ~cmOrderDirectories();
  void AddRuntimeLibrary(std::string const& fullPath,
                         const char* soname = CM_NULLPTR);
  void AddLinkLibrary(std::string const& fullPath);
  void AddUserDirectories(std::vector<std::string> const& extra);
  void AddLanguageDirectories(std::vector<std::string> const& dirs);
  void SetImplicitDirectories(std::set<std::string> const& implicitDirs);
  void SetLinkExtensionInfo(std::vector<std::string> const& linkExtensions,
                            std::string const& removeExtRegex);

  std::vector<std::string> const& GetOrderedDirectories();

private:
  cmGlobalGenerator* GlobalGenerator;
  cmGeneratorTarget const* Target;
  std::string Purpose;

  std::vector<std::string> OrderedDirectories;

  std::vector<cmOrderDirectoriesConstraint*> ConstraintEntries;
  std::vector<cmOrderDirectoriesConstraint*> ImplicitDirEntries;
  std::vector<std::string> UserDirectories;
  std::vector<std::string> LanguageDirectories;
  cmsys::RegularExpression RemoveLibraryExtension;
  std::vector<std::string> LinkExtensions;
  std::set<std::string> ImplicitDirectories;
  std::set<std::string> EmmittedConstraintSOName;
  std::set<std::string> EmmittedConstraintLibrary;
  std::vector<std::string> OriginalDirectories;
  std::map<std::string, int> DirectoryIndex;
  std::vector<int> DirectoryVisited;
  void CollectOriginalDirectories();
  int AddOriginalDirectory(std::string const& dir);
  void AddOriginalDirectories(std::vector<std::string> const& dirs);
  void FindConflicts();
  void FindImplicitConflicts();
  void OrderDirectories();
  void VisitDirectory(unsigned int i);
  void DiagnoseCycle();
  int WalkId;
  bool CycleDiagnosed;
  bool Computed;

  // Adjacency-list representation of runtime path ordering graph.
  // This maps from directory to those that must come *before* it.
  // Each entry that must come before is a pair.  The first element is
  // the index of the directory that must come first.  The second
  // element is the index of the runtime library that added the
  // constraint.
  typedef std::pair<int, int> ConflictPair;
  struct ConflictList : public std::vector<ConflictPair>
  {
  };
  std::vector<ConflictList> ConflictGraph;

  // Compare directories after resolving symlinks.
  bool IsSameDirectory(std::string const& l, std::string const& r);

  bool IsImplicitDirectory(std::string const& dir);

  std::string const& GetRealPath(std::string const& dir);
  std::map<std::string, std::string> RealPaths;

  friend class cmOrderDirectoriesConstraint;
  friend class cmOrderDirectoriesConstraintLibrary;
};

#endif
