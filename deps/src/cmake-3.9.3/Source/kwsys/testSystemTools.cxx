/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4786)
#endif

#include KWSYS_HEADER(FStream.hxx)
#include KWSYS_HEADER(SystemTools.hxx)

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#include "FStream.hxx.in"
#include "SystemTools.hxx.in"
#endif

// Include with <> instead of "" to avoid getting any in-source copy
// left on disk.
#include <testSystemTools.h>

#include <iostream>
#include <sstream>
#include <string.h> /* strcmp */
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <io.h> /* _umask (MSVC) / umask (Borland) */
#ifdef _MSC_VER
#define umask _umask // Note this is still umask on Borland
#endif
#endif
#include <sys/stat.h> /* umask (POSIX), _S_I* constants (Windows) */
// Visual C++ does not define mode_t (note that Borland does, however).
#if defined(_MSC_VER)
typedef unsigned short mode_t;
#endif

static const char* toUnixPaths[][2] = {
  { "/usr/local/bin/passwd", "/usr/local/bin/passwd" },
  { "/usr/lo cal/bin/pa sswd", "/usr/lo cal/bin/pa sswd" },
  { "/usr/lo\\ cal/bin/pa\\ sswd", "/usr/lo\\ cal/bin/pa\\ sswd" },
  { "c:/usr/local/bin/passwd", "c:/usr/local/bin/passwd" },
  { "c:/usr/lo cal/bin/pa sswd", "c:/usr/lo cal/bin/pa sswd" },
  { "c:/usr/lo\\ cal/bin/pa\\ sswd", "c:/usr/lo\\ cal/bin/pa\\ sswd" },
  { "\\usr\\local\\bin\\passwd", "/usr/local/bin/passwd" },
  { "\\usr\\lo cal\\bin\\pa sswd", "/usr/lo cal/bin/pa sswd" },
  { "\\usr\\lo\\ cal\\bin\\pa\\ sswd", "/usr/lo\\ cal/bin/pa\\ sswd" },
  { "c:\\usr\\local\\bin\\passwd", "c:/usr/local/bin/passwd" },
  { "c:\\usr\\lo cal\\bin\\pa sswd", "c:/usr/lo cal/bin/pa sswd" },
  { "c:\\usr\\lo\\ cal\\bin\\pa\\ sswd", "c:/usr/lo\\ cal/bin/pa\\ sswd" },
  { "\\\\usr\\local\\bin\\passwd", "//usr/local/bin/passwd" },
  { "\\\\usr\\lo cal\\bin\\pa sswd", "//usr/lo cal/bin/pa sswd" },
  { "\\\\usr\\lo\\ cal\\bin\\pa\\ sswd", "//usr/lo\\ cal/bin/pa\\ sswd" },
  { 0, 0 }
};

static bool CheckConvertToUnixSlashes(std::string const& input,
                                      std::string const& output)
{
  std::string result = input;
  kwsys::SystemTools::ConvertToUnixSlashes(result);
  if (result != output) {
    std::cerr << "Problem with ConvertToUnixSlashes - input: " << input
              << " output: " << result << " expected: " << output << std::endl;
    return false;
  }
  return true;
}

static const char* checkEscapeChars[][4] = { { "1 foo 2 bar 2", "12", "\\",
                                               "\\1 foo \\2 bar \\2" },
                                             { " {} ", "{}", "#", " #{#} " },
                                             { 0, 0, 0, 0 } };

static bool CheckEscapeChars(std::string const& input,
                             const char* chars_to_escape, char escape_char,
                             std::string const& output)
{
  std::string result = kwsys::SystemTools::EscapeChars(
    input.c_str(), chars_to_escape, escape_char);
  if (result != output) {
    std::cerr << "Problem with CheckEscapeChars - input: " << input
              << " output: " << result << " expected: " << output << std::endl;
    return false;
  }
  return true;
}

