/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCMakeHostSystemInformationCommand.h"

#include <sstream>

#include "cmMakefile.h"
#include "cmsys/SystemInformation.hxx"

#if defined(_WIN32)
#  include "cmAlgorithms.h"
#  include "cmGlobalGenerator.h"
#  include "cmGlobalVisualStudio15Generator.h"
#  include "cmSystemTools.h"
#  include "cmVSSetupHelper.h"
#  define HAVE_VS_SETUP_HELPER
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
  } else if (key == "IS_64BIT") {
    value = this->ValueToString(info.Is64Bits());
  } else if (key == "HAS_FPU") {
    value = this->ValueToString(
      info.DoesCPUSupportFeature(cmsys::SystemInformation::CPU_FEATURE_FPU));
  } else if (key == "HAS_MMX") {
    value = this->ValueToString(
      info.DoesCPUSupportFeature(cmsys::SystemInformation::CPU_FEATURE_MMX));
  } else if (key == "HAS_MMX_PLUS") {
    value = this->ValueToString(info.DoesCPUSupportFeature(
      cmsys::SystemInformation::CPU_FEATURE_MMX_PLUS));
  } else if (key == "HAS_SSE") {
    value = this->ValueToString(
      info.DoesCPUSupportFeature(cmsys::SystemInformation::CPU_FEATURE_SSE));
  } else if (key == "HAS_SSE2") {
    value = this->ValueToString(
      info.DoesCPUSupportFeature(cmsys::SystemInformation::CPU_FEATURE_SSE2));
  } else if (key == "HAS_SSE_FP") {
    value = this->ValueToString(info.DoesCPUSupportFeature(
      cmsys::SystemInformation::CPU_FEATURE_SSE_FP));
  } else if (key == "HAS_SSE_MMX") {
    value = this->ValueToString(info.DoesCPUSupportFeature(
      cmsys::SystemInformation::CPU_FEATURE_SSE_MMX));
  } else if (key == "HAS_AMD_3DNOW") {
    value = this->ValueToString(info.DoesCPUSupportFeature(
      cmsys::SystemInformation::CPU_FEATURE_AMD_3DNOW));
  } else if (key == "HAS_AMD_3DNOW_PLUS") {
    value = this->ValueToString(info.DoesCPUSupportFeature(
      cmsys::SystemInformation::CPU_FEATURE_AMD_3DNOW_PLUS));
  } else if (key == "HAS_IA64") {
    value = this->ValueToString(
      info.DoesCPUSupportFeature(cmsys::SystemInformation::CPU_FEATURE_IA64));
  } else if (key == "HAS_SERIAL_NUMBER") {
    value = this->ValueToString(info.DoesCPUSupportFeature(
      cmsys::SystemInformation::CPU_FEATURE_SERIALNUMBER));
  } else if (key == "PROCESSOR_NAME") {
    value = this->ValueToString(info.GetExtendedProcessorName());
  } else if (key == "PROCESSOR_DESCRIPTION") {
    value = info.GetCPUDescription();
  } else if (key == "PROCESSOR_SERIAL_NUMBER") {
    value = this->ValueToString(info.GetProcessorSerialNumber());
  } else if (key == "OS_NAME") {
    value = this->ValueToString(info.GetOSName());
  } else if (key == "OS_RELEASE") {
    value = this->ValueToString(info.GetOSRelease());
  } else if (key == "OS_VERSION") {
    value = this->ValueToString(info.GetOSVersion());
  } else if (key == "OS_PLATFORM") {
    value = this->ValueToString(info.GetOSPlatform());
#ifdef HAVE_VS_SETUP_HELPER
  } else if (key == "VS_15_DIR") {
    // If generating for the VS 15 IDE, use the same instance.
    cmGlobalGenerator* gg = this->Makefile->GetGlobalGenerator();
    if (cmHasLiteralPrefix(gg->GetName(), "Visual Studio 15 ")) {
      cmGlobalVisualStudio15Generator* vs15gen =
        static_cast<cmGlobalVisualStudio15Generator*>(gg);
      if (vs15gen->GetVSInstance(value)) {
        return true;
      }
    }

    // Otherwise, find a VS 15 instance ourselves.
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
