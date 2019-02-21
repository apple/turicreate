/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmsys/FStream.hxx"
#include "cmsys/Process.h"
#include "cmsys/SystemTools.hxx"
#include <iostream>
#include <stddef.h>
#include <string>
#include <vector>

#include <CoreFoundation/CoreFoundation.h>

// For the PATH_MAX constant
#include <sys/syslimits.h>

#define DebugError(x)                                                         \
  ofs << x << std::endl;                                                      \
  std::cout << x << std::endl

int main(int argc, char* argv[])
{
  // if ( cmsys::SystemTools::FileExists(
  cmsys::ofstream ofs("/tmp/output.txt");

  CFStringRef fileName;
  CFBundleRef appBundle;
  CFURLRef scriptFileURL;
  UInt8* path;

  // get CF URL for script
  if (!(appBundle = CFBundleGetMainBundle())) {
    DebugError("Cannot get main bundle");
    return 1;
  }
  fileName = CFSTR("RuntimeScript");
  if (!(scriptFileURL =
          CFBundleCopyResourceURL(appBundle, fileName, nullptr, nullptr))) {
    DebugError("CFBundleCopyResourceURL failed");
    return 1;
  }

  // create path string
  if (!(path = new UInt8[PATH_MAX])) {
    return 1;
  }

  // get the file system path of the url as a cstring
  // in an encoding suitable for posix apis
  if (CFURLGetFileSystemRepresentation(scriptFileURL, true, path, PATH_MAX) ==
      false) {
    DebugError("CFURLGetFileSystemRepresentation failed");
    return 1;
  }

  // dispose of the CF variable
  CFRelease(scriptFileURL);

  std::string fullScriptPath = reinterpret_cast<char*>(path);
  delete[] path;

  if (!cmsys::SystemTools::FileExists(fullScriptPath.c_str())) {
    return 1;
  }

  std::string scriptDirectory =
    cmsys::SystemTools::GetFilenamePath(fullScriptPath);
  ofs << fullScriptPath << std::endl;
  std::vector<const char*> args;
  args.push_back(fullScriptPath.c_str());
  int cc;
  for (cc = 1; cc < argc; ++cc) {
    args.push_back(argv[cc]);
  }
  args.push_back(nullptr);

  cmsysProcess* cp = cmsysProcess_New();
  cmsysProcess_SetCommand(cp, &*args.begin());
  cmsysProcess_SetWorkingDirectory(cp, scriptDirectory.c_str());
  cmsysProcess_SetOption(cp, cmsysProcess_Option_HideWindow, 1);
  cmsysProcess_SetTimeout(cp, 0);
  cmsysProcess_Execute(cp);

  std::vector<char> tempOutput;
  char* data;
  int length;
  while (cmsysProcess_WaitForData(cp, &data, &length, nullptr)) {
    // Translate NULL characters in the output into valid text.
    for (int i = 0; i < length; ++i) {
      if (data[i] == '\0') {
        data[i] = ' ';
      }
    }
    std::cout.write(data, length);
  }

  cmsysProcess_WaitForExit(cp, nullptr);

  bool result = true;
  if (cmsysProcess_GetState(cp) == cmsysProcess_State_Exited) {
    if (cmsysProcess_GetExitValue(cp) != 0) {
      result = false;
    }
  } else if (cmsysProcess_GetState(cp) == cmsysProcess_State_Exception) {
    const char* exception_str = cmsysProcess_GetExceptionString(cp);
    std::cerr << exception_str << std::endl;
    result = false;
  } else if (cmsysProcess_GetState(cp) == cmsysProcess_State_Error) {
    const char* error_str = cmsysProcess_GetErrorString(cp);
    std::cerr << error_str << std::endl;
    result = false;
  } else if (cmsysProcess_GetState(cp) == cmsysProcess_State_Expired) {
    const char* error_str = "Process terminated due to timeout\n";
    std::cerr << error_str << std::endl;
    result = false;
  }

  cmsysProcess_Delete(cp);

  return 0;
}