static bool CheckFileOperations()
{
  bool res = true;
  const std::string testNonExistingFile(TEST_SYSTEMTOOLS_SOURCE_DIR
                                        "/testSystemToolsNonExistingFile");
  const std::string testDotFile(TEST_SYSTEMTOOLS_SOURCE_DIR "/.");
  const std::string testBinFile(TEST_SYSTEMTOOLS_SOURCE_DIR
                                "/testSystemTools.bin");
  const std::string testTxtFile(TEST_SYSTEMTOOLS_SOURCE_DIR
                                "/testSystemTools.cxx");
  const std::string testNewDir(TEST_SYSTEMTOOLS_BINARY_DIR
                               "/testSystemToolsNewDir");
  const std::string testNewFile(testNewDir + "/testNewFile.txt");

  if (kwsys::SystemTools::DetectFileType(testNonExistingFile.c_str()) !=
      kwsys::SystemTools::FileTypeUnknown) {
    std::cerr << "Problem with DetectFileType - failed to detect type of: "
              << testNonExistingFile << std::endl;
    res = false;
  }

  if (kwsys::SystemTools::DetectFileType(testDotFile.c_str()) !=
      kwsys::SystemTools::FileTypeUnknown) {
    std::cerr << "Problem with DetectFileType - failed to detect type of: "
              << testDotFile << std::endl;
    res = false;
  }

  if (kwsys::SystemTools::DetectFileType(testBinFile.c_str()) !=
      kwsys::SystemTools::FileTypeBinary) {
    std::cerr << "Problem with DetectFileType - failed to detect type of: "
              << testBinFile << std::endl;
    res = false;
  }

  if (kwsys::SystemTools::DetectFileType(testTxtFile.c_str()) !=
      kwsys::SystemTools::FileTypeText) {
    std::cerr << "Problem with DetectFileType - failed to detect type of: "
              << testTxtFile << std::endl;
    res = false;
  }

  if (kwsys::SystemTools::FileLength(testBinFile) != 766) {
    std::cerr << "Problem with FileLength - incorrect length for: "
              << testBinFile << std::endl;
    res = false;
  }

  kwsys::SystemTools::Stat_t buf;
  if (kwsys::SystemTools::Stat(testTxtFile.c_str(), &buf) != 0) {
    std::cerr << "Problem with Stat - unable to stat text file: "
              << testTxtFile << std::endl;
    res = false;
  }

  if (kwsys::SystemTools::Stat(testBinFile, &buf) != 0) {
    std::cerr << "Problem with Stat - unable to stat bin file: " << testBinFile
              << std::endl;
    res = false;
  }

  if (!kwsys::SystemTools::MakeDirectory(testNewDir)) {
    std::cerr << "Problem with MakeDirectory for: " << testNewDir << std::endl;
    res = false;
  }
  // calling it again should just return true
  if (!kwsys::SystemTools::MakeDirectory(testNewDir)) {
    std::cerr << "Problem with second call to MakeDirectory for: "
              << testNewDir << std::endl;
    res = false;
  }
  // calling with 0 pointer should return false
  if (kwsys::SystemTools::MakeDirectory(0)) {
    std::cerr << "Problem with MakeDirectory(0)" << std::endl;
    res = false;
  }
  // calling with an empty string should return false
  if (kwsys::SystemTools::MakeDirectory(std::string())) {
    std::cerr << "Problem with MakeDirectory(std::string())" << std::endl;
    res = false;
  }
  // check existence
  if (!kwsys::SystemTools::FileExists(testNewDir.c_str(), false)) {
    std::cerr << "Problem with FileExists as C string and not file for: "
              << testNewDir << std::endl;
    res = false;
  }
  // check existence
  if (!kwsys::SystemTools::PathExists(testNewDir)) {
    std::cerr << "Problem with PathExists for: " << testNewDir << std::endl;
    res = false;
  }
  // remove it
  if (!kwsys::SystemTools::RemoveADirectory(testNewDir)) {
    std::cerr << "Problem with RemoveADirectory for: " << testNewDir
              << std::endl;
    res = false;
  }
  // check existence
  if (kwsys::SystemTools::FileExists(testNewDir.c_str(), false)) {
    std::cerr << "After RemoveADirectory: "
              << "Problem with FileExists as C string and not file for: "
              << testNewDir << std::endl;
    res = false;
  }
  // check existence
  if (kwsys::SystemTools::PathExists(testNewDir)) {
    std::cerr << "After RemoveADirectory: "
              << "Problem with PathExists for: " << testNewDir << std::endl;
    res = false;
  }
  // create it using the char* version
  if (!kwsys::SystemTools::MakeDirectory(testNewDir.c_str())) {
    std::cerr << "Problem with second call to MakeDirectory as C string for: "
              << testNewDir << std::endl;
    res = false;
  }

  if (!kwsys::SystemTools::Touch(testNewFile.c_str(), true)) {
    std::cerr << "Problem with Touch for: " << testNewFile << std::endl;
    res = false;
  }
  // calling MakeDirectory with something that is no file should fail
  if (kwsys::SystemTools::MakeDirectory(testNewFile)) {
    std::cerr << "Problem with to MakeDirectory for: " << testNewFile
              << std::endl;
    res = false;
  }

  // calling with 0 pointer should return false
  if (kwsys::SystemTools::FileExists(0)) {
    std::cerr << "Problem with FileExists(0)" << std::endl;
    res = false;
  }
  if (kwsys::SystemTools::FileExists(0, true)) {
    std::cerr << "Problem with FileExists(0) as file" << std::endl;
    res = false;
  }
  // calling with an empty string should return false
  if (kwsys::SystemTools::FileExists(std::string())) {
    std::cerr << "Problem with FileExists(std::string())" << std::endl;
    res = false;
  }
  // FileExists(x, true) should return false on a directory
  if (kwsys::SystemTools::FileExists(testNewDir, true)) {
    std::cerr << "Problem with FileExists as file for: " << testNewDir
              << std::endl;
    res = false;
  }
  if (kwsys::SystemTools::FileExists(testNewDir.c_str(), true)) {
    std::cerr << "Problem with FileExists as C string and file for: "
              << testNewDir << std::endl;
    res = false;
  }
  // FileExists(x, false) should return true even on a directory
  if (!kwsys::SystemTools::FileExists(testNewDir, false)) {
    std::cerr << "Problem with FileExists as not file for: " << testNewDir
              << std::endl;
    res = false;
  }
  if (!kwsys::SystemTools::FileExists(testNewDir.c_str(), false)) {
    std::cerr << "Problem with FileExists as C string and not file for: "
              << testNewDir << std::endl;
    res = false;
  }
  // should work, was created as new file before
  if (!kwsys::SystemTools::FileExists(testNewFile)) {
    std::cerr << "Problem with FileExists for: " << testNewDir << std::endl;
    res = false;
  }
  if (!kwsys::SystemTools::FileExists(testNewFile.c_str())) {
    std::cerr << "Problem with FileExists as C string for: " << testNewDir
              << std::endl;
    res = false;
  }
  if (!kwsys::SystemTools::FileExists(testNewFile, true)) {
    std::cerr << "Problem with FileExists as file for: " << testNewDir
              << std::endl;
    res = false;
  }
  if (!kwsys::SystemTools::FileExists(testNewFile.c_str(), true)) {
    std::cerr << "Problem with FileExists as C string and file for: "
              << testNewDir << std::endl;
    res = false;
  }

  // calling with an empty string should return false
  if (kwsys::SystemTools::PathExists(std::string())) {
    std::cerr << "Problem with PathExists(std::string())" << std::endl;
    res = false;
  }
  // PathExists(x) should return true on a directory
  if (!kwsys::SystemTools::PathExists(testNewDir)) {
    std::cerr << "Problem with PathExists for: " << testNewDir << std::endl;
    res = false;
  }
  // should work, was created as new file before
  if (!kwsys::SystemTools::PathExists(testNewFile)) {
    std::cerr << "Problem with PathExists for: " << testNewDir << std::endl;
    res = false;
  }

// Reset umask
#if defined(_WIN32) && !defined(__CYGWIN__)
  // NOTE:  Windows doesn't support toggling _S_IREAD.
  mode_t fullMask = _S_IWRITE;
#else
  // On a normal POSIX platform, we can toggle all permissions.
  mode_t fullMask = S_IRWXU | S_IRWXG | S_IRWXO;
#endif
  mode_t orig_umask = umask(fullMask);

  // Test file permissions without umask
  mode_t origPerm, thisPerm;
  if (!kwsys::SystemTools::GetPermissions(testNewFile, origPerm)) {
    std::cerr << "Problem with GetPermissions (1) for: " << testNewFile
              << std::endl;
    res = false;
  }

  if (!kwsys::SystemTools::SetPermissions(testNewFile, 0)) {
    std::cerr << "Problem with SetPermissions (1) for: " << testNewFile
              << std::endl;
    res = false;
  }

  if (!kwsys::SystemTools::GetPermissions(testNewFile, thisPerm)) {
    std::cerr << "Problem with GetPermissions (2) for: " << testNewFile
              << std::endl;
    res = false;
  }

  if ((thisPerm & fullMask) != 0) {
    std::cerr << "SetPermissions failed to set permissions (1) for: "
              << testNewFile << ": actual = " << thisPerm
              << "; expected = " << 0 << std::endl;
    res = false;
  }

  // While we're at it, check proper TestFileAccess functionality.
  if (kwsys::SystemTools::TestFileAccess(testNewFile,
                                         kwsys::TEST_FILE_WRITE)) {
    std::cerr
      << "TestFileAccess incorrectly indicated that this is a writable file:"
      << testNewFile << std::endl;
    res = false;
  }

  if (!kwsys::SystemTools::TestFileAccess(testNewFile, kwsys::TEST_FILE_OK)) {
    std::cerr
      << "TestFileAccess incorrectly indicated that this file does not exist:"
      << testNewFile << std::endl;
    res = false;
  }

  // Test restoring/setting full permissions.
  if (!kwsys::SystemTools::SetPermissions(testNewFile, fullMask)) {
    std::cerr << "Problem with SetPermissions (2) for: " << testNewFile
              << std::endl;
    res = false;
  }

  if (!kwsys::SystemTools::GetPermissions(testNewFile, thisPerm)) {
    std::cerr << "Problem with GetPermissions (3) for: " << testNewFile
              << std::endl;
    res = false;
  }

  if ((thisPerm & fullMask) != fullMask) {
    std::cerr << "SetPermissions failed to set permissions (2) for: "
              << testNewFile << ": actual = " << thisPerm
              << "; expected = " << fullMask << std::endl;
    res = false;
  }

  // Test setting file permissions while honoring umask
  if (!kwsys::SystemTools::SetPermissions(testNewFile, fullMask, true)) {
    std::cerr << "Problem with SetPermissions (3) for: " << testNewFile
              << std::endl;
    res = false;
  }

  if (!kwsys::SystemTools::GetPermissions(testNewFile, thisPerm)) {
    std::cerr << "Problem with GetPermissions (4) for: " << testNewFile
              << std::endl;
    res = false;
  }

  if ((thisPerm & fullMask) != 0) {
    std::cerr << "SetPermissions failed to honor umask for: " << testNewFile
              << ": actual = " << thisPerm << "; expected = " << 0
              << std::endl;
    res = false;
  }

  // Restore umask
  umask(orig_umask);

  // Restore file permissions
  if (!kwsys::SystemTools::SetPermissions(testNewFile, origPerm)) {
    std::cerr << "Problem with SetPermissions (4) for: " << testNewFile
              << std::endl;
    res = false;
  }

  // Remove the test file
  if (!kwsys::SystemTools::RemoveFile(testNewFile)) {
    std::cerr << "Problem with RemoveFile: " << testNewFile << std::endl;
    res = false;
  }

  std::string const testFileMissing(testNewDir + "/testMissingFile.txt");
  if (!kwsys::SystemTools::RemoveFile(testFileMissing)) {
    std::string const& msg = kwsys::SystemTools::GetLastSystemError();
    std::cerr << "RemoveFile(\"" << testFileMissing << "\") failed: " << msg
              << "\n";
    res = false;
  }

  std::string const testFileMissingDir(testNewDir + "/missing/file.txt");
  if (!kwsys::SystemTools::RemoveFile(testFileMissingDir)) {
    std::string const& msg = kwsys::SystemTools::GetLastSystemError();
    std::cerr << "RemoveFile(\"" << testFileMissingDir << "\") failed: " << msg
              << "\n";
    res = false;
  }

  kwsys::SystemTools::Touch(testNewFile.c_str(), true);
  if (!kwsys::SystemTools::RemoveADirectory(testNewDir)) {
    std::cerr << "Problem with RemoveADirectory for: " << testNewDir
              << std::endl;
    res = false;
  }

#ifdef KWSYS_TEST_SYSTEMTOOLS_LONG_PATHS
  // Perform the same file and directory creation and deletion tests but
  // with paths > 256 characters in length.

  const std::string testNewLongDir(
    TEST_SYSTEMTOOLS_BINARY_DIR
    "/"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "01234567890123");
  const std::string testNewLongFile(
    testNewLongDir +
    "/"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "012345678901234567890123456789012345678901234567890123456789"
    "0123456789.txt");

  if (!kwsys::SystemTools::MakeDirectory(testNewLongDir)) {
    std::cerr << "Problem with MakeDirectory for: " << testNewLongDir
              << std::endl;
    res = false;
  }

  if (!kwsys::SystemTools::Touch(testNewLongFile.c_str(), true)) {
    std::cerr << "Problem with Touch for: " << testNewLongFile << std::endl;
    res = false;
  }

  if (!kwsys::SystemTools::RemoveFile(testNewLongFile)) {
    std::cerr << "Problem with RemoveFile: " << testNewLongFile << std::endl;
    res = false;
  }

  kwsys::SystemTools::Touch(testNewLongFile.c_str(), true);
  if (!kwsys::SystemTools::RemoveADirectory(testNewLongDir)) {
    std::cerr << "Problem with RemoveADirectory for: " << testNewLongDir
              << std::endl;
    res = false;
  }
#endif

  return res;
}

