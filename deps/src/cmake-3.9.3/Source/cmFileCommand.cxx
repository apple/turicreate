/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFileCommand.h"

#include "cm_kwiml.h"
#include "cmsys/Directory.hxx"
#include "cmsys/FStream.hxx"
#include "cmsys/Glob.hxx"
#include "cmsys/RegularExpression.hxx"
#include "cmsys/String.hxx"
#include <algorithm>
#include <assert.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "cmAlgorithms.h"
#include "cmCommandArgumentsHelper.h"
#include "cmCryptoHash.h"
#include "cmFileLockPool.h"
#include "cmFileTimeComparison.h"
#include "cmGeneratorExpression.h"
#include "cmGlobalGenerator.h"
#include "cmHexFileConverter.h"
#include "cmInstallType.h"
#include "cmListFileCache.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmSystemTools.h"
#include "cmTimestamp.h"
#include "cm_auto_ptr.hxx"
#include "cm_sys_stat.h"
#include "cmake.h"

#if defined(CMAKE_BUILD_WITH_CMAKE)
#include "cmCurl.h"
#include "cmFileLockResult.h"
#include "cm_curl.h"
#endif

#if defined(CMAKE_USE_ELF_PARSER)
#include "cmELF.h"
#endif

#if defined(_WIN32)
#include <windows.h>
#endif

class cmSystemToolsFileTime;

// Table of permissions flags.
#if defined(_WIN32) && !defined(__CYGWIN__)
static mode_t mode_owner_read = S_IREAD;
static mode_t mode_owner_write = S_IWRITE;
static mode_t mode_owner_execute = S_IEXEC;
static mode_t mode_group_read = 0;
static mode_t mode_group_write = 0;
static mode_t mode_group_execute = 0;
static mode_t mode_world_read = 0;
static mode_t mode_world_write = 0;
static mode_t mode_world_execute = 0;
static mode_t mode_setuid = 0;
static mode_t mode_setgid = 0;
#else
static mode_t mode_owner_read = S_IRUSR;
static mode_t mode_owner_write = S_IWUSR;
static mode_t mode_owner_execute = S_IXUSR;
static mode_t mode_group_read = S_IRGRP;
static mode_t mode_group_write = S_IWGRP;
static mode_t mode_group_execute = S_IXGRP;
static mode_t mode_world_read = S_IROTH;
static mode_t mode_world_write = S_IWOTH;
static mode_t mode_world_execute = S_IXOTH;
static mode_t mode_setuid = S_ISUID;
static mode_t mode_setgid = S_ISGID;
#endif

#if defined(_WIN32)
// libcurl doesn't support file:// urls for unicode filenames on Windows.
// Convert string from UTF-8 to ACP if this is a file:// URL.
static std::string fix_file_url_windows(const std::string& url)
{
  std::string ret = url;
  if (strncmp(url.c_str(), "file://", 7) == 0) {
    std::wstring wurl = cmsys::Encoding::ToWide(url);
    if (!wurl.empty()) {
      int mblen =
        WideCharToMultiByte(CP_ACP, 0, wurl.c_str(), -1, NULL, 0, NULL, NULL);
      if (mblen > 0) {
        std::vector<char> chars(mblen);
        mblen = WideCharToMultiByte(CP_ACP, 0, wurl.c_str(), -1, &chars[0],
                                    mblen, NULL, NULL);
        if (mblen > 0) {
          ret = &chars[0];
        }
      }
    }
  }
  return ret;
}
#endif

// cmLibraryCommand
bool cmFileCommand::InitialPass(std::vector<std::string> const& args,
                                cmExecutionStatus&)
{
  if (args.size() < 2) {
    this->SetError("must be called with at least two arguments.");
    return false;
  }
  std::string const& subCommand = args[0];
  if (subCommand == "WRITE") {
    return this->HandleWriteCommand(args, false);
  }
  if (subCommand == "APPEND") {
    return this->HandleWriteCommand(args, true);
  }
  if (subCommand == "DOWNLOAD") {
    return this->HandleDownloadCommand(args);
  }
  if (subCommand == "UPLOAD") {
    return this->HandleUploadCommand(args);
  }
  if (subCommand == "READ") {
    return this->HandleReadCommand(args);
  }
  if (subCommand == "MD5" || subCommand == "SHA1" || subCommand == "SHA224" ||
      subCommand == "SHA256" || subCommand == "SHA384" ||
      subCommand == "SHA512" || subCommand == "SHA3_224" ||
      subCommand == "SHA3_256" || subCommand == "SHA3_384" ||
      subCommand == "SHA3_512") {
    return this->HandleHashCommand(args);
  }
  if (subCommand == "STRINGS") {
    return this->HandleStringsCommand(args);
  }
  if (subCommand == "GLOB") {
    return this->HandleGlobCommand(args, false);
  }
  if (subCommand == "GLOB_RECURSE") {
    return this->HandleGlobCommand(args, true);
  }
  if (subCommand == "MAKE_DIRECTORY") {
    return this->HandleMakeDirectoryCommand(args);
  }
  if (subCommand == "RENAME") {
    return this->HandleRename(args);
  }
  if (subCommand == "REMOVE") {
    return this->HandleRemove(args, false);
  }
  if (subCommand == "REMOVE_RECURSE") {
    return this->HandleRemove(args, true);
  }
  if (subCommand == "COPY") {
    return this->HandleCopyCommand(args);
  }
  if (subCommand == "INSTALL") {
    return this->HandleInstallCommand(args);
  }
  if (subCommand == "DIFFERENT") {
    return this->HandleDifferentCommand(args);
  }
  if (subCommand == "RPATH_CHANGE" || subCommand == "CHRPATH") {
    return this->HandleRPathChangeCommand(args);
  }
  if (subCommand == "RPATH_CHECK") {
    return this->HandleRPathCheckCommand(args);
  }
  if (subCommand == "RPATH_REMOVE") {
    return this->HandleRPathRemoveCommand(args);
  }
  if (subCommand == "READ_ELF") {
    return this->HandleReadElfCommand(args);
  }
  if (subCommand == "RELATIVE_PATH") {
    return this->HandleRelativePathCommand(args);
  }
  if (subCommand == "TO_CMAKE_PATH") {
    return this->HandleCMakePathCommand(args, false);
  }
  if (subCommand == "TO_NATIVE_PATH") {
    return this->HandleCMakePathCommand(args, true);
  }
  if (subCommand == "TIMESTAMP") {
    return this->HandleTimestampCommand(args);
  }
  if (subCommand == "GENERATE") {
    return this->HandleGenerateCommand(args);
  }
  if (subCommand == "LOCK") {
    return this->HandleLockCommand(args);
  }

  std::string e = "does not recognize sub-command " + subCommand;
  this->SetError(e);
  return false;
}

bool cmFileCommand::HandleWriteCommand(std::vector<std::string> const& args,
                                       bool append)
{
  std::vector<std::string>::const_iterator i = args.begin();

  i++; // Get rid of subcommand

  std::string fileName = *i;
  if (!cmsys::SystemTools::FileIsFullPath(i->c_str())) {
    fileName = this->Makefile->GetCurrentSourceDirectory();
    fileName += "/" + *i;
  }

  i++;

  if (!this->Makefile->CanIWriteThisFile(fileName.c_str())) {
    std::string e =
      "attempted to write a file: " + fileName + " into a source directory.";
    this->SetError(e);
    cmSystemTools::SetFatalErrorOccured();
    return false;
  }
  std::string dir = cmSystemTools::GetFilenamePath(fileName);
  cmSystemTools::MakeDirectory(dir.c_str());

  mode_t mode = 0;

  // Set permissions to writable
  if (cmSystemTools::GetPermissions(fileName.c_str(), mode)) {
    cmSystemTools::SetPermissions(fileName.c_str(),
#if defined(_MSC_VER) || defined(__MINGW32__)
                                  mode | S_IWRITE
#else
                                  mode | S_IWUSR | S_IWGRP
#endif
                                  );
  }
  // If GetPermissions fails, pretend like it is ok. File open will fail if
  // the file is not writable
  cmsys::ofstream file(fileName.c_str(),
                       append ? std::ios::app : std::ios::out);
  if (!file) {
    std::string error = "failed to open for writing (";
    error += cmSystemTools::GetLastSystemError();
    error += "):\n  ";
    error += fileName;
    this->SetError(error);
    return false;
  }
  std::string message = cmJoin(cmMakeRange(i, args.end()), std::string());
  file << message;
  file.close();
  if (mode) {
    cmSystemTools::SetPermissions(fileName.c_str(), mode);
  }
  return true;
}

bool cmFileCommand::HandleReadCommand(std::vector<std::string> const& args)
{
  if (args.size() < 3) {
    this->SetError("READ must be called with at least two additional "
                   "arguments");
    return false;
  }

  cmCommandArgumentsHelper argHelper;
  cmCommandArgumentGroup group;

  cmCAString readArg(&argHelper, "READ");
  cmCAString fileNameArg(&argHelper, CM_NULLPTR);
  cmCAString resultArg(&argHelper, CM_NULLPTR);

  cmCAString offsetArg(&argHelper, "OFFSET", &group);
  cmCAString limitArg(&argHelper, "LIMIT", &group);
  cmCAEnabler hexOutputArg(&argHelper, "HEX", &group);
  readArg.Follows(CM_NULLPTR);
  fileNameArg.Follows(&readArg);
  resultArg.Follows(&fileNameArg);
  group.Follows(&resultArg);
  argHelper.Parse(&args, CM_NULLPTR);

  std::string fileName = fileNameArg.GetString();
  if (!cmsys::SystemTools::FileIsFullPath(fileName.c_str())) {
    fileName = this->Makefile->GetCurrentSourceDirectory();
    fileName += "/" + fileNameArg.GetString();
  }

  std::string variable = resultArg.GetString();

// Open the specified file.
#if defined(_WIN32) || defined(__CYGWIN__)
  cmsys::ifstream file(
    fileName.c_str(), std::ios::in |
      (hexOutputArg.IsEnabled() ? std::ios::binary : std::ios::in));
#else
  cmsys::ifstream file(fileName.c_str());
#endif

  if (!file) {
    std::string error = "failed to open for reading (";
    error += cmSystemTools::GetLastSystemError();
    error += "):\n  ";
    error += fileName;
    this->SetError(error);
    return false;
  }

  // is there a limit?
  long sizeLimit = -1;
  if (!limitArg.GetString().empty()) {
    sizeLimit = atoi(limitArg.GetCString());
  }

  // is there an offset?
  long offset = 0;
  if (!offsetArg.GetString().empty()) {
    offset = atoi(offsetArg.GetCString());
  }

  file.seekg(offset, std::ios::beg); // explicit ios::beg for IBM VisualAge 6

  std::string output;

  if (hexOutputArg.IsEnabled()) {
    // Convert part of the file into hex code
    char c;
    while ((sizeLimit != 0) && (file.get(c))) {
      char hex[4];
      sprintf(hex, "%.2x", c & 0xff);
      output += hex;
      if (sizeLimit > 0) {
        sizeLimit--;
      }
    }
  } else {
    std::string line;
    bool has_newline = false;
    while (sizeLimit != 0 && cmSystemTools::GetLineFromStream(
                               file, line, &has_newline, sizeLimit)) {
      if (sizeLimit > 0) {
        sizeLimit = sizeLimit - static_cast<long>(line.size());
        if (has_newline) {
          sizeLimit--;
        }
        if (sizeLimit < 0) {
          sizeLimit = 0;
        }
      }
      output += line;
      if (has_newline) {
        output += "\n";
      }
    }
  }
  this->Makefile->AddDefinition(variable, output.c_str());
  return true;
}

bool cmFileCommand::HandleHashCommand(std::vector<std::string> const& args)
{
#if defined(CMAKE_BUILD_WITH_CMAKE)
  if (args.size() != 3) {
    std::ostringstream e;
    e << args[0] << " requires a file name and output variable";
    this->SetError(e.str());
    return false;
  }

  CM_AUTO_PTR<cmCryptoHash> hash(cmCryptoHash::New(args[0].c_str()));
  if (hash.get()) {
    std::string out = hash->HashFile(args[1]);
    if (!out.empty()) {
      this->Makefile->AddDefinition(args[2], out.c_str());
      return true;
    }
    std::ostringstream e;
    e << args[0] << " failed to read file \"" << args[1]
      << "\": " << cmSystemTools::GetLastSystemError();
    this->SetError(e.str());
  }
  return false;
#else
  std::ostringstream e;
  e << args[0] << " not available during bootstrap";
  this->SetError(e.str().c_str());
  return false;
#endif
}

