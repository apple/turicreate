/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmSystemTools.h"

#include "cmAlgorithms.h"
#include "cmProcessOutput.h"
#include "cm_sys_stat.h"

#if defined(CMAKE_BUILD_WITH_CMAKE)
#include "cmArchiveWrite.h"
#include "cmLocale.h"
#include "cm_libarchive.h"
#ifndef __LA_INT64_T
#define __LA_INT64_T la_int64_t
#endif
#endif

#if defined(CMAKE_BUILD_WITH_CMAKE)
#include "cmCryptoHash.h"
#endif

#if defined(CMAKE_USE_ELF_PARSER)
#include "cmELF.h"
#endif

#if defined(CMAKE_USE_MACH_PARSER)
#include "cmMachO.h"
#endif

#include "cmsys/Directory.hxx"
#include "cmsys/Encoding.hxx"
#include "cmsys/FStream.hxx"
#include "cmsys/RegularExpression.hxx"
#include "cmsys/System.h"
#include "cmsys/Terminal.h"
#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <iostream>
#include <set>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <utility>

#if defined(_WIN32)
#include <windows.h>
// include wincrypt.h after windows.h
#include <wincrypt.h>
#else
#include <sys/time.h>
#include <unistd.h>
#include <utime.h>
#endif

#if defined(_WIN32) &&                                                        \
  (defined(_MSC_VER) || defined(__WATCOMC__) || defined(__MINGW32__))
#include <io.h>
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#ifdef __QNX__
#include <malloc.h> /* for malloc/free on QNX */
#endif

static bool cm_isspace(char c)
{
  return ((c & 0x80) == 0) && isspace(c);
}

class cmSystemToolsFileTime
{
public:
#if defined(_WIN32) && !defined(__CYGWIN__)
  FILETIME timeCreation;
  FILETIME timeLastAccess;
  FILETIME timeLastWrite;
#else
  struct utimbuf timeBuf;
#endif
};

#if !defined(HAVE_ENVIRON_NOT_REQUIRE_PROTOTYPE)
// For GetEnvironmentVariables
#if defined(_WIN32)
extern __declspec(dllimport) char** environ;
#else
extern char** environ;
#endif
#endif

#if defined(CMAKE_BUILD_WITH_CMAKE)
static std::string cm_archive_entry_pathname(struct archive_entry* entry)
{
#if cmsys_STL_HAS_WSTRING
  return cmsys::Encoding::ToNarrow(archive_entry_pathname_w(entry));
#else
  return archive_entry_pathname(entry);
#endif
}

static int cm_archive_read_open_file(struct archive* a, const char* file,
                                     int block_size)
{
#if cmsys_STL_HAS_WSTRING
  std::wstring wfile = cmsys::Encoding::ToWide(file);
  return archive_read_open_filename_w(a, wfile.c_str(), block_size);
#else
  return archive_read_open_filename(a, file, block_size);
#endif
}
#endif

#ifdef _WIN32
class cmSystemToolsWindowsHandle
{
public:
  cmSystemToolsWindowsHandle(HANDLE h)
    : handle_(h)
  {
  }
  ~cmSystemToolsWindowsHandle()
  {
    if (this->handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(this->handle_);
    }
  }
  operator bool() const { return this->handle_ != INVALID_HANDLE_VALUE; }
  bool operator!() const { return this->handle_ == INVALID_HANDLE_VALUE; }
  operator HANDLE() const { return this->handle_; }

private:
  HANDLE handle_;
};
#elif defined(__APPLE__)
#include <crt_externs.h>

#define environ (*_NSGetEnviron())
#endif

bool cmSystemTools::s_RunCommandHideConsole = false;
bool cmSystemTools::s_DisableRunCommandOutput = false;
bool cmSystemTools::s_ErrorOccured = false;
bool cmSystemTools::s_FatalErrorOccured = false;
bool cmSystemTools::s_DisableMessages = false;
bool cmSystemTools::s_ForceUnixPaths = false;

cmSystemTools::MessageCallback cmSystemTools::s_MessageCallback;
cmSystemTools::OutputCallback cmSystemTools::s_StdoutCallback;
cmSystemTools::OutputCallback cmSystemTools::s_StderrCallback;
cmSystemTools::InterruptCallback cmSystemTools::s_InterruptCallback;
void* cmSystemTools::s_MessageCallbackClientData;
void* cmSystemTools::s_StdoutCallbackClientData;
void* cmSystemTools::s_StderrCallbackClientData;
void* cmSystemTools::s_InterruptCallbackClientData;

// replace replace with with as many times as it shows up in source.
// write the result into source.
#if defined(_WIN32) && !defined(__CYGWIN__)
void cmSystemTools::ExpandRegistryValues(std::string& source, KeyWOW64 view)
{
  // Regular expression to match anything inside [...] that begins in HKEY.
  // Note that there is a special rule for regular expressions to match a
  // close square-bracket inside a list delimited by square brackets.
  // The "[^]]" part of this expression will match any character except
  // a close square-bracket.  The ']' character must be the first in the
  // list of characters inside the [^...] block of the expression.
  cmsys::RegularExpression regEntry("\\[(HKEY[^]]*)\\]");

  // check for black line or comment
  while (regEntry.find(source)) {
    // the arguments are the second match
    std::string key = regEntry.match(1);
    std::string val;
    if (ReadRegistryValue(key.c_str(), val, view)) {
      std::string reg = "[";
      reg += key + "]";
      cmSystemTools::ReplaceString(source, reg.c_str(), val.c_str());
    } else {
      std::string reg = "[";
      reg += key + "]";
      cmSystemTools::ReplaceString(source, reg.c_str(), "/registry");
    }
  }
}
#else
void cmSystemTools::ExpandRegistryValues(std::string& source,
                                         KeyWOW64 /*unused*/)
{
  cmsys::RegularExpression regEntry("\\[(HKEY[^]]*)\\]");
  while (regEntry.find(source)) {
    // the arguments are the second match
    std::string key = regEntry.match(1);
    std::string reg = "[";
    reg += key + "]";
    cmSystemTools::ReplaceString(source, reg.c_str(), "/registry");
  }
}
#endif

std::string cmSystemTools::EscapeQuotes(const std::string& str)
{
  std::string result;
  result.reserve(str.size());
  for (const char* ch = str.c_str(); *ch != '\0'; ++ch) {
    if (*ch == '"') {
      result += '\\';
    }
    result += *ch;
  }
  return result;
}

std::string cmSystemTools::HelpFileName(std::string name)
{
  cmSystemTools::ReplaceString(name, "<", "");
  cmSystemTools::ReplaceString(name, ">", "");
  return name;
}

std::string cmSystemTools::TrimWhitespace(const std::string& s)
{
  std::string::const_iterator start = s.begin();
  while (start != s.end() && cm_isspace(*start)) {
    ++start;
  }
  if (start == s.end()) {
    return "";
  }
  std::string::const_iterator stop = s.end() - 1;
  while (cm_isspace(*stop)) {
    --stop;
  }
  return std::string(start, stop + 1);
}

void cmSystemTools::Error(const char* m1, const char* m2, const char* m3,
                          const char* m4)
{
  std::string message = "CMake Error: ";
  if (m1) {
    message += m1;
  }
  if (m2) {
    message += m2;
  }
  if (m3) {
    message += m3;
  }
  if (m4) {
    message += m4;
  }
  cmSystemTools::s_ErrorOccured = true;
  cmSystemTools::Message(message.c_str(), "Error");
}

void cmSystemTools::SetInterruptCallback(InterruptCallback f, void* clientData)
{
  s_InterruptCallback = f;
  s_InterruptCallbackClientData = clientData;
}

bool cmSystemTools::GetInterruptFlag()
{
  if (s_InterruptCallback) {
    return (*s_InterruptCallback)(s_InterruptCallbackClientData);
  }
  return false;
}

void cmSystemTools::SetMessageCallback(MessageCallback f, void* clientData)
{
  s_MessageCallback = f;
  s_MessageCallbackClientData = clientData;
}

void cmSystemTools::SetStdoutCallback(OutputCallback f, void* clientData)
{
  s_StdoutCallback = f;
  s_StdoutCallbackClientData = clientData;
}

void cmSystemTools::SetStderrCallback(OutputCallback f, void* clientData)
{
  s_StderrCallback = f;
  s_StderrCallbackClientData = clientData;
}

void cmSystemTools::Stdout(const char* s)
{
  cmSystemTools::Stdout(s, strlen(s));
}

void cmSystemTools::Stderr(const char* s)
{
  cmSystemTools::Stderr(s, strlen(s));
}

void cmSystemTools::Stderr(const char* s, size_t length)
{
  if (s_StderrCallback) {
    (*s_StderrCallback)(s, length, s_StderrCallbackClientData);
  } else {
    std::cerr.write(s, length);
    std::cerr.flush();
  }
}

void cmSystemTools::Stdout(const char* s, size_t length)
{
  if (s_StdoutCallback) {
    (*s_StdoutCallback)(s, length, s_StdoutCallbackClientData);
  } else {
    std::cout.write(s, length);
    std::cout.flush();
  }
}

void cmSystemTools::Message(const char* m1, const char* title)
{
  if (s_DisableMessages) {
    return;
  }
  if (s_MessageCallback) {
    (*s_MessageCallback)(m1, title, s_DisableMessages,
                         s_MessageCallbackClientData);
    return;
  }
  std::cerr << m1 << std::endl << std::flush;
}

void cmSystemTools::ReportLastSystemError(const char* msg)
{
  std::string m = msg;
  m += ": System Error: ";
  m += Superclass::GetLastSystemError();
  cmSystemTools::Error(m.c_str());
}

bool cmSystemTools::IsInternallyOn(const char* val)
{
  if (!val) {
    return false;
  }
  std::string v = val;
  if (v.size() > 4) {
    return false;
  }

  for (std::string::iterator c = v.begin(); c != v.end(); c++) {
    *c = static_cast<char>(toupper(*c));
  }
  return v == "I_ON";
}

bool cmSystemTools::IsOn(const char* val)
{
  if (!val) {
    return false;
  }
  size_t len = strlen(val);
  if (len > 4) {
    return false;
  }
  std::string v(val, len);

  static std::set<std::string> onValues;
  if (onValues.empty()) {
    onValues.insert("ON");
    onValues.insert("1");
    onValues.insert("YES");
    onValues.insert("TRUE");
    onValues.insert("Y");
  }
  for (std::string::iterator c = v.begin(); c != v.end(); c++) {
    *c = static_cast<char>(toupper(*c));
  }
  return (onValues.count(v) > 0);
}

bool cmSystemTools::IsNOTFOUND(const char* val)
{
  if (strcmp(val, "NOTFOUND") == 0) {
    return true;
  }
  return cmHasLiteralSuffix(val, "-NOTFOUND");
}

bool cmSystemTools::IsOff(const char* val)
{
  if (!val || !*val) {
    return true;
  }
  size_t len = strlen(val);
  // Try and avoid toupper() for large strings.
  if (len > 6) {
    return cmSystemTools::IsNOTFOUND(val);
  }

  static std::set<std::string> offValues;
  if (offValues.empty()) {
    offValues.insert("OFF");
    offValues.insert("0");
    offValues.insert("NO");
    offValues.insert("FALSE");
    offValues.insert("N");
    offValues.insert("IGNORE");
  }
  // Try and avoid toupper().
  std::string v(val, len);
  for (std::string::iterator c = v.begin(); c != v.end(); c++) {
    *c = static_cast<char>(toupper(*c));
  }
  return (offValues.count(v) > 0);
}

