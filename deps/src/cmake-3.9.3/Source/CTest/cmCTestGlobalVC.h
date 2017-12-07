/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestGlobalVC_h
#define cmCTestGlobalVC_h

#include "cmConfigure.h"

#include "cmCTestVC.h"

#include <iosfwd>
#include <list>
#include <map>
#include <string>
#include <vector>

class cmCTest;
class cmXMLWriter;

/** \class cmCTestGlobalVC
 * \brief Base class for handling globally-versioned trees
 *
 */
class cmCTestGlobalVC : public cmCTestVC
{
public:
  /** Construct with a CTest instance and update log stream.  */
  cmCTestGlobalVC(cmCTest* ctest, std::ostream& log);

  ~cmCTestGlobalVC() CM_OVERRIDE;

protected:
  // Implement cmCTestVC internal API.
  bool WriteXMLUpdates(cmXMLWriter& xml) CM_OVERRIDE;

  /** Represent a vcs-reported action for one path in a revision.  */
  struct Change
  {
    char Action;
    std::string Path;
    Change(char a = '?')
      : Action(a)
    {
    }
  };

  // Update status for files in each directory.
  class Directory : public std::map<std::string, File>
  {
  };
  std::map<std::string, Directory> Dirs;

  // Old and new repository revisions.
  std::string OldRevision;
  std::string NewRevision;

  // Information known about old revision.
  Revision PriorRev;

  // Information about revisions from a svn log.
  std::list<Revision> Revisions;

  virtual const char* LocalPath(std::string const& path);

  virtual void DoRevision(Revision const& revision,
                          std::vector<Change> const& changes);
  virtual void DoModification(PathStatus status, std::string const& path);
  virtual bool LoadModifications() = 0;
  virtual bool LoadRevisions() = 0;

  virtual void WriteXMLGlobal(cmXMLWriter& xml);
  void WriteXMLDirectory(cmXMLWriter& xml, std::string const& path,
                         Directory const& dir);
};

#endif