bool cmFileCommand::HandleStringsCommand(std::vector<std::string> const& args)
{
  if (args.size() < 3) {
    this->SetError("STRINGS requires a file name and output variable");
    return false;
  }

  // Get the file to read.
  std::string fileName = args[1];
  if (!cmsys::SystemTools::FileIsFullPath(fileName.c_str())) {
    fileName = this->Makefile->GetCurrentSourceDirectory();
    fileName += "/" + args[1];
  }

  // Get the variable in which to store the results.
  std::string outVar = args[2];

  // Parse the options.
  enum
  {
    arg_none,
    arg_limit_input,
    arg_limit_output,
    arg_limit_count,
    arg_length_minimum,
    arg_length_maximum,
    arg__maximum,
    arg_regex,
    arg_encoding
  };
  unsigned int minlen = 0;
  unsigned int maxlen = 0;
  int limit_input = -1;
  int limit_output = -1;
  unsigned int limit_count = 0;
  cmsys::RegularExpression regex;
  bool have_regex = false;
  bool newline_consume = false;
  bool hex_conversion_enabled = true;
  enum
  {
    encoding_none = cmsys::FStream::BOM_None,
    encoding_utf8 = cmsys::FStream::BOM_UTF8,
    encoding_utf16le = cmsys::FStream::BOM_UTF16LE,
    encoding_utf16be = cmsys::FStream::BOM_UTF16BE,
    encoding_utf32le = cmsys::FStream::BOM_UTF32LE,
    encoding_utf32be = cmsys::FStream::BOM_UTF32BE
  };
  int encoding = encoding_none;
  int arg_mode = arg_none;
  for (unsigned int i = 3; i < args.size(); ++i) {
    if (args[i] == "LIMIT_INPUT") {
      arg_mode = arg_limit_input;
    } else if (args[i] == "LIMIT_OUTPUT") {
      arg_mode = arg_limit_output;
    } else if (args[i] == "LIMIT_COUNT") {
      arg_mode = arg_limit_count;
    } else if (args[i] == "LENGTH_MINIMUM") {
      arg_mode = arg_length_minimum;
    } else if (args[i] == "LENGTH_MAXIMUM") {
      arg_mode = arg_length_maximum;
    } else if (args[i] == "REGEX") {
      arg_mode = arg_regex;
    } else if (args[i] == "NEWLINE_CONSUME") {
      newline_consume = true;
      arg_mode = arg_none;
    } else if (args[i] == "NO_HEX_CONVERSION") {
      hex_conversion_enabled = false;
      arg_mode = arg_none;
    } else if (args[i] == "ENCODING") {
      arg_mode = arg_encoding;
    } else if (arg_mode == arg_limit_input) {
      if (sscanf(args[i].c_str(), "%d", &limit_input) != 1 ||
          limit_input < 0) {
        std::ostringstream e;
        e << "STRINGS option LIMIT_INPUT value \"" << args[i]
          << "\" is not an unsigned integer.";
        this->SetError(e.str());
        return false;
      }
      arg_mode = arg_none;
    } else if (arg_mode == arg_limit_output) {
      if (sscanf(args[i].c_str(), "%d", &limit_output) != 1 ||
          limit_output < 0) {
        std::ostringstream e;
        e << "STRINGS option LIMIT_OUTPUT value \"" << args[i]
          << "\" is not an unsigned integer.";
        this->SetError(e.str());
        return false;
      }
      arg_mode = arg_none;
    } else if (arg_mode == arg_limit_count) {
      int count;
      if (sscanf(args[i].c_str(), "%d", &count) != 1 || count < 0) {
        std::ostringstream e;
        e << "STRINGS option LIMIT_COUNT value \"" << args[i]
          << "\" is not an unsigned integer.";
        this->SetError(e.str());
        return false;
      }
      limit_count = count;
      arg_mode = arg_none;
    } else if (arg_mode == arg_length_minimum) {
      int len;
      if (sscanf(args[i].c_str(), "%d", &len) != 1 || len < 0) {
        std::ostringstream e;
        e << "STRINGS option LENGTH_MINIMUM value \"" << args[i]
          << "\" is not an unsigned integer.";
        this->SetError(e.str());
        return false;
      }
      minlen = len;
      arg_mode = arg_none;
    } else if (arg_mode == arg_length_maximum) {
      int len;
      if (sscanf(args[i].c_str(), "%d", &len) != 1 || len < 0) {
        std::ostringstream e;
        e << "STRINGS option LENGTH_MAXIMUM value \"" << args[i]
          << "\" is not an unsigned integer.";
        this->SetError(e.str());
        return false;
      }
      maxlen = len;
      arg_mode = arg_none;
    } else if (arg_mode == arg_regex) {
      if (!regex.compile(args[i].c_str())) {
        std::ostringstream e;
        e << "STRINGS option REGEX value \"" << args[i]
          << "\" could not be compiled.";
        this->SetError(e.str());
        return false;
      }
      have_regex = true;
      arg_mode = arg_none;
    } else if (arg_mode == arg_encoding) {
      if (args[i] == "UTF-8") {
        encoding = encoding_utf8;
      } else if (args[i] == "UTF-16LE") {
        encoding = encoding_utf16le;
      } else if (args[i] == "UTF-16BE") {
        encoding = encoding_utf16be;
      } else if (args[i] == "UTF-32LE") {
        encoding = encoding_utf32le;
      } else if (args[i] == "UTF-32BE") {
        encoding = encoding_utf32be;
      } else {
        std::ostringstream e;
        e << "STRINGS option ENCODING \"" << args[i] << "\" not recognized.";
        this->SetError(e.str());
        return false;
      }
      arg_mode = arg_none;
    } else {
      std::ostringstream e;
      e << "STRINGS given unknown argument \"" << args[i] << "\"";
      this->SetError(e.str());
      return false;
    }
  }

  if (hex_conversion_enabled) {
    // TODO: should work without temp file, but just on a memory buffer
    std::string binaryFileName = this->Makefile->GetCurrentBinaryDirectory();
    binaryFileName += cmake::GetCMakeFilesDirectory();
    binaryFileName += "/FileCommandStringsBinaryFile";
    if (cmHexFileConverter::TryConvert(fileName.c_str(),
                                       binaryFileName.c_str())) {
      fileName = binaryFileName;
    }
  }

// Open the specified file.
#if defined(_WIN32) || defined(__CYGWIN__)
  cmsys::ifstream fin(fileName.c_str(), std::ios::in | std::ios::binary);
#else
  cmsys::ifstream fin(fileName.c_str());
#endif
  if (!fin) {
    std::ostringstream e;
    e << "STRINGS file \"" << fileName << "\" cannot be read.";
    this->SetError(e.str());
    return false;
  }

  // If BOM is found and encoding was not specified, use the BOM
  int bom_found = cmsys::FStream::ReadBOM(fin);
  if (encoding == encoding_none && bom_found != cmsys::FStream::BOM_None) {
    encoding = bom_found;
  }

  unsigned int bytes_rem = 0;
  if (encoding == encoding_utf16le || encoding == encoding_utf16be) {
    bytes_rem = 1;
  }
  if (encoding == encoding_utf32le || encoding == encoding_utf32be) {
    bytes_rem = 3;
  }

  // Parse strings out of the file.
  int output_size = 0;
  std::vector<std::string> strings;
  std::string s;
  while ((!limit_count || strings.size() < limit_count) &&
         (limit_input < 0 || static_cast<int>(fin.tellg()) < limit_input) &&
         fin) {
    std::string current_str;

    int c = fin.get();
    for (unsigned int i = 0; i < bytes_rem; ++i) {
      int c1 = fin.get();
      if (!fin) {
        fin.putback(static_cast<char>(c1));
        break;
      }
      c = (c << 8) | c1;
    }
    if (encoding == encoding_utf16le) {
      c = ((c & 0xFF) << 8) | ((c & 0xFF00) >> 8);
    } else if (encoding == encoding_utf32le) {
      c = (((c & 0xFF) << 24) | ((c & 0xFF00) << 8) | ((c & 0xFF0000) >> 8) |
           ((c & 0xFF000000) >> 24));
    }

    if (c == '\r') {
      // Ignore CR character to make output always have UNIX newlines.
      continue;
    }

    if ((c >= 0x20 && c < 0x7F) || c == '\t' ||
        (c == '\n' && newline_consume)) {
      // This is an ASCII character that may be part of a string.
      // Cast added to avoid compiler warning. Cast is ok because
      // c is guaranteed to fit in char by the above if...
      current_str += static_cast<char>(c);
    } else if (encoding == encoding_utf8) {
      // Check for UTF-8 encoded string (up to 4 octets)
      static const unsigned char utf8_check_table[3][2] = {
        { 0xE0, 0xC0 }, { 0xF0, 0xE0 }, { 0xF8, 0xF0 },
      };

      // how many octets are there?
      unsigned int num_utf8_bytes = 0;
      for (unsigned int j = 0; num_utf8_bytes == 0 && j < 3; j++) {
        if ((c & utf8_check_table[j][0]) == utf8_check_table[j][1]) {
          num_utf8_bytes = j + 2;
        }
      }

      // get subsequent octets and check that they are valid
      for (unsigned int j = 0; j < num_utf8_bytes; j++) {
        if (j != 0) {
          c = fin.get();
          if (!fin || (c & 0xC0) != 0x80) {
            fin.putback(static_cast<char>(c));
            break;
          }
        }
        current_str += static_cast<char>(c);
      }

      // if this was an invalid utf8 sequence, discard the data, and put
      // back subsequent characters
      if ((current_str.length() != num_utf8_bytes)) {
        for (unsigned int j = 0; j < current_str.size() - 1; j++) {
          c = current_str[current_str.size() - 1 - j];
          fin.putback(static_cast<char>(c));
        }
        current_str = "";
      }
    }

    if (c == '\n' && !newline_consume) {
      // The current line has been terminated.  Check if the current
      // string matches the requirements.  The length may now be as
      // low as zero since blank lines are allowed.
      if (s.length() >= minlen && (!have_regex || regex.find(s.c_str()))) {
        output_size += static_cast<int>(s.size()) + 1;
        if (limit_output >= 0 && output_size >= limit_output) {
          s = "";
          break;
        }
        strings.push_back(s);
      }

      // Reset the string to empty.
      s = "";
    } else if (current_str.empty()) {
      // A non-string character has been found.  Check if the current
      // string matches the requirements.  We require that the length
      // be at least one no matter what the user specified.
      if (s.length() >= minlen && !s.empty() &&
          (!have_regex || regex.find(s.c_str()))) {
        output_size += static_cast<int>(s.size()) + 1;
        if (limit_output >= 0 && output_size >= limit_output) {
          s = "";
          break;
        }
        strings.push_back(s);
      }

      // Reset the string to empty.
      s = "";
    } else {
      s += current_str;
    }

    if (maxlen > 0 && s.size() == maxlen) {
      // Terminate a string if the maximum length is reached.
      if (s.length() >= minlen && (!have_regex || regex.find(s.c_str()))) {
        output_size += static_cast<int>(s.size()) + 1;
        if (limit_output >= 0 && output_size >= limit_output) {
          s = "";
          break;
        }
        strings.push_back(s);
      }
      s = "";
    }
  }

  // If there is a non-empty current string we have hit the end of the
  // input file or the input size limit.  Check if the current string
  // matches the requirements.
  if ((!limit_count || strings.size() < limit_count) && !s.empty() &&
      s.length() >= minlen && (!have_regex || regex.find(s.c_str()))) {
    output_size += static_cast<int>(s.size()) + 1;
    if (limit_output < 0 || output_size < limit_output) {
      strings.push_back(s);
    }
  }

  // Encode the result in a CMake list.
  const char* sep = "";
  std::string output;
  for (std::vector<std::string>::const_iterator si = strings.begin();
       si != strings.end(); ++si) {
    // Separate the strings in the output to make it a list.
    output += sep;
    sep = ";";

    // Store the string in the output, but escape semicolons to
    // make sure it is a list.
    std::string const& sr = *si;
    for (unsigned int i = 0; i < sr.size(); ++i) {
      if (sr[i] == ';') {
        output += '\\';
      }
      output += sr[i];
    }
  }

  // Save the output in a makefile variable.
  this->Makefile->AddDefinition(outVar, output.c_str());
  return true;
}