void cmSystemTools::ParseWindowsCommandLine(const char* command,
                                            std::vector<std::string>& args)
{
  // See the MSDN document "Parsing C Command-Line Arguments" at
  // http://msdn2.microsoft.com/en-us/library/a1y7w461.aspx for rules
  // of parsing the windows command line.

  bool in_argument = false;
  bool in_quotes = false;
  int backslashes = 0;
  std::string arg;
  for (const char* c = command; *c; ++c) {
    if (*c == '\\') {
      ++backslashes;
      in_argument = true;
    } else if (*c == '"') {
      int backslash_pairs = backslashes >> 1;
      int backslash_escaped = backslashes & 1;
      arg.append(backslash_pairs, '\\');
      backslashes = 0;
      if (backslash_escaped) {
        /* An odd number of backslashes precede this quote.
           It is escaped.  */
        arg.append(1, '"');
      } else {
        /* An even number of backslashes precede this quote.
           It is not escaped.  */
        in_quotes = !in_quotes;
      }
      in_argument = true;
    } else {
      arg.append(backslashes, '\\');
      backslashes = 0;
      if (cm_isspace(*c)) {
        if (in_quotes) {
          arg.append(1, *c);
        } else if (in_argument) {
          args.push_back(arg);
          arg = "";
          in_argument = false;
        }
      } else {
        in_argument = true;
        arg.append(1, *c);
      }
    }
  }
  arg.append(backslashes, '\\');
  if (in_argument) {
    args.push_back(arg);
  }
}

class cmSystemToolsArgV
{
  char** ArgV;

public:
  cmSystemToolsArgV(char** argv)
    : ArgV(argv)
  {
  }
  ~cmSystemToolsArgV()
  {
    for (char** arg = this->ArgV; arg && *arg; ++arg) {
      free(*arg);
    }
    free(this->ArgV);
  }
  void Store(std::vector<std::string>& args) const
  {
    for (char** arg = this->ArgV; arg && *arg; ++arg) {
      args.push_back(*arg);
    }
  }
};

void cmSystemTools::ParseUnixCommandLine(const char* command,
                                         std::vector<std::string>& args)
{
  // Invoke the underlying parser.
  cmSystemToolsArgV argv = cmsysSystem_Parse_CommandForUnix(command, 0);
  argv.Store(args);
}

std::vector<std::string> cmSystemTools::HandleResponseFile(
  std::vector<std::string>::const_iterator argBeg,
  std::vector<std::string>::const_iterator argEnd)
{
  std::vector<std::string> arg_full;
  for (std::vector<std::string>::const_iterator a = argBeg; a != argEnd; ++a) {
    std::string const& arg = *a;
    if (cmHasLiteralPrefix(arg, "@")) {
      cmsys::ifstream responseFile(arg.substr(1).c_str(), std::ios::in);
      if (!responseFile) {
        std::string error = "failed to open for reading (";
        error += cmSystemTools::GetLastSystemError();
        error += "):\n  ";
        error += arg.substr(1);
        cmSystemTools::Error(error.c_str());
      } else {
        std::string line;
        cmSystemTools::GetLineFromStream(responseFile, line);
        std::vector<std::string> args2;
#ifdef _WIN32
        cmSystemTools::ParseWindowsCommandLine(line.c_str(), args2);
#else
        cmSystemTools::ParseUnixCommandLine(line.c_str(), args2);
#endif
        arg_full.insert(arg_full.end(), args2.begin(), args2.end());
      }
    } else {
      arg_full.push_back(arg);
    }
  }
  return arg_full;
}

std::vector<std::string> cmSystemTools::ParseArguments(const char* command)
{
  std::vector<std::string> args;
  std::string arg;

  bool win_path = false;

  if ((command[0] != '/' && command[1] == ':' && command[2] == '\\') ||
      (command[0] == '\"' && command[1] != '/' && command[2] == ':' &&
       command[3] == '\\') ||
      (command[0] == '\'' && command[1] != '/' && command[2] == ':' &&
       command[3] == '\\') ||
      (command[0] == '\\' && command[1] == '\\')) {
    win_path = true;
  }
  // Split the command into an argv array.
  for (const char* c = command; *c;) {
    // Skip over whitespace.
    while (*c == ' ' || *c == '\t') {
      ++c;
    }
    arg = "";
    if (*c == '"') {
      // Parse a quoted argument.
      ++c;
      while (*c && *c != '"') {
        arg.append(1, *c);
        ++c;
      }
      if (*c) {
        ++c;
      }
      args.push_back(arg);
    } else if (*c == '\'') {
      // Parse a quoted argument.
      ++c;
      while (*c && *c != '\'') {
        arg.append(1, *c);
        ++c;
      }
      if (*c) {
        ++c;
      }
      args.push_back(arg);
    } else if (*c) {
      // Parse an unquoted argument.
      while (*c && *c != ' ' && *c != '\t') {
        if (*c == '\\' && !win_path) {
          ++c;
          if (*c) {
            arg.append(1, *c);
            ++c;
          }
        } else {
          arg.append(1, *c);
          ++c;
        }
      }
      args.push_back(arg);
    }
  }

  return args;
}

size_t cmSystemTools::CalculateCommandLineLengthLimit()
{
  size_t sz =
#ifdef _WIN32
    // There's a maximum of 65536 bytes and thus 32768 WCHARs on Windows
    // However, cmd.exe itself can only handle 8191 WCHARs and Ninja for
    // example uses it to spawn processes.
    size_t(8191);
#elif defined(__linux)
    // MAX_ARG_STRLEN is the maximum length of a string permissible for
    // the execve() syscall on Linux. It's defined as (PAGE_SIZE * 32)
    // in Linux's binfmts.h
    static_cast<size_t>(sysconf(_SC_PAGESIZE) * 32);
#else
    size_t(0);
#endif

#if defined(_SC_ARG_MAX)
  // ARG_MAX is the maximum size of the command and environment
  // that can be passed to the exec functions on UNIX.
  // The value in limits.h does not need to be present and may
  // depend upon runtime memory constraints, hence sysconf()
  // should be used to query it.
  long szArgMax = sysconf(_SC_ARG_MAX);
  // A return value of -1 signifies an undetermined limit, but
  // it does not imply an infinite limit, and thus is ignored.
  if (szArgMax != -1) {
    // We estimate the size of the environment block to be 1000.
    // This isn't accurate at all, but leaves some headroom.
    szArgMax = szArgMax < 1000 ? 0 : szArgMax - 1000;
#if defined(_WIN32) || defined(__linux)
    sz = std::min(sz, static_cast<size_t>(szArgMax));
#else
    sz = static_cast<size_t>(szArgMax);
#endif
  }
#endif
  return sz;
}

bool cmSystemTools::RunSingleCommand(std::vector<std::string> const& command,
                                     std::string* captureStdOut,
                                     std::string* captureStdErr, int* retVal,
                                     const char* dir, OutputOption outputflag,
                                     double timeout, Encoding encoding)
{
  std::vector<const char*> argv;
  for (std::vector<std::string>::const_iterator a = command.begin();
       a != command.end(); ++a) {
    argv.push_back(a->c_str());
  }
  argv.push_back(CM_NULLPTR);

  cmsysProcess* cp = cmsysProcess_New();
  cmsysProcess_SetCommand(cp, &*argv.begin());
  cmsysProcess_SetWorkingDirectory(cp, dir);
  if (cmSystemTools::GetRunCommandHideConsole()) {
    cmsysProcess_SetOption(cp, cmsysProcess_Option_HideWindow, 1);
  }

  if (outputflag == OUTPUT_PASSTHROUGH) {
    cmsysProcess_SetPipeShared(cp, cmsysProcess_Pipe_STDOUT, 1);
    cmsysProcess_SetPipeShared(cp, cmsysProcess_Pipe_STDERR, 1);
    captureStdOut = CM_NULLPTR;
    captureStdErr = CM_NULLPTR;
  } else if (outputflag == OUTPUT_MERGE ||
             (captureStdErr && captureStdErr == captureStdOut)) {
    cmsysProcess_SetOption(cp, cmsysProcess_Option_MergeOutput, 1);
    captureStdErr = CM_NULLPTR;
  }
  assert(!captureStdErr || captureStdErr != captureStdOut);

  cmsysProcess_SetTimeout(cp, timeout);
  cmsysProcess_Execute(cp);

  std::vector<char> tempStdOut;
  std::vector<char> tempStdErr;
  char* data;
  int length;
  int pipe;
  cmProcessOutput processOutput(encoding);
  std::string strdata;
  if (outputflag != OUTPUT_PASSTHROUGH &&
      (captureStdOut || captureStdErr || outputflag != OUTPUT_NONE)) {
    while ((pipe = cmsysProcess_WaitForData(cp, &data, &length, CM_NULLPTR)) >
           0) {
      // Translate NULL characters in the output into valid text.
      for (int i = 0; i < length; ++i) {
        if (data[i] == '\0') {
          data[i] = ' ';
        }
      }

      if (pipe == cmsysProcess_Pipe_STDOUT) {
        if (outputflag != OUTPUT_NONE) {
          processOutput.DecodeText(data, length, strdata, 1);
          cmSystemTools::Stdout(strdata.c_str(), strdata.size());
        }
        if (captureStdOut) {
          tempStdOut.insert(tempStdOut.end(), data, data + length);
        }
      } else if (pipe == cmsysProcess_Pipe_STDERR) {
        if (outputflag != OUTPUT_NONE) {
          processOutput.DecodeText(data, length, strdata, 2);
          cmSystemTools::Stderr(strdata.c_str(), strdata.size());
        }
        if (captureStdErr) {
          tempStdErr.insert(tempStdErr.end(), data, data + length);
        }
      }
    }

    if (outputflag != OUTPUT_NONE) {
      processOutput.DecodeText(std::string(), strdata, 1);
      if (!strdata.empty()) {
        cmSystemTools::Stdout(strdata.c_str(), strdata.size());
      }
      processOutput.DecodeText(std::string(), strdata, 2);
      if (!strdata.empty()) {
        cmSystemTools::Stderr(strdata.c_str(), strdata.size());
      }
    }
  }

  cmsysProcess_WaitForExit(cp, CM_NULLPTR);

  if (captureStdOut) {
    captureStdOut->assign(tempStdOut.begin(), tempStdOut.end());
    processOutput.DecodeText(*captureStdOut, *captureStdOut);
  }
  if (captureStdErr) {
    captureStdErr->assign(tempStdErr.begin(), tempStdErr.end());
    processOutput.DecodeText(*captureStdErr, *captureStdErr);
  }

  bool result = true;
  if (cmsysProcess_GetState(cp) == cmsysProcess_State_Exited) {
    if (retVal) {
      *retVal = cmsysProcess_GetExitValue(cp);
    } else {
      if (cmsysProcess_GetExitValue(cp) != 0) {
        result = false;
      }
    }
  } else if (cmsysProcess_GetState(cp) == cmsysProcess_State_Exception) {
    const char* exception_str = cmsysProcess_GetExceptionString(cp);
    if (outputflag != OUTPUT_NONE) {
      std::cerr << exception_str << std::endl;
    }
    if (captureStdErr) {
      captureStdErr->append(exception_str, strlen(exception_str));
    }
    result = false;
  } else if (cmsysProcess_GetState(cp) == cmsysProcess_State_Error) {
    const char* error_str = cmsysProcess_GetErrorString(cp);
    if (outputflag != OUTPUT_NONE) {
      std::cerr << error_str << std::endl;
    }
    if (captureStdErr) {
      captureStdErr->append(error_str, strlen(error_str));
    }
    result = false;
  } else if (cmsysProcess_GetState(cp) == cmsysProcess_State_Expired) {
    const char* error_str = "Process terminated due to timeout\n";
    if (outputflag != OUTPUT_NONE) {
      std::cerr << error_str << std::endl;
    }
    if (captureStdErr) {
      captureStdErr->append(error_str, strlen(error_str));
    }
    result = false;
  }

  cmsysProcess_Delete(cp);
  return result;
}

