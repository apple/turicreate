/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmSystemTools_h
#define cmSystemTools_h

#include "cmConfigure.h"

#include "cmProcessOutput.h"
#include "cmsys/Process.h"
#include "cmsys/SystemTools.hxx" // IWYU pragma: export
#include <stddef.h>
#include <string>
#include <vector>

class cmSystemToolsFileTime;

/** \class cmSystemTools
 * \brief A collection of useful functions for CMake.
 *
 * cmSystemTools is a class that provides helper functions
 * for the CMake build system.
 */
class cmSystemTools : public cmsys::SystemTools
{
public:
  typedef cmsys::SystemTools Superclass;
  typedef cmProcessOutput::Encoding Encoding;

  /** Expand out any arguments in the vector that have ; separated
   *  strings into multiple arguments.  A new vector is created
   *  containing the expanded versions of all arguments in argsIn.
   */
  static void ExpandList(std::vector<std::string> const& argsIn,
                         std::vector<std::string>& argsOut);
  static void ExpandListArgument(const std::string& arg,
                                 std::vector<std::string>& argsOut,
                                 bool emptyArgs = false);

  /**
   * Look for and replace registry values in a string
   */
  static void ExpandRegistryValues(std::string& source,
                                   KeyWOW64 view = KeyWOW64_Default);

  ///! Escape quotes in a string.
  static std::string EscapeQuotes(const std::string& str);

  /** Map help document name to file name.  */
  static std::string HelpFileName(std::string);

  /**
   * Returns a string that has whitespace removed from the start and the end.
   */
  static std::string TrimWhitespace(const std::string& s);

  typedef void (*MessageCallback)(const char*, const char*, bool&, void*);
  /**
   *  Set the function used by GUIs to display error messages
   *  Function gets passed: message as a const char*,
   *  title as a const char*, and a reference to bool that when
   *  set to false, will disable furthur messages (cancel).
   */
  static void SetMessageCallback(MessageCallback f,
                                 void* clientData = CM_NULLPTR);

  /**
   * Display an error message.
   */
  static void Error(const char* m, const char* m2 = CM_NULLPTR,
                    const char* m3 = CM_NULLPTR, const char* m4 = CM_NULLPTR);

  /**
   * Display a message.
   */
  static void Message(const char* m, const char* title = CM_NULLPTR);

  typedef void (*OutputCallback)(const char*, size_t length, void*);

  ///! Send a string to stdout
  static void Stdout(const char* s);
  static void Stdout(const char* s, size_t length);
  static void SetStdoutCallback(OutputCallback, void* clientData = CM_NULLPTR);

  ///! Send a string to stderr
  static void Stderr(const char* s);
  static void Stderr(const char* s, size_t length);
  static void SetStderrCallback(OutputCallback, void* clientData = CM_NULLPTR);

  typedef bool (*InterruptCallback)(void*);
  static void SetInterruptCallback(InterruptCallback f,
                                   void* clientData = CM_NULLPTR);
  static bool GetInterruptFlag();

  ///! Return true if there was an error at any point.
  static bool GetErrorOccuredFlag()
  {
    return cmSystemTools::s_ErrorOccured ||
      cmSystemTools::s_FatalErrorOccured || GetInterruptFlag();
  }
  ///! If this is set to true, cmake stops processing commands.
  static void SetFatalErrorOccured()
  {
    cmSystemTools::s_FatalErrorOccured = true;
  }
  static void SetErrorOccured() { cmSystemTools::s_ErrorOccured = true; }
  ///! Return true if there was an error at any point.
  static bool GetFatalErrorOccured()
  {
    return cmSystemTools::s_FatalErrorOccured || GetInterruptFlag();
  }

  ///! Set the error occurred flag and fatal error back to false
  static void ResetErrorOccuredFlag()
  {
    cmSystemTools::s_FatalErrorOccured = false;
    cmSystemTools::s_ErrorOccured = false;
  }

  /**
   * Does a string indicates that CMake/CPack/CTest internally
   * forced this value. This is not the same as On, but this
   * may be considered as "internally switched on".
   */
  static bool IsInternallyOn(const char* val);
  /**
   * does a string indicate a true or on value ? This is not the same
   * as ifdef.
   */
  static bool IsOn(const char* val);

  /**
   * does a string indicate a false or off value ? Note that this is
   * not the same as !IsOn(...) because there are a number of
   * ambiguous values such as "/usr/local/bin" a path will result in
   * IsON and IsOff both returning false. Note that the special path
   * NOTFOUND, *-NOTFOUND or IGNORE will cause IsOff to return true.
   */
  static bool IsOff(const char* val);