static bool CheckStringOperations()
{
  bool res = true;

  std::string test = "mary had a little lamb.";
  if (kwsys::SystemTools::CapitalizedWords(test) !=
      "Mary Had A Little Lamb.") {
    std::cerr << "Problem with CapitalizedWords " << '"' << test << '"'
              << std::endl;
    res = false;
  }

  test = "Mary Had A Little Lamb.";
  if (kwsys::SystemTools::UnCapitalizedWords(test) !=
      "mary had a little lamb.") {
    std::cerr << "Problem with UnCapitalizedWords " << '"' << test << '"'
              << std::endl;
    res = false;
  }

  test = "MaryHadTheLittleLamb.";
  if (kwsys::SystemTools::AddSpaceBetweenCapitalizedWords(test) !=
      "Mary Had The Little Lamb.") {
    std::cerr << "Problem with AddSpaceBetweenCapitalizedWords " << '"' << test
              << '"' << std::endl;
    res = false;
  }

  char* cres =
    kwsys::SystemTools::AppendStrings("Mary Had A", " Little Lamb.");
  if (strcmp(cres, "Mary Had A Little Lamb.")) {
    std::cerr << "Problem with AppendStrings "
              << "\"Mary Had A\" \" Little Lamb.\"" << std::endl;
    res = false;
  }
  delete[] cres;

  cres = kwsys::SystemTools::AppendStrings("Mary Had", " A ", "Little Lamb.");
  if (strcmp(cres, "Mary Had A Little Lamb.")) {
    std::cerr << "Problem with AppendStrings "
              << "\"Mary Had\" \" A \" \"Little Lamb.\"" << std::endl;
    res = false;
  }
  delete[] cres;

  if (kwsys::SystemTools::CountChar("Mary Had A Little Lamb.", 'a') != 3) {
    std::cerr << "Problem with CountChar "
              << "\"Mary Had A Little Lamb.\"" << std::endl;
    res = false;
  }

  cres = kwsys::SystemTools::RemoveChars("Mary Had A Little Lamb.", "aeiou");
  if (strcmp(cres, "Mry Hd A Lttl Lmb.")) {
    std::cerr << "Problem with RemoveChars "
              << "\"Mary Had A Little Lamb.\"" << std::endl;
    res = false;
  }
  delete[] cres;

  cres = kwsys::SystemTools::RemoveCharsButUpperHex("Mary Had A Little Lamb.");
  if (strcmp(cres, "A")) {
    std::cerr << "Problem with RemoveCharsButUpperHex "
              << "\"Mary Had A Little Lamb.\"" << std::endl;
    res = false;
  }
  delete[] cres;

  char* cres2 = new char[strlen("Mary Had A Little Lamb.") + 1];
  strcpy(cres2, "Mary Had A Little Lamb.");
  kwsys::SystemTools::ReplaceChars(cres2, "aeiou", 'X');
  if (strcmp(cres2, "MXry HXd A LXttlX LXmb.")) {
    std::cerr << "Problem with ReplaceChars "
              << "\"Mary Had A Little Lamb.\"" << std::endl;
    res = false;
  }
  delete[] cres2;

  if (!kwsys::SystemTools::StringStartsWith("Mary Had A Little Lamb.",
                                            "Mary ")) {
    std::cerr << "Problem with StringStartsWith "
              << "\"Mary Had A Little Lamb.\"" << std::endl;
    res = false;
  }

  if (!kwsys::SystemTools::StringEndsWith("Mary Had A Little Lamb.",
                                          " Lamb.")) {
    std::cerr << "Problem with StringEndsWith "
              << "\"Mary Had A Little Lamb.\"" << std::endl;
    res = false;
  }

  cres = kwsys::SystemTools::DuplicateString("Mary Had A Little Lamb.");
  if (strcmp(cres, "Mary Had A Little Lamb.")) {
    std::cerr << "Problem with DuplicateString "
              << "\"Mary Had A Little Lamb.\"" << std::endl;
    res = false;
  }
  delete[] cres;

  test = "Mary Had A Little Lamb.";
  if (kwsys::SystemTools::CropString(test, 13) != "Mary ...Lamb.") {
    std::cerr << "Problem with CropString "
              << "\"Mary Had A Little Lamb.\"" << std::endl;
    res = false;
  }

  std::vector<std::string> lines;
  kwsys::SystemTools::Split("Mary Had A Little Lamb.", lines, ' ');
  if (lines[0] != "Mary" || lines[1] != "Had" || lines[2] != "A" ||
      lines[3] != "Little" || lines[4] != "Lamb.") {
    std::cerr << "Problem with Split "
              << "\"Mary Had A Little Lamb.\"" << std::endl;
    res = false;
  }

  if (kwsys::SystemTools::ConvertToWindowsOutputPath(
        "L://Local Mojo/Hex Power Pack/Iffy Voodoo") !=
      "\"L:\\Local Mojo\\Hex Power Pack\\Iffy Voodoo\"") {
    std::cerr << "Problem with ConvertToWindowsOutputPath "
              << "\"L://Local Mojo/Hex Power Pack/Iffy Voodoo\"" << std::endl;
    res = false;
  }

  if (kwsys::SystemTools::ConvertToWindowsOutputPath(
        "//grayson/Local Mojo/Hex Power Pack/Iffy Voodoo") !=
      "\"\\\\grayson\\Local Mojo\\Hex Power Pack\\Iffy Voodoo\"") {
    std::cerr << "Problem with ConvertToWindowsOutputPath "
              << "\"//grayson/Local Mojo/Hex Power Pack/Iffy Voodoo\""
              << std::endl;
    res = false;
  }

  if (kwsys::SystemTools::ConvertToUnixOutputPath(
        "//Local Mojo/Hex Power Pack/Iffy Voodoo") !=
      "//Local\\ Mojo/Hex\\ Power\\ Pack/Iffy\\ Voodoo") {
    std::cerr << "Problem with ConvertToUnixOutputPath "
              << "\"//Local Mojo/Hex Power Pack/Iffy Voodoo\"" << std::endl;
    res = false;
  }

  return res;
}