bool cmFileCommand::HandleGlobCommand(std::vector<std::string> const& args,
                                      bool recurse)
{
  // File commands has at least one argument
  assert(args.size() > 1);

  std::vector<std::string>::const_iterator i = args.begin();

  i++; // Get rid of subcommand

  std::string variable = *i;
  i++;
  cmsys::Glob g;
  g.SetRecurse(recurse);

  bool explicitFollowSymlinks = false;
  cmPolicies::PolicyStatus status =
    this->Makefile->GetPolicyStatus(cmPolicies::CMP0009);
  if (recurse) {
    switch (status) {
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::REQUIRED_ALWAYS:
      case cmPolicies::NEW:
        g.RecurseThroughSymlinksOff();
        break;
      case cmPolicies::OLD:
      case cmPolicies::WARN:
        g.RecurseThroughSymlinksOn();
        break;
    }
  }

  std::string output;
  bool first = true;
  for (; i != args.end(); ++i) {
    if (*i == "LIST_DIRECTORIES") {
      ++i;
      if (i != args.end()) {
        if (cmSystemTools::IsOn(i->c_str())) {
          g.SetListDirs(true);
          g.SetRecurseListDirs(true);
        } else if (cmSystemTools::IsOff(i->c_str())) {
          g.SetListDirs(false);
          g.SetRecurseListDirs(false);
        } else {
          this->SetError("LIST_DIRECTORIES missing bool value.");
          return false;
        }
      } else {
        this->SetError("LIST_DIRECTORIES missing bool value.");
        return false;
      }
      continue;
    }

    if (recurse && (*i == "FOLLOW_SYMLINKS")) {
      explicitFollowSymlinks = true;
      g.RecurseThroughSymlinksOn();
      ++i;
      if (i == args.end()) {
        this->SetError(
          "GLOB_RECURSE requires a glob expression after FOLLOW_SYMLINKS");
        return false;
      }
    }

    if (*i == "RELATIVE") {
      ++i; // skip RELATIVE
      if (i == args.end()) {
        this->SetError("GLOB requires a directory after the RELATIVE tag");
        return false;
      }
      g.SetRelative(i->c_str());
      ++i;
      if (i == args.end()) {
        this->SetError("GLOB requires a glob expression after the directory");
        return false;
      }
    }

    cmsys::Glob::GlobMessages globMessages;
    if (!cmsys::SystemTools::FileIsFullPath(i->c_str())) {
      std::string expr = this->Makefile->GetCurrentSourceDirectory();
      // Handle script mode
      if (!expr.empty()) {
        expr += "/" + *i;
        g.FindFiles(expr, &globMessages);
      } else {
        g.FindFiles(*i, &globMessages);
      }
    } else {
      g.FindFiles(*i, &globMessages);
    }

    if (!globMessages.empty()) {
      bool shouldExit = false;
      for (cmsys::Glob::GlobMessagesIterator it = globMessages.begin();
           it != globMessages.end(); ++it) {
        if (it->type == cmsys::Glob::cyclicRecursion) {
          this->Makefile->IssueMessage(
            cmake::AUTHOR_WARNING,
            "Cyclic recursion detected while globbing for '" + *i + "':\n" +
              it->content);
        } else {
          this->Makefile->IssueMessage(
            cmake::FATAL_ERROR, "Error has occurred while globbing for '" +
              *i + "' - " + it->content);
          shouldExit = true;
        }
      }
      if (shouldExit) {
        return false;
      }
    }

    std::vector<std::string>::size_type cc;
    std::vector<std::string>& files = g.GetFiles();
    std::sort(files.begin(), files.end());
    for (cc = 0; cc < files.size(); cc++) {
      if (!first) {
        output += ";";
      }
      output += files[cc];
      first = false;
    }
  }

  if (recurse && !explicitFollowSymlinks) {
    switch (status) {
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::REQUIRED_ALWAYS:
      case cmPolicies::NEW:
        // Correct behavior, yay!
        break;
      case cmPolicies::OLD:
      // Probably not really the expected behavior, but the author explicitly
      // asked for the old behavior... no warning.
      case cmPolicies::WARN:
        // Possibly unexpected old behavior *and* we actually traversed
        // symlinks without being explicitly asked to: warn the author.
        if (g.GetFollowedSymlinkCount() != 0) {
          this->Makefile->IssueMessage(
            cmake::AUTHOR_WARNING,
            cmPolicies::GetPolicyWarning(cmPolicies::CMP0009));
        }
        break;
    }
  }

  this->Makefile->AddDefinition(variable, output.c_str());
  return true;
}

bool cmFileCommand::HandleMakeDirectoryCommand(
  std::vector<std::string> const& args)
{
  // File command has at least one argument
  assert(args.size() > 1);

  std::vector<std::string>::const_iterator i = args.begin();

  i++; // Get rid of subcommand

  std::string expr;
  for (; i != args.end(); ++i) {
    const std::string* cdir = &(*i);
    if (!cmsys::SystemTools::FileIsFullPath(i->c_str())) {
      expr = this->Makefile->GetCurrentSourceDirectory();
      expr += "/" + *i;
      cdir = &expr;
    }
    if (!this->Makefile->CanIWriteThisFile(cdir->c_str())) {
      std::string e = "attempted to create a directory: " + *cdir +
        " into a source directory.";
      this->SetError(e);
      cmSystemTools::SetFatalErrorOccured();
      return false;
    }
    if (!cmSystemTools::MakeDirectory(cdir->c_str())) {
      std::string error = "problem creating directory: " + *cdir;
      this->SetError(error);
      return false;
    }
  }
  return true;
}

bool cmFileCommand::HandleDifferentCommand(
  std::vector<std::string> const& args)
{
  /*
    FILE(DIFFERENT <variable> FILES <lhs> <rhs>)
   */

  // Evaluate arguments.
  const char* file_lhs = CM_NULLPTR;
  const char* file_rhs = CM_NULLPTR;
  const char* var = CM_NULLPTR;
  enum Doing
  {
    DoingNone,
    DoingVar,
    DoingFileLHS,
    DoingFileRHS
  };
  Doing doing = DoingVar;
  for (unsigned int i = 1; i < args.size(); ++i) {
    if (args[i] == "FILES") {
      doing = DoingFileLHS;
    } else if (doing == DoingVar) {
      var = args[i].c_str();
      doing = DoingNone;
    } else if (doing == DoingFileLHS) {
      file_lhs = args[i].c_str();
      doing = DoingFileRHS;
    } else if (doing == DoingFileRHS) {
      file_rhs = args[i].c_str();
      doing = DoingNone;
    } else {
      std::ostringstream e;
      e << "DIFFERENT given unknown argument " << args[i];
      this->SetError(e.str());
      return false;
    }
  }
  if (!var) {
    this->SetError("DIFFERENT not given result variable name.");
    return false;
  }
  if (!file_lhs || !file_rhs) {
    this->SetError("DIFFERENT not given FILES option with two file names.");
    return false;
  }

  // Compare the files.
  const char* result =
    cmSystemTools::FilesDiffer(file_lhs, file_rhs) ? "1" : "0";
  this->Makefile->AddDefinition(var, result);
  return true;
}

// File installation helper class.
struct cmFileCopier
{
  cmFileCopier(cmFileCommand* command, const char* name = "COPY")
    : FileCommand(command)
    , Makefile(command->GetMakefile())
    , Name(name)
    , Always(false)
    , MatchlessFiles(true)
    , FilePermissions(0)
    , DirPermissions(0)
    , CurrentMatchRule(CM_NULLPTR)
    , UseGivenPermissionsFile(false)
    , UseGivenPermissionsDir(false)
    , UseSourcePermissions(true)
    , Doing(DoingNone)
  {
  }
  virtual ~cmFileCopier() {}

  bool Run(std::vector<std::string> const& args);

protected:
  cmFileCommand* FileCommand;
  cmMakefile* Makefile;
  const char* Name;
  bool Always;
  cmFileTimeComparison FileTimes;

  // Whether to install a file not matching any expression.
  bool MatchlessFiles;

  // Permissions for files and directories installed by this object.
  mode_t FilePermissions;
  mode_t DirPermissions;

  // Properties set by pattern and regex match rules.
  struct MatchProperties
  {
    bool Exclude;
    mode_t Permissions;
    MatchProperties()
      : Exclude(false)
      , Permissions(0)
    {
    }
  };
  struct MatchRule
  {
    cmsys::RegularExpression Regex;
    MatchProperties Properties;
    std::string RegexString;
    MatchRule(std::string const& regex)
      : Regex(regex.c_str())
      , RegexString(regex)
    {
    }
  };
  std::vector<MatchRule> MatchRules;

  // Get the properties from rules matching this input file.
  MatchProperties CollectMatchProperties(const char* file)
  {
// Match rules are case-insensitive on some platforms.
#if defined(_WIN32) || defined(__APPLE__) || defined(__CYGWIN__)
    std::string lower = cmSystemTools::LowerCase(file);
    const char* file_to_match = lower.c_str();
#else
    const char* file_to_match = file;
#endif

    // Collect properties from all matching rules.
    bool matched = false;
    MatchProperties result;
    for (std::vector<MatchRule>::iterator mr = this->MatchRules.begin();
         mr != this->MatchRules.end(); ++mr) {
      if (mr->Regex.find(file_to_match)) {
        matched = true;
        result.Exclude |= mr->Properties.Exclude;
        result.Permissions |= mr->Properties.Permissions;
      }
    }
    if (!matched && !this->MatchlessFiles) {
      result.Exclude = !cmSystemTools::FileIsDirectory(file);
    }
    return result;
  }

  bool SetPermissions(const char* toFile, mode_t permissions)
  {
    if (permissions && !cmSystemTools::SetPermissions(toFile, permissions)) {
      std::ostringstream e;
      e << this->Name << " cannot set permissions on \"" << toFile << "\"";
      this->FileCommand->SetError(e.str());
      return false;
    }
    return true;
  }

  // Translate an argument to a permissions bit.
  bool CheckPermissions(std::string const& arg, mode_t& permissions)
  {
    if (arg == "OWNER_READ") {
      permissions |= mode_owner_read;
    } else if (arg == "OWNER_WRITE") {
      permissions |= mode_owner_write;
    } else if (arg == "OWNER_EXECUTE") {
      permissions |= mode_owner_execute;
    } else if (arg == "GROUP_READ") {
      permissions |= mode_group_read;
    } else if (arg == "GROUP_WRITE") {
      permissions |= mode_group_write;
    } else if (arg == "GROUP_EXECUTE") {
      permissions |= mode_group_execute;
    } else if (arg == "WORLD_READ") {
      permissions |= mode_world_read;
    } else if (arg == "WORLD_WRITE") {
      permissions |= mode_world_write;
    } else if (arg == "WORLD_EXECUTE") {
      permissions |= mode_world_execute;
    } else if (arg == "SETUID") {
      permissions |= mode_setuid;
    } else if (arg == "SETGID") {
      permissions |= mode_setgid;
    } else {
      std::ostringstream e;
      e << this->Name << " given invalid permission \"" << arg << "\".";
      this->FileCommand->SetError(e.str());
      return false;
    }
    return true;
  }

  bool InstallSymlink(const char* fromFile, const char* toFile);
  bool InstallFile(const char* fromFile, const char* toFile,
                   MatchProperties match_properties);
  bool InstallDirectory(const char* source, const char* destination,
                        MatchProperties match_properties);
  virtual bool Install(const char* fromFile, const char* toFile);
  virtual std::string const& ToName(std::string const& fromName)
  {
    return fromName;
  }

  enum Type
  {
    TypeFile,
    TypeDir,
    TypeLink
  };
  virtual void ReportCopy(const char*, Type, bool) {}
  virtual bool ReportMissing(const char* fromFile)
  {
    // The input file does not exist and installation is not optional.
    std::ostringstream e;
    e << this->Name << " cannot find \"" << fromFile << "\".";
    this->FileCommand->SetError(e.str());
    return false;
  }

  MatchRule* CurrentMatchRule;
  bool UseGivenPermissionsFile;
  bool UseGivenPermissionsDir;
  bool UseSourcePermissions;
  std::string Destination;
  std::string FilesFromDir;
  std::vector<std::string> Files;
  int Doing;

  virtual bool Parse(std::vector<std::string> const& args);
  enum
  {
    DoingNone,
    DoingError,
    DoingDestination,
    DoingFilesFromDir,
    DoingFiles,
    DoingPattern,
    DoingRegex,
    DoingPermissionsFile,
    DoingPermissionsDir,
    DoingPermissionsMatch,
    DoingLast1
  };
  virtual bool CheckKeyword(std::string const& arg);
  virtual bool CheckValue(std::string const& arg);

  void NotBeforeMatch(std::string const& arg)
  {
    std::ostringstream e;
    e << "option " << arg << " may not appear before PATTERN or REGEX.";
    this->FileCommand->SetError(e.str());
    this->Doing = DoingError;
  }
  void NotAfterMatch(std::string const& arg)
  {
    std::ostringstream e;
    e << "option " << arg << " may not appear after PATTERN or REGEX.";
    this->FileCommand->SetError(e.str());
    this->Doing = DoingError;
  }
  virtual void DefaultFilePermissions()
  {
    // Use read/write permissions.
    this->FilePermissions = 0;
    this->FilePermissions |= mode_owner_read;
    this->FilePermissions |= mode_owner_write;
    this->FilePermissions |= mode_group_read;
    this->FilePermissions |= mode_world_read;
  }
  virtual void DefaultDirectoryPermissions()
  {
    // Use read/write/executable permissions.
    this->DirPermissions = 0;
    this->DirPermissions |= mode_owner_read;
    this->DirPermissions |= mode_owner_write;
    this->DirPermissions |= mode_owner_execute;
    this->DirPermissions |= mode_group_read;
    this->DirPermissions |= mode_group_execute;
    this->DirPermissions |= mode_world_read;
    this->DirPermissions |= mode_world_execute;
  }
};

bool cmFileCopier::Parse(std::vector<std::string> const& args)
{
  this->Doing = DoingFiles;
  for (unsigned int i = 1; i < args.size(); ++i) {
    // Check this argument.
    if (!this->CheckKeyword(args[i]) && !this->CheckValue(args[i])) {
      std::ostringstream e;
      e << "called with unknown argument \"" << args[i] << "\".";
      this->FileCommand->SetError(e.str());
      return false;
    }

    // Quit if an argument is invalid.
    if (this->Doing == DoingError) {
      return false;
    }
  }

  // Require a destination.
  if (this->Destination.empty()) {
    std::ostringstream e;
    e << this->Name << " given no DESTINATION";
    this->FileCommand->SetError(e.str());
    return false;
  }

  // If file permissions were not specified set default permissions.
  if (!this->UseGivenPermissionsFile && !this->UseSourcePermissions) {
    this->DefaultFilePermissions();
  }

  // If directory permissions were not specified set default permissions.
  if (!this->UseGivenPermissionsDir && !this->UseSourcePermissions) {
    this->DefaultDirectoryPermissions();
  }

  return true;
}

