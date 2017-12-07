/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmWIXAccessControlList_h
#define cmWIXAccessControlList_h

#include "cmWIXSourceWriter.h"

#include "cmCPackLog.h"
#include "cmInstalledFile.h"

class cmWIXAccessControlList
{
public:
  cmWIXAccessControlList(cmCPackLog* logger,
                         cmInstalledFile const& installedFile,
                         cmWIXSourceWriter& sourceWriter);

  bool Apply();

private:
  void CreatePermissionElement(std::string const& entry);

  void ReportError(std::string const& entry, std::string const& message);

  bool IsBooleanAttribute(std::string const& name);

  void EmitBooleanAttribute(std::string const& entry, std::string const& name);

  cmCPackLog* Logger;
  cmInstalledFile const& InstalledFile;
  cmWIXSourceWriter& SourceWriter;
};

#endif