static bool CheckPutEnv(const std::string& env, const char* name,
                        const char* value)
{
  if (!kwsys::SystemTools::PutEnv(env)) {
    std::cerr << "PutEnv(\"" << env << "\") failed!" << std::endl;
    return false;
  }
  std::string v = "(null)";
  kwsys::SystemTools::GetEnv(name, v);
  if (v != value) {
    std::cerr << "GetEnv(\"" << name << "\") returned \"" << v << "\", not \""
              << value << "\"!" << std::endl;
    return false;
  }
  return true;
}

static bool CheckUnPutEnv(const char* env, const char* name)
{
  if (!kwsys::SystemTools::UnPutEnv(env)) {
    std::cerr << "UnPutEnv(\"" << env << "\") failed!" << std::endl;
    return false;
  }
  std::string v;
  if (kwsys::SystemTools::GetEnv(name, v)) {
    std::cerr << "GetEnv(\"" << name << "\") returned \"" << v
              << "\", not (null)!" << std::endl;
    return false;
  }
  return true;
}

static bool CheckEnvironmentOperations()
{
  bool res = true;
  res &= CheckPutEnv("A=B", "A", "B");
  res &= CheckPutEnv("B=C", "B", "C");
  res &= CheckPutEnv("C=D", "C", "D");
  res &= CheckPutEnv("D=E", "D", "E");
  res &= CheckUnPutEnv("A", "A");
  res &= CheckUnPutEnv("B=", "B");
  res &= CheckUnPutEnv("C=D", "C");
  /* Leave "D=E" in environment so a memory checker can test for leaks.  */
  return res;
}