bool cmFileCopier::CheckKeyword(std::string const& arg)
{
  if (arg == "DESTINATION") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingDestination;
    }
  } else if (arg == "FILES_FROM_DIR") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingFilesFromDir;
    }
  } else if (arg == "PATTERN") {
    this->Doing = DoingPattern;
  } else if (arg == "REGEX") {
    this->Doing = DoingRegex;
  } else if (arg == "EXCLUDE") {
    // Add this property to the current match rule.
    if (this->CurrentMatchRule) {
      this->CurrentMatchRule->Properties.Exclude = true;
      this->Doing = DoingNone;
    } else {
      this->NotBeforeMatch(arg);
    }
  } else if (arg == "PERMISSIONS") {
    if (this->CurrentMatchRule) {
      this->Doing = DoingPermissionsMatch;
    } else {
      this->NotBeforeMatch(arg);
    }
  } else if (arg == "FILE_PERMISSIONS") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingPermissionsFile;
      this->UseGivenPermissionsFile = true;
    }
  } else if (arg == "DIRECTORY_PERMISSIONS") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingPermissionsDir;
      this->UseGivenPermissionsDir = true;
    }
  } else if (arg == "USE_SOURCE_PERMISSIONS") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingNone;
      this->UseSourcePermissions = true;
    }
  } else if (arg == "NO_SOURCE_PERMISSIONS") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingNone;
      this->UseSourcePermissions = false;
    }
  } else if (arg == "FILES_MATCHING") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingNone;
      this->MatchlessFiles = false;
    }
  } else {
    return false;
  }
  return true;
}

bool cmFileCopier::CheckValue(std::string const& arg)
{
  switch (this->Doing) {
    case DoingFiles:
      this->Files.push_back(arg);
      break;
    case DoingDestination:
      if (arg.empty() || cmSystemTools::FileIsFullPath(arg.c_str())) {
        this->Destination = arg;
      } else {
        this->Destination = this->Makefile->GetCurrentBinaryDirectory();
        this->Destination += "/" + arg;
      }
      this->Doing = DoingNone;
      break;
    case DoingFilesFromDir:
      if (cmSystemTools::FileIsFullPath(arg.c_str())) {
        this->FilesFromDir = arg;
      } else {
        this->FilesFromDir = this->Makefile->GetCurrentSourceDirectory();
        this->FilesFromDir += "/" + arg;
      }
      cmSystemTools::ConvertToUnixSlashes(this->FilesFromDir);
      this->Doing = DoingNone;
      break;
    case DoingPattern: {
      // Convert the pattern to a regular expression.  Require a
      // leading slash and trailing end-of-string in the matched
      // string to make sure the pattern matches only whole file
      // names.
      std::string regex = "/";
      regex += cmsys::Glob::PatternToRegex(arg, false);
      regex += "$";
      this->MatchRules.push_back(MatchRule(regex));
      this->CurrentMatchRule = &*(this->MatchRules.end() - 1);
      if (this->CurrentMatchRule->Regex.is_valid()) {
        this->Doing = DoingNone;
      } else {
        std::ostringstream e;
        e << "could not compile PATTERN \"" << arg << "\".";
        this->FileCommand->SetError(e.str());
        this->Doing = DoingError;
      }
    } break;
    case DoingRegex:
      this->MatchRules.push_back(MatchRule(arg));
      this->CurrentMatchRule = &*(this->MatchRules.end() - 1);
      if (this->CurrentMatchRule->Regex.is_valid()) {
        this->Doing = DoingNone;
      } else {
        std::ostringstream e;
        e << "could not compile REGEX \"" << arg << "\".";
        this->FileCommand->SetError(e.str());
        this->Doing = DoingError;
      }
      break;
    case DoingPermissionsFile:
      if (!this->CheckPermissions(arg, this->FilePermissions)) {
        this->Doing = DoingError;
      }
      break;
    case DoingPermissionsDir:
      if (!this->CheckPermissions(arg, this->DirPermissions)) {
        this->Doing = DoingError;
      }
      break;
    case DoingPermissionsMatch:
      if (!this->CheckPermissions(
            arg, this->CurrentMatchRule->Properties.Permissions)) {
        this->Doing = DoingError;
      }
      break;
    default:
      return false;
  }
  return true;
}

bool cmFileCopier::Run(std::vector<std::string> const& args)
{
  if (!this->Parse(args)) {
    return false;
  }

  for (std::vector<std::string>::const_iterator i = this->Files.begin();
       i != this->Files.end(); ++i) {
    std::string file;
    if (!i->empty() && !cmSystemTools::FileIsFullPath(*i)) {
      if (!this->FilesFromDir.empty()) {
        file = this->FilesFromDir;
      } else {
        file = this->Makefile->GetCurrentSourceDirectory();
      }
      file += "/";
      file += *i;
    } else if (!this->FilesFromDir.empty()) {
      this->FileCommand->SetError("option FILES_FROM_DIR requires all files "
                                  "to be specified as relative paths.");
      return false;
    } else {
      file = *i;
    }

    // Split the input file into its directory and name components.
    std::vector<std::string> fromPathComponents;
    cmSystemTools::SplitPath(file, fromPathComponents);
    std::string fromName = *(fromPathComponents.end() - 1);
    std::string fromDir = cmSystemTools::JoinPath(
      fromPathComponents.begin(), fromPathComponents.end() - 1);

    // Compute the full path to the destination file.
    std::string toFile = this->Destination;
    if (!this->FilesFromDir.empty()) {
      std::string dir = cmSystemTools::GetFilenamePath(*i);
      if (!dir.empty()) {
        toFile += "/";
        toFile += dir;
      }
    }
    std::string const& toName = this->ToName(fromName);
    if (!toName.empty()) {
      toFile += "/";
      toFile += toName;
    }

    // Construct the full path to the source file.  The file name may
    // have been changed above.
    std::string fromFile = fromDir;
    if (!fromName.empty()) {
      fromFile += "/";
      fromFile += fromName;
    }

    if (!this->Install(fromFile.c_str(), toFile.c_str())) {
      return false;
    }
  }
  return true;
}

bool cmFileCopier::Install(const char* fromFile, const char* toFile)
{
  if (!*fromFile) {
    std::ostringstream e;
    e << "INSTALL encountered an empty string input file name.";
    this->FileCommand->SetError(e.str());
    return false;
  }

  // Collect any properties matching this file name.
  MatchProperties match_properties = this->CollectMatchProperties(fromFile);

  // Skip the file if it is excluded.
  if (match_properties.Exclude) {
    return true;
  }

  if (cmSystemTools::SameFile(fromFile, toFile)) {
    return true;
  }
  if (cmSystemTools::FileIsSymlink(fromFile)) {
    return this->InstallSymlink(fromFile, toFile);
  }
  if (cmSystemTools::FileIsDirectory(fromFile)) {
    return this->InstallDirectory(fromFile, toFile, match_properties);
  }
  if (cmSystemTools::FileExists(fromFile)) {
    return this->InstallFile(fromFile, toFile, match_properties);
  }
  return this->ReportMissing(fromFile);
}

bool cmFileCopier::InstallSymlink(const char* fromFile, const char* toFile)
{
  // Read the original symlink.
  std::string symlinkTarget;
  if (!cmSystemTools::ReadSymlink(fromFile, symlinkTarget)) {
    std::ostringstream e;
    e << this->Name << " cannot read symlink \"" << fromFile
      << "\" to duplicate at \"" << toFile << "\".";
    this->FileCommand->SetError(e.str());
    return false;
  }

  // Compare the symlink value to that at the destination if not
  // always installing.
  bool copy = true;
  if (!this->Always) {
    std::string oldSymlinkTarget;
    if (cmSystemTools::ReadSymlink(toFile, oldSymlinkTarget)) {
      if (symlinkTarget == oldSymlinkTarget) {
        copy = false;
      }
    }
  }

  // Inform the user about this file installation.
  this->ReportCopy(toFile, TypeLink, copy);

  if (copy) {
    // Remove the destination file so we can always create the symlink.
    cmSystemTools::RemoveFile(toFile);

    // Create destination directory if it doesn't exist
    cmSystemTools::MakeDirectory(cmSystemTools::GetFilenamePath(toFile));

    // Create the symlink.
    if (!cmSystemTools::CreateSymlink(symlinkTarget, toFile)) {
      std::ostringstream e;
      e << this->Name << " cannot duplicate symlink \"" << fromFile
        << "\" at \"" << toFile << "\".";
      this->FileCommand->SetError(e.str());
      return false;
    }
  }

  return true;
}

bool cmFileCopier::InstallFile(const char* fromFile, const char* toFile,
                               MatchProperties match_properties)
{
  // Determine whether we will copy the file.
  bool copy = true;
  if (!this->Always) {
    // If both files exist with the same time do not copy.
    if (!this->FileTimes.FileTimesDiffer(fromFile, toFile)) {
      copy = false;
    }
  }

  // Inform the user about this file installation.
  this->ReportCopy(toFile, TypeFile, copy);

  // Copy the file.
  if (copy && !cmSystemTools::CopyAFile(fromFile, toFile, true)) {
    std::ostringstream e;
    e << this->Name << " cannot copy file \"" << fromFile << "\" to \""
      << toFile << "\".";
    this->FileCommand->SetError(e.str());
    return false;
  }

  // Set the file modification time of the destination file.
  if (copy && !this->Always) {
    // Add write permission so we can set the file time.
    // Permissions are set unconditionally below anyway.
    mode_t perm = 0;
    if (cmSystemTools::GetPermissions(toFile, perm)) {
      cmSystemTools::SetPermissions(toFile, perm | mode_owner_write);
    }
    if (!cmSystemTools::CopyFileTime(fromFile, toFile)) {
      std::ostringstream e;
      e << this->Name << " cannot set modification time on \"" << toFile
        << "\"";
      this->FileCommand->SetError(e.str());
      return false;
    }
  }

  // Set permissions of the destination file.
  mode_t permissions =
    (match_properties.Permissions ? match_properties.Permissions
                                  : this->FilePermissions);
  if (!permissions) {
    // No permissions were explicitly provided but the user requested
    // that the source file permissions be used.
    cmSystemTools::GetPermissions(fromFile, permissions);
  }
  return this->SetPermissions(toFile, permissions);
}

bool cmFileCopier::InstallDirectory(const char* source,
                                    const char* destination,
                                    MatchProperties match_properties)
{
  // Inform the user about this directory installation.
  this->ReportCopy(destination, TypeDir,
                   !cmSystemTools::FileIsDirectory(destination));

  // Make sure the destination directory exists.
  if (!cmSystemTools::MakeDirectory(destination)) {
    std::ostringstream e;
    e << this->Name << " cannot make directory \"" << destination
      << "\": " << cmSystemTools::GetLastSystemError();
    this->FileCommand->SetError(e.str());
    return false;
  }

  // Compute the requested permissions for the destination directory.
  mode_t permissions =
    (match_properties.Permissions ? match_properties.Permissions
                                  : this->DirPermissions);
  if (!permissions) {
    // No permissions were explicitly provided but the user requested
    // that the source directory permissions be used.
    cmSystemTools::GetPermissions(source, permissions);
  }

  // Compute the set of permissions required on this directory to
  // recursively install files and subdirectories safely.
  mode_t required_permissions =
    mode_owner_read | mode_owner_write | mode_owner_execute;

  // If the required permissions are specified it is safe to set the
  // final permissions now.  Otherwise we must add the required
  // permissions temporarily during file installation.
  mode_t permissions_before = 0;
  mode_t permissions_after = 0;
  if ((permissions & required_permissions) == required_permissions) {
    permissions_before = permissions;
  } else {
    permissions_before = permissions | required_permissions;
    permissions_after = permissions;
  }

  // Set the required permissions of the destination directory.
  if (!this->SetPermissions(destination, permissions_before)) {
    return false;
  }

  // Load the directory contents to traverse it recursively.
  cmsys::Directory dir;
  if (source && *source) {
    dir.Load(source);
  }
  unsigned long numFiles = static_cast<unsigned long>(dir.GetNumberOfFiles());
  for (unsigned long fileNum = 0; fileNum < numFiles; ++fileNum) {
    if (!(strcmp(dir.GetFile(fileNum), ".") == 0 ||
          strcmp(dir.GetFile(fileNum), "..") == 0)) {
      std::string fromPath = source;
      fromPath += "/";
      fromPath += dir.GetFile(fileNum);
      std::string toPath = destination;
      toPath += "/";
      toPath += dir.GetFile(fileNum);
      if (!this->Install(fromPath.c_str(), toPath.c_str())) {
        return false;
      }
    }
  }

  // Set the requested permissions of the destination directory.
  return this->SetPermissions(destination, permissions_after);
}

bool cmFileCommand::HandleCopyCommand(std::vector<std::string> const& args)
{
  cmFileCopier copier(this);
  return copier.Run(args);
}

struct cmFileInstaller : public cmFileCopier
{
  cmFileInstaller(cmFileCommand* command)
    : cmFileCopier(command, "INSTALL")
    , InstallType(cmInstallType_FILES)
    , Optional(false)
    , MessageAlways(false)
    , MessageLazy(false)
    , MessageNever(false)
    , DestDirLength(0)
  {
    // Installation does not use source permissions by default.
    this->UseSourcePermissions = false;
    // Check whether to copy files always or only if they have changed.
    std::string install_always;
    if (cmSystemTools::GetEnv("CMAKE_INSTALL_ALWAYS", install_always)) {
      this->Always = cmSystemTools::IsOn(install_always.c_str());
    }
    // Get the current manifest.
    this->Manifest =
      this->Makefile->GetSafeDefinition("CMAKE_INSTALL_MANIFEST_FILES");
  }
  ~cmFileInstaller() CM_OVERRIDE
  {
    // Save the updated install manifest.
    this->Makefile->AddDefinition("CMAKE_INSTALL_MANIFEST_FILES",
                                  this->Manifest.c_str());
  }

protected:
  cmInstallType InstallType;
  bool Optional;
  bool MessageAlways;
  bool MessageLazy;
  bool MessageNever;
  int DestDirLength;
  std::string Rename;

