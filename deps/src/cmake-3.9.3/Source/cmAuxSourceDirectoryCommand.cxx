/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmAuxSourceDirectoryCommand.h"

#include "cmsys/Directory.hxx"
#include <algorithm>
#include <stddef.h>

#include "cmAlgorithms.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmSystemTools.h"
#include "cmake.h"

class cmExecutionStatus;

// cmAuxSourceDirectoryCommand
bool cmAuxSourceDirectoryCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.size() != 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  std::string sourceListValue;
  std::string const& templateDirectory = args[0];
  std::string tdir;
  if (!cmSystemTools::FileIsFullPath(templateDirectory.c_str())) {
    tdir = this->Makefile->GetCurrentSourceDirectory();
    tdir += "/";
    tdir += templateDirectory;
  } else {
    tdir = templateDirectory;
  }

  // was the list already populated
  const char* def = this->Makefile->GetDefinition(args[1]);
  if (def) {
    sourceListValue = def;
  }

  std::vector<std::string> files;

  // Load all the files in the directory
  cmsys::Directory dir;
  if (dir.Load(tdir)) {
    size_t numfiles = dir.GetNumberOfFiles();
    for (size_t i = 0; i < numfiles; ++i) {
      std::string file = dir.GetFile(static_cast<unsigned long>(i));
      // Split the filename into base and extension
      std::string::size_type dotpos = file.rfind('.');
      if (dotpos != std::string::npos) {
        std::string ext = file.substr(dotpos + 1);
        std::string base = file.substr(0, dotpos);
        // Process only source files
        std::vector<std::string> const& srcExts =
          this->Makefile->GetCMakeInstance()->GetSourceExtensions();
        if (!base.empty() &&
            std::find(srcExts.begin(), srcExts.end(), ext) != srcExts.end()) {
          std::string fullname = templateDirectory;
          fullname += "/";
          fullname += file;
          // add the file as a class file so
          // depends can be done
          cmSourceFile* sf = this->Makefile->GetOrCreateSource(fullname);
          sf->SetProperty("ABSTRACT", "0");
          files.push_back(fullname);
        }
      }
    }
  }
  std::sort(files.begin(), files.end());
  if (!sourceListValue.empty()) {
    sourceListValue += ";";
  }
  sourceListValue += cmJoin(files, ";");
  this->Makefile->AddDefinition(args[1], sourceListValue.c_str());
  return true;
}