static bool CheckRelativePath(const std::string& local,
                              const std::string& remote,
                              const std::string& expected)
{
  std::string result = kwsys::SystemTools::RelativePath(local, remote);
  if (!kwsys::SystemTools::ComparePath(expected, result)) {
    std::cerr << "RelativePath(" << local << ", " << remote << ")  yielded "
              << result << " instead of " << expected << std::endl;
    return false;
  }
  return true;
}

static bool CheckRelativePaths()
{
  bool res = true;
  res &= CheckRelativePath("/usr/share", "/bin/bash", "../../bin/bash");
  res &= CheckRelativePath("/usr/./share/", "/bin/bash", "../../bin/bash");
  res &= CheckRelativePath("/usr//share/", "/bin/bash", "../../bin/bash");
  res &=
    CheckRelativePath("/usr/share/../bin/", "/bin/bash", "../../bin/bash");
  res &= CheckRelativePath("/usr/share", "/usr/share//bin", "bin");
  return res;
}

static bool CheckCollapsePath(const std::string& path,
                              const std::string& expected)
{
  std::string result = kwsys::SystemTools::CollapseFullPath(path);
  if (!kwsys::SystemTools::ComparePath(expected, result)) {
    std::cerr << "CollapseFullPath(" << path << ")  yielded " << result
              << " instead of " << expected << std::endl;
    return false;
  }
  return true;
}

