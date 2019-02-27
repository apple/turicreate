/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackDragNDropGenerator_h
#define cmCPackDragNDropGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <sstream>
#include <stddef.h>
#include <string>
#include <vector>

#include "cmCPackGenerator.h"

class cmGeneratedFileStream;

/** \class cmCPackDragNDropGenerator
 * \brief A generator for OSX drag-n-drop installs
 */
class cmCPackDragNDropGenerator : public cmCPackGenerator
{
public:
  cmCPackTypeMacro(cmCPackDragNDropGenerator, cmCPackGenerator);

  cmCPackDragNDropGenerator();
  ~cmCPackDragNDropGenerator() override;

protected:
  int InitializeInternal() override;
  const char* GetOutputExtension() override;
  int PackageFiles() override;
  bool SupportsComponentInstallation() const override;

  bool CopyFile(std::ostringstream& source, std::ostringstream& target);
  bool CreateEmptyFile(std::ostringstream& target, size_t size);
  bool RunCommand(std::ostringstream& command, std::string* output = 0);

  std::string GetComponentInstallDirNameSuffix(
    const std::string& componentName) override;

  int CreateDMG(const std::string& src_dir, const std::string& output_file);

private:
  std::string slaDirectory;
  bool singleLicense;

  bool WriteLicense(cmGeneratedFileStream& outputStream, int licenseNumber,
                    std::string licenseLanguage,
                    const std::string& licenseFile, std::string* error);
  bool BreakLongLine(const std::string& line, std::vector<std::string>& lines,
                     std::string* error);
  void EscapeQuotesAndBackslashes(std::string& line);
};

#endif