bool cmSystemTools::RunSingleCommand(const char* command,
                                     std::string* captureStdOut,
                                     std::string* captureStdErr, int* retVal,
                                     const char* dir, OutputOption outputflag,
                                     double timeout)
{
  if (s_DisableRunCommandOutput) {
    outputflag = OUTPUT_NONE;
  }

  std::vector<std::string> args = cmSystemTools::ParseArguments(command);

  if (args.empty()) {
    return false;
  }
  return cmSystemTools::RunSingleCommand(args, captureStdOut, captureStdErr,
                                         retVal, dir, outputflag, timeout);
}

std::string cmSystemTools::PrintSingleCommand(
  std::vector<std::string> const& command)
{
  if (command.empty()) {
    return std::string();
  }

  return cmWrap('"', command, '"', " ");
}

bool cmSystemTools::DoesFileExistWithExtensions(
  const char* name, const std::vector<std::string>& headerExts)
{
  std::string hname;

  for (std::vector<std::string>::const_iterator ext = headerExts.begin();
       ext != headerExts.end(); ++ext) {
    hname = name;
    hname += ".";
    hname += *ext;
    if (cmSystemTools::FileExists(hname.c_str())) {
      return true;
    }
  }
  return false;
}

std::string cmSystemTools::FileExistsInParentDirectories(const char* fname,
                                                         const char* directory,
                                                         const char* toplevel)
{
  std::string file = fname;
  cmSystemTools::ConvertToUnixSlashes(file);
  std::string dir = directory;
  cmSystemTools::ConvertToUnixSlashes(dir);
  std::string prevDir;
  while (dir != prevDir) {
    std::string path = dir + "/" + file;
    if (cmSystemTools::FileExists(path.c_str())) {
      return path;
    }
    if (dir.size() < strlen(toplevel)) {
      break;
    }
    prevDir = dir;
    dir = cmSystemTools::GetParentDirectory(dir);
  }
  return "";
}

bool cmSystemTools::cmCopyFile(const char* source, const char* destination)
{
  return Superclass::CopyFileAlways(source, destination);
}

bool cmSystemTools::CopyFileIfDifferent(const char* source,
                                        const char* destination)
{
  return Superclass::CopyFileIfDifferent(source, destination);
}

#ifdef _WIN32
cmSystemTools::WindowsFileRetry cmSystemTools::GetWindowsFileRetry()
{
  static WindowsFileRetry retry = { 0, 0 };
  if (!retry.Count) {
    unsigned int data[2] = { 0, 0 };
    HKEY const keys[2] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
    wchar_t const* const values[2] = { L"FilesystemRetryCount",
                                       L"FilesystemRetryDelay" };
    for (int k = 0; k < 2; ++k) {
      HKEY hKey;
      if (RegOpenKeyExW(keys[k], L"Software\\Kitware\\CMake\\Config", 0,
                        KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        for (int v = 0; v < 2; ++v) {
          DWORD dwData, dwType, dwSize = 4;
          if (!data[v] &&
              RegQueryValueExW(hKey, values[v], 0, &dwType, (BYTE*)&dwData,
                               &dwSize) == ERROR_SUCCESS &&
              dwType == REG_DWORD && dwSize == 4) {
            data[v] = static_cast<unsigned int>(dwData);
          }
        }
        RegCloseKey(hKey);
      }
    }
    retry.Count = data[0] ? data[0] : 5;
    retry.Delay = data[1] ? data[1] : 500;
  }
  return retry;
}
#endif

bool cmSystemTools::RenameFile(const char* oldname, const char* newname)
{
#ifdef _WIN32
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
  /* Windows MoveFileEx may not replace read-only or in-use files.  If it
     fails then remove the read-only attribute from any existing destination.
     Try multiple times since we may be racing against another process
     creating/opening the destination file just before our MoveFileEx.  */
  WindowsFileRetry retry = cmSystemTools::GetWindowsFileRetry();
  while (
    !MoveFileExW(SystemTools::ConvertToWindowsExtendedPath(oldname).c_str(),
                 SystemTools::ConvertToWindowsExtendedPath(newname).c_str(),
                 MOVEFILE_REPLACE_EXISTING) &&
    --retry.Count) {
    DWORD last_error = GetLastError();
    // Try again only if failure was due to access/sharing permissions.
    if (last_error != ERROR_ACCESS_DENIED &&
        last_error != ERROR_SHARING_VIOLATION) {
      return false;
    }
    DWORD attrs = GetFileAttributesW(
      SystemTools::ConvertToWindowsExtendedPath(newname).c_str());
    if ((attrs != INVALID_FILE_ATTRIBUTES) &&
        (attrs & FILE_ATTRIBUTE_READONLY)) {
      // Remove the read-only attribute from the destination file.
      SetFileAttributesW(
        SystemTools::ConvertToWindowsExtendedPath(newname).c_str(),
        attrs & ~FILE_ATTRIBUTE_READONLY);
    } else {
      // The file may be temporarily in use so wait a bit.
      cmSystemTools::Delay(retry.Delay);
    }
  }
  return retry.Count > 0;
#else
  /* On UNIX we have an OS-provided call to do this atomically.  */
  return rename(oldname, newname) == 0;
#endif
}

bool cmSystemTools::ComputeFileMD5(const std::string& source, char* md5out)
{
#if defined(CMAKE_BUILD_WITH_CMAKE)
  cmCryptoHash md5(cmCryptoHash::AlgoMD5);
  std::string const str = md5.HashFile(source);
  strncpy(md5out, str.c_str(), 32);
  return !str.empty();
#else
  (void)source;
  (void)md5out;
  cmSystemTools::Message("md5sum not supported in bootstrapping mode",
                         "Error");
  return false;
#endif
}

std::string cmSystemTools::ComputeStringMD5(const std::string& input)
{
#if defined(CMAKE_BUILD_WITH_CMAKE)
  cmCryptoHash md5(cmCryptoHash::AlgoMD5);
  return md5.HashString(input);
#else
  (void)input;
  cmSystemTools::Message("md5sum not supported in bootstrapping mode",
                         "Error");
  return "";
#endif
}

std::string cmSystemTools::ComputeCertificateThumbprint(
  const std::string& source)
{
  std::string thumbprint;

#if defined(CMAKE_BUILD_WITH_CMAKE) && defined(_WIN32)
  BYTE* certData = NULL;
  CRYPT_INTEGER_BLOB cryptBlob;
  HCERTSTORE certStore = NULL;
  PCCERT_CONTEXT certContext = NULL;

  HANDLE certFile = CreateFileW(
    cmsys::Encoding::ToWide(source.c_str()).c_str(), GENERIC_READ,
    FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (certFile != INVALID_HANDLE_VALUE && certFile != NULL) {
    DWORD fileSize = GetFileSize(certFile, NULL);
    if (fileSize != INVALID_FILE_SIZE) {
      certData = new BYTE[fileSize];
      if (certData != NULL) {
        DWORD dwRead = 0;
        if (ReadFile(certFile, certData, fileSize, &dwRead, NULL)) {
          cryptBlob.cbData = fileSize;
          cryptBlob.pbData = certData;

          // Verify that this is a valid cert
          if (PFXIsPFXBlob(&cryptBlob)) {
            // Open the certificate as a store
            certStore = PFXImportCertStore(&cryptBlob, NULL, CRYPT_EXPORTABLE);
            if (certStore != NULL) {
              // There should only be 1 cert.
              certContext =
                CertEnumCertificatesInStore(certStore, certContext);
              if (certContext != NULL) {
                // The hash is 20 bytes
                BYTE hashData[20];
                DWORD hashLength = 20;

                // Buffer to print the hash. Each byte takes 2 chars +
                // terminating character
                char hashPrint[41];
                char* pHashPrint = hashPrint;
                // Get the hash property from the certificate
                if (CertGetCertificateContextProperty(
                      certContext, CERT_HASH_PROP_ID, hashData, &hashLength)) {
                  for (DWORD i = 0; i < hashLength; i++) {
                    // Convert each byte to hexadecimal
                    sprintf(pHashPrint, "%02X", hashData[i]);
                    pHashPrint += 2;
                  }
                  *pHashPrint = '\0';
                  thumbprint = hashPrint;
                }
                CertFreeCertificateContext(certContext);
              }
              CertCloseStore(certStore, 0);
            }
          }
        }
        delete[] certData;
      }
    }
    CloseHandle(certFile);
  }
#else
  (void)source;
  cmSystemTools::Message("ComputeCertificateThumbprint is not implemented",
                         "Error");
#endif

  return thumbprint;
}

void cmSystemTools::Glob(const std::string& directory,
                         const std::string& regexp,
                         std::vector<std::string>& files)
{
  cmsys::Directory d;
  cmsys::RegularExpression reg(regexp.c_str());

  if (d.Load(directory)) {
    size_t numf;
    unsigned int i;
    numf = d.GetNumberOfFiles();
    for (i = 0; i < numf; i++) {
      std::string fname = d.GetFile(i);
      if (reg.find(fname)) {
        files.push_back(fname);
      }
    }
  }
}

void cmSystemTools::GlobDirs(const std::string& path,
                             std::vector<std::string>& files)
{
  std::string::size_type pos = path.find("/*");
  if (pos == std::string::npos) {
    files.push_back(path);
    return;
  }
  std::string startPath = path.substr(0, pos);
  std::string finishPath = path.substr(pos + 2);

  cmsys::Directory d;
  if (d.Load(startPath)) {
    for (unsigned int i = 0; i < d.GetNumberOfFiles(); ++i) {
      if ((std::string(d.GetFile(i)) != ".") &&
          (std::string(d.GetFile(i)) != "..")) {
        std::string fname = startPath;
        fname += "/";
        fname += d.GetFile(i);
        if (cmSystemTools::FileIsDirectory(fname)) {
          fname += finishPath;
          cmSystemTools::GlobDirs(fname, files);
        }
      }
    }
  }
}

void cmSystemTools::ExpandList(std::vector<std::string> const& arguments,
                               std::vector<std::string>& newargs)
{
  std::vector<std::string>::const_iterator i;
  for (i = arguments.begin(); i != arguments.end(); ++i) {
    cmSystemTools::ExpandListArgument(*i, newargs);
  }
}

void cmSystemTools::ExpandListArgument(const std::string& arg,
                                       std::vector<std::string>& newargs,
                                       bool emptyArgs)
{
  // If argument is empty, it is an empty list.
  if (!emptyArgs && arg.empty()) {
    return;
  }
  // if there are no ; in the name then just copy the current string
  if (arg.find(';') == std::string::npos) {
    newargs.push_back(arg);
    return;
  }
  std::string newArg;
  const char* last = arg.c_str();
  // Break the string at non-escaped semicolons not nested in [].
  int squareNesting = 0;
  for (const char* c = last; *c; ++c) {
    switch (*c) {
      case '\\': {
        // We only want to allow escaping of semicolons.  Other
        // escapes should not be processed here.
        const char* next = c + 1;
        if (*next == ';') {
          newArg.append(last, c - last);
          // Skip over the escape character
          last = c = next;
        }
      } break;
      case '[': {
        ++squareNesting;
      } break;
      case ']': {
        --squareNesting;
      } break;
      case ';': {
        // Break the string here if we are not nested inside square
        // brackets.
        if (squareNesting == 0) {
          newArg.append(last, c - last);
          // Skip over the semicolon
          last = c + 1;
          if (!newArg.empty() || emptyArgs) {
            // Add the last argument if the string is not empty.
            newargs.push_back(newArg);
            newArg = "";
          }
        }
      } break;
      default: {
        // Just append this character.
      } break;
    }
  }
  newArg.append(last);
  if (!newArg.empty() || emptyArgs) {
    // Add the last argument if the string is not empty.
    newargs.push_back(newArg);
  }
}

bool cmSystemTools::SimpleGlob(const std::string& glob,
                               std::vector<std::string>& files,
                               int type /* = 0 */)
{
  files.clear();
  if (glob[glob.size() - 1] != '*') {
    return false;
  }
  std::string path = cmSystemTools::GetFilenamePath(glob);
  std::string ppath = cmSystemTools::GetFilenameName(glob);
  ppath = ppath.substr(0, ppath.size() - 1);
  if (path.empty()) {
    path = "/";
  }

  bool res = false;
  cmsys::Directory d;
  if (d.Load(path)) {
    for (unsigned int i = 0; i < d.GetNumberOfFiles(); ++i) {
      if ((std::string(d.GetFile(i)) != ".") &&
          (std::string(d.GetFile(i)) != "..")) {
        std::string fname = path;
        if (path[path.size() - 1] != '/') {
          fname += "/";
        }
        fname += d.GetFile(i);
        std::string sfname = d.GetFile(i);
        if (type > 0 && cmSystemTools::FileIsDirectory(fname)) {
          continue;
        }
        if (type < 0 && !cmSystemTools::FileIsDirectory(fname)) {
          continue;
        }
        if (sfname.size() >= ppath.size() &&
            sfname.substr(0, ppath.size()) == ppath) {
          files.push_back(fname);
          res = true;
        }
      }
    }
  }
  return res;
}

cmSystemTools::FileFormat cmSystemTools::GetFileFormat(const char* cext)
{
  if (!cext || *cext == 0) {
    return cmSystemTools::NO_FILE_FORMAT;
  }
  // std::string ext = cmSystemTools::LowerCase(cext);
  std::string ext = cext;
  if (ext == "c" || ext == ".c" || ext == "m" || ext == ".m") {
    return cmSystemTools::C_FILE_FORMAT;
  }
  if (ext == "C" || ext == ".C" || ext == "M" || ext == ".M" || ext == "c++" ||
      ext == ".c++" || ext == "cc" || ext == ".cc" || ext == "cpp" ||
      ext == ".cpp" || ext == "cxx" || ext == ".cxx" || ext == "mm" ||
      ext == ".mm") {
    return cmSystemTools::CXX_FILE_FORMAT;
  }
  if (ext == "f" || ext == ".f" || ext == "F" || ext == ".F" || ext == "f77" ||
      ext == ".f77" || ext == "f90" || ext == ".f90" || ext == "for" ||
      ext == ".for" || ext == "f95" || ext == ".f95") {
    return cmSystemTools::FORTRAN_FILE_FORMAT;
  }
  if (ext == "java" || ext == ".java") {
    return cmSystemTools::JAVA_FILE_FORMAT;
  }
  if (ext == "H" || ext == ".H" || ext == "h" || ext == ".h" || ext == "h++" ||
      ext == ".h++" || ext == "hm" || ext == ".hm" || ext == "hpp" ||
      ext == ".hpp" || ext == "hxx" || ext == ".hxx" || ext == "in" ||
      ext == ".in" || ext == "txx" || ext == ".txx") {
    return cmSystemTools::HEADER_FILE_FORMAT;
  }
  if (ext == "rc" || ext == ".rc") {
    return cmSystemTools::RESOURCE_FILE_FORMAT;
  }
  if (ext == "def" || ext == ".def") {
    return cmSystemTools::DEFINITION_FILE_FORMAT;
  }
  if (ext == "lib" || ext == ".lib" || ext == "a" || ext == ".a") {
    return cmSystemTools::STATIC_LIBRARY_FILE_FORMAT;
  }
  if (ext == "o" || ext == ".o" || ext == "obj" || ext == ".obj") {
    return cmSystemTools::OBJECT_FILE_FORMAT;
  }
#ifdef __APPLE__
  if (ext == "dylib" || ext == ".dylib") {
    return cmSystemTools::SHARED_LIBRARY_FILE_FORMAT;
  }
  if (ext == "so" || ext == ".so" || ext == "bundle" || ext == ".bundle") {
    return cmSystemTools::MODULE_FILE_FORMAT;
  }
#else  // __APPLE__
  if (ext == "so" || ext == ".so" || ext == "sl" || ext == ".sl" ||
      ext == "dll" || ext == ".dll") {
    return cmSystemTools::SHARED_LIBRARY_FILE_FORMAT;
  }
#endif // __APPLE__
  return cmSystemTools::UNKNOWN_FILE_FORMAT;
}

bool cmSystemTools::Split(const char* s, std::vector<std::string>& l)
{
  std::vector<std::string> temp;
  bool res = Superclass::Split(s, temp);
  l.insert(l.end(), temp.begin(), temp.end());
  return res;
}

std::string cmSystemTools::ConvertToOutputPath(const char* path)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  if (s_ForceUnixPaths) {
    return cmSystemTools::ConvertToUnixOutputPath(path);
  }
  return cmSystemTools::ConvertToWindowsOutputPath(path);
#else
  return cmSystemTools::ConvertToUnixOutputPath(path);
#endif
}

void cmSystemTools::ConvertToOutputSlashes(std::string& path)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  if (!s_ForceUnixPaths) {
    // Convert to windows slashes.
    std::string::size_type pos = 0;
    while ((pos = path.find('/', pos)) != std::string::npos) {
      path[pos++] = '\\';
    }
  }
#else
  static_cast<void>(path);
#endif
}

