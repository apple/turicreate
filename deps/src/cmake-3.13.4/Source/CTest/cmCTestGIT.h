/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestGIT_h
#define cmCTestGIT_h

#include "cmConfigure.h" // IWYU pragma: keep

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

  ~cmCTestGIT() override;

private:
  unsigned int CurrentGitVersion;
  unsigned int GetGitVersion();
  std::string GetWorkingRevision();
  bool NoteOldRevision() override;
  bool NoteNewRevision() override;
  bool UpdateImpl() override;

  std::string FindGitDir();
  std::string FindTopDir();

  bool UpdateByFetchAndReset();
  bool UpdateByCustom(std::string const& custom);
  bool UpdateInternal();

  bool LoadRevisions() override;
  bool LoadModifications() override;

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
