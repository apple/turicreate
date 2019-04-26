/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCMakeHostSystemInformationCommand_h
#define cmCMakeHostSystemInformationCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <stddef.h>
#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;
namespace cmsys {
class SystemInformation;
} // namespace cmsys

/** \class cmCMakeHostSystemInformationCommand
 * \brief Query host system specific information
 *
 * cmCMakeHostSystemInformationCommand queries system information of
 * the system on which CMake runs.
 */
class cmCMakeHostSystemInformationCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override
  {
    return new cmCMakeHostSystemInformationCommand;
  }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

private:
  bool GetValue(cmsys::SystemInformation& info, std::string const& key,
                std::string& value);

  std::string ValueToString(size_t value) const;
  std::string ValueToString(const char* value) const;
  std::string ValueToString(std::string const& value) const;
};

#endif
