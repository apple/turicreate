/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCMakeHostSystemInformationCommand.h"

#include <sstream>

#include "cmMakefile.h"
#include "cmsys/SystemInformation.hxx"

#if defined(_WIN32)
#include "cmSystemTools.h"
#include "cmVSSetupHelper.h"
#define HAVE_VS_SETUP_HELPER
#endif

class cmExecutionStatus;

// cmCMakeHostSystemInformation
bool cmCMakeHostSystemInformationCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  size_t current_index = 0;

  if (args.size() < (current_index + 2) || args[current_index] != "RESULT") {
    this->SetError("missing RESULT specification.");
    return false;
  }

  std::string const& variable = args[current_index + 1];
  current_index += 2;

  if (args.size() < (current_index + 2) || args[current_index] != "QUERY") {
    this->SetError("missing QUERY specification");
    return false;
  }

  cmsys::SystemInformation info;
  info.RunCPUCheck();
  info.RunOSCheck();
  info.RunMemoryCheck();

  std::string result_list;
  for (size_t i = current_index + 1; i < args.size(); ++i) {
    std::string const& key = args[i];
    if (i != current_index + 1) {
      result_list += ";";
    }
    std::string value;
    if (!this->GetValue(info, key, value)) {
      return false;
    }
    result_list += value;
  }

  this->Makefile->AddDefinition(variable, result_list.c_str());

  return true;
}

bool cmCMakeHostSystemInformationCommand::GetValue(
  cmsys::SystemInformation& info, std::string const& key, std::string& value)
{
  if (key == "NUMBER_OF_LOGICAL_CORES") {
    value = this->ValueToString(info.GetNumberOfLogicalCPU());
  } else if (key == "NUMBER_OF_PHYSICAL_CORES") {
    value = this->ValueToString(info.GetNumberOfPhysicalCPU());
  } else if (key == "HOSTNAME") {
    value = this->ValueToString(info.GetHostname());
  } else if (key == "FQDN") {
    value = this->ValueToString(info.GetFullyQualifiedDomainName());
  } else if (key == "TOTAL_VIRTUAL_MEMORY") {
    value = this->ValueToString(info.GetTotalVirtualMemory());
  } else if (key == "AVAILABLE_VIRTUAL_MEMORY") {
    value = this->ValueToString(info.GetAvailableVirtualMemory());
  } else if (key == "TOTAL_PHYSICAL_MEMORY") {
    value = this->ValueToString(info.GetTotalPhysicalMemory());
  } else if (key == "AVAILABLE_PHYSICAL_MEMORY") {
    value = this->ValueToString(info.GetAvailablePhysicalMemory());
#ifdef HAVE_VS_SETUP_HELPER
  } else if (key == "VS_15_DIR") {
    cmVSSetupAPIHelper vsSetupAPIHelper;
    if (vsSetupAPIHelper.GetVSInstanceInfo(value)) {
      cmSystemTools::ConvertToUnixSlashes(value);
    }
#endif
  } else {
    std::string e = "does not recognize <key> " + key;
    this->SetError(e);
    return false;
  }

  return true;
}

std::string cmCMakeHostSystemInformationCommand::ValueToString(
  size_t value) const
{
  std::ostringstream tmp;
  tmp << value;
  return tmp.str();
}

std::string cmCMakeHostSystemInformationCommand::ValueToString(
  const char* value) const
{
  std::string safe_string = value ? value : "";
  return safe_string;
}

std::string cmCMakeHostSystemInformationCommand::ValueToString(
  std::string const& value) const
{
  return value;
}
