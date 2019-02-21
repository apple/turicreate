/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmVisualStudio10ToolsetOptions.h"

#include "cmAlgorithms.h"
#include "cmIDEFlagTable.h"
#include "cmVisualStudioGeneratorOptions.h"

#include "cmVS10CLFlagTable.h"
#include "cmVS10CSharpFlagTable.h"
#include "cmVS10LibFlagTable.h"
#include "cmVS10LinkFlagTable.h"
#include "cmVS10MASMFlagTable.h"
#include "cmVS10RCFlagTable.h"
#include "cmVS11CLFlagTable.h"
#include "cmVS11CSharpFlagTable.h"
#include "cmVS11LibFlagTable.h"
#include "cmVS11LinkFlagTable.h"
#include "cmVS11MASMFlagTable.h"
#include "cmVS11RCFlagTable.h"
#include "cmVS12CLFlagTable.h"
#include "cmVS12CSharpFlagTable.h"
#include "cmVS12LibFlagTable.h"
#include "cmVS12LinkFlagTable.h"
#include "cmVS12MASMFlagTable.h"
#include "cmVS12RCFlagTable.h"
#include "cmVS140CLFlagTable.h"
#include "cmVS140CSharpFlagTable.h"
#include "cmVS140LinkFlagTable.h"
#include "cmVS141CLFlagTable.h"
#include "cmVS141CSharpFlagTable.h"
#include "cmVS141LinkFlagTable.h"
#include "cmVS14LibFlagTable.h"
#include "cmVS14MASMFlagTable.h"
#include "cmVS14RCFlagTable.h"

cmIDEFlagTable const* cmVisualStudio10ToolsetOptions::GetClFlagTable(
  std::string const& name, std::string const& toolset) const
{
  std::string const useToolset = this->GetToolsetName(name, toolset);

  if (toolset == "v141") {
    return cmVS141CLFlagTable;
  } else if (useToolset == "v140") {
    return cmVS140CLFlagTable;
  } else if (useToolset == "v120") {
    return cmVS12CLFlagTable;
  } else if (useToolset == "v110") {
    return cmVS11CLFlagTable;
  } else if (useToolset == "v100") {
    return cmVS10CLFlagTable;
  } else {
    return 0;
  }
}

cmIDEFlagTable const* cmVisualStudio10ToolsetOptions::GetCSharpFlagTable(
  std::string const& name, std::string const& toolset) const
{
  std::string const useToolset = this->GetToolsetName(name, toolset);

  if ((useToolset == "v141")) {
    return cmVS141CSharpFlagTable;
  } else if (useToolset == "v140") {
    return cmVS140CSharpFlagTable;
  } else if (useToolset == "v120") {
    return cmVS12CSharpFlagTable;
  } else if (useToolset == "v110") {
    return cmVS11CSharpFlagTable;
  } else if (useToolset == "v100") {
    return cmVS10CSharpFlagTable;
  } else {
    return 0;
  }
}

cmIDEFlagTable const* cmVisualStudio10ToolsetOptions::GetRcFlagTable(
  std::string const& name, std::string const& toolset) const
{
  std::string const useToolset = this->GetToolsetName(name, toolset);

  if ((useToolset == "v140") || (useToolset == "v141")) {
    return cmVS14RCFlagTable;
  } else if (useToolset == "v120") {
    return cmVS12RCFlagTable;
  } else if (useToolset == "v110") {
    return cmVS11RCFlagTable;
  } else if (useToolset == "v100") {
    return cmVS10RCFlagTable;
  } else {
    return 0;
  }
}

cmIDEFlagTable const* cmVisualStudio10ToolsetOptions::GetLibFlagTable(
  std::string const& name, std::string const& toolset) const
{
  std::string const useToolset = this->GetToolsetName(name, toolset);

  if ((useToolset == "v140") || (useToolset == "v141")) {
    return cmVS14LibFlagTable;
  } else if (useToolset == "v120") {
    return cmVS12LibFlagTable;
  } else if (useToolset == "v110") {
    return cmVS11LibFlagTable;
  } else if (useToolset == "v100") {
    return cmVS10LibFlagTable;
  } else {
    return 0;
  }
}

cmIDEFlagTable const* cmVisualStudio10ToolsetOptions::GetLinkFlagTable(
  std::string const& name, std::string const& toolset) const
{
  std::string const useToolset = this->GetToolsetName(name, toolset);

  if (useToolset == "v141") {
    return cmVS141LinkFlagTable;
  } else if (useToolset == "v140") {
    return cmVS140LinkFlagTable;
  } else if (useToolset == "v120") {
    return cmVS12LinkFlagTable;
  } else if (useToolset == "v110") {
    return cmVS11LinkFlagTable;
  } else if (useToolset == "v100") {
    return cmVS10LinkFlagTable;
  } else {
    return 0;
  }
}

cmIDEFlagTable const* cmVisualStudio10ToolsetOptions::GetMasmFlagTable(
  std::string const& name, std::string const& toolset) const
{
  std::string const useToolset = this->GetToolsetName(name, toolset);

  if ((useToolset == "v140") || (useToolset == "v141")) {
    return cmVS14MASMFlagTable;
  } else if (useToolset == "v120") {
    return cmVS12MASMFlagTable;
  } else if (useToolset == "v110") {
    return cmVS11MASMFlagTable;
  } else if (useToolset == "v100") {
    return cmVS10MASMFlagTable;
  } else {
    return 0;
  }
}

std::string cmVisualStudio10ToolsetOptions::GetToolsetName(
  std::string const& name, std::string const& toolset) const
{
  static_cast<void>(name);
  std::size_t length = toolset.length();

  if (cmHasLiteralSuffix(toolset, "_xp")) {
    length -= 3;
  }

  return toolset.substr(0, length);
}
