/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestCVS_h
#define cmCTestCVS_h

#include "cmConfigure.h"

#include "cmCTestVC.h"

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

class cmCTest;
class cmXMLWriter;

/** \class cmCTestCVS
 * \brief Interaction with cvs command-line tool
 *
 */
class cmCTestCVS : public cmCTestVC
{
public:
  /** Construct with a CTest instance and update log stream.  */
  cmCTestCVS(cmCTest* ctest, std::ostream& log);

  ~cmCTestCVS() CM_OVERRIDE;

private:
  // Implement cmCTestVC internal API.
  bool UpdateImpl() CM_OVERRIDE;
  bool WriteXMLUpdates(cmXMLWriter& xml) CM_OVERRIDE;

  // Update status for files in each directory.
  class Directory : public std::map<std::string, PathStatus>
  {
  };
  std::map<std::string, Directory> Dirs;

  std::string ComputeBranchFlag(std::string const& dir);
  void LoadRevisions(std::string const& file, const char* branchFlag,
                     std::vector<Revision>& revisions);
  void WriteXMLDirectory(cmXMLWriter& xml, std::string const& path,
                         Directory const& dir);

  // Parsing helper classes.
  class LogParser;
  class UpdateParser;

  friend class LogParser;
  friend class UpdateParser;
};

#endif