  ///! Return true if value is NOTFOUND or ends in -NOTFOUND.
  static bool IsNOTFOUND(const char* value);
  ///! Return true if the path is a framework
  static bool IsPathToFramework(const char* value);

  static bool DoesFileExistWithExtensions(
    const char* name, const std::vector<std::string>& sourceExts);

  /**
   * Check if the given file exists in one of the parent directory of the
   * given file or directory and if it does, return the name of the file.
   * Toplevel specifies the top-most directory to where it will look.
   */
  static std::string FileExistsInParentDirectories(const char* fname,
                                                   const char* directory,
                                                   const char* toplevel);

  static void Glob(const std::string& directory, const std::string& regexp,
                   std::vector<std::string>& files);
  static void GlobDirs(const std::string& fullPath,
                       std::vector<std::string>& files);

  /**
   * Try to find a list of files that match the "simple" globbing
   * expression. At this point in time the globbing expressions have
   * to be in form: /directory/partial_file_name*. The * character has
   * to be at the end of the string and it does not support ?
   * []... The optional argument type specifies what kind of files you
   * want to find. 0 means all files, -1 means directories, 1 means
   * files only. This method returns true if search was succesfull.
   */
  static bool SimpleGlob(const std::string& glob,
                         std::vector<std::string>& files, int type = 0);

  ///! Copy a file.
  static bool cmCopyFile(const char* source, const char* destination);
  static bool CopyFileIfDifferent(const char* source, const char* destination);

  /** Rename a file or directory within a single disk volume (atomic
      if possible).  */
  static bool RenameFile(const char* oldname, const char* newname);

  ///! Compute the md5sum of a file
  static bool ComputeFileMD5(const std::string& source, char* md5out);

  /** Compute the md5sum of a string.  */
  static std::string ComputeStringMD5(const std::string& input);

  ///! Get the SHA thumbprint for a certificate file
  static std::string ComputeCertificateThumbprint(const std::string& source);

  /**
   * Run a single executable command
   *
   * Output is controlled with outputflag. If outputflag is OUTPUT_NONE, no
   * user-viewable output from the program being run will be generated.
   * OUTPUT_MERGE is the legacy behaviour where stdout and stderr are merged
   * into stdout.  OUTPUT_FORWARD copies the output to stdout/stderr as
   * it was received.  OUTPUT_PASSTHROUGH passes through the original handles.
   *
   * If timeout is specified, the command will be terminated after
   * timeout expires. Timeout is specified in seconds.
   *
   * Argument retVal should be a pointer to the location where the
   * exit code will be stored. If the retVal is not specified and
   * the program exits with a code other than 0, then the this
   * function will return false.
   *
   * If the command has spaces in the path the caller MUST call
   * cmSystemTools::ConvertToRunCommandPath on the command before passing
   * it into this function or it will not work.  The command must be correctly
   * escaped for this to with spaces.
   */
  enum OutputOption
  {
    OUTPUT_NONE = 0,
    OUTPUT_MERGE,
    OUTPUT_FORWARD,
    OUTPUT_PASSTHROUGH
  };
  static bool RunSingleCommand(const char* command,
                               std::string* captureStdOut = CM_NULLPTR,
                               std::string* captureStdErr = CM_NULLPTR,
                               int* retVal = CM_NULLPTR,
                               const char* dir = CM_NULLPTR,
                               OutputOption outputflag = OUTPUT_MERGE,
                               double timeout = 0.0);
  /**
   * In this version of RunSingleCommand, command[0] should be
   * the command to run, and each argument to the command should
   * be in comand[1]...command[command.size()]
   */
  static bool RunSingleCommand(std::vector<std::string> const& command,
                               std::string* captureStdOut = CM_NULLPTR,
                               std::string* captureStdErr = CM_NULLPTR,
                               int* retVal = CM_NULLPTR,
                               const char* dir = CM_NULLPTR,
                               OutputOption outputflag = OUTPUT_MERGE,
                               double timeout = 0.0,
                               Encoding encoding = cmProcessOutput::Auto);

  static std::string PrintSingleCommand(std::vector<std::string> const&);

  /**
   * Parse arguments out of a single string command
   */
  static std::vector<std::string> ParseArguments(const char* command);

  /** Parse arguments out of a windows command line string.  */
  static void ParseWindowsCommandLine(const char* command,
                                      std::vector<std::string>& args);

  /** Parse arguments out of a unix command line string.  */
  static void ParseUnixCommandLine(const char* command,
                                   std::vector<std::string>& args);

  /**
   * Handle response file in an argument list and return a new argument list
   * **/
  static std::vector<std::string> HandleResponseFile(
    std::vector<std::string>::const_iterator argBeg,
    std::vector<std::string>::const_iterator argEnd);

  static size_t CalculateCommandLineLengthLimit();