  std::string Manifest;
  void ManifestAppend(std::string const& file)
  {
    if (!this->Manifest.empty()) {
      this->Manifest += ";";
    }
    this->Manifest += file.substr(this->DestDirLength);
  }

  std::string const& ToName(std::string const& fromName) CM_OVERRIDE
  {
    return this->Rename.empty() ? fromName : this->Rename;
  }

  void ReportCopy(const char* toFile, Type type, bool copy) CM_OVERRIDE
  {
    if (!this->MessageNever && (copy || !this->MessageLazy)) {
      std::string message = (copy ? "Installing: " : "Up-to-date: ");
      message += toFile;
      this->Makefile->DisplayStatus(message.c_str(), -1);
    }
    if (type != TypeDir) {
      // Add the file to the manifest.
      this->ManifestAppend(toFile);
    }
  }
  bool ReportMissing(const char* fromFile) CM_OVERRIDE
  {
    return (this->Optional || this->cmFileCopier::ReportMissing(fromFile));
  }
  bool Install(const char* fromFile, const char* toFile) CM_OVERRIDE
  {
    // Support installing from empty source to make a directory.
    if (this->InstallType == cmInstallType_DIRECTORY && !*fromFile) {
      return this->InstallDirectory(fromFile, toFile, MatchProperties());
    }
    return this->cmFileCopier::Install(fromFile, toFile);
  }

  bool Parse(std::vector<std::string> const& args) CM_OVERRIDE;
  enum
  {
    DoingType = DoingLast1,
    DoingRename,
    DoingLast2
  };
  bool CheckKeyword(std::string const& arg) CM_OVERRIDE;
  bool CheckValue(std::string const& arg) CM_OVERRIDE;
  void DefaultFilePermissions() CM_OVERRIDE
  {
    this->cmFileCopier::DefaultFilePermissions();
    // Add execute permissions based on the target type.
    switch (this->InstallType) {
      case cmInstallType_SHARED_LIBRARY:
      case cmInstallType_MODULE_LIBRARY:
        if (this->Makefile->IsOn("CMAKE_INSTALL_SO_NO_EXE")) {
          break;
        }
        CM_FALLTHROUGH;
      case cmInstallType_EXECUTABLE:
      case cmInstallType_PROGRAMS:
        this->FilePermissions |= mode_owner_execute;
        this->FilePermissions |= mode_group_execute;
        this->FilePermissions |= mode_world_execute;
        break;
      default:
        break;
    }
  }
  bool GetTargetTypeFromString(const std::string& stype);
  bool HandleInstallDestination();
};

bool cmFileInstaller::Parse(std::vector<std::string> const& args)
{
  if (!this->cmFileCopier::Parse(args)) {
    return false;
  }

  if (!this->Rename.empty()) {
    if (!this->FilesFromDir.empty()) {
      this->FileCommand->SetError("INSTALL option RENAME may not be "
                                  "combined with FILES_FROM_DIR.");
      return false;
    }
    if (this->InstallType != cmInstallType_FILES &&
        this->InstallType != cmInstallType_PROGRAMS) {
      this->FileCommand->SetError("INSTALL option RENAME may be used "
                                  "only with FILES or PROGRAMS.");
      return false;
    }
    if (this->Files.size() > 1) {
      this->FileCommand->SetError("INSTALL option RENAME may be used "
                                  "only with one file.");
      return false;
    }
  }

  if (!this->HandleInstallDestination()) {
    return false;
  }

  if (((this->MessageAlways ? 1 : 0) + (this->MessageLazy ? 1 : 0) +
       (this->MessageNever ? 1 : 0)) > 1) {
    this->FileCommand->SetError("INSTALL options MESSAGE_ALWAYS, "
                                "MESSAGE_LAZY, and MESSAGE_NEVER "
                                "are mutually exclusive.");
    return false;
  }

  return true;
}

bool cmFileInstaller::CheckKeyword(std::string const& arg)
{
  if (arg == "TYPE") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingType;
    }
  } else if (arg == "FILES") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingFiles;
    }
  } else if (arg == "RENAME") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingRename;
    }
  } else if (arg == "OPTIONAL") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingNone;
      this->Optional = true;
    }
  } else if (arg == "MESSAGE_ALWAYS") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingNone;
      this->MessageAlways = true;
    }
  } else if (arg == "MESSAGE_LAZY") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingNone;
      this->MessageLazy = true;
    }
  } else if (arg == "MESSAGE_NEVER") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      this->Doing = DoingNone;
      this->MessageNever = true;
    }
  } else if (arg == "PERMISSIONS") {
    if (this->CurrentMatchRule) {
      this->Doing = DoingPermissionsMatch;
    } else {
      // file(INSTALL) aliases PERMISSIONS to FILE_PERMISSIONS
      this->Doing = DoingPermissionsFile;
      this->UseGivenPermissionsFile = true;
    }
  } else if (arg == "DIR_PERMISSIONS") {
    if (this->CurrentMatchRule) {
      this->NotAfterMatch(arg);
    } else {
      // file(INSTALL) aliases DIR_PERMISSIONS to DIRECTORY_PERMISSIONS
      this->Doing = DoingPermissionsDir;
      this->UseGivenPermissionsDir = true;
    }
  } else if (arg == "COMPONENTS" || arg == "CONFIGURATIONS" ||
             arg == "PROPERTIES") {
    std::ostringstream e;
    e << "INSTALL called with old-style " << arg << " argument.  "
      << "This script was generated with an older version of CMake.  "
      << "Re-run this cmake version on your build tree.";
    this->FileCommand->SetError(e.str());
    this->Doing = DoingError;
  } else {
    return this->cmFileCopier::CheckKeyword(arg);
  }
  return true;
}

bool cmFileInstaller::CheckValue(std::string const& arg)
{
  switch (this->Doing) {
    case DoingType:
      if (!this->GetTargetTypeFromString(arg)) {
        this->Doing = DoingError;
      }
      break;
    case DoingRename:
      this->Rename = arg;
      break;
    default:
      return this->cmFileCopier::CheckValue(arg);
  }
  return true;
}

bool cmFileInstaller::GetTargetTypeFromString(const std::string& stype)
{
  if (stype == "EXECUTABLE") {
    this->InstallType = cmInstallType_EXECUTABLE;
  } else if (stype == "FILE") {
    this->InstallType = cmInstallType_FILES;
  } else if (stype == "PROGRAM") {
    this->InstallType = cmInstallType_PROGRAMS;
  } else if (stype == "STATIC_LIBRARY") {
    this->InstallType = cmInstallType_STATIC_LIBRARY;
  } else if (stype == "SHARED_LIBRARY") {
    this->InstallType = cmInstallType_SHARED_LIBRARY;
  } else if (stype == "MODULE") {
    this->InstallType = cmInstallType_MODULE_LIBRARY;
  } else if (stype == "DIRECTORY") {
    this->InstallType = cmInstallType_DIRECTORY;
  } else {
    std::ostringstream e;
    e << "Option TYPE given unknown value \"" << stype << "\".";
    this->FileCommand->SetError(e.str());
    return false;
  }
  return true;
}

bool cmFileInstaller::HandleInstallDestination()
{
  std::string& destination = this->Destination;

  // allow for / to be a valid destination
  if (destination.size() < 2 && destination != "/") {
    this->FileCommand->SetError("called with inappropriate arguments. "
                                "No DESTINATION provided or .");
    return false;
  }

  std::string sdestdir;
  if (cmSystemTools::GetEnv("DESTDIR", sdestdir) && !sdestdir.empty()) {
    cmSystemTools::ConvertToUnixSlashes(sdestdir);
    char ch1 = destination[0];
    char ch2 = destination[1];
    char ch3 = 0;
    if (destination.size() > 2) {
      ch3 = destination[2];
    }
    int skip = 0;
    if (ch1 != '/') {
      int relative = 0;
      if (((ch1 >= 'a' && ch1 <= 'z') || (ch1 >= 'A' && ch1 <= 'Z')) &&
          ch2 == ':') {
        // Assume windows
        // let's do some destdir magic:
        skip = 2;
        if (ch3 != '/') {
          relative = 1;
        }
      } else {
        relative = 1;
      }
      if (relative) {
        // This is relative path on unix or windows. Since we are doing
        // destdir, this case does not make sense.
        this->FileCommand->SetError(
          "called with relative DESTINATION. This "
          "does not make sense when using DESTDIR. Specify "
          "absolute path or remove DESTDIR environment variable.");
        return false;
      }
    } else {
      if (ch2 == '/') {
        // looks like a network path.
        std::string message =
          "called with network path DESTINATION. This "
          "does not make sense when using DESTDIR. Specify local "
          "absolute path or remove DESTDIR environment variable."
          "\nDESTINATION=\n";
        message += destination;
        this->FileCommand->SetError(message);
        return false;
      }
    }
    destination = sdestdir + (destination.c_str() + skip);
    this->DestDirLength = int(sdestdir.size());
  }

  if (this->InstallType != cmInstallType_DIRECTORY) {
    if (!cmSystemTools::FileExists(destination.c_str())) {
      if (!cmSystemTools::MakeDirectory(destination.c_str())) {
        std::string errstring = "cannot create directory: " + destination +
          ". Maybe need administrative privileges.";
        this->FileCommand->SetError(errstring);
        return false;
      }
    }
    if (!cmSystemTools::FileIsDirectory(destination)) {
      std::string errstring =
        "INSTALL destination: " + destination + " is not a directory.";
      this->FileCommand->SetError(errstring);
      return false;
    }
  }
  return true;
}

bool cmFileCommand::HandleRPathChangeCommand(
  std::vector<std::string> const& args)
{
  // Evaluate arguments.
  const char* file = CM_NULLPTR;
  const char* oldRPath = CM_NULLPTR;
  const char* newRPath = CM_NULLPTR;
  enum Doing
  {
    DoingNone,
    DoingFile,
    DoingOld,
    DoingNew
  };
  Doing doing = DoingNone;
  for (unsigned int i = 1; i < args.size(); ++i) {
    if (args[i] == "OLD_RPATH") {
      doing = DoingOld;
    } else if (args[i] == "NEW_RPATH") {
      doing = DoingNew;
    } else if (args[i] == "FILE") {
      doing = DoingFile;
    } else if (doing == DoingFile) {
      file = args[i].c_str();
      doing = DoingNone;
    } else if (doing == DoingOld) {
      oldRPath = args[i].c_str();
      doing = DoingNone;
    } else if (doing == DoingNew) {
      newRPath = args[i].c_str();
      doing = DoingNone;
    } else {
      std::ostringstream e;
      e << "RPATH_CHANGE given unknown argument " << args[i];
      this->SetError(e.str());
      return false;
    }
  }
  if (!file) {
    this->SetError("RPATH_CHANGE not given FILE option.");
    return false;
  }
  if (!oldRPath) {
    this->SetError("RPATH_CHANGE not given OLD_RPATH option.");
    return false;
  }
  if (!newRPath) {
    this->SetError("RPATH_CHANGE not given NEW_RPATH option.");
    return false;
  }
  if (!cmSystemTools::FileExists(file, true)) {
    std::ostringstream e;
    e << "RPATH_CHANGE given FILE \"" << file << "\" that does not exist.";
    this->SetError(e.str());
    return false;
  }
  bool success = true;
  cmSystemToolsFileTime* ft = cmSystemTools::FileTimeNew();
  bool have_ft = cmSystemTools::FileTimeGet(file, ft);
  std::string emsg;
  bool changed;
  if (!cmSystemTools::ChangeRPath(file, oldRPath, newRPath, &emsg, &changed)) {
    std::ostringstream e;
    /* clang-format off */
    e << "RPATH_CHANGE could not write new RPATH:\n"
      << "  " << newRPath << "\n"
      << "to the file:\n"
      << "  " << file << "\n"
      << emsg;
    /* clang-format on */
    this->SetError(e.str());
    success = false;
  }
  if (success) {
    if (changed) {
      std::string message = "Set runtime path of \"";
      message += file;
      message += "\" to \"";
      message += newRPath;
      message += "\"";
      this->Makefile->DisplayStatus(message.c_str(), -1);
    }
    if (have_ft) {
      cmSystemTools::FileTimeSet(file, ft);
    }
  }
  cmSystemTools::FileTimeDelete(ft);
  return success;
}

