/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackSTGZGenerator.h"

#include "cmsys/FStream.hxx"
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>

#include "cmCPackGenerator.h"
#include "cmCPackLog.h"
#include "cmSystemTools.h"
#include "cm_sys_stat.h"

cmCPackSTGZGenerator::cmCPackSTGZGenerator()
{
}

cmCPackSTGZGenerator::~cmCPackSTGZGenerator()
{
}

int cmCPackSTGZGenerator::InitializeInternal()
{
  this->SetOptionIfNotSet("CPACK_INCLUDE_TOPLEVEL_DIRECTORY", "0");

  std::string inFile = this->FindTemplate("CPack.STGZ_Header.sh.in");
  if (inFile.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Cannot find template file: " << inFile << std::endl);
    return 0;
  }
  this->SetOptionIfNotSet("CPACK_STGZ_HEADER_FILE", inFile.c_str());
  this->SetOptionIfNotSet("CPACK_AT_SIGN", "@");

  return this->Superclass::InitializeInternal();
}

int cmCPackSTGZGenerator::PackageFiles()
{
  bool retval = true;
  if (!this->Superclass::PackageFiles()) {
    return 0;
  }

  /* TGZ generator (our Superclass) may
   * have generated several packages (component packaging)
   * so we must iterate over generated packages.
   */
  for (std::vector<std::string>::iterator it = packageFileNames.begin();
       it != packageFileNames.end(); ++it) {
    retval &= cmSystemTools::SetPermissions((*it).c_str(),
#if defined(_MSC_VER) || defined(__MINGW32__)
                                            S_IREAD | S_IWRITE | S_IEXEC
#else
                                            S_IRUSR | S_IWUSR | S_IXUSR |
                                              S_IRGRP | S_IWGRP | S_IXGRP |
                                              S_IROTH | S_IWOTH | S_IXOTH
#endif
                                            );
  }
  return retval;
}

int cmCPackSTGZGenerator::GenerateHeader(std::ostream* os)
{
  cmCPackLogger(cmCPackLog::LOG_DEBUG, "Writing header" << std::endl);
  std::ostringstream str;
  int counter = 0;

  std::string inLicFile = this->GetOption("CPACK_RESOURCE_FILE_LICENSE");
  std::string line;
  cmsys::ifstream ilfs(inLicFile.c_str());
  std::string licenseText;
  while (cmSystemTools::GetLineFromStream(ilfs, line)) {
    licenseText += line + "\n";
  }
  this->SetOptionIfNotSet("CPACK_RESOURCE_FILE_LICENSE_CONTENT",
                          licenseText.c_str());

  const char headerLengthTag[] = "###CPACK_HEADER_LENGTH###";

  // Create the header
  std::string inFile = this->GetOption("CPACK_STGZ_HEADER_FILE");
  cmsys::ifstream ifs(inFile.c_str());
  std::string packageHeaderText;
  while (cmSystemTools::GetLineFromStream(ifs, line)) {
    packageHeaderText += line + "\n";
  }

  // Configure in the values
  std::string res;
  this->ConfigureString(packageHeaderText, res);

  // Count the lines
  const char* ptr = res.c_str();
  while (*ptr) {
    if (*ptr == '\n') {
      counter++;
    }
    ++ptr;
  }
  counter++;
  cmCPackLogger(cmCPackLog::LOG_DEBUG, "Number of lines: " << counter
                                                           << std::endl);
  char buffer[1024];
  sprintf(buffer, "%d", counter);
  cmSystemTools::ReplaceString(res, headerLengthTag, buffer);

  // Write in file
  *os << res;
  return this->Superclass::GenerateHeader(os);
}