  static void EnableMessages() { s_DisableMessages = false; }
  static void DisableMessages() { s_DisableMessages = true; }
  static void DisableRunCommandOutput() { s_DisableRunCommandOutput = true; }
  static void EnableRunCommandOutput() { s_DisableRunCommandOutput = false; }
  static bool GetRunCommandOutput() { return s_DisableRunCommandOutput; }

  /**
   * Some constants for different file formats.
   */
  enum FileFormat
  {
    NO_FILE_FORMAT = 0,
    C_FILE_FORMAT,
    CXX_FILE_FORMAT,
    FORTRAN_FILE_FORMAT,
    JAVA_FILE_FORMAT,
    HEADER_FILE_FORMAT,
    RESOURCE_FILE_FORMAT,
    DEFINITION_FILE_FORMAT,
    STATIC_LIBRARY_FILE_FORMAT,
    SHARED_LIBRARY_FILE_FORMAT,
    MODULE_FILE_FORMAT,
    OBJECT_FILE_FORMAT,
    UNKNOWN_FILE_FORMAT
  };

  enum CompareOp
  {
    OP_EQUAL = 1,
    OP_LESS = 2,
    OP_GREATER = 4,
    OP_LESS_EQUAL = OP_LESS | OP_EQUAL,
    OP_GREATER_EQUAL = OP_GREATER | OP_EQUAL
  };

  /**
   * Compare versions
   */
  static bool VersionCompare(CompareOp op, const char* lhs, const char* rhs);
  static bool VersionCompareEqual(std::string const& lhs,
                                  std::string const& rhs);
  static bool VersionCompareGreater(std::string const& lhs,
                                    std::string const& rhs);
  static bool VersionCompareGreaterEq(std::string const& lhs,
                                      std::string const& rhs);

  /**
   * Compare two ASCII strings using natural versioning order.
   * Non-numerical characters are compared directly.
   * Numerical characters are first globbed such that, e.g.
   * `test000 < test01 < test0 < test1 < test10`.
   * Return a value less than, equal to, or greater than zero if lhs
   * precedes, equals, or succeeds rhs in the defined ordering.
   */
  static int strverscmp(std::string const& lhs, std::string const& rhs);

  /**
   * Determine the file type based on the extension
   */
  static FileFormat GetFileFormat(const char* ext);

  /** Windows if this is true, the CreateProcess in RunCommand will
   *  not show new consol windows when running programs.
   */
  static void SetRunCommandHideConsole(bool v) { s_RunCommandHideConsole = v; }
  static bool GetRunCommandHideConsole() { return s_RunCommandHideConsole; }
  /** Call cmSystemTools::Error with the message m, plus the
   * result of strerror(errno)
   */
  static void ReportLastSystemError(const char* m);

  /** a general output handler for cmsysProcess  */
  static int WaitForLine(cmsysProcess* process, std::string& line,
                         double timeout, std::vector<char>& out,
                         std::vector<char>& err);

  /** Split a string on its newlines into multiple lines.  Returns
      false only if the last line stored had no newline.  */
  static bool Split(const char* s, std::vector<std::string>& l);
  static void SetForceUnixPaths(bool v) { s_ForceUnixPaths = v; }
  static bool GetForceUnixPaths() { return s_ForceUnixPaths; }

  // ConvertToOutputPath use s_ForceUnixPaths
  static std::string ConvertToOutputPath(const char* path);
  static void ConvertToOutputSlashes(std::string& path);

  // ConvertToRunCommandPath does not use s_ForceUnixPaths and should
  // be used when RunCommand is called from cmake, because the
  // running cmake needs paths to be in its format
  static std::string ConvertToRunCommandPath(const char* path);

  /** compute the relative path from local to remote.  local must
      be a directory.  remote can be a file or a directory.
      Both remote and local must be full paths.  Basically, if
      you are in directory local and you want to access the file in remote
      what is the relative path to do that.  For example:
      /a/b/c/d to /a/b/c1/d1 -> ../../c1/d1
      from /usr/src to /usr/src/test/blah/foo.cpp -> test/blah/foo.cpp
  */
  static std::string RelativePath(const char* local, const char* remote);

  /** Joins two paths while collapsing x/../ parts
   * For example CollapseCombinedPath("a/b/c", "../../d") results in "a/d"
   */
  static std::string CollapseCombinedPath(std::string const& dir,
                                          std::string const& file);

#ifdef CMAKE_BUILD_WITH_CMAKE
  /** Remove an environment variable */
  static bool UnsetEnv(const char* value);

  /** Get the list of all environment variables */
  static std::vector<std::string> GetEnvironmentVariables();

  /** Append multiple variables to the current environment. */
  static void AppendEnv(std::vector<std::string> const& env);

