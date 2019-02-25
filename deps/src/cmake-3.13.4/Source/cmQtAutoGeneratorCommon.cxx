/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmQtAutoGeneratorCommon.h"
#include "cmAlgorithms.h"
#include "cmSystemTools.h"

#include "cmsys/FStream.hxx"
#include "cmsys/RegularExpression.hxx"

#include <sstream>
#include <stddef.h>

// - Static functions

static std::string utilStripCR(std::string const& line)
{
  // Strip CR characters rcc may have printed (possibly more than one!).
  std::string::size_type cr = line.find('\r');
  if (cr != std::string::npos) {
    return line.substr(0, cr);
  }
  return line;
}

/// @brief Reads the resource files list from from a .qrc file - Qt4 version
/// @return True if the .qrc file was successfully parsed
static bool RccListInputsQt4(const std::string& fileName,
                             std::vector<std::string>& files,
                             std::string* errorMessage)
{
  bool allGood = true;
  // Read qrc file content into string
  std::string qrcContents;
  {
    cmsys::ifstream ifs(fileName.c_str());
    if (ifs) {
      std::ostringstream osst;
      osst << ifs.rdbuf();
      qrcContents = osst.str();
    } else {
      if (errorMessage != CM_NULLPTR) {
        std::ostringstream ost;
        ost << "AutoRcc: Error: Rcc file not readable:\n"
            << cmQtAutoGeneratorCommon::Quoted(fileName) << "\n";
        *errorMessage = ost.str();
      }
      allGood = false;
    }
  }
  if (allGood) {
    // qrc file directory
    std::string qrcDir(cmsys::SystemTools::GetFilenamePath(fileName));
    if (!qrcDir.empty()) {
      qrcDir += '/';
    }

    cmsys::RegularExpression fileMatchRegex("(<file[^<]+)");
    cmsys::RegularExpression fileReplaceRegex("(^<file[^>]*>)");

    size_t offset = 0;
    while (fileMatchRegex.find(qrcContents.c_str() + offset)) {
      std::string qrcEntry = fileMatchRegex.match(1);
      offset += qrcEntry.size();
      {
        fileReplaceRegex.find(qrcEntry);
        std::string tag = fileReplaceRegex.match(1);
        qrcEntry = qrcEntry.substr(tag.size());
      }
      if (!cmSystemTools::FileIsFullPath(qrcEntry.c_str())) {
        qrcEntry = qrcDir + qrcEntry;
      }
      files.push_back(qrcEntry);
    }
  }
  return allGood;
}

/// @brief Reads the resource files list from from a .qrc file - Qt5 version
/// @return True if the .qrc file was successfully parsed
static bool RccListInputsQt5(const std::string& rccCommand,
                             const std::string& fileName,
                             std::vector<std::string>& files,
                             std::string* errorMessage)
{
  if (rccCommand.empty()) {
    cmSystemTools::Error("AutoRcc: Error: rcc executable not available\n");
    return false;
  }

  // Read rcc features
  bool hasDashDashList = false;
  {
    std::vector<std::string> command;
    command.push_back(rccCommand);
    command.push_back("--help");
    std::string rccStdOut;
    std::string rccStdErr;
    int retVal = 0;
    bool result =
      cmSystemTools::RunSingleCommand(command, &rccStdOut, &rccStdErr, &retVal,
                                      CM_NULLPTR, cmSystemTools::OUTPUT_NONE);
    if (result && retVal == 0 &&
        rccStdOut.find("--list") != std::string::npos) {
      hasDashDashList = true;
    }
  }

  // Run rcc list command
  bool result = false;
  int retVal = 0;
  std::string rccStdOut;
  std::string rccStdErr;
  {
    std::vector<std::string> command;
    command.push_back(rccCommand);
    command.push_back(hasDashDashList ? "--list" : "-list");
    command.push_back(fileName);
    result =
      cmSystemTools::RunSingleCommand(command, &rccStdOut, &rccStdErr, &retVal,
                                      CM_NULLPTR, cmSystemTools::OUTPUT_NONE);
  }
  if (!result || retVal) {
    if (errorMessage != CM_NULLPTR) {
      std::ostringstream ost;
      ost << "AutoRcc: Error: Rcc list process for " << fileName
          << " failed:\n"
          << rccStdOut << "\n"
          << rccStdErr << "\n";
      *errorMessage = ost.str();
    }
    return false;
  }

  // Parse rcc std output
  {
    std::istringstream ostr(rccStdOut);
    std::string oline;
    while (std::getline(ostr, oline)) {
      oline = utilStripCR(oline);
      if (!oline.empty()) {
        files.push_back(oline);
      }
    }
  }
  // Parse rcc error output
  {
    std::istringstream estr(rccStdErr);
    std::string eline;
    while (std::getline(estr, eline)) {
      eline = utilStripCR(eline);
      if (cmHasLiteralPrefix(eline, "RCC: Error in")) {
        static std::string searchString = "Cannot find file '";

        std::string::size_type pos = eline.find(searchString);
        if (pos == std::string::npos) {
          if (errorMessage != CM_NULLPTR) {
            std::ostringstream ost;
            ost << "AutoRcc: Error: Rcc lists unparsable output:\n"
                << cmQtAutoGeneratorCommon::Quoted(eline) << "\n";
            *errorMessage = ost.str();
          }
          return false;
        }
        pos += searchString.length();
        std::string::size_type sz = eline.size() - pos - 1;
        files.push_back(eline.substr(pos, sz));
      }
    }
  }

  return true;
}

// - Class definitions

const char* cmQtAutoGeneratorCommon::listSep = "@LSEP@";

std::string cmQtAutoGeneratorCommon::Quoted(const std::string& text)
{
  static const char* rep[18] = { "\\", "\\\\", "\"", "\\\"", "\a", "\\a",
                                 "\b", "\\b",  "\f", "\\f",  "\n", "\\n",
                                 "\r", "\\r",  "\t", "\\t",  "\v", "\\v" };

  std::string res = text;
  for (const char* const* it = cmArrayBegin(rep); it != cmArrayEnd(rep);
       it += 2) {
    cmSystemTools::ReplaceString(res, *it, *(it + 1));
  }
  res = '"' + res;
  res += '"';
  return res;
}

bool cmQtAutoGeneratorCommon::RccListInputs(const std::string& qtMajorVersion,
                                            const std::string& rccCommand,
                                            const std::string& fileName,
                                            std::vector<std::string>& files,
                                            std::string* errorMessage)
{
  bool allGood = false;
  if (cmsys::SystemTools::FileExists(fileName.c_str())) {
    if (qtMajorVersion == "4") {
      allGood = RccListInputsQt4(fileName, files, errorMessage);
    } else {
      allGood = RccListInputsQt5(rccCommand, fileName, files, errorMessage);
    }
  } else {
    if (errorMessage != CM_NULLPTR) {
      std::ostringstream ost;
      ost << "AutoRcc: Error: Rcc file does not exist:\n"
          << cmQtAutoGeneratorCommon::Quoted(fileName) << "\n";
      *errorMessage = ost.str();
    }
  }
  return allGood;
}
