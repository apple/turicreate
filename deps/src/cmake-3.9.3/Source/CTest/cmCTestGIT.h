/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestGIT_h
#define cmCTestGIT_h

#include "cmConfigure.h"

#include "cmCTestGlobalVC.h"

#include <iosfwd>
#include <string>

class cmCTest;

/** \class cmCTestGIT
 * \brief Interaction with git command-line tool
 *
 */
class cmCTestGIT : public cmCTestGlobalVC
{
public:
  /** Construct with a CTest instance and update log stream.  */
  cmCTestGIT(cmCTest* ctest, std::ostream& log);

  ~cmCTestGIT() CM_OVERRIDE;

private:
  unsigned int CurrentGitVersion;
  unsigned int GetGitVersion();
  std::string GetWorkingRevision();
  bool NoteOldRevision() CM_OVERRIDE;
  bool NoteNewRevision() CM_OVERRIDE;
  bool UpdateImpl() CM_OVERRIDE;

  std::string FindGitDir();
  std::string FindTopDir();

  bool UpdateByFetchAndReset();
  bool UpdateByCustom(std::string const& custom);
  bool UpdateInternal();

  bool LoadRevisions() CM_OVERRIDE;
  bool LoadModifications() CM_OVERRIDE;

  // "public" needed by older Sun compilers
public:
  // Parsing helper classes.
  class CommitParser;
  class DiffParser;
  class OneLineParser;

  friend class CommitParser;
  friend class DiffParser;
  friend class OneLineParser;
};

#endif