static bool CheckCollapsePath()
{
  bool res = true;
  res &= CheckCollapsePath("/usr/share/*", "/usr/share/*");
  res &= CheckCollapsePath("C:/Windows/*", "C:/Windows/*");
  return res;
}

static std::string StringVectorToString(const std::vector<std::string>& vec)
{
  std::stringstream ss;
  ss << "vector(";
  for (std::vector<std::string>::const_iterator i = vec.begin();
       i != vec.end(); ++i) {
    if (i != vec.begin()) {
      ss << ", ";
    }
    ss << *i;
  }
  ss << ")";
  return ss.str();
}

static bool CheckGetPath()
{
  const char* envName = "S";
#ifdef _WIN32
  const char* envValue = "C:\\Somewhere\\something;D:\\Temp";
#else
  const char* envValue = "/Somewhere/something:/tmp";
#endif
  const char* registryPath = "[HKEY_LOCAL_MACHINE\\SOFTWARE\\MyApp; MyKey]";

  std::vector<std::string> originalPathes;
  originalPathes.push_back(registryPath);

  std::vector<std::string> expectedPathes;
  expectedPathes.push_back(registryPath);
#ifdef _WIN32
  expectedPathes.push_back("C:/Somewhere/something");
  expectedPathes.push_back("D:/Temp");
#else
  expectedPathes.push_back("/Somewhere/something");
  expectedPathes.push_back("/tmp");
#endif

  bool res = true;
  res &= CheckPutEnv(std::string(envName) + "=" + envValue, envName, envValue);

  std::vector<std::string> pathes = originalPathes;
  kwsys::SystemTools::GetPath(pathes, envName);

  if (pathes != expectedPathes) {
    std::cerr << "GetPath(" << StringVectorToString(originalPathes) << ", "
              << envName << ")  yielded " << StringVectorToString(pathes)
              << " instead of " << StringVectorToString(expectedPathes)
              << std::endl;
    res = false;
  }

  res &= CheckUnPutEnv(envName, envName);
  return res;
}

