/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestSVN_h
#define cmCTestSVN_h

#include "cmConfigure.h"

#include "cmCTestGlobalVC.h"

#include <iosfwd>
#include <string>
#include <vector>

class cmCTest;
class cmXMLWriter;

/** \class cmCTestSVN
 * \brief Interaction with subversion command-line tool
 *
 */
class cmCTestSVN : public cmCTestGlobalVC
{
public:
  /** Construct with a CTest instance and update log stream.  */
  cmCTestSVN(cmCTest* ctest, std::ostream& log);

  ~cmCTestSVN() CM_OVERRIDE;

private:
  // Implement cmCTestVC internal API.
  void CleanupImpl() CM_OVERRIDE;
  bool NoteOldRevision() CM_OVERRIDE;
  bool NoteNewRevision() CM_OVERRIDE;
  bool UpdateImpl() CM_OVERRIDE;

  bool RunSVNCommand(std::vector<char const*> const& parameters,
                     OutputParser* out, OutputParser* err);

  // Information about an SVN repository (root repository or external)
  struct SVNInfo
  {

    SVNInfo(const char* path)
      : LocalPath(path)
    {
    }
    // Remove base from the filename
    std::string BuildLocalPath(std::string const& path) const;

    // LocalPath relative to the main source directory.
    std::string LocalPath;

    // URL of repository directory checked out in the working tree.
    std::string URL;

    // URL of repository root directory.
    std::string Root;

    // Directory under repository root checked out in working tree.
    std::string Base;

    // Old and new repository revisions.
    std::string OldRevision;
    std::string NewRevision;
  };

  // Extended revision structure to include info about external it refers to.
  struct Revision;

  friend struct Revision;

  // Info of all the repositories (root, externals and nested ones).
  std::vector<SVNInfo> Repositories;

  // Pointer to the infos of the root repository.
  SVNInfo* RootInfo;

  std::string LoadInfo(SVNInfo& svninfo);
  bool LoadRepositories();
  bool LoadModifications() CM_OVERRIDE;
  bool LoadRevisions() CM_OVERRIDE;
  bool LoadRevisions(SVNInfo& svninfo);

  void GuessBase(SVNInfo& svninfo, std::vector<Change> const& changes);

  void DoRevisionSVN(Revision const& revision,
                     std::vector<Change> const& changes);

  void WriteXMLGlobal(cmXMLWriter& xml) CM_OVERRIDE;

  class ExternalParser;
  // Parsing helper classes.
  class InfoParser;
  class LogParser;
  class StatusParser;
  class UpdateParser;

  friend class InfoParser;
  friend class LogParser;
  friend class StatusParser;
  friend class UpdateParser;
  friend class ExternalParser;
};

#endif
