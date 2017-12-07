/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestP4_h
#define cmCTestP4_h

#include "cmConfigure.h"

#include "cmCTestGlobalVC.h"

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

class cmCTest;

/** \class cmCTestP4
 * \brief Interaction with the Perforce command-line tool
 *
 */
class cmCTestP4 : public cmCTestGlobalVC
{
public:
  /** Construct with a CTest instance and update log stream.  */
  cmCTestP4(cmCTest* ctest, std::ostream& log);

  ~cmCTestP4() CM_OVERRIDE;

private:
  std::vector<std::string> ChangeLists;

  struct User
  {
    std::string UserName;
    std::string Name;
    std::string EMail;
    std::string AccessTime;

    User()
      : UserName()
      , Name()
      , EMail()
      , AccessTime()
    {
    }
  };
  std::map<std::string, User> Users;
  std::vector<std::string> P4Options;

  User GetUserData(const std::string& username);
  void SetP4Options(std::vector<char const*>& options);

  std::string GetWorkingRevision();
  bool NoteOldRevision() CM_OVERRIDE;
  bool NoteNewRevision() CM_OVERRIDE;
  bool UpdateImpl() CM_OVERRIDE;
  bool UpdateCustom(const std::string& custom);

  bool LoadRevisions() CM_OVERRIDE;
  bool LoadModifications() CM_OVERRIDE;

  class ChangesParser;
  class DescribeParser;
  class DiffParser;
  // Parsing helper classes.
  class IdentifyParser;
  class UserParser;

  friend class IdentifyParser;
  friend class ChangesParser;
  friend class UserParser;
  friend class DescribeParser;
  friend class DiffParser;
};

#endif
