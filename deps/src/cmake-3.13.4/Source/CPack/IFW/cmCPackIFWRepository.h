/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackIFWRepository_h
#define cmCPackIFWRepository_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCPackIFWCommon.h"

#include <string>
#include <vector>

class cmXMLWriter;

/** \class cmCPackIFWRepository
 * \brief A remote repository to be created CPack IFW generator
 */
class cmCPackIFWRepository : public cmCPackIFWCommon
{
public:
  // Types

  enum Action
  {
    None,
    Add,
    Remove,
    Replace
  };

  typedef std::vector<cmCPackIFWRepository*> RepositoriesVector;

public:
  // Constructor

  /**
   * Construct repository
   */
  cmCPackIFWRepository();

public:
  // Configuration

  /// Internal repository name
  std::string Name;

  /// Optional update action
  Action Update;

  /// Is points to a list of available components
  std::string Url;

  /// Is points to a list that will replaced
  std::string OldUrl;

  /// Is points to a list that will replace to
  std::string NewUrl;

  /// With "0" disabling this repository
  std::string Enabled;

  /// Is used as user on a protected repository
  std::string Username;

  /// Is password to use on a protected repository
  std::string Password;

  /// Is optional string to display instead of the URL
  std::string DisplayName;

public:
  // Internal implementation

  bool IsValid() const;

  bool ConfigureFromOptions();

  bool PatchUpdatesXml();

  void WriteRepositoryConfig(cmXMLWriter& xout);
  void WriteRepositoryUpdate(cmXMLWriter& xout);
  void WriteRepositoryUpdates(cmXMLWriter& xout);

  RepositoriesVector RepositoryUpdate;
  std::string Directory;
};

#endif // cmCPackIFWRepository_h