bool cmFileCommand::HandleRPathRemoveCommand(
  std::vector<std::string> const& args)
{
  // Evaluate arguments.
  const char* file = CM_NULLPTR;
  enum Doing
  {
    DoingNone,
    DoingFile
  };
  Doing doing = DoingNone;
  for (unsigned int i = 1; i < args.size(); ++i) {
    if (args[i] == "FILE") {
      doing = DoingFile;
    } else if (doing == DoingFile) {
      file = args[i].c_str();
      doing = DoingNone;
    } else {
      std::ostringstream e;
      e << "RPATH_REMOVE given unknown argument " << args[i];
      this->SetError(e.str());
      return false;
    }
  }
  if (!file) {
    this->SetError("RPATH_REMOVE not given FILE option.");
    return false;
  }
  if (!cmSystemTools::FileExists(file, true)) {
    std::ostringstream e;
    e << "RPATH_REMOVE given FILE \"" << file << "\" that does not exist.";
    this->SetError(e.str());
    return false;
  }
  bool success = true;
  cmSystemToolsFileTime* ft = cmSystemTools::FileTimeNew();
  bool have_ft = cmSystemTools::FileTimeGet(file, ft);
  std::string emsg;
  bool removed;
  if (!cmSystemTools::RemoveRPath(file, &emsg, &removed)) {
    std::ostringstream e;
    /* clang-format off */
    e << "RPATH_REMOVE could not remove RPATH from file:\n"
      << "  " << file << "\n"
      << emsg;
    /* clang-format on */
    this->SetError(e.str());
    success = false;
  }
  if (success) {
    if (removed) {
      std::string message = "Removed runtime path from \"";
      message += file;
      message += "\"";
      this->Makefile->DisplayStatus(message.c_str(), -1);
    }
    if (have_ft) {
      cmSystemTools::FileTimeSet(file, ft);
    }
  }
  cmSystemTools::FileTimeDelete(ft);
  return success;
}

bool cmFileCommand::HandleRPathCheckCommand(
  std::vector<std::string> const& args)
{
  // Evaluate arguments.
  const char* file = CM_NULLPTR;
  const char* rpath = CM_NULLPTR;
  enum Doing
  {
    DoingNone,
    DoingFile,
    DoingRPath
  };
  Doing doing = DoingNone;
  for (unsigned int i = 1; i < args.size(); ++i) {
    if (args[i] == "RPATH") {
      doing = DoingRPath;
    } else if (args[i] == "FILE") {
      doing = DoingFile;
    } else if (doing == DoingFile) {
      file = args[i].c_str();
      doing = DoingNone;
    } else if (doing == DoingRPath) {
      rpath = args[i].c_str();
      doing = DoingNone;
    } else {
      std::ostringstream e;
      e << "RPATH_CHECK given unknown argument " << args[i];
      this->SetError(e.str());
      return false;
    }
  }
  if (!file) {
    this->SetError("RPATH_CHECK not given FILE option.");
    return false;
  }
  if (!rpath) {
    this->SetError("RPATH_CHECK not given RPATH option.");
    return false;
  }

  // If the file exists but does not have the desired RPath then
  // delete it.  This is used during installation to re-install a file
  // if its RPath will change.
  if (cmSystemTools::FileExists(file, true) &&
      !cmSystemTools::CheckRPath(file, rpath)) {
    cmSystemTools::RemoveFile(file);
  }

  return true;
}

bool cmFileCommand::HandleReadElfCommand(std::vector<std::string> const& args)
{
  if (args.size() < 4) {
    this->SetError("READ_ELF must be called with at least three additional "
                   "arguments.");
    return false;
  }

  cmCommandArgumentsHelper argHelper;
  cmCommandArgumentGroup group;

  cmCAString readArg(&argHelper, "READ_ELF");
  cmCAString fileNameArg(&argHelper, CM_NULLPTR);

  cmCAString rpathArg(&argHelper, "RPATH", &group);
  cmCAString runpathArg(&argHelper, "RUNPATH", &group);
  cmCAString errorArg(&argHelper, "CAPTURE_ERROR", &group);

  readArg.Follows(CM_NULLPTR);
  fileNameArg.Follows(&readArg);
  group.Follows(&fileNameArg);
  argHelper.Parse(&args, CM_NULLPTR);

  if (!cmSystemTools::FileExists(fileNameArg.GetString(), true)) {
    std::ostringstream e;
    e << "READ_ELF given FILE \"" << fileNameArg.GetString()
      << "\" that does not exist.";
    this->SetError(e.str());
    return false;
  }

#if defined(CMAKE_USE_ELF_PARSER)
  cmELF elf(fileNameArg.GetCString());

  if (!rpathArg.GetString().empty()) {
    if (cmELF::StringEntry const* se_rpath = elf.GetRPath()) {
      std::string rpath(se_rpath->Value);
      std::replace(rpath.begin(), rpath.end(), ':', ';');
      this->Makefile->AddDefinition(rpathArg.GetString(), rpath.c_str());
    }
  }
  if (!runpathArg.GetString().empty()) {
    if (cmELF::StringEntry const* se_runpath = elf.GetRunPath()) {
      std::string runpath(se_runpath->Value);
      std::replace(runpath.begin(), runpath.end(), ':', ';');
      this->Makefile->AddDefinition(runpathArg.GetString(), runpath.c_str());
    }
  }

  return true;
#else
  std::string error = "ELF parser not available on this platform.";
  if (errorArg.GetString().empty()) {
    this->SetError(error);
    return false;
  } else {
    this->Makefile->AddDefinition(errorArg.GetString(), error.c_str());
    return true;
  }
#endif
}

bool cmFileCommand::HandleInstallCommand(std::vector<std::string> const& args)
{
  cmFileInstaller installer(this);
  return installer.Run(args);
}

bool cmFileCommand::HandleRelativePathCommand(
  std::vector<std::string> const& args)
{
  if (args.size() != 4) {
    this->SetError("RELATIVE_PATH called with incorrect number of arguments");
    return false;
  }

  const std::string& outVar = args[1];
  const std::string& directoryName = args[2];
  const std::string& fileName = args[3];

  if (!cmSystemTools::FileIsFullPath(directoryName.c_str())) {
    std::string errstring =
      "RELATIVE_PATH must be passed a full path to the directory: " +
      directoryName;
    this->SetError(errstring);
    return false;
  }
  if (!cmSystemTools::FileIsFullPath(fileName.c_str())) {
    std::string errstring =
      "RELATIVE_PATH must be passed a full path to the file: " + fileName;
    this->SetError(errstring);
    return false;
  }

  std::string res =
    cmSystemTools::RelativePath(directoryName.c_str(), fileName.c_str());
  this->Makefile->AddDefinition(outVar, res.c_str());
  return true;
}

bool cmFileCommand::HandleRename(std::vector<std::string> const& args)
{
  if (args.size() != 3) {
    this->SetError("RENAME given incorrect number of arguments.");
    return false;
  }

  // Compute full path for old and new names.
  std::string oldname = args[1];
  if (!cmsys::SystemTools::FileIsFullPath(oldname.c_str())) {
    oldname = this->Makefile->GetCurrentSourceDirectory();
    oldname += "/" + args[1];
  }
  std::string newname = args[2];
  if (!cmsys::SystemTools::FileIsFullPath(newname.c_str())) {
    newname = this->Makefile->GetCurrentSourceDirectory();
    newname += "/" + args[2];
  }

  if (!cmSystemTools::RenameFile(oldname.c_str(), newname.c_str())) {
    std::string err = cmSystemTools::GetLastSystemError();
    std::ostringstream e;
    /* clang-format off */
    e << "RENAME failed to rename\n"
      << "  " << oldname << "\n"
      << "to\n"
      << "  " << newname << "\n"
      << "because: " << err << "\n";
    /* clang-format on */
    this->SetError(e.str());
    return false;
  }
  return true;
}

bool cmFileCommand::HandleRemove(std::vector<std::string> const& args,
                                 bool recurse)
{

  std::string message;
  std::vector<std::string>::const_iterator i = args.begin();

  i++; // Get rid of subcommand
  for (; i != args.end(); ++i) {
    std::string fileName = *i;
    if (!cmsys::SystemTools::FileIsFullPath(fileName.c_str())) {
      fileName = this->Makefile->GetCurrentSourceDirectory();
      fileName += "/" + *i;
    }

    if (cmSystemTools::FileIsDirectory(fileName) &&
        !cmSystemTools::FileIsSymlink(fileName) && recurse) {
      cmSystemTools::RemoveADirectory(fileName);
    } else {
      cmSystemTools::RemoveFile(fileName);
    }
  }
  return true;
}

bool cmFileCommand::HandleCMakePathCommand(
  std::vector<std::string> const& args, bool nativePath)
{
  std::vector<std::string>::const_iterator i = args.begin();
  if (args.size() != 3) {
    this->SetError("FILE([TO_CMAKE_PATH|TO_NATIVE_PATH] path result) must be "
                   "called with exactly three arguments.");
    return false;
  }
  i++; // Get rid of subcommand
#if defined(_WIN32) && !defined(__CYGWIN__)
  char pathSep = ';';
#else
  char pathSep = ':';
#endif
  std::vector<cmsys::String> path = cmSystemTools::SplitString(*i, pathSep);
  i++;
  const char* var = i->c_str();
  std::string value;
  for (std::vector<cmsys::String>::iterator j = path.begin(); j != path.end();
       ++j) {
    if (j != path.begin()) {
      value += ";";
    }
    if (!nativePath) {
      cmSystemTools::ConvertToUnixSlashes(*j);
    } else {
      *j = cmSystemTools::ConvertToOutputPath(j->c_str());
      // remove double quotes in the path
      cmsys::String& s = *j;

      if (s.size() > 1 && s[0] == '\"' && s[s.size() - 1] == '\"') {
        s = s.substr(1, s.size() - 2);
      }
    }
    value += *j;
  }
  this->Makefile->AddDefinition(var, value.c_str());
  return true;
}

#if defined(CMAKE_BUILD_WITH_CMAKE)

// Stuff for curl download/upload
typedef std::vector<char> cmFileCommandVectorOfChar;

namespace {

size_t cmWriteToFileCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
  int realsize = (int)(size * nmemb);
  cmsys::ofstream* fout = static_cast<cmsys::ofstream*>(data);
  const char* chPtr = static_cast<char*>(ptr);
  fout->write(chPtr, realsize);
  return realsize;
}

size_t cmWriteToMemoryCallback(void* ptr, size_t size, size_t nmemb,
                               void* data)
{
  int realsize = (int)(size * nmemb);
  cmFileCommandVectorOfChar* vec =
    static_cast<cmFileCommandVectorOfChar*>(data);
  const char* chPtr = static_cast<char*>(ptr);
  vec->insert(vec->end(), chPtr, chPtr + realsize);
  return realsize;
}

size_t cmFileCommandCurlDebugCallback(CURL*, curl_infotype type, char* chPtr,
                                      size_t size, void* data)
{
  cmFileCommandVectorOfChar* vec =
    static_cast<cmFileCommandVectorOfChar*>(data);
  switch (type) {
    case CURLINFO_TEXT:
    case CURLINFO_HEADER_IN:
    case CURLINFO_HEADER_OUT:
      vec->insert(vec->end(), chPtr, chPtr + size);
      break;
    case CURLINFO_DATA_IN:
    case CURLINFO_DATA_OUT:
    case CURLINFO_SSL_DATA_IN:
    case CURLINFO_SSL_DATA_OUT: {
      char buf[128];
      int n = sprintf(buf, "[%" KWIML_INT_PRIu64 " bytes data]\n",
                      static_cast<KWIML_INT_uint64_t>(size));
      if (n > 0) {
        vec->insert(vec->end(), buf, buf + n);
      }
    } break;
    default:
      break;
  }
  return 0;
}

class cURLProgressHelper
{
public:
  cURLProgressHelper(cmFileCommand* fc, const char* text)
  {
    this->CurrentPercentage = -1;
    this->FileCommand = fc;
    this->Text = text;
  }

  bool UpdatePercentage(double value, double total, std::string& status)
  {
    int OldPercentage = this->CurrentPercentage;

    if (total > 0.0) {
      this->CurrentPercentage = static_cast<int>(value / total * 100.0 + 0.5);
      if (this->CurrentPercentage > 100) {
        // Avoid extra progress reports for unexpected data beyond total.
        this->CurrentPercentage = 100;
      }
    }

    bool updated = (OldPercentage != this->CurrentPercentage);

    if (updated) {
      std::ostringstream oss;
      oss << "[" << this->Text << " " << this->CurrentPercentage
          << "% complete]";
      status = oss.str();
    }

    return updated;
  }

  cmFileCommand* GetFileCommand() { return this->FileCommand; }

private:
  int CurrentPercentage;
  cmFileCommand* FileCommand;
  std::string Text;
};

int cmFileDownloadProgressCallback(void* clientp, double dltotal, double dlnow,
                                   double ultotal, double ulnow)
{
  cURLProgressHelper* helper = reinterpret_cast<cURLProgressHelper*>(clientp);

  static_cast<void>(ultotal);
  static_cast<void>(ulnow);

  std::string status;
  if (helper->UpdatePercentage(dlnow, dltotal, status)) {
    cmFileCommand* fc = helper->GetFileCommand();
    cmMakefile* mf = fc->GetMakefile();
    mf->DisplayStatus(status.c_str(), -1);
  }

  return 0;
}

int cmFileUploadProgressCallback(void* clientp, double dltotal, double dlnow,
                                 double ultotal, double ulnow)
{
  cURLProgressHelper* helper = reinterpret_cast<cURLProgressHelper*>(clientp);

  static_cast<void>(dltotal);
  static_cast<void>(dlnow);

  std::string status;
  if (helper->UpdatePercentage(ulnow, ultotal, status)) {
    cmFileCommand* fc = helper->GetFileCommand();
    cmMakefile* mf = fc->GetMakefile();
    mf->DisplayStatus(status.c_str(), -1);
  }

  return 0;
}
}