static bool CheckFind()
{
  bool res = true;
  const std::string testFindFileName("testFindFile.txt");
  const std::string testFindFile(TEST_SYSTEMTOOLS_BINARY_DIR "/" +
                                 testFindFileName);

  if (!kwsys::SystemTools::Touch(testFindFile.c_str(), true)) {
    std::cerr << "Problem with Touch for: " << testFindFile << std::endl;
    // abort here as the existence of the file only makes the test meaningful
    return false;
  }

  std::vector<std::string> searchPaths;
  searchPaths.push_back(TEST_SYSTEMTOOLS_BINARY_DIR);
  if (kwsys::SystemTools::FindFile(testFindFileName, searchPaths, true)
        .empty()) {
    std::cerr << "Problem with FindFile without system paths for: "
              << testFindFileName << std::endl;
    res = false;
  }
  if (kwsys::SystemTools::FindFile(testFindFileName, searchPaths, false)
        .empty()) {
    std::cerr << "Problem with FindFile with system paths for: "
              << testFindFileName << std::endl;
    res = false;
  }

  return res;
}

static bool CheckGetLineFromStream()
{
  const std::string fileWithFiveCharsOnFirstLine(TEST_SYSTEMTOOLS_SOURCE_DIR
                                                 "/README.rst");

  kwsys::ifstream file(fileWithFiveCharsOnFirstLine.c_str(), std::ios::in);

  if (!file) {
    std::cerr << "Problem opening: " << fileWithFiveCharsOnFirstLine
              << std::endl;
    return false;
  }

  std::string line;
  bool has_newline = false;
  bool result;

  file.seekg(0, std::ios::beg);
  result = kwsys::SystemTools::GetLineFromStream(file, line, &has_newline, -1);
  if (!result || line.size() != 5) {
    std::cerr << "First line does not have five characters: " << line.size()
              << std::endl;
    return false;
  }

  file.seekg(0, std::ios::beg);
  result = kwsys::SystemTools::GetLineFromStream(file, line, &has_newline, -1);
  if (!result || line.size() != 5) {
    std::cerr << "First line does not have five characters after rewind: "
              << line.size() << std::endl;
    return false;
  }

  bool ret = true;

  for (size_t size = 1; size <= 5; ++size) {
    file.seekg(0, std::ios::beg);
    result = kwsys::SystemTools::GetLineFromStream(file, line, &has_newline,
                                                   static_cast<long>(size));
    if (!result || line.size() != size) {
      std::cerr << "Should have read " << size << " characters but got "
                << line.size() << std::endl;
      ret = false;
    }
  }

  return ret;
}

int testSystemTools(int, char* [])
{
  bool res = true;

  int cc;
  for (cc = 0; toUnixPaths[cc][0]; cc++) {
    res &= CheckConvertToUnixSlashes(toUnixPaths[cc][0], toUnixPaths[cc][1]);
  }

  // Special check for ~
  std::string output;
  if (kwsys::SystemTools::GetEnv("HOME", output)) {
    output += "/foo bar/lala";
    res &= CheckConvertToUnixSlashes("~/foo bar/lala", output);
  }

  for (cc = 0; checkEscapeChars[cc][0]; cc++) {
    res &= CheckEscapeChars(checkEscapeChars[cc][0], checkEscapeChars[cc][1],
                            *checkEscapeChars[cc][2], checkEscapeChars[cc][3]);
  }

  res &= CheckFileOperations();

  res &= CheckStringOperations();

  res &= CheckEnvironmentOperations();

  res &= CheckRelativePaths();

  res &= CheckCollapsePath();

  res &= CheckGetPath();

  res &= CheckFind();

  res &= CheckGetLineFromStream();

  return res ? 0 : 1;
}
