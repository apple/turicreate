/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackCygwinBinaryGenerator.h"

#include "cmCPackLog.h"
#include "cmGeneratedFileStream.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cmake.h"

#include "cmsys/SystemTools.hxx"

cmCPackCygwinBinaryGenerator::cmCPackCygwinBinaryGenerator()
{
}

cmCPackCygwinBinaryGenerator::~cmCPackCygwinBinaryGenerator()
{
}

int cmCPackCygwinBinaryGenerator::InitializeInternal()
{
  this->SetOptionIfNotSet("CPACK_PACKAGING_INSTALL_PREFIX", "/usr");
  this->SetOptionIfNotSet("CPACK_INCLUDE_TOPLEVEL_DIRECTORY", "0");
  return this->Superclass::InitializeInternal();
}

int cmCPackCygwinBinaryGenerator::PackageFiles()
{
  std::string packageName = this->GetOption("CPACK_PACKAGE_NAME");
  packageName += "-";
  packageName += this->GetOption("CPACK_PACKAGE_VERSION");
  packageName = cmsys::SystemTools::LowerCase(packageName);
  std::string manifest = "/usr/share/doc/";
  manifest += packageName;
  manifest += "/MANIFEST";
  std::string manifestFile = this->GetOption("CPACK_TEMPORARY_DIRECTORY");
  // Create a MANIFEST file that contains all of the files in
  // the tar file
  std::string tempdir = manifestFile;
  manifestFile += manifest;
  // create an extra scope to force the stream
  // to create the file before the super class is called
  {
    cmGeneratedFileStream ofs(manifestFile.c_str());
    for (std::vector<std::string>::const_iterator i = files.begin();
         i != files.end(); ++i) {
      // remove the temp dir and replace with /usr
      ofs << (*i).substr(tempdir.size()) << "\n";
    }
    ofs << manifest << "\n";
  }
  // add the manifest file to the list of all files
  files.push_back(manifestFile);

  // create the bzip2 tar file
  return this->Superclass::PackageFiles();
}

const char* cmCPackCygwinBinaryGenerator::GetOutputExtension()
{
  this->OutputExtension = "-";
  const char* patchNumber = this->GetOption("CPACK_CYGWIN_PATCH_NUMBER");
  if (!patchNumber) {
    patchNumber = "1";
    cmCPackLogger(cmCPackLog::LOG_WARNING,
                  "CPACK_CYGWIN_PATCH_NUMBER not specified using 1"
                    << std::endl);
  }
  this->OutputExtension += patchNumber;
  this->OutputExtension += ".tar.bz2";
  return this->OutputExtension.c_str();
}