namespace {

class cURLEasyGuard
{
public:
  cURLEasyGuard(CURL* easy)
    : Easy(easy)
  {
  }

  ~cURLEasyGuard()
  {
    if (this->Easy) {
      ::curl_easy_cleanup(this->Easy);
    }
  }

  void release() { this->Easy = CM_NULLPTR; }

private:
  ::CURL* Easy;
};
}
#endif

#define check_curl_result(result, errstr)                                     \
  if (result != CURLE_OK) {                                                   \
    std::string e(errstr);                                                    \
    e += ::curl_easy_strerror(result);                                        \
    this->SetError(e);                                                        \
    return false;                                                             \
  }

bool cmFileCommand::HandleDownloadCommand(std::vector<std::string> const& args)
{
#if defined(CMAKE_BUILD_WITH_CMAKE)
  std::vector<std::string>::const_iterator i = args.begin();
  if (args.size() < 3) {
    this->SetError("DOWNLOAD must be called with at least three arguments.");
    return false;
  }
  ++i; // Get rid of subcommand
  std::string url = *i;
  ++i;
  std::string file = *i;
  ++i;

  long timeout = 0;
  long inactivity_timeout = 0;
  std::string logVar;
  std::string statusVar;
  bool tls_verify = this->Makefile->IsOn("CMAKE_TLS_VERIFY");
  const char* cainfo = this->Makefile->GetDefinition("CMAKE_TLS_CAINFO");
  std::string expectedHash;
  std::string hashMatchMSG;
  CM_AUTO_PTR<cmCryptoHash> hash;
  bool showProgress = false;
  std::string userpwd;

  std::vector<std::string> curl_headers;

  while (i != args.end()) {
    if (*i == "TIMEOUT") {
      ++i;
      if (i != args.end()) {
        timeout = atol(i->c_str());
      } else {
        this->SetError("DOWNLOAD missing time for TIMEOUT.");
        return false;
      }
    } else if (*i == "INACTIVITY_TIMEOUT") {
      ++i;
      if (i != args.end()) {
        inactivity_timeout = atol(i->c_str());
      } else {
        this->SetError("DOWNLOAD missing time for INACTIVITY_TIMEOUT.");
        return false;
      }
    } else if (*i == "LOG") {
      ++i;
      if (i == args.end()) {
        this->SetError("DOWNLOAD missing VAR for LOG.");
        return false;
      }
      logVar = *i;
    } else if (*i == "STATUS") {
      ++i;
      if (i == args.end()) {
        this->SetError("DOWNLOAD missing VAR for STATUS.");
        return false;
      }
      statusVar = *i;
    } else if (*i == "TLS_VERIFY") {
      ++i;
      if (i != args.end()) {
        tls_verify = cmSystemTools::IsOn(i->c_str());
      } else {
        this->SetError("TLS_VERIFY missing bool value.");
        return false;
      }
    } else if (*i == "TLS_CAINFO") {
      ++i;
      if (i != args.end()) {
        cainfo = i->c_str();
      } else {
        this->SetError("TLS_CAFILE missing file value.");
        return false;
      }
    } else if (*i == "EXPECTED_MD5") {
      ++i;
      if (i == args.end()) {
        this->SetError("DOWNLOAD missing sum value for EXPECTED_MD5.");
        return false;
      }
      hash =
        CM_AUTO_PTR<cmCryptoHash>(new cmCryptoHash(cmCryptoHash::AlgoMD5));
      hashMatchMSG = "MD5 sum";
      expectedHash = cmSystemTools::LowerCase(*i);
    } else if (*i == "SHOW_PROGRESS") {
      showProgress = true;
    } else if (*i == "EXPECTED_HASH") {
      ++i;
      if (i == args.end()) {
        this->SetError("DOWNLOAD missing ALGO=value for EXPECTED_HASH.");
        return false;
      }
      std::string::size_type pos = i->find("=");
      if (pos == std::string::npos) {
        std::string err =
          "DOWNLOAD EXPECTED_HASH expects ALGO=value but got: ";
        err += *i;
        this->SetError(err);
        return false;
      }
      std::string algo = i->substr(0, pos);
      expectedHash = cmSystemTools::LowerCase(i->substr(pos + 1));
      hash = CM_AUTO_PTR<cmCryptoHash>(cmCryptoHash::New(algo.c_str()));
      if (!hash.get()) {
        std::string err = "DOWNLOAD EXPECTED_HASH given unknown ALGO: ";
        err += algo;
        this->SetError(err);
        return false;
      }
      hashMatchMSG = algo + " hash";
    } else if (*i == "USERPWD") {
      ++i;
      if (i == args.end()) {
        this->SetError("DOWNLOAD missing string for USERPWD.");
        return false;
      }
      userpwd = *i;
    } else if (*i == "HTTPHEADER") {
      ++i;
      if (i == args.end()) {
        this->SetError("DOWNLOAD missing string for HTTPHEADER.");
        return false;
      }
      curl_headers.push_back(*i);
    } else {
      // Do not return error for compatibility reason.
      std::string err = "Unexpected argument: ";
      err += *i;
      this->Makefile->IssueMessage(cmake::AUTHOR_WARNING, err);
    }
    ++i;
  }
  // If file exists already, and caller specified an expected md5 or sha,
  // and the existing file already has the expected hash, then simply
  // return.
  //
  if (cmSystemTools::FileExists(file.c_str()) && hash.get()) {
    std::string msg;
    std::string actualHash = hash->HashFile(file);
    if (actualHash == expectedHash) {
      msg = "returning early; file already exists with expected ";
      msg += hashMatchMSG;
      msg += "\"";
      if (!statusVar.empty()) {
        std::ostringstream result;
        result << (int)0 << ";\"" << msg;
        this->Makefile->AddDefinition(statusVar, result.str().c_str());
      }
      return true;
    }
  }
  // Make sure parent directory exists so we can write to the file
  // as we receive downloaded bits from curl...
  //
  std::string dir = cmSystemTools::GetFilenamePath(file);
  if (!cmSystemTools::FileExists(dir.c_str()) &&
      !cmSystemTools::MakeDirectory(dir.c_str())) {
    std::string errstring = "DOWNLOAD error: cannot create directory '" + dir +
      "' - Specify file by full path name and verify that you "
      "have directory creation and file write privileges.";
    this->SetError(errstring);
    return false;
  }

  cmsys::ofstream fout(file.c_str(), std::ios::binary);
  if (!fout) {
    this->SetError("DOWNLOAD cannot open file for write.");
    return false;
  }

#if defined(_WIN32)
  url = fix_file_url_windows(url);
#endif

  ::CURL* curl;
  ::curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = ::curl_easy_init();
  if (!curl) {
    this->SetError("DOWNLOAD error initializing curl.");
    return false;
  }

  cURLEasyGuard g_curl(curl);
  ::CURLcode res = ::curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  check_curl_result(res, "DOWNLOAD cannot set url: ");

  // enable HTTP ERROR parsing
  res = ::curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
  check_curl_result(res, "DOWNLOAD cannot set http failure option: ");

  res = ::curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/" LIBCURL_VERSION);
  check_curl_result(res, "DOWNLOAD cannot set user agent option: ");

  res = ::curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cmWriteToFileCallback);
  check_curl_result(res, "DOWNLOAD cannot set write function: ");

  res = ::curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION,
                           cmFileCommandCurlDebugCallback);
  check_curl_result(res, "DOWNLOAD cannot set debug function: ");

  // check to see if TLS verification is requested
  if (tls_verify) {
    res = ::curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
    check_curl_result(res, "Unable to set TLS/SSL Verify on: ");
  } else {
    res = ::curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    check_curl_result(res, "Unable to set TLS/SSL Verify off: ");
  }
  // check to see if a CAINFO file has been specified
  // command arg comes first
  std::string const& cainfo_err = cmCurlSetCAInfo(curl, cainfo);
  if (!cainfo_err.empty()) {
    this->SetError(cainfo_err);
    return false;
  }

  cmFileCommandVectorOfChar chunkDebug;

  res = ::curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&fout);
  check_curl_result(res, "DOWNLOAD cannot set write data: ");

  res = ::curl_easy_setopt(curl, CURLOPT_DEBUGDATA, (void*)&chunkDebug);
  check_curl_result(res, "DOWNLOAD cannot set debug data: ");

  res = ::curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  check_curl_result(res, "DOWNLOAD cannot set follow-redirect option: ");

  if (!logVar.empty()) {
    res = ::curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    check_curl_result(res, "DOWNLOAD cannot set verbose: ");
  }

  if (timeout > 0) {
    res = ::curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    check_curl_result(res, "DOWNLOAD cannot set timeout: ");
  }

  if (inactivity_timeout > 0) {
    // Give up if there is no progress for a long time.
    ::curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1);
    ::curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, inactivity_timeout);
  }

  // Need the progress helper's scope to last through the duration of
  // the curl_easy_perform call... so this object is declared at function
  // scope intentionally, rather than inside the "if(showProgress)"
  // block...
  //
  cURLProgressHelper helper(this, "download");

  if (showProgress) {
    res = ::curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    check_curl_result(res, "DOWNLOAD cannot set noprogress value: ");

    res = ::curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION,
                             cmFileDownloadProgressCallback);
    check_curl_result(res, "DOWNLOAD cannot set progress function: ");

    res = ::curl_easy_setopt(curl, CURLOPT_PROGRESSDATA,
                             reinterpret_cast<void*>(&helper));
    check_curl_result(res, "DOWNLOAD cannot set progress data: ");
  }

  if (!userpwd.empty()) {
    res = ::curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd.c_str());
    check_curl_result(res, "DOWNLOAD cannot set user password: ");
  }

  struct curl_slist* headers = CM_NULLPTR;
  for (std::vector<std::string>::const_iterator h = curl_headers.begin();
       h != curl_headers.end(); ++h) {
    headers = ::curl_slist_append(headers, h->c_str());
  }
  ::curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  res = ::curl_easy_perform(curl);

  ::curl_slist_free_all(headers);

  /* always cleanup */
  g_curl.release();
  ::curl_easy_cleanup(curl);

  if (!statusVar.empty()) {
    std::ostringstream result;
    result << (int)res << ";\"" << ::curl_easy_strerror(res) << "\"";
    this->Makefile->AddDefinition(statusVar, result.str().c_str());
  }

  ::curl_global_cleanup();

  // Explicitly flush/close so we can measure the md5 accurately.
  //
  fout.flush();
  fout.close();

  // Verify MD5 sum if requested:
  //
  if (hash.get()) {
    std::string actualHash = hash->HashFile(file);
    if (actualHash.empty()) {
      this->SetError("DOWNLOAD cannot compute hash on downloaded file");
      return false;
    }

    if (expectedHash != actualHash) {
      std::ostringstream oss;
      oss << "DOWNLOAD HASH mismatch" << std::endl
          << "  for file: [" << file << "]" << std::endl
          << "    expected hash: [" << expectedHash << "]" << std::endl
          << "      actual hash: [" << actualHash << "]" << std::endl
          << "           status: [" << (int)res << ";\""
          << ::curl_easy_strerror(res) << "\"]" << std::endl;

      if (!statusVar.empty() && res == 0) {
        std::string status = "1;HASH mismatch: "
                             "expected: " +
          expectedHash + " actual: " + actualHash;
        this->Makefile->AddDefinition(statusVar, status.c_str());
      }

      this->SetError(oss.str());
      return false;
    }
  }

  if (!logVar.empty()) {
    chunkDebug.push_back(0);
    this->Makefile->AddDefinition(logVar, &*chunkDebug.begin());
  }

  return true;
#else
  this->SetError("DOWNLOAD not supported by bootstrap cmake.");
  return false;
#endif
}