  /** Helper class to save and restore the environment.
      Instantiate this class as an automatic variable on
      the stack. Its constructor saves a copy of the current
      environment and then its destructor restores the
      original environment. */
  class SaveRestoreEnvironment
  {
    CM_DISABLE_COPY(SaveRestoreEnvironment)
  public:
    SaveRestoreEnvironment();
    ~SaveRestoreEnvironment();

  private:
    std::vector<std::string> Env;
  };
#endif

  /** Setup the environment to enable VS 8 IDE output.  */
  static void EnableVSConsoleOutput();

  /** Create tar */
  enum cmTarCompression
  {
    TarCompressGZip,
    TarCompressBZip2,
    TarCompressXZ,
    TarCompressNone
  };
  static bool ListTar(const char* outFileName, bool verbose);
  static bool CreateTar(const char* outFileName,
                        const std::vector<std::string>& files,
                        cmTarCompression compressType, bool verbose,
                        std::string const& mtime = std::string(),
                        std::string const& format = std::string());
  static bool ExtractTar(const char* inFileName, bool verbose);
  // This should be called first thing in main
  // it will keep child processes from inheriting the
  // stdin and stdout of this process.  This is important
  // if you want to be able to kill child processes and
  // not get stuck waiting for all the output on the pipes.
  static void DoNotInheritStdPipes();

  /** Copy the file create/access/modify times from the file named by
      the first argument to that named by the second.  */
  static bool CopyFileTime(const char* fromFile, const char* toFile);

  /** Save and restore file times.  */
  static cmSystemToolsFileTime* FileTimeNew();
  static void FileTimeDelete(cmSystemToolsFileTime*);
  static bool FileTimeGet(const char* fname, cmSystemToolsFileTime* t);
  static bool FileTimeSet(const char* fname, cmSystemToolsFileTime* t);

  /** Random seed generation.  */
  static unsigned int RandomSeed();

  /** Find the directory containing CMake executables.  */
  static void FindCMakeResources(const char* argv0);

  /** Get the CMake resource paths, after FindCMakeResources.  */
  static std::string const& GetCTestCommand();
  static std::string const& GetCPackCommand();
  static std::string const& GetCMakeCommand();
  static std::string const& GetCMakeGUICommand();
  static std::string const& GetCMakeCursesCommand();
  static std::string const& GetCMClDepsCommand();
  static std::string const& GetCMakeRoot();

  /** Echo a message in color using KWSys's Terminal cprintf.  */
  static void MakefileColorEcho(int color, const char* message, bool newLine,
                                bool enabled);

  /** Try to guess the soname of a shared library.  */
  static bool GuessLibrarySOName(std::string const& fullPath,
                                 std::string& soname);

  /** Try to guess the install name of a shared library.  */
  static bool GuessLibraryInstallName(std::string const& fullPath,
                                      std::string& soname);

  /** Try to set the RPATH in an ELF binary.  */
  static bool ChangeRPath(std::string const& file, std::string const& oldRPath,
                          std::string const& newRPath,
                          std::string* emsg = CM_NULLPTR,
                          bool* changed = CM_NULLPTR);

  /** Try to remove the RPATH from an ELF binary.  */
  static bool RemoveRPath(std::string const& file,
                          std::string* emsg = CM_NULLPTR,
                          bool* removed = CM_NULLPTR);

  /** Check whether the RPATH in an ELF binary contains the path
      given.  */
  static bool CheckRPath(std::string const& file, std::string const& newRPath);

  /** Remove a directory; repeat a few times in case of locked files.  */
  static bool RepeatedRemoveDirectory(const char* dir);

  /** Tokenize a string */
  static std::vector<std::string> tokenize(const std::string& str,
                                           const std::string& sep);

  /** Convert string to long. Expected that the whole string is an integer */
  static bool StringToLong(const char* str, long* value);
  static bool StringToULong(const char* str, unsigned long* value);

#ifdef _WIN32
  struct WindowsFileRetry
  {
    unsigned int Count;
    unsigned int Delay;
  };
  static WindowsFileRetry GetWindowsFileRetry();
#endif
private:
  static bool s_ForceUnixPaths;
  static bool s_RunCommandHideConsole;
  static bool s_ErrorOccured;
  static bool s_FatalErrorOccured;
  static bool s_DisableMessages;
  static bool s_DisableRunCommandOutput;
  static MessageCallback s_MessageCallback;
  static OutputCallback s_StdoutCallback;
  static OutputCallback s_StderrCallback;
  static InterruptCallback s_InterruptCallback;
  static void* s_MessageCallbackClientData;
  static void* s_StdoutCallbackClientData;
  static void* s_StderrCallbackClientData;
  static void* s_InterruptCallbackClientData;
};

#endif