std::string cmSystemTools::ConvertToRunCommandPath(const char* path)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  return cmSystemTools::ConvertToWindowsOutputPath(path);
#else
  return cmSystemTools::ConvertToUnixOutputPath(path);
#endif
}

// compute the relative path from here to there
std::string cmSystemTools::RelativePath(const char* local, const char* remote)
{
  if (!cmSystemTools::FileIsFullPath(local)) {
    cmSystemTools::Error("RelativePath must be passed a full path to local: ",
                         local);
  }
  if (!cmSystemTools::FileIsFullPath(remote)) {
    cmSystemTools::Error("RelativePath must be passed a full path to remote: ",
                         remote);
  }
  return cmsys::SystemTools::RelativePath(local, remote);
}

std::string cmSystemTools::CollapseCombinedPath(std::string const& dir,
                                                std::string const& file)
{
  if (dir.empty() || dir == ".") {
    return file;
  }

  std::vector<std::string> dirComponents;
  std::vector<std::string> fileComponents;
  cmSystemTools::SplitPath(dir, dirComponents);
  cmSystemTools::SplitPath(file, fileComponents);

  if (fileComponents.empty()) {
    return dir;
  }
  if (fileComponents[0] != "") {
    // File is not a relative path.
    return file;
  }

  std::vector<std::string>::iterator i = fileComponents.begin() + 1;
  while (i != fileComponents.end() && *i == ".." && dirComponents.size() > 1) {
    ++i;                      // Remove ".." file component.
    dirComponents.pop_back(); // Remove last dir component.
  }

  dirComponents.insert(dirComponents.end(), i, fileComponents.end());
  return cmSystemTools::JoinPath(dirComponents);
}

#ifdef CMAKE_BUILD_WITH_CMAKE
bool cmSystemTools::UnsetEnv(const char* value)
{
#if !defined(HAVE_UNSETENV)
  std::string var = value;
  var += "=";
  return cmSystemTools::PutEnv(var.c_str());
#else
  unsetenv(value);
  return true;
#endif
}

std::vector<std::string> cmSystemTools::GetEnvironmentVariables()
{
  std::vector<std::string> env;
  int cc;
  for (cc = 0; environ[cc]; ++cc) {
    env.push_back(environ[cc]);
  }
  return env;
}

void cmSystemTools::AppendEnv(std::vector<std::string> const& env)
{
  for (std::vector<std::string>::const_iterator eit = env.begin();
       eit != env.end(); ++eit) {
    cmSystemTools::PutEnv(*eit);
  }
}

cmSystemTools::SaveRestoreEnvironment::SaveRestoreEnvironment()
{
  this->Env = cmSystemTools::GetEnvironmentVariables();
}

cmSystemTools::SaveRestoreEnvironment::~SaveRestoreEnvironment()
{
  // First clear everything in the current environment:
  std::vector<std::string> currentEnv = GetEnvironmentVariables();
  for (std::vector<std::string>::const_iterator eit = currentEnv.begin();
       eit != currentEnv.end(); ++eit) {
    std::string var(*eit);

    std::string::size_type pos = var.find('=');
    if (pos != std::string::npos) {
      var = var.substr(0, pos);
    }

    cmSystemTools::UnsetEnv(var.c_str());
  }

  // Then put back each entry from the original environment:
  cmSystemTools::AppendEnv(this->Env);
}
#endif

void cmSystemTools::EnableVSConsoleOutput()
{
#ifdef _WIN32
  // Visual Studio 8 2005 (devenv.exe or VCExpress.exe) will not
  // display output to the console unless this environment variable is
  // set.  We need it to capture the output of these build tools.
  // Note for future work that one could pass "/out \\.\pipe\NAME" to
  // either of these executables where NAME is created with
  // CreateNamedPipe.  This would bypass the internal buffering of the
  // output and allow it to be captured on the fly.
  cmSystemTools::PutEnv("vsconsoleoutput=1");

#ifdef CMAKE_BUILD_WITH_CMAKE
  // VS sets an environment variable to tell MS tools like "cl" to report
  // output through a backdoor pipe instead of stdout/stderr.  Unset the
  // environment variable to close this backdoor for any path of process
  // invocations that passes through CMake so we can capture the output.
  cmSystemTools::UnsetEnv("VS_UNICODE_OUTPUT");
#endif
#endif
}

bool cmSystemTools::IsPathToFramework(const char* path)
{
  return (cmSystemTools::FileIsFullPath(path) &&
          cmHasLiteralSuffix(path, ".framework"));
}

bool cmSystemTools::CreateTar(const char* outFileName,
                              const std::vector<std::string>& files,
                              cmTarCompression compressType, bool verbose,
                              std::string const& mtime,
                              std::string const& format)
{
#if defined(CMAKE_BUILD_WITH_CMAKE)
  std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();
  cmsys::ofstream fout(outFileName, std::ios::out | std::ios::binary);
  if (!fout) {
    std::string e = "Cannot open output file \"";
    e += outFileName;
    e += "\": ";
    e += cmSystemTools::GetLastSystemError();
    cmSystemTools::Error(e.c_str());
    return false;
  }
  cmArchiveWrite::Compress compress = cmArchiveWrite::CompressNone;
  switch (compressType) {
    case TarCompressGZip:
      compress = cmArchiveWrite::CompressGZip;
      break;
    case TarCompressBZip2:
      compress = cmArchiveWrite::CompressBZip2;
      break;
    case TarCompressXZ:
      compress = cmArchiveWrite::CompressXZ;
      break;
    case TarCompressNone:
      compress = cmArchiveWrite::CompressNone;
      break;
  }

  cmArchiveWrite a(fout, compress, format.empty() ? "paxr" : format);

  a.SetMTime(mtime);
  a.SetVerbose(verbose);
  for (std::vector<std::string>::const_iterator i = files.begin();
       i != files.end(); ++i) {
    std::string path = *i;
    if (cmSystemTools::FileIsFullPath(path.c_str())) {
      // Get the relative path to the file.
      path = cmSystemTools::RelativePath(cwd.c_str(), path.c_str());
    }
    if (!a.Add(path)) {
      break;
    }
  }
  if (!a) {
    cmSystemTools::Error(a.GetError().c_str());
    return false;
  }
  return true;
#else
  (void)outFileName;
  (void)files;
  (void)verbose;
  return false;
#endif
}