bool cmFileCommand::HandleUploadCommand(std::vector<std::string> const& args)
{
#if defined(CMAKE_BUILD_WITH_CMAKE)
  if (args.size() < 3) {
    this->SetError("UPLOAD must be called with at least three arguments.");
    return false;
  }
  std::vector<std::string>::const_iterator i = args.begin();
  ++i;
  std::string filename = *i;
  ++i;
  std::string url = *i;
  ++i;

  long timeout = 0;
  long inactivity_timeout = 0;
  std::string logVar;
  std::string statusVar;
  bool showProgress = false;
  std::string userpwd;

  std::vector<std::string> curl_headers;

  while (i != args.end()) {
    if (*i == "TIMEOUT") {
      ++i;
      if (i != args.end()) {
        timeout = atol(i->c_str());
      } else {
        this->SetError("UPLOAD missing time for TIMEOUT.");
        return false;
      }
    } else if (*i == "INACTIVITY_TIMEOUT") {
      ++i;
      if (i != args.end()) {
        inactivity_timeout = atol(i->c_str());
      } else {
        this->SetError("UPLOAD missing time for INACTIVITY_TIMEOUT.");
        return false;
      }
    } else if (*i == "LOG") {
      ++i;
      if (i == args.end()) {
        this->SetError("UPLOAD missing VAR for LOG.");
        return false;
      }
      logVar = *i;
    } else if (*i == "STATUS") {
      ++i;
      if (i == args.end()) {
        this->SetError("UPLOAD missing VAR for STATUS.");
        return false;
      }
      statusVar = *i;
    } else if (*i == "SHOW_PROGRESS") {
      showProgress = true;
    } else if (*i == "USERPWD") {
      ++i;
      if (i == args.end()) {
        this->SetError("UPLOAD missing string for USERPWD.");
        return false;
      }
      userpwd = *i;
    } else if (*i == "HTTPHEADER") {
      ++i;
      if (i == args.end()) {
        this->SetError("UPLOAD missing string for HTTPHEADER.");
        return false;
      }
      curl_headers.push_back(*i);
    } else {
      // Do not return error for compatibility reason.
      std::string err = "Unexpected argument: ";
      err += *i;
      this->Makefile->IssueMessage(cmake::AUTHOR_WARNING, err);
    }

    ++i;
  }

  // Open file for reading:
  //
  FILE* fin = cmsys::SystemTools::Fopen(filename, "rb");
  if (!fin) {
    std::string errStr = "UPLOAD cannot open file '";
    errStr += filename + "' for reading.";
    this->SetError(errStr);
    return false;
  }

  unsigned long file_size = cmsys::SystemTools::FileLength(filename);

#if defined(_WIN32)
  url = fix_file_url_windows(url);
#endif

  ::CURL* curl;
  ::curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = ::curl_easy_init();
  if (!curl) {
    this->SetError("UPLOAD error initializing curl.");
    fclose(fin);
    return false;
  }

  cURLEasyGuard g_curl(curl);

  // enable HTTP ERROR parsing
  ::CURLcode res = ::curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
  check_curl_result(res, "UPLOAD cannot set fail on error flag: ");

  // enable uploading
  res = ::curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
  check_curl_result(res, "UPLOAD cannot set upload flag: ");

  res = ::curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  check_curl_result(res, "UPLOAD cannot set url: ");

  res =
    ::curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cmWriteToMemoryCallback);
  check_curl_result(res, "UPLOAD cannot set write function: ");

  res = ::curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION,
                           cmFileCommandCurlDebugCallback);
  check_curl_result(res, "UPLOAD cannot set debug function: ");

  cmFileCommandVectorOfChar chunkResponse;
  cmFileCommandVectorOfChar chunkDebug;

  res = ::curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunkResponse);
  check_curl_result(res, "UPLOAD cannot set write data: ");

  res = ::curl_easy_setopt(curl, CURLOPT_DEBUGDATA, (void*)&chunkDebug);
  check_curl_result(res, "UPLOAD cannot set debug data: ");

  res = ::curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  check_curl_result(res, "UPLOAD cannot set follow-redirect option: ");

  if (!logVar.empty()) {
    res = ::curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    check_curl_result(res, "UPLOAD cannot set verbose: ");
  }

  if (timeout > 0) {
    res = ::curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    check_curl_result(res, "UPLOAD cannot set timeout: ");
  }

  if (inactivity_timeout > 0) {
    // Give up if there is no progress for a long time.
    ::curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1);
    ::curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, inactivity_timeout);
  }

  // Need the progress helper's scope to last through the duration of
  // the curl_easy_perform call... so this object is declared at function
  // scope intentionally, rather than inside the "if(showProgress)"
  // block...
  //
  cURLProgressHelper helper(this, "upload");

  if (showProgress) {
    res = ::curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    check_curl_result(res, "UPLOAD cannot set noprogress value: ");

    res = ::curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION,
                             cmFileUploadProgressCallback);
    check_curl_result(res, "UPLOAD cannot set progress function: ");

    res = ::curl_easy_setopt(curl, CURLOPT_PROGRESSDATA,
                             reinterpret_cast<void*>(&helper));
    check_curl_result(res, "UPLOAD cannot set progress data: ");
  }

  // now specify which file to upload
  res = ::curl_easy_setopt(curl, CURLOPT_INFILE, fin);
  check_curl_result(res, "UPLOAD cannot set input file: ");

  // and give the size of the upload (optional)
  res =
    ::curl_easy_setopt(curl, CURLOPT_INFILESIZE, static_cast<long>(file_size));
  check_curl_result(res, "UPLOAD cannot set input file size: ");

  if (!userpwd.empty()) {
    res = ::curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd.c_str());
    check_curl_result(res, "UPLOAD cannot set user password: ");
  }

  struct curl_slist* headers = CM_NULLPTR;
  for (std::vector<std::string>::const_iterator h = curl_headers.begin();
       h != curl_headers.end(); ++h) {
    headers = ::curl_slist_append(headers, h->c_str());
  }
  ::curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  res = ::curl_easy_perform(curl);

  ::curl_slist_free_all(headers);

  /* always cleanup */
  g_curl.release();
  ::curl_easy_cleanup(curl);

  if (!statusVar.empty()) {
    std::ostringstream result;
    result << (int)res << ";\"" << ::curl_easy_strerror(res) << "\"";
    this->Makefile->AddDefinition(statusVar, result.str().c_str());
  }

  ::curl_global_cleanup();

  fclose(fin);
  fin = CM_NULLPTR;

  if (!logVar.empty()) {
    std::string log;

    if (!chunkResponse.empty()) {
      chunkResponse.push_back(0);
      log += "Response:\n";
      log += &*chunkResponse.begin();
      log += "\n";
    }

    if (!chunkDebug.empty()) {
      chunkDebug.push_back(0);
      log += "Debug:\n";
      log += &*chunkDebug.begin();
      log += "\n";
    }

    this->Makefile->AddDefinition(logVar, log.c_str());
  }

  return true;
#else
  this->SetError("UPLOAD not supported by bootstrap cmake.");
  return false;
#endif
}

void cmFileCommand::AddEvaluationFile(const std::string& inputName,
                                      const std::string& outputExpr,
                                      const std::string& condition,
                                      bool inputIsContent)
{
  cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();

  cmGeneratorExpression outputGe(lfbt);
  CM_AUTO_PTR<cmCompiledGeneratorExpression> outputCge =
    outputGe.Parse(outputExpr);

  cmGeneratorExpression conditionGe(lfbt);
  CM_AUTO_PTR<cmCompiledGeneratorExpression> conditionCge =
    conditionGe.Parse(condition);

  this->Makefile->AddEvaluationFile(inputName, outputCge, conditionCge,
                                    inputIsContent);
}

bool cmFileCommand::HandleGenerateCommand(std::vector<std::string> const& args)
{
  if (args.size() < 5) {
    this->SetError("Incorrect arguments to GENERATE subcommand.");
    return false;
  }
  if (args[1] != "OUTPUT") {
    this->SetError("Incorrect arguments to GENERATE subcommand.");
    return false;
  }
  std::string condition;
  if (args.size() > 5) {
    if (args[5] != "CONDITION") {
      this->SetError("Incorrect arguments to GENERATE subcommand.");
      return false;
    }
    if (args.size() != 7) {
      this->SetError("Incorrect arguments to GENERATE subcommand.");
      return false;
    }
    condition = args[6];
    if (condition.empty()) {
      this->SetError("CONDITION of sub-command GENERATE must not be empty if "
                     "specified.");
      return false;
    }
  }
  std::string output = args[2];
  const bool inputIsContent = args[3] != "INPUT";
  if (inputIsContent && args[3] != "CONTENT") {
    this->SetError("Incorrect arguments to GENERATE subcommand.");
    return false;
  }
  std::string input = args[4];

  this->AddEvaluationFile(input, output, condition, inputIsContent);
  return true;
}

bool cmFileCommand::HandleLockCommand(std::vector<std::string> const& args)
{
#if defined(CMAKE_BUILD_WITH_CMAKE)
  // Default values
  bool directory = false;
  bool release = false;
  enum Guard
  {
    GUARD_FUNCTION,
    GUARD_FILE,
    GUARD_PROCESS
  };
  Guard guard = GUARD_PROCESS;
  std::string resultVariable;
  unsigned long timeout = static_cast<unsigned long>(-1);

  // Parse arguments
  if (args.size() < 2) {
    this->Makefile->IssueMessage(
      cmake::FATAL_ERROR, "sub-command LOCK requires at least two arguments.");
    return false;
  }

  std::string path = args[1];
  for (unsigned i = 2; i < args.size(); ++i) {
    if (args[i] == "DIRECTORY") {
      directory = true;
    } else if (args[i] == "RELEASE") {
      release = true;
    } else if (args[i] == "GUARD") {
      ++i;
      const char* merr = "expected FUNCTION, FILE or PROCESS after GUARD";
      if (i >= args.size()) {
        this->Makefile->IssueMessage(cmake::FATAL_ERROR, merr);
        return false;
      }
      if (args[i] == "FUNCTION") {
        guard = GUARD_FUNCTION;
      } else if (args[i] == "FILE") {
        guard = GUARD_FILE;
      } else if (args[i] == "PROCESS") {
        guard = GUARD_PROCESS;
      } else {
        std::ostringstream e;
        e << merr << ", but got:\n  \"" << args[i] << "\".";
        this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
        return false;
      }

    } else if (args[i] == "RESULT_VARIABLE") {
      ++i;
      if (i >= args.size()) {
        this->Makefile->IssueMessage(
          cmake::FATAL_ERROR, "expected variable name after RESULT_VARIABLE");
        return false;
      }
      resultVariable = args[i];
    } else if (args[i] == "TIMEOUT") {
      ++i;
      if (i >= args.size()) {
        this->Makefile->IssueMessage(cmake::FATAL_ERROR,
                                     "expected timeout value after TIMEOUT");
        return false;
      }
      long scanned;
      if (!cmSystemTools::StringToLong(args[i].c_str(), &scanned) ||
          scanned < 0) {
        std::ostringstream e;
        e << "TIMEOUT value \"" << args[i] << "\" is not an unsigned integer.";
        this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
        return false;
      }
      timeout = static_cast<unsigned long>(scanned);
    } else {
      std::ostringstream e;
      e << "expected DIRECTORY, RELEASE, GUARD, RESULT_VARIABLE or TIMEOUT\n";
      e << "but got: \"" << args[i] << "\".";
      this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
      return false;
    }
  }

  if (directory) {
    path += "/cmake.lock";
  }

  if (!cmsys::SystemTools::FileIsFullPath(path)) {
    path = this->Makefile->GetCurrentSourceDirectory() + ("/" + path);
  }

  // Unify path (remove '//', '/../', ...)
  path = cmSystemTools::CollapseFullPath(path);

  // Create file and directories if needed
  std::string parentDir = cmSystemTools::GetParentDirectory(path);
  if (!cmSystemTools::MakeDirectory(parentDir)) {
    std::ostringstream e;
    e << "directory\n  \"" << parentDir << "\"\ncreation failed ";
    e << "(check permissions).";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    cmSystemTools::SetFatalErrorOccured();
    return false;
  }
  FILE* file = cmsys::SystemTools::Fopen(path, "w");
  if (!file) {
    std::ostringstream e;
    e << "file\n  \"" << path << "\"\ncreation failed (check permissions).";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    cmSystemTools::SetFatalErrorOccured();
    return false;
  }
  fclose(file);

  // Actual lock/unlock
  cmFileLockPool& lockPool =
    this->Makefile->GetGlobalGenerator()->GetFileLockPool();

  cmFileLockResult fileLockResult(cmFileLockResult::MakeOk());
  if (release) {
    fileLockResult = lockPool.Release(path);
  } else {
    switch (guard) {
      case GUARD_FUNCTION:
        fileLockResult = lockPool.LockFunctionScope(path, timeout);
        break;
      case GUARD_FILE:
        fileLockResult = lockPool.LockFileScope(path, timeout);
        break;
      case GUARD_PROCESS:
        fileLockResult = lockPool.LockProcessScope(path, timeout);
        break;
      default:
        cmSystemTools::SetFatalErrorOccured();
        return false;
    }
  }

  const std::string result = fileLockResult.GetOutputMessage();

  if (resultVariable.empty() && !fileLockResult.IsOk()) {
    std::ostringstream e;
    e << "error locking file\n  \"" << path << "\"\n" << result << ".";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    cmSystemTools::SetFatalErrorOccured();
    return false;
  }

  if (!resultVariable.empty()) {
    this->Makefile->AddDefinition(resultVariable, result.c_str());
  }

  return true;
#else
  static_cast<void>(args);
  this->SetError("sub-command LOCK not implemented in bootstrap cmake");
  return false;
#endif
}

bool cmFileCommand::HandleTimestampCommand(
  std::vector<std::string> const& args)
{
  if (args.size() < 3) {
    this->SetError("sub-command TIMESTAMP requires at least two arguments.");
    return false;
  }
  if (args.size() > 5) {
    this->SetError("sub-command TIMESTAMP takes at most four arguments.");
    return false;
  }

  unsigned int argsIndex = 1;

  const std::string& filename = args[argsIndex++];

  const std::string& outputVariable = args[argsIndex++];

  std::string formatString;
  if (args.size() > argsIndex && args[argsIndex] != "UTC") {
    formatString = args[argsIndex++];
  }

  bool utcFlag = false;
  if (args.size() > argsIndex) {
    if (args[argsIndex] == "UTC") {
      utcFlag = true;
    } else {
      std::string e = " TIMESTAMP sub-command does not recognize option " +
        args[argsIndex] + ".";
      this->SetError(e);
      return false;
    }
  }

  cmTimestamp timestamp;
  std::string result =
    timestamp.FileModificationTime(filename.c_str(), formatString, utcFlag);
  this->Makefile->AddDefinition(outputVariable, result.c_str());

  return true;
}
