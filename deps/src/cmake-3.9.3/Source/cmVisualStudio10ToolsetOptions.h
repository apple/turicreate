/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmVisualStudio10ToolsetOptions_h
#define cmVisualStudio10ToolsetOptions_h

#include "cmConfigure.h"

#include <string>

struct cmIDEFlagTable;

/** \class cmVisualStudio10ToolsetOptions
 * \brief Retrieves toolset options for MSBuild.
 *
 * cmVisualStudio10ToolsetOptions manages toolsets within MSBuild
 */
class cmVisualStudio10ToolsetOptions
{
public:
  cmIDEFlagTable const* GetClFlagTable(std::string const& name,
                                       std::string const& toolset) const;
  cmIDEFlagTable const* GetCSharpFlagTable(std::string const& name,
                                           std::string const& toolset) const;
  cmIDEFlagTable const* GetRcFlagTable(std::string const& name,
                                       std::string const& toolset) const;
  cmIDEFlagTable const* GetLibFlagTable(std::string const& name,
                                        std::string const& toolset) const;
  cmIDEFlagTable const* GetLinkFlagTable(std::string const& name,
                                         std::string const& toolset) const;
  cmIDEFlagTable const* GetMasmFlagTable(std::string const& name,
                                         std::string const& toolset) const;

private:
  std::string GetToolsetName(std::string const& name,
                             std::string const& toolset) const;
};
#endif