#if defined(CMAKE_BUILD_WITH_CMAKE)
namespace {
#define BSDTAR_FILESIZE_PRINTF "%lu"
#define BSDTAR_FILESIZE_TYPE unsigned long
void list_item_verbose(FILE* out, struct archive_entry* entry)
{
  char tmp[100];
  size_t w;
  const char* p;
  const char* fmt;
  time_t tim;
  static time_t now;
  size_t u_width = 6;
  size_t gs_width = 13;

  /*
   * We avoid collecting the entire list in memory at once by
   * listing things as we see them.  However, that also means we can't
   * just pre-compute the field widths.  Instead, we start with guesses
   * and just widen them as necessary.  These numbers are completely
   * arbitrary.
   */
  if (!now) {
    time(&now);
  }
  fprintf(out, "%s %d ", archive_entry_strmode(entry),
          archive_entry_nlink(entry));

  /* Use uname if it's present, else uid. */
  p = archive_entry_uname(entry);
  if ((p == CM_NULLPTR) || (*p == '\0')) {
    sprintf(tmp, "%lu ", (unsigned long)archive_entry_uid(entry));
    p = tmp;
  }
  w = strlen(p);
  if (w > u_width) {
    u_width = w;
  }
  fprintf(out, "%-*s ", (int)u_width, p);
  /* Use gname if it's present, else gid. */
  p = archive_entry_gname(entry);
  if (p != CM_NULLPTR && p[0] != '\0') {
    fprintf(out, "%s", p);
    w = strlen(p);
  } else {
    sprintf(tmp, "%lu", (unsigned long)archive_entry_gid(entry));
    w = strlen(tmp);
    fprintf(out, "%s", tmp);
  }

  /*
   * Print device number or file size, right-aligned so as to make
   * total width of group and devnum/filesize fields be gs_width.
   * If gs_width is too small, grow it.
   */
  if (archive_entry_filetype(entry) == AE_IFCHR ||
      archive_entry_filetype(entry) == AE_IFBLK) {
    sprintf(tmp, "%lu,%lu", (unsigned long)archive_entry_rdevmajor(entry),
            (unsigned long)archive_entry_rdevminor(entry));
  } else {
    /*
     * Note the use of platform-dependent macros to format
     * the filesize here.  We need the format string and the
     * corresponding type for the cast.
     */
    sprintf(tmp, BSDTAR_FILESIZE_PRINTF,
            (BSDTAR_FILESIZE_TYPE)archive_entry_size(entry));
  }
  if (w + strlen(tmp) >= gs_width) {
    gs_width = w + strlen(tmp) + 1;
  }
  fprintf(out, "%*s", (int)(gs_width - w), tmp);

  /* Format the time using 'ls -l' conventions. */
  tim = archive_entry_mtime(entry);
#define HALF_YEAR ((time_t)365 * 86400 / 2)
#if defined(_WIN32) && !defined(__CYGWIN__)
/* Windows' strftime function does not support %e format. */
#define DAY_FMT "%d"
#else
#define DAY_FMT "%e" /* Day number without leading zeros */
#endif
  if (tim < now - HALF_YEAR || tim > now + HALF_YEAR) {
    fmt = DAY_FMT " %b  %Y";
  } else {
    fmt = DAY_FMT " %b %H:%M";
  }
  strftime(tmp, sizeof(tmp), fmt, localtime(&tim));
  fprintf(out, " %s ", tmp);
  fprintf(out, "%s", cm_archive_entry_pathname(entry).c_str());

  /* Extra information for links. */
  if (archive_entry_hardlink(entry)) /* Hard link */
  {
    fprintf(out, " link to %s", archive_entry_hardlink(entry));
  } else if (archive_entry_symlink(entry)) /* Symbolic link */
  {
    fprintf(out, " -> %s", archive_entry_symlink(entry));
  }
  fflush(out);
}

long copy_data(struct archive* ar, struct archive* aw)
{
  long r;
  const void* buff;
  size_t size;
#if defined(ARCHIVE_VERSION_NUMBER) && ARCHIVE_VERSION_NUMBER >= 3000000
  __LA_INT64_T offset;
#else
  off_t offset;
#endif

  for (;;) {
    r = archive_read_data_block(ar, &buff, &size, &offset);
    if (r == ARCHIVE_EOF) {
      return (ARCHIVE_OK);
    }
    if (r != ARCHIVE_OK) {
      return (r);
    }
    r = archive_write_data_block(aw, buff, size, offset);
    if (r != ARCHIVE_OK) {
      cmSystemTools::Message("archive_write_data_block()",
                             archive_error_string(aw));
      return (r);
    }
  }
#if !defined(__clang__) && !defined(__HP_aCC)
  return r; /* this should not happen but it quiets some compilers */
#endif
}

bool extract_tar(const char* outFileName, bool verbose, bool extract)
{
  cmLocaleRAII localeRAII;
  static_cast<void>(localeRAII);
  struct archive* a = archive_read_new();
  struct archive* ext = archive_write_disk_new();
  archive_read_support_filter_all(a);
  archive_read_support_format_all(a);
  struct archive_entry* entry;
  int r = cm_archive_read_open_file(a, outFileName, 10240);
  if (r) {
    cmSystemTools::Error("Problem with archive_read_open_file(): ",
                         archive_error_string(a));
    archive_write_free(ext);
    archive_read_close(a);
    return false;
  }
  for (;;) {
    r = archive_read_next_header(a, &entry);
    if (r == ARCHIVE_EOF) {
      break;
    }
    if (r != ARCHIVE_OK) {
      cmSystemTools::Error("Problem with archive_read_next_header(): ",
                           archive_error_string(a));
      break;
    }
    if (verbose) {
      if (extract) {
        cmSystemTools::Stdout("x ");
        cmSystemTools::Stdout(cm_archive_entry_pathname(entry).c_str());
      } else {
        list_item_verbose(stdout, entry);
      }
      cmSystemTools::Stdout("\n");
    } else if (!extract) {
      cmSystemTools::Stdout(cm_archive_entry_pathname(entry).c_str());
      cmSystemTools::Stdout("\n");
    }
    if (extract) {
      r = archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME);
      if (r != ARCHIVE_OK) {
        cmSystemTools::Error("Problem with archive_write_disk_set_options(): ",
                             archive_error_string(ext));
        break;
      }

      r = archive_write_header(ext, entry);
      if (r == ARCHIVE_OK) {
        copy_data(a, ext);
        r = archive_write_finish_entry(ext);
        if (r != ARCHIVE_OK) {
          cmSystemTools::Error("Problem with archive_write_finish_entry(): ",
                               archive_error_string(ext));
          break;
        }
      }
#ifdef _WIN32
      else if (const char* linktext = archive_entry_symlink(entry)) {
        std::cerr << "cmake -E tar: warning: skipping symbolic link \""
                  << cm_archive_entry_pathname(entry) << "\" -> \"" << linktext
                  << "\"." << std::endl;
      }
#endif
      else {
        cmSystemTools::Error("Problem with archive_write_header(): ",
                             archive_error_string(ext));
        cmSystemTools::Error("Current file: ",
                             cm_archive_entry_pathname(entry).c_str());
        break;
      }
    }
  }
  archive_write_free(ext);
  archive_read_close(a);
  archive_read_free(a);
  return r == ARCHIVE_EOF || r == ARCHIVE_OK;
}
}
#endif

bool cmSystemTools::ExtractTar(const char* outFileName, bool verbose)
{
#if defined(CMAKE_BUILD_WITH_CMAKE)
  return extract_tar(outFileName, verbose, true);
#else
  (void)outFileName;
  (void)verbose;
  return false;
#endif
}

bool cmSystemTools::ListTar(const char* outFileName, bool verbose)
{
#if defined(CMAKE_BUILD_WITH_CMAKE)
  return extract_tar(outFileName, verbose, false);
#else
  (void)outFileName;
  (void)verbose;
  return false;
#endif
}

int cmSystemTools::WaitForLine(cmsysProcess* process, std::string& line,
                               double timeout, std::vector<char>& out,
                               std::vector<char>& err)
{
  line = "";
  std::vector<char>::iterator outiter = out.begin();
  std::vector<char>::iterator erriter = err.begin();
  cmProcessOutput processOutput;
  std::string strdata;
  while (true) {
    // Check for a newline in stdout.
    for (; outiter != out.end(); ++outiter) {
      if ((*outiter == '\r') && ((outiter + 1) == out.end())) {
        break;
      }
      if (*outiter == '\n' || *outiter == '\0') {
        std::vector<char>::size_type length = outiter - out.begin();
        if (length > 1 && *(outiter - 1) == '\r') {
          --length;
        }
        if (length > 0) {
          line.append(&out[0], length);
        }
        out.erase(out.begin(), outiter + 1);
        return cmsysProcess_Pipe_STDOUT;
      }
    }

    // Check for a newline in stderr.
    for (; erriter != err.end(); ++erriter) {
      if ((*erriter == '\r') && ((erriter + 1) == err.end())) {
        break;
      }
      if (*erriter == '\n' || *erriter == '\0') {
        std::vector<char>::size_type length = erriter - err.begin();
        if (length > 1 && *(erriter - 1) == '\r') {
          --length;
        }
        if (length > 0) {
          line.append(&err[0], length);
        }
        err.erase(err.begin(), erriter + 1);
        return cmsysProcess_Pipe_STDERR;
      }
    }

    // No newlines found.  Wait for more data from the process.
    int length;
    char* data;
    int pipe = cmsysProcess_WaitForData(process, &data, &length, &timeout);
    if (pipe == cmsysProcess_Pipe_Timeout) {
      // Timeout has been exceeded.
      return pipe;
    }
    if (pipe == cmsysProcess_Pipe_STDOUT) {
      processOutput.DecodeText(data, length, strdata, 1);
      // Append to the stdout buffer.
      std::vector<char>::size_type size = out.size();
      out.insert(out.end(), strdata.begin(), strdata.end());
      outiter = out.begin() + size;
    } else if (pipe == cmsysProcess_Pipe_STDERR) {
      processOutput.DecodeText(data, length, strdata, 2);
      // Append to the stderr buffer.
      std::vector<char>::size_type size = err.size();
      err.insert(err.end(), strdata.begin(), strdata.end());
      erriter = err.begin() + size;
    } else if (pipe == cmsysProcess_Pipe_None) {
      // Both stdout and stderr pipes have broken.  Return leftover data.
      processOutput.DecodeText(std::string(), strdata, 1);
      if (!strdata.empty()) {
        std::vector<char>::size_type size = out.size();
        out.insert(out.end(), strdata.begin(), strdata.end());
        outiter = out.begin() + size;
      }
      processOutput.DecodeText(std::string(), strdata, 2);
      if (!strdata.empty()) {
        std::vector<char>::size_type size = err.size();
        err.insert(err.end(), strdata.begin(), strdata.end());
        erriter = err.begin() + size;
      }
      if (!out.empty()) {
        line.append(&out[0], outiter - out.begin());
        out.erase(out.begin(), out.end());
        return cmsysProcess_Pipe_STDOUT;
      }
      if (!err.empty()) {
        line.append(&err[0], erriter - err.begin());
        err.erase(err.begin(), err.end());
        return cmsysProcess_Pipe_STDERR;
      }
      return cmsysProcess_Pipe_None;
    }
  }
}

void cmSystemTools::DoNotInheritStdPipes()
{
#ifdef _WIN32
  // Check to see if we are attached to a console
  // if so, then do not stop the inherited pipes
  // or stdout and stderr will not show up in dos
  // shell windows
  CONSOLE_SCREEN_BUFFER_INFO hOutInfo;
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (GetConsoleScreenBufferInfo(hOut, &hOutInfo)) {
    return;
  }
  {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DuplicateHandle(GetCurrentProcess(), out, GetCurrentProcess(), &out, 0,
                    FALSE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    SetStdHandle(STD_OUTPUT_HANDLE, out);
  }
  {
    HANDLE out = GetStdHandle(STD_ERROR_HANDLE);
    DuplicateHandle(GetCurrentProcess(), out, GetCurrentProcess(), &out, 0,
                    FALSE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    SetStdHandle(STD_ERROR_HANDLE, out);
  }
#endif
}

bool cmSystemTools::CopyFileTime(const char* fromFile, const char* toFile)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  cmSystemToolsWindowsHandle hFrom = CreateFileW(
    SystemTools::ConvertToWindowsExtendedPath(fromFile).c_str(), GENERIC_READ,
    FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
  cmSystemToolsWindowsHandle hTo = CreateFileW(
    SystemTools::ConvertToWindowsExtendedPath(toFile).c_str(),
    FILE_WRITE_ATTRIBUTES, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
  if (!hFrom || !hTo) {
    return false;
  }
  FILETIME timeCreation;
  FILETIME timeLastAccess;
  FILETIME timeLastWrite;
  if (!GetFileTime(hFrom, &timeCreation, &timeLastAccess, &timeLastWrite)) {
    return false;
  }
  return SetFileTime(hTo, &timeCreation, &timeLastAccess, &timeLastWrite) != 0;
#else
  struct stat fromStat;
  if (stat(fromFile, &fromStat) < 0) {
    return false;
  }

  struct utimbuf buf;
  buf.actime = fromStat.st_atime;
  buf.modtime = fromStat.st_mtime;
  return utime(toFile, &buf) >= 0;
#endif
}

cmSystemToolsFileTime* cmSystemTools::FileTimeNew()
{
  return new cmSystemToolsFileTime;
}

void cmSystemTools::FileTimeDelete(cmSystemToolsFileTime* t)
{
  delete t;
}

bool cmSystemTools::FileTimeGet(const char* fname, cmSystemToolsFileTime* t)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  cmSystemToolsWindowsHandle h = CreateFileW(
    SystemTools::ConvertToWindowsExtendedPath(fname).c_str(), GENERIC_READ,
    FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
  if (!h) {
    return false;
  }
  if (!GetFileTime(h, &t->timeCreation, &t->timeLastAccess,
                   &t->timeLastWrite)) {
    return false;
  }
#else
  struct stat st;
  if (stat(fname, &st) < 0) {
    return false;
  }
  t->timeBuf.actime = st.st_atime;
  t->timeBuf.modtime = st.st_mtime;
#endif
  return true;
}

bool cmSystemTools::FileTimeSet(const char* fname, cmSystemToolsFileTime* t)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  cmSystemToolsWindowsHandle h = CreateFileW(
    SystemTools::ConvertToWindowsExtendedPath(fname).c_str(),
    FILE_WRITE_ATTRIBUTES, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
  if (!h) {
    return false;
  }
  return SetFileTime(h, &t->timeCreation, &t->timeLastAccess,
                     &t->timeLastWrite) != 0;
#else
  return utime(fname, &t->timeBuf) >= 0;
#endif
}

#ifdef _WIN32
#ifndef CRYPT_SILENT
#define CRYPT_SILENT 0x40 /* Not defined by VS 6 version of header.  */
#endif
static int WinCryptRandom(void* data, size_t size)
{
  int result = 0;
  HCRYPTPROV hProvider = 0;
  if (CryptAcquireContextW(&hProvider, 0, 0, PROV_RSA_FULL,
                           CRYPT_VERIFYCONTEXT | CRYPT_SILENT)) {
    result = CryptGenRandom(hProvider, (DWORD)size, (BYTE*)data) ? 1 : 0;
    CryptReleaseContext(hProvider, 0);
  }
  return result;
}
#endif

unsigned int cmSystemTools::RandomSeed()
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  unsigned int seed = 0;

  // Try using a real random source.
  if (WinCryptRandom(&seed, sizeof(seed))) {
    return seed;
  }

  // Fall back to the time and pid.
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  unsigned int t1 = static_cast<unsigned int>(ft.dwHighDateTime);
  unsigned int t2 = static_cast<unsigned int>(ft.dwLowDateTime);
  unsigned int pid = static_cast<unsigned int>(GetCurrentProcessId());
  return t1 ^ t2 ^ pid;
#else
  union
  {
    unsigned int integer;
    char bytes[sizeof(unsigned int)];
  } seed;

  // Try using a real random source.
  cmsys::ifstream fin;
  fin.rdbuf()->pubsetbuf(CM_NULLPTR, 0); // Unbuffered read.
  fin.open("/dev/urandom");
  if (fin.good() && fin.read(seed.bytes, sizeof(seed)) &&
      fin.gcount() == sizeof(seed)) {
    return seed.integer;
  }

  // Fall back to the time and pid.
  struct timeval t;
  gettimeofday(&t, CM_NULLPTR);
  unsigned int pid = static_cast<unsigned int>(getpid());
  unsigned int tv_sec = static_cast<unsigned int>(t.tv_sec);
  unsigned int tv_usec = static_cast<unsigned int>(t.tv_usec);
  // Since tv_usec never fills more than 11 bits we shift it to fill
  // in the slow-changing high-order bits of tv_sec.
  return tv_sec ^ (tv_usec << 21) ^ pid;
#endif
}

static std::string cmSystemToolsCMakeCommand;
static std::string cmSystemToolsCTestCommand;
static std::string cmSystemToolsCPackCommand;
static std::string cmSystemToolsCMakeCursesCommand;
static std::string cmSystemToolsCMakeGUICommand;
static std::string cmSystemToolsCMClDepsCommand;
static std::string cmSystemToolsCMakeRoot;
void cmSystemTools::FindCMakeResources(const char* argv0)
{
  std::string exe_dir;
#if defined(_WIN32) && !defined(__CYGWIN__)
  (void)argv0; // ignore this on windows
  wchar_t modulepath[_MAX_PATH];
  ::GetModuleFileNameW(NULL, modulepath, sizeof(modulepath));
  exe_dir =
    cmSystemTools::GetFilenamePath(cmsys::Encoding::ToNarrow(modulepath));
#elif defined(__APPLE__)
  (void)argv0; // ignore this on OS X
#define CM_EXE_PATH_LOCAL_SIZE 16384
  char exe_path_local[CM_EXE_PATH_LOCAL_SIZE];
#if defined(MAC_OS_X_VERSION_10_3) && !defined(MAC_OS_X_VERSION_10_4)
  unsigned long exe_path_size = CM_EXE_PATH_LOCAL_SIZE;
#else
  uint32_t exe_path_size = CM_EXE_PATH_LOCAL_SIZE;
#endif
#undef CM_EXE_PATH_LOCAL_SIZE
  char* exe_path = exe_path_local;
  if (_NSGetExecutablePath(exe_path, &exe_path_size) < 0) {
    exe_path = (char*)malloc(exe_path_size);
    _NSGetExecutablePath(exe_path, &exe_path_size);
  }
  exe_dir =
    cmSystemTools::GetFilenamePath(cmSystemTools::GetRealPath(exe_path));
  if (exe_path != exe_path_local) {
    free(exe_path);
  }
  if (cmSystemTools::GetFilenameName(exe_dir) == "MacOS") {
    // The executable is inside an application bundle.
    // Look for ..<CMAKE_BIN_DIR> (install tree) and then fall back to
    // ../../../bin (build tree).
    exe_dir = cmSystemTools::GetFilenamePath(exe_dir);
    if (cmSystemTools::FileExists(exe_dir + CMAKE_BIN_DIR "/cmake")) {
      exe_dir += CMAKE_BIN_DIR;
    } else {
      exe_dir = cmSystemTools::GetFilenamePath(exe_dir);
      exe_dir = cmSystemTools::GetFilenamePath(exe_dir);
    }
  }
#else
  std::string errorMsg;
  std::string exe;
  if (cmSystemTools::FindProgramPath(argv0, exe, errorMsg)) {
    // remove symlinks
    exe = cmSystemTools::GetRealPath(exe);
    exe_dir = cmSystemTools::GetFilenamePath(exe);
  } else {
    // ???
  }
#endif
  exe_dir = cmSystemTools::GetActualCaseForPath(exe_dir);
  cmSystemToolsCMakeCommand = exe_dir;
  cmSystemToolsCMakeCommand += "/cmake";
  cmSystemToolsCMakeCommand += cmSystemTools::GetExecutableExtension();
#ifndef CMAKE_BUILD_WITH_CMAKE
  // The bootstrap cmake does not provide the other tools,
  // so use the directory where they are about to be built.
  exe_dir = CMAKE_BOOTSTRAP_BINARY_DIR "/bin";
#endif
  cmSystemToolsCTestCommand = exe_dir;
  cmSystemToolsCTestCommand += "/ctest";
  cmSystemToolsCTestCommand += cmSystemTools::GetExecutableExtension();
  cmSystemToolsCPackCommand = exe_dir;
  cmSystemToolsCPackCommand += "/cpack";
  cmSystemToolsCPackCommand += cmSystemTools::GetExecutableExtension();
  cmSystemToolsCMakeGUICommand = exe_dir;
  cmSystemToolsCMakeGUICommand += "/cmake-gui";
  cmSystemToolsCMakeGUICommand += cmSystemTools::GetExecutableExtension();
  if (!cmSystemTools::FileExists(cmSystemToolsCMakeGUICommand.c_str())) {
    cmSystemToolsCMakeGUICommand = "";
  }
  cmSystemToolsCMakeCursesCommand = exe_dir;
  cmSystemToolsCMakeCursesCommand += "/ccmake";
  cmSystemToolsCMakeCursesCommand += cmSystemTools::GetExecutableExtension();
  if (!cmSystemTools::FileExists(cmSystemToolsCMakeCursesCommand.c_str())) {
    cmSystemToolsCMakeCursesCommand = "";
  }
  cmSystemToolsCMClDepsCommand = exe_dir;
  cmSystemToolsCMClDepsCommand += "/cmcldeps";
  cmSystemToolsCMClDepsCommand += cmSystemTools::GetExecutableExtension();
  if (!cmSystemTools::FileExists(cmSystemToolsCMClDepsCommand.c_str())) {
    cmSystemToolsCMClDepsCommand = "";
  }

#ifdef CMAKE_BUILD_WITH_CMAKE
  // Install tree has
  // - "<prefix><CMAKE_BIN_DIR>/cmake"
  // - "<prefix><CMAKE_DATA_DIR>"
  if (cmHasSuffix(exe_dir, CMAKE_BIN_DIR)) {
    std::string const prefix =
      exe_dir.substr(0, exe_dir.size() - strlen(CMAKE_BIN_DIR));
    cmSystemToolsCMakeRoot = prefix + CMAKE_DATA_DIR;
  }
  if (cmSystemToolsCMakeRoot.empty() ||
      !cmSystemTools::FileExists(
        (cmSystemToolsCMakeRoot + "/Modules/CMake.cmake").c_str())) {
    // Build tree has "<build>/bin[/<config>]/cmake" and
    // "<build>/CMakeFiles/CMakeSourceDir.txt".
    std::string dir = cmSystemTools::GetFilenamePath(exe_dir);
    std::string src_dir_txt = dir + "/CMakeFiles/CMakeSourceDir.txt";
    cmsys::ifstream fin(src_dir_txt.c_str());
    std::string src_dir;
    if (fin && cmSystemTools::GetLineFromStream(fin, src_dir) &&
        cmSystemTools::FileIsDirectory(src_dir)) {
      cmSystemToolsCMakeRoot = src_dir;
    } else {
      dir = cmSystemTools::GetFilenamePath(dir);
      src_dir_txt = dir + "/CMakeFiles/CMakeSourceDir.txt";
      cmsys::ifstream fin2(src_dir_txt.c_str());
      if (fin2 && cmSystemTools::GetLineFromStream(fin2, src_dir) &&
          cmSystemTools::FileIsDirectory(src_dir)) {
        cmSystemToolsCMakeRoot = src_dir;
      }
    }
  }
#else
  // Bootstrap build knows its source.
  cmSystemToolsCMakeRoot = CMAKE_BOOTSTRAP_SOURCE_DIR;
#endif
}

std::string const& cmSystemTools::GetCMakeCommand()
{
  return cmSystemToolsCMakeCommand;
}

std::string const& cmSystemTools::GetCTestCommand()
{
  return cmSystemToolsCTestCommand;
}

std::string const& cmSystemTools::GetCPackCommand()
{
  return cmSystemToolsCPackCommand;
}

std::string const& cmSystemTools::GetCMakeCursesCommand()
{
  return cmSystemToolsCMakeCursesCommand;
}

std::string const& cmSystemTools::GetCMakeGUICommand()
{
  return cmSystemToolsCMakeGUICommand;
}

std::string const& cmSystemTools::GetCMClDepsCommand()
{
  return cmSystemToolsCMClDepsCommand;
}

std::string const& cmSystemTools::GetCMakeRoot()
{
  return cmSystemToolsCMakeRoot;
}

void cmSystemTools::MakefileColorEcho(int color, const char* message,
                                      bool newline, bool enabled)
{
  // On some platforms (an MSYS prompt) cmsysTerminal may not be able
  // to determine whether the stream is displayed on a tty.  In this
  // case it assumes no unless we tell it otherwise.  Since we want
  // color messages to be displayed for users we will assume yes.
  // However, we can test for some situations when the answer is most
  // likely no.
  int assumeTTY = cmsysTerminal_Color_AssumeTTY;
  if (cmSystemTools::HasEnv("DART_TEST_FROM_DART") ||
      cmSystemTools::HasEnv("DASHBOARD_TEST_FROM_CTEST") ||
      cmSystemTools::HasEnv("CTEST_INTERACTIVE_DEBUG_MODE")) {
    // Avoid printing color escapes during dashboard builds.
    assumeTTY = 0;
  }

  if (enabled && color != cmsysTerminal_Color_Normal) {
    // Print with color.  Delay the newline until later so that
    // all color restore sequences appear before it.
    cmsysTerminal_cfprintf(color | assumeTTY, stdout, "%s", message);
  } else {
    // Color is disabled.  Print without color.
    fprintf(stdout, "%s", message);
  }

  if (newline) {
    fprintf(stdout, "\n");
  }
}

bool cmSystemTools::GuessLibrarySOName(std::string const& fullPath,
                                       std::string& soname)
{
// For ELF shared libraries use a real parser to get the correct
// soname.
#if defined(CMAKE_USE_ELF_PARSER)
  cmELF elf(fullPath.c_str());
  if (elf) {
    return elf.GetSOName(soname);
  }
#endif

  // If the file is not a symlink we have no guess for its soname.
  if (!cmSystemTools::FileIsSymlink(fullPath)) {
    return false;
  }
  if (!cmSystemTools::ReadSymlink(fullPath, soname)) {
    return false;
  }

  // If the symlink has a path component we have no guess for the soname.
  if (!cmSystemTools::GetFilenamePath(soname).empty()) {
    return false;
  }

  // If the symlink points at an extended version of the same name
  // assume it is the soname.
  std::string name = cmSystemTools::GetFilenameName(fullPath);
  return soname.length() > name.length() &&
    soname.compare(0, name.length(), name) == 0;
}

bool cmSystemTools::GuessLibraryInstallName(std::string const& fullPath,
                                            std::string& soname)
{
#if defined(CMAKE_USE_MACH_PARSER)
  cmMachO macho(fullPath.c_str());
  if (macho) {
    return macho.GetInstallName(soname);
  }
#else
  (void)fullPath;
  (void)soname;
#endif

  return false;
}

#if defined(CMAKE_USE_ELF_PARSER)
std::string::size_type cmSystemToolsFindRPath(std::string const& have,
                                              std::string const& want)
{
  std::string::size_type pos = 0;
  while (pos < have.size()) {
    // Look for an occurrence of the string.
    std::string::size_type const beg = have.find(want, pos);
    if (beg == std::string::npos) {
      return std::string::npos;
    }

    // Make sure it is separated from preceding entries.
    if (beg > 0 && have[beg - 1] != ':') {
      pos = beg + 1;
      continue;
    }

    // Make sure it is separated from following entries.
    std::string::size_type const end = beg + want.size();
    if (end < have.size() && have[end] != ':') {
      pos = beg + 1;
      continue;
    }

    // Return the position of the path portion.
    return beg;
  }

  // The desired rpath was not found.
  return std::string::npos;
}
#endif

#if defined(CMAKE_USE_ELF_PARSER)
struct cmSystemToolsRPathInfo
{
  unsigned long Position;
  unsigned long Size;
  std::string Name;
  std::string Value;
};
#endif

bool cmSystemTools::ChangeRPath(std::string const& file,
                                std::string const& oldRPath,
                                std::string const& newRPath, std::string* emsg,
                                bool* changed)
{
#if defined(CMAKE_USE_ELF_PARSER)
  if (changed) {
    *changed = false;
  }
  int rp_count = 0;
  bool remove_rpath = true;
  cmSystemToolsRPathInfo rp[2];
  {
    // Parse the ELF binary.
    cmELF elf(file.c_str());

    // Get the RPATH and RUNPATH entries from it.
    int se_count = 0;
    cmELF::StringEntry const* se[2] = { CM_NULLPTR, CM_NULLPTR };
    const char* se_name[2] = { CM_NULLPTR, CM_NULLPTR };
    if (cmELF::StringEntry const* se_rpath = elf.GetRPath()) {
      se[se_count] = se_rpath;
      se_name[se_count] = "RPATH";
      ++se_count;
    }
    if (cmELF::StringEntry const* se_runpath = elf.GetRunPath()) {
      se[se_count] = se_runpath;
      se_name[se_count] = "RUNPATH";
      ++se_count;
    }
    if (se_count == 0) {
      if (newRPath.empty()) {
        // The new rpath is empty and there is no rpath anyway so it is
        // okay.
        return true;
      }
      if (emsg) {
        *emsg = "No valid ELF RPATH or RUNPATH entry exists in the file; ";
        *emsg += elf.GetErrorMessage();
      }
      return false;
    }

    for (int i = 0; i < se_count; ++i) {
      // If both RPATH and RUNPATH refer to the same string literal it
      // needs to be changed only once.
      if (rp_count && rp[0].Position == se[i]->Position) {
        continue;
      }

      // Make sure the current rpath contains the old rpath.
      std::string::size_type pos =
        cmSystemToolsFindRPath(se[i]->Value, oldRPath);
      if (pos == std::string::npos) {
        // If it contains the new rpath instead then it is okay.
        if (cmSystemToolsFindRPath(se[i]->Value, newRPath) !=
            std::string::npos) {
          remove_rpath = false;
          continue;
        }
        if (emsg) {
          std::ostringstream e;
          /* clang-format off */
        e << "The current " << se_name[i] << " is:\n"
          << "  " << se[i]->Value << "\n"
          << "which does not contain:\n"
          << "  " << oldRPath << "\n"
          << "as was expected.";
          /* clang-format on */
          *emsg = e.str();
        }
        return false;
      }

      // Store information about the entry in the file.
      rp[rp_count].Position = se[i]->Position;
      rp[rp_count].Size = se[i]->Size;
      rp[rp_count].Name = se_name[i];

      std::string::size_type prefix_len = pos;

      // If oldRPath was at the end of the file's RPath, and newRPath is empty,
      // we should remove the unnecessary ':' at the end.
      if (newRPath.empty() && pos > 0 && se[i]->Value[pos - 1] == ':' &&
          pos + oldRPath.length() == se[i]->Value.length()) {
        prefix_len--;
      }

      // Construct the new value which preserves the part of the path
      // not being changed.
      rp[rp_count].Value = se[i]->Value.substr(0, prefix_len);
      rp[rp_count].Value += newRPath;
      rp[rp_count].Value += se[i]->Value.substr(pos + oldRPath.length());

      if (!rp[rp_count].Value.empty()) {
        remove_rpath = false;
      }

      // Make sure there is enough room to store the new rpath and at
      // least one null terminator.
      if (rp[rp_count].Size < rp[rp_count].Value.length() + 1) {
        if (emsg) {
          *emsg = "The replacement path is too long for the ";
          *emsg += se_name[i];
          *emsg += " entry.";
        }
        return false;
      }

      // This entry is ready for update.
      ++rp_count;
    }
  }

  // If no runtime path needs to be changed, we are done.
  if (rp_count == 0) {
    return true;
  }

  // If the resulting rpath is empty, just remove the entire entry instead.
  if (remove_rpath) {
    return cmSystemTools::RemoveRPath(file, emsg, changed);
  }

  {
    // Open the file for update.
    cmsys::ofstream f(file.c_str(),
                      std::ios::in | std::ios::out | std::ios::binary);
    if (!f) {
      if (emsg) {
        *emsg = "Error opening file for update.";
      }
      return false;
    }

    // Store the new RPATH and RUNPATH strings.
    for (int i = 0; i < rp_count; ++i) {
      // Seek to the RPATH position.
      if (!f.seekp(rp[i].Position)) {
        if (emsg) {
          *emsg = "Error seeking to ";
          *emsg += rp[i].Name;
          *emsg += " position.";
        }
        return false;
      }

      // Write the new rpath.  Follow it with enough null terminators to
      // fill the string table entry.
      f << rp[i].Value;
      for (unsigned long j = rp[i].Value.length(); j < rp[i].Size; ++j) {
        f << '\0';
      }

      // Make sure it wrote correctly.
      if (!f) {
        if (emsg) {
          *emsg = "Error writing the new ";
          *emsg += rp[i].Name;
          *emsg += " string to the file.";
        }
        return false;
      }
    }
  }

  // Everything was updated successfully.
  if (changed) {
    *changed = true;
  }
  return true;
#else
  (void)file;
  (void)oldRPath;
  (void)newRPath;
  (void)emsg;
  (void)changed;
  return false;
#endif
}

bool cmSystemTools::VersionCompare(cmSystemTools::CompareOp op,
                                   const char* lhss, const char* rhss)
{
  const char* endl = lhss;
  const char* endr = rhss;
  unsigned long lhs, rhs;

  while (((*endl >= '0') && (*endl <= '9')) ||
         ((*endr >= '0') && (*endr <= '9'))) {
    // Do component-wise comparison.
    lhs = strtoul(endl, const_cast<char**>(&endl), 10);
    rhs = strtoul(endr, const_cast<char**>(&endr), 10);

    if (lhs < rhs) {
      // lhs < rhs, so true if operation is LESS
      return (op & cmSystemTools::OP_LESS) != 0;
    }
    if (lhs > rhs) {
      // lhs > rhs, so true if operation is GREATER
      return (op & cmSystemTools::OP_GREATER) != 0;
    }

    if (*endr == '.') {
      endr++;
    }

    if (*endl == '.') {
      endl++;
    }
  }
  // lhs == rhs, so true if operation is EQUAL
  return (op & cmSystemTools::OP_EQUAL) != 0;
}

bool cmSystemTools::VersionCompareEqual(std::string const& lhs,
                                        std::string const& rhs)
{
  return cmSystemTools::VersionCompare(cmSystemTools::OP_EQUAL, lhs.c_str(),
                                       rhs.c_str());
}

bool cmSystemTools::VersionCompareGreater(std::string const& lhs,
                                          std::string const& rhs)
{
  return cmSystemTools::VersionCompare(cmSystemTools::OP_GREATER, lhs.c_str(),
                                       rhs.c_str());
}

bool cmSystemTools::VersionCompareGreaterEq(std::string const& lhs,
                                            std::string const& rhs)
{
  return cmSystemTools::VersionCompare(cmSystemTools::OP_GREATER_EQUAL,
                                       lhs.c_str(), rhs.c_str());
}

static size_t cm_strverscmp_find_first_difference_or_end(const char* lhs,
                                                         const char* rhs)
{
  size_t i = 0;
  /* Step forward until we find a difference or both strings end together.
     The difference may lie on the null-terminator of one string.  */
  while (lhs[i] == rhs[i] && lhs[i] != 0) {
    ++i;
  }
  return i;
}

static size_t cm_strverscmp_find_digits_begin(const char* s, size_t i)
{
  /* Step back until we are not preceded by a digit.  */
  while (i > 0 && isdigit(s[i - 1])) {
    --i;
  }
  return i;
}

static size_t cm_strverscmp_find_digits_end(const char* s, size_t i)
{
  /* Step forward over digits.  */
  while (isdigit(s[i])) {
    ++i;
  }
  return i;
}

static size_t cm_strverscmp_count_leading_zeros(const char* s, size_t b)
{
  size_t i = b;
  /* Step forward over zeros that are followed by another digit.  */
  while (s[i] == '0' && isdigit(s[i + 1])) {
    ++i;
  }
  return i - b;
}

static int cm_strverscmp(const char* lhs, const char* rhs)
{
  size_t const i = cm_strverscmp_find_first_difference_or_end(lhs, rhs);
  if (lhs[i] != rhs[i]) {
    /* The strings differ starting at 'i'.  Check for a digit sequence.  */
    size_t const b = cm_strverscmp_find_digits_begin(lhs, i);
    if (b != i || (isdigit(lhs[i]) && isdigit(rhs[i]))) {
      /* A digit sequence starts at 'b', preceding or at 'i'.  */

      /* Look for leading zeros, implying a leading decimal point.  */
      size_t const lhs_zeros = cm_strverscmp_count_leading_zeros(lhs, b);
      size_t const rhs_zeros = cm_strverscmp_count_leading_zeros(rhs, b);
      if (lhs_zeros != rhs_zeros) {
        /* The side with more leading zeros orders first.  */
        return rhs_zeros > lhs_zeros ? 1 : -1;
      }
      if (lhs_zeros == 0) {
        /* No leading zeros; compare digit sequence lengths.  */
        size_t const lhs_end = cm_strverscmp_find_digits_end(lhs, i);
        size_t const rhs_end = cm_strverscmp_find_digits_end(rhs, i);
        if (lhs_end != rhs_end) {
          /* The side with fewer digits orders first.  */
          return lhs_end > rhs_end ? 1 : -1;
        }
      }
    }
  }

  /* Ordering was not decided by digit sequence lengths; compare bytes.  */
  return lhs[i] - rhs[i];
}

int cmSystemTools::strverscmp(std::string const& lhs, std::string const& rhs)
{
  return cm_strverscmp(lhs.c_str(), rhs.c_str());
}

bool cmSystemTools::RemoveRPath(std::string const& file, std::string* emsg,
                                bool* removed)
{
#if defined(CMAKE_USE_ELF_PARSER)
  if (removed) {
    *removed = false;
  }
  int zeroCount = 0;
  unsigned long zeroPosition[2] = { 0, 0 };
  unsigned long zeroSize[2] = { 0, 0 };
  unsigned long bytesBegin = 0;
  std::vector<char> bytes;
  {
    // Parse the ELF binary.
    cmELF elf(file.c_str());

    // Get the RPATH and RUNPATH entries from it and sort them by index
    // in the dynamic section header.
    int se_count = 0;
    cmELF::StringEntry const* se[2] = { CM_NULLPTR, CM_NULLPTR };
    if (cmELF::StringEntry const* se_rpath = elf.GetRPath()) {
      se[se_count++] = se_rpath;
    }
    if (cmELF::StringEntry const* se_runpath = elf.GetRunPath()) {
      se[se_count++] = se_runpath;
    }
    if (se_count == 0) {
      // There is no RPATH or RUNPATH anyway.
      return true;
    }
    if (se_count == 2 && se[1]->IndexInSection < se[0]->IndexInSection) {
      std::swap(se[0], se[1]);
    }

    // Obtain a copy of the dynamic entries
    cmELF::DynamicEntryList dentries = elf.GetDynamicEntries();
    if (dentries.empty()) {
      // This should happen only for invalid ELF files where a DT_NULL
      // appears before the end of the table.
      if (emsg) {
        *emsg = "DYNAMIC section contains a DT_NULL before the end.";
      }
      return false;
    }

    // Save information about the string entries to be zeroed.
    zeroCount = se_count;
    for (int i = 0; i < se_count; ++i) {
      zeroPosition[i] = se[i]->Position;
      zeroSize[i] = se[i]->Size;
    }

    // Get size of one DYNAMIC entry
    unsigned long const sizeof_dentry =
      elf.GetDynamicEntryPosition(1) - elf.GetDynamicEntryPosition(0);

    // Adjust the entry list as necessary to remove the run path
    unsigned long entriesErased = 0;
    for (cmELF::DynamicEntryList::iterator it = dentries.begin();
         it != dentries.end();) {
      if (it->first == cmELF::TagRPath || it->first == cmELF::TagRunPath) {
        it = dentries.erase(it);
        entriesErased++;
        continue;
      }
      if (cmELF::TagMipsRldMapRel != 0 &&
          it->first == cmELF::TagMipsRldMapRel) {
        // Background: debuggers need to know the "linker map" which contains
        // the addresses each dynamic object is loaded at. Most arches use
        // the DT_DEBUG tag which the dynamic linker writes to (directly) and
        // contain the location of the linker map, however on MIPS the
        // .dynamic section is always read-only so this is not possible. MIPS
        // objects instead contain a DT_MIPS_RLD_MAP tag which contains the
        // address where the dyanmic linker will write to (an indirect
        // version of DT_DEBUG). Since this doesn't work when using PIE, a
        // relative equivalent was created - DT_MIPS_RLD_MAP_REL. Since this
        // version contains a relative offset, moving it changes the
        // calculated address. This may cause the dyanmic linker to write
        // into memory it should not be changing.
        //
        // To fix this, we adjust the value of DT_MIPS_RLD_MAP_REL here. If
        // we move it up by n bytes, we add n bytes to the value of this tag.
        it->second += entriesErased * sizeof_dentry;
      }

      it++;
    }

    // Encode new entries list
    bytes = elf.EncodeDynamicEntries(dentries);
    bytesBegin = elf.GetDynamicEntryPosition(0);
  }

  // Open the file for update.
  cmsys::ofstream f(file.c_str(),
                    std::ios::in | std::ios::out | std::ios::binary);
  if (!f) {
    if (emsg) {
      *emsg = "Error opening file for update.";
    }
    return false;
  }

  // Write the new DYNAMIC table header.
  if (!f.seekp(bytesBegin)) {
    if (emsg) {
      *emsg = "Error seeking to DYNAMIC table header for RPATH.";
    }
    return false;
  }
  if (!f.write(&bytes[0], bytes.size())) {
    if (emsg) {
      *emsg = "Error replacing DYNAMIC table header.";
    }
    return false;
  }

  // Fill the RPATH and RUNPATH strings with zero bytes.
  for (int i = 0; i < zeroCount; ++i) {
    if (!f.seekp(zeroPosition[i])) {
      if (emsg) {
        *emsg = "Error seeking to RPATH position.";
      }
      return false;
    }
    for (unsigned long j = 0; j < zeroSize[i]; ++j) {
      f << '\0';
    }
    if (!f) {
      if (emsg) {
        *emsg = "Error writing the empty rpath string to the file.";
      }
      return false;
    }
  }

  // Everything was updated successfully.
  if (removed) {
    *removed = true;
  }
  return true;
#else
  (void)file;
  (void)emsg;
  (void)removed;
  return false;
#endif
}

bool cmSystemTools::CheckRPath(std::string const& file,
                               std::string const& newRPath)
{
#if defined(CMAKE_USE_ELF_PARSER)
  // Parse the ELF binary.
  cmELF elf(file.c_str());

  // Get the RPATH or RUNPATH entry from it.
  cmELF::StringEntry const* se = elf.GetRPath();
  if (!se) {
    se = elf.GetRunPath();
  }

  // Make sure the current rpath contains the new rpath.
  if (newRPath.empty()) {
    if (!se) {
      return true;
    }
  } else {
    if (se &&
        cmSystemToolsFindRPath(se->Value, newRPath) != std::string::npos) {
      return true;
    }
  }
  return false;
#else
  (void)file;
  (void)newRPath;
  return false;
#endif
}

bool cmSystemTools::RepeatedRemoveDirectory(const char* dir)
{
  // Windows sometimes locks files temporarily so try a few times.
  for (int i = 0; i < 10; ++i) {
    if (cmSystemTools::RemoveADirectory(dir)) {
      return true;
    }
    cmSystemTools::Delay(100);
  }
  return false;
}

std::vector<std::string> cmSystemTools::tokenize(const std::string& str,
                                                 const std::string& sep)
{
  std::vector<std::string> tokens;
  std::string::size_type tokend = 0;

  do {
    std::string::size_type tokstart = str.find_first_not_of(sep, tokend);
    if (tokstart == std::string::npos) {
      break; // no more tokens
    }
    tokend = str.find_first_of(sep, tokstart);
    if (tokend == std::string::npos) {
      tokens.push_back(str.substr(tokstart));
    } else {
      tokens.push_back(str.substr(tokstart, tokend - tokstart));
    }
  } while (tokend != std::string::npos);

  if (tokens.empty()) {
    tokens.push_back("");
  }
  return tokens;
}

bool cmSystemTools::StringToLong(const char* str, long* value)
{
  errno = 0;
  char* endp;
  *value = strtol(str, &endp, 10);
  return (*endp == '\0') && (endp != str) && (errno == 0);
}

bool cmSystemTools::StringToULong(const char* str, unsigned long* value)
{
  errno = 0;
  char* endp;
  *value = strtoul(str, &endp, 10);
  return (*endp == '\0') && (endp != str) && (errno == 0);
}
