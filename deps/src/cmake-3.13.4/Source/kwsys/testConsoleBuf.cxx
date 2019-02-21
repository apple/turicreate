/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"

// Ignore Windows version levels defined by command-line flags.  This
// source needs access to all APIs available on the host in order for
// the test to run properly.  The test binary is not installed anyway.
#undef _WIN32_WINNT
#undef NTDDI_VERSION

#include KWSYS_HEADER(Encoding.hxx)

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#  include "Encoding.hxx.in"
#endif

#if defined(_WIN32)

#  include <algorithm>
#  include <iomanip>
#  include <iostream>
#  include <stdexcept>
#  include <string.h>
#  include <wchar.h>
#  include <windows.h>

#  include "testConsoleBuf.hxx"

#  if defined(_MSC_VER) && _MSC_VER >= 1800
#    define KWSYS_WINDOWS_DEPRECATED_GetVersion
#  endif
// يونيكود
static const WCHAR UnicodeInputTestString[] =
  L"\u064A\u0648\u0646\u064A\u0643\u0648\u062F!";
static UINT TestCodepage = KWSYS_ENCODING_DEFAULT_CODEPAGE;

static const DWORD waitTimeout = 10 * 1000;
static STARTUPINFO startupInfo;
static PROCESS_INFORMATION processInfo;
static HANDLE beforeInputEvent;
static HANDLE afterOutputEvent;
static std::string encodedInputTestString;
static std::string encodedTestString;

static void displayError(DWORD errorCode)
{
  std::cerr.setf(std::ios::hex, std::ios::basefield);
  std::cerr << "Failed with error: 0x" << errorCode << "!" << std::endl;
  LPWSTR message;
  if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM,
                     NULL, errorCode, 0, (LPWSTR)&message, 0, NULL)) {
    std::cerr << "Error message: " << kwsys::Encoding::ToNarrow(message)
              << std::endl;
    HeapFree(GetProcessHeap(), 0, message);
  } else {
    std::cerr << "FormatMessage() failed with error: 0x" << GetLastError()
              << "!" << std::endl;
  }
  std::cerr.unsetf(std::ios::hex);
}

std::basic_streambuf<char>* errstream(const char* unused)
{
  static_cast<void>(unused);
  return std::cerr.rdbuf();
}

std::basic_streambuf<wchar_t>* errstream(const wchar_t* unused)
{
  static_cast<void>(unused);
  return std::wcerr.rdbuf();
}

template <typename T>
static void dumpBuffers(const T* expected, const T* received, size_t size)
{
  std::basic_ostream<T> err(errstream(expected));
  err << "Expected output: '" << std::basic_string<T>(expected, size) << "'"
      << std::endl;
  if (err.fail()) {
    err.clear();
    err << "--- Error while outputting ---" << std::endl;
  }
  err << "Received output: '" << std::basic_string<T>(received, size) << "'"
      << std::endl;
  if (err.fail()) {
    err.clear();
    err << "--- Error while outputting ---" << std::endl;
  }
  std::cerr << "Expected output | Received output" << std::endl;
  for (size_t i = 0; i < size; i++) {
    std::cerr << std::setbase(16) << std::setfill('0') << "     "
              << "0x" << std::setw(8) << static_cast<unsigned int>(expected[i])
              << " | "
              << "0x" << std::setw(8)
              << static_cast<unsigned int>(received[i]);
    if (static_cast<unsigned int>(expected[i]) !=
        static_cast<unsigned int>(received[i])) {
      std::cerr << "   MISMATCH!";
    }
    std::cerr << std::endl;
  }
  std::cerr << std::endl;
}

static bool createProcess(HANDLE hIn, HANDLE hOut, HANDLE hErr)
{
  BOOL bInheritHandles = FALSE;
  DWORD dwCreationFlags = 0;
  memset(&processInfo, 0, sizeof(processInfo));
  memset(&startupInfo, 0, sizeof(startupInfo));
  startupInfo.cb = sizeof(startupInfo);
  startupInfo.dwFlags = STARTF_USESHOWWINDOW;
  startupInfo.wShowWindow = SW_HIDE;
  if (hIn || hOut || hErr) {
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;
    startupInfo.hStdInput = hIn;
    startupInfo.hStdOutput = hOut;
    startupInfo.hStdError = hErr;
    bInheritHandles = TRUE;
  }

  WCHAR cmd[MAX_PATH];
  if (GetModuleFileNameW(NULL, cmd, MAX_PATH) == 0) {
    std::cerr << "GetModuleFileName failed!" << std::endl;
    return false;
  }
  WCHAR* p = cmd + wcslen(cmd);
  while (p > cmd && *p != L'\\')
    p--;
  *(p + 1) = 0;
  wcscat(cmd, cmdConsoleBufChild);
  wcscat(cmd, L".exe");

  bool success =
    CreateProcessW(NULL,            // No module name (use command line)
                   cmd,             // Command line
                   NULL,            // Process handle not inheritable
                   NULL,            // Thread handle not inheritable
                   bInheritHandles, // Set handle inheritance
                   dwCreationFlags,
                   NULL,         // Use parent's environment block
                   NULL,         // Use parent's starting directory
                   &startupInfo, // Pointer to STARTUPINFO structure
                   &processInfo) !=
    0; // Pointer to PROCESS_INFORMATION structure
  if (!success) {
    DWORD lastError = GetLastError();
    std::cerr << "CreateProcess(" << kwsys::Encoding::ToNarrow(cmd) << ")"
              << std::endl;
    displayError(lastError);
  }
  return success;
}

static void finishProcess(bool success)
{
  if (success) {
    success =
      WaitForSingleObject(processInfo.hProcess, waitTimeout) == WAIT_OBJECT_0;
  };
  if (!success) {
    TerminateProcess(processInfo.hProcess, 1);
  }
  CloseHandle(processInfo.hProcess);
  CloseHandle(processInfo.hThread);
}

static bool createPipe(PHANDLE readPipe, PHANDLE writePipe)
{
  SECURITY_ATTRIBUTES securityAttributes;
  securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  securityAttributes.bInheritHandle = TRUE;
  securityAttributes.lpSecurityDescriptor = NULL;
  return CreatePipe(readPipe, writePipe, &securityAttributes, 0) == 0 ? false
                                                                      : true;
}

static void finishPipe(HANDLE readPipe, HANDLE writePipe)
{
  if (readPipe != INVALID_HANDLE_VALUE) {
    CloseHandle(readPipe);
  }
  if (writePipe != INVALID_HANDLE_VALUE) {
    CloseHandle(writePipe);
  }
}

static HANDLE createFile(LPCWSTR fileName)
{
  SECURITY_ATTRIBUTES securityAttributes;
  securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  securityAttributes.bInheritHandle = TRUE;
  securityAttributes.lpSecurityDescriptor = NULL;

  HANDLE file =
    CreateFileW(fileName, GENERIC_READ | GENERIC_WRITE,
                0, // do not share
                &securityAttributes,
                CREATE_ALWAYS, // overwrite existing
                FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
                NULL); // no template
  if (file == INVALID_HANDLE_VALUE) {
    DWORD lastError = GetLastError();
    std::cerr << "CreateFile(" << kwsys::Encoding::ToNarrow(fileName) << ")"
              << std::endl;
    displayError(lastError);
  }
  return file;
}

static void finishFile(HANDLE file)
{
  if (file != INVALID_HANDLE_VALUE) {
    CloseHandle(file);
  }
}

#  ifndef MAPVK_VK_TO_VSC
#    define MAPVK_VK_TO_VSC (0)
#  endif

static void writeInputKeyEvent(INPUT_RECORD inputBuffer[], WCHAR chr)
{
  inputBuffer[0].EventType = KEY_EVENT;
  inputBuffer[0].Event.KeyEvent.bKeyDown = TRUE;
  inputBuffer[0].Event.KeyEvent.wRepeatCount = 1;
  SHORT keyCode = VkKeyScanW(chr);
  if (keyCode == -1) {
    // Character can't be entered with current keyboard layout
    // Just set any, it doesn't really matter
    keyCode = 'K';
  }
  inputBuffer[0].Event.KeyEvent.wVirtualKeyCode = LOBYTE(keyCode);
  inputBuffer[0].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(
    inputBuffer[0].Event.KeyEvent.wVirtualKeyCode, MAPVK_VK_TO_VSC);
  inputBuffer[0].Event.KeyEvent.uChar.UnicodeChar = chr;
  inputBuffer[0].Event.KeyEvent.dwControlKeyState = 0;
  if ((HIBYTE(keyCode) & 1) == 1) {
    inputBuffer[0].Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
  }
  if ((HIBYTE(keyCode) & 2) == 2) {
    inputBuffer[0].Event.KeyEvent.dwControlKeyState |= RIGHT_CTRL_PRESSED;
  }
  if ((HIBYTE(keyCode) & 4) == 4) {
    inputBuffer[0].Event.KeyEvent.dwControlKeyState |= RIGHT_ALT_PRESSED;
  }
  inputBuffer[1].EventType = inputBuffer[0].EventType;
  inputBuffer[1].Event.KeyEvent.bKeyDown = FALSE;
  inputBuffer[1].Event.KeyEvent.wRepeatCount = 1;
  inputBuffer[1].Event.KeyEvent.wVirtualKeyCode =
    inputBuffer[0].Event.KeyEvent.wVirtualKeyCode;
  inputBuffer[1].Event.KeyEvent.wVirtualScanCode =
    inputBuffer[0].Event.KeyEvent.wVirtualScanCode;
  inputBuffer[1].Event.KeyEvent.uChar.UnicodeChar =
    inputBuffer[0].Event.KeyEvent.uChar.UnicodeChar;
  inputBuffer[1].Event.KeyEvent.dwControlKeyState = 0;
}

static int testPipe()
{
  int didFail = 1;
  HANDLE inPipeRead = INVALID_HANDLE_VALUE;
  HANDLE inPipeWrite = INVALID_HANDLE_VALUE;
  HANDLE outPipeRead = INVALID_HANDLE_VALUE;
  HANDLE outPipeWrite = INVALID_HANDLE_VALUE;
  HANDLE errPipeRead = INVALID_HANDLE_VALUE;
  HANDLE errPipeWrite = INVALID_HANDLE_VALUE;
  UINT currentCodepage = GetConsoleCP();
  char buffer[200];
  char buffer2[200];
  try {
    if (!createPipe(&inPipeRead, &inPipeWrite) ||
        !createPipe(&outPipeRead, &outPipeWrite) ||
        !createPipe(&errPipeRead, &errPipeWrite)) {
      throw std::runtime_error("createFile failed!");
    }
    if (TestCodepage == CP_ACP) {
      TestCodepage = GetACP();
    }
    if (!SetConsoleCP(TestCodepage)) {
      throw std::runtime_error("SetConsoleCP failed!");
    }

    DWORD bytesWritten = 0;
    if (!WriteFile(inPipeWrite, encodedInputTestString.c_str(),
                   (DWORD)encodedInputTestString.size(), &bytesWritten,
                   NULL) ||
        bytesWritten == 0) {
      throw std::runtime_error("WriteFile failed!");
    }

    if (createProcess(inPipeRead, outPipeWrite, errPipeWrite)) {
      try {
        DWORD status;
        if ((status = WaitForSingleObject(afterOutputEvent, waitTimeout)) !=
            WAIT_OBJECT_0) {
          std::cerr.setf(std::ios::hex, std::ios::basefield);
          std::cerr << "WaitForSingleObject returned unexpected status 0x"
                    << status << std::endl;
          std::cerr.unsetf(std::ios::hex);
          throw std::runtime_error("WaitForSingleObject failed!");
        }
        DWORD bytesRead = 0;
        if (!ReadFile(outPipeRead, buffer, sizeof(buffer), &bytesRead, NULL) ||
            bytesRead == 0) {
          throw std::runtime_error("ReadFile#1 failed!");
        }
        buffer[bytesRead] = 0;
        if ((bytesRead <
               encodedTestString.size() + 1 + encodedInputTestString.size() &&
             !ReadFile(outPipeRead, buffer + bytesRead,
                       sizeof(buffer) - bytesRead, &bytesRead, NULL)) ||
            bytesRead == 0) {
          throw std::runtime_error("ReadFile#2 failed!");
        }
        if (memcmp(buffer, encodedTestString.c_str(),
                   encodedTestString.size()) == 0 &&
            memcmp(buffer + encodedTestString.size() + 1,
                   encodedInputTestString.c_str(),
                   encodedInputTestString.size()) == 0) {
          bytesRead = 0;
          if (!ReadFile(errPipeRead, buffer2, sizeof(buffer2), &bytesRead,
                        NULL) ||
              bytesRead == 0) {
            throw std::runtime_error("ReadFile#3 failed!");
          }
          buffer2[bytesRead] = 0;
          didFail = encodedTestString.compare(0, std::string::npos, buffer2,
                                              encodedTestString.size()) == 0
            ? 0
            : 1;
        }
        if (didFail != 0) {
          std::cerr << "Pipe's output didn't match expected output!"
                    << std::endl;
          dumpBuffers<char>(encodedTestString.c_str(), buffer,
                            encodedTestString.size());
          dumpBuffers<char>(encodedInputTestString.c_str(),
                            buffer + encodedTestString.size() + 1,
                            encodedInputTestString.size());
          dumpBuffers<char>(encodedTestString.c_str(), buffer2,
                            encodedTestString.size());
        }
      } catch (const std::runtime_error& ex) {
        DWORD lastError = GetLastError();
        std::cerr << "In function testPipe, line " << __LINE__ << ": "
                  << ex.what() << std::endl;
        displayError(lastError);
      }
      finishProcess(didFail == 0);
    }
  } catch (const std::runtime_error& ex) {
    DWORD lastError = GetLastError();
    std::cerr << "In function testPipe, line " << __LINE__ << ": " << ex.what()
              << std::endl;
    displayError(lastError);
  }
  finishPipe(inPipeRead, inPipeWrite);
  finishPipe(outPipeRead, outPipeWrite);
  finishPipe(errPipeRead, errPipeWrite);
  SetConsoleCP(currentCodepage);
  return didFail;
}

static int testFile()
{
  int didFail = 1;
  HANDLE inFile = INVALID_HANDLE_VALUE;
  HANDLE outFile = INVALID_HANDLE_VALUE;
  HANDLE errFile = INVALID_HANDLE_VALUE;
  try {
    if ((inFile = createFile(L"stdinFile.txt")) == INVALID_HANDLE_VALUE ||
        (outFile = createFile(L"stdoutFile.txt")) == INVALID_HANDLE_VALUE ||
        (errFile = createFile(L"stderrFile.txt")) == INVALID_HANDLE_VALUE) {
      throw std::runtime_error("createFile failed!");
    }
    DWORD bytesWritten = 0;
    char buffer[200];
    char buffer2[200];

    int length;
    if ((length =
           WideCharToMultiByte(TestCodepage, 0, UnicodeInputTestString, -1,
                               buffer, sizeof(buffer), NULL, NULL)) == 0) {
      throw std::runtime_error("WideCharToMultiByte failed!");
    }
    buffer[length - 1] = '\n';
    if (!WriteFile(inFile, buffer, length, &bytesWritten, NULL) ||
        bytesWritten == 0) {
      throw std::runtime_error("WriteFile failed!");
    }
    if (SetFilePointer(inFile, 0, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
      throw std::runtime_error("SetFilePointer failed!");
    }

    if (createProcess(inFile, outFile, errFile)) {
      DWORD bytesRead = 0;
      try {
        DWORD status;
        if ((status = WaitForSingleObject(afterOutputEvent, waitTimeout)) !=
            WAIT_OBJECT_0) {
          std::cerr.setf(std::ios::hex, std::ios::basefield);
          std::cerr << "WaitForSingleObject returned unexpected status 0x"
                    << status << std::endl;
          std::cerr.unsetf(std::ios::hex);
          throw std::runtime_error("WaitForSingleObject failed!");
        }
        if (SetFilePointer(outFile, 0, 0, FILE_BEGIN) ==
            INVALID_SET_FILE_POINTER) {
          throw std::runtime_error("SetFilePointer#1 failed!");
        }
        if (!ReadFile(outFile, buffer, sizeof(buffer), &bytesRead, NULL) ||
            bytesRead == 0) {
          throw std::runtime_error("ReadFile#1 failed!");
        }
        buffer[bytesRead] = 0;
        if (memcmp(buffer, encodedTestString.c_str(),
                   encodedTestString.size()) == 0 &&
            memcmp(buffer + encodedTestString.size() + 1,
                   encodedInputTestString.c_str(),
                   encodedInputTestString.size()) == 0) {
          bytesRead = 0;
          if (SetFilePointer(errFile, 0, 0, FILE_BEGIN) ==
              INVALID_SET_FILE_POINTER) {
            throw std::runtime_error("SetFilePointer#2 failed!");
          }

          if (!ReadFile(errFile, buffer2, sizeof(buffer2), &bytesRead, NULL) ||
              bytesRead == 0) {
            throw std::runtime_error("ReadFile#2 failed!");
          }
          buffer2[bytesRead] = 0;
          didFail = encodedTestString.compare(0, std::string::npos, buffer2,
                                              encodedTestString.size()) == 0
            ? 0
            : 1;
        }
        if (didFail != 0) {
          std::cerr << "File's output didn't match expected output!"
                    << std::endl;
          dumpBuffers<char>(encodedTestString.c_str(), buffer,
                            encodedTestString.size());
          dumpBuffers<char>(encodedInputTestString.c_str(),
                            buffer + encodedTestString.size() + 1,
                            encodedInputTestString.size());
          dumpBuffers<char>(encodedTestString.c_str(), buffer2,
                            encodedTestString.size());
        }
      } catch (const std::runtime_error& ex) {
        DWORD lastError = GetLastError();
        std::cerr << "In function testFile, line " << __LINE__ << ": "
                  << ex.what() << std::endl;
        displayError(lastError);
      }
      finishProcess(didFail == 0);
    }
  } catch (const std::runtime_error& ex) {
    DWORD lastError = GetLastError();
    std::cerr << "In function testFile, line " << __LINE__ << ": " << ex.what()
              << std::endl;
    displayError(lastError);
  }
  finishFile(inFile);
  finishFile(outFile);
  finishFile(errFile);
  return didFail;
}

#  ifndef _WIN32_WINNT_VISTA
#    define _WIN32_WINNT_VISTA 0x0600
#  endif

static int testConsole()
{
  int didFail = 1;
  HANDLE parentIn = GetStdHandle(STD_INPUT_HANDLE);
  HANDLE parentOut = GetStdHandle(STD_OUTPUT_HANDLE);
  HANDLE parentErr = GetStdHandle(STD_ERROR_HANDLE);
  HANDLE hIn = parentIn;
  HANDLE hOut = parentOut;
  DWORD consoleMode;
  bool newConsole = false;
  bool forceNewConsole = false;
  bool restoreConsole = false;
  LPCWSTR TestFaceName = L"Lucida Console";
  const DWORD TestFontFamily = 0x00000036;
  const DWORD TestFontSize = 0x000c0000;
  HKEY hConsoleKey;
  WCHAR FaceName[200];
  FaceName[0] = 0;
  DWORD FaceNameSize = sizeof(FaceName);
  DWORD FontFamily = TestFontFamily;
  DWORD FontSize = TestFontSize;
#  ifdef KWSYS_WINDOWS_DEPRECATED_GetVersion
#    pragma warning(push)
#    ifdef __INTEL_COMPILER
#      pragma warning(disable : 1478)
#    else
#      pragma warning(disable : 4996)
#    endif
#  endif
  const bool isVistaOrGreater =
    LOBYTE(LOWORD(GetVersion())) >= HIBYTE(_WIN32_WINNT_VISTA);
#  ifdef KWSYS_WINDOWS_DEPRECATED_GetVersion
#    pragma warning(pop)
#  endif
  if (!isVistaOrGreater) {
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Console", 0, KEY_READ | KEY_WRITE,
                      &hConsoleKey) == ERROR_SUCCESS) {
      DWORD dwordSize = sizeof(DWORD);
      if (RegQueryValueExW(hConsoleKey, L"FontFamily", NULL, NULL,
                           (LPBYTE)&FontFamily, &dwordSize) == ERROR_SUCCESS) {
        if (FontFamily != TestFontFamily) {
          RegQueryValueExW(hConsoleKey, L"FaceName", NULL, NULL,
                           (LPBYTE)FaceName, &FaceNameSize);
          RegQueryValueExW(hConsoleKey, L"FontSize", NULL, NULL,
                           (LPBYTE)&FontSize, &dwordSize);

          RegSetValueExW(hConsoleKey, L"FontFamily", 0, REG_DWORD,
                         (BYTE*)&TestFontFamily, sizeof(TestFontFamily));
          RegSetValueExW(hConsoleKey, L"FaceName", 0, REG_SZ,
                         (BYTE*)TestFaceName,
                         (DWORD)((wcslen(TestFaceName) + 1) * sizeof(WCHAR)));
          RegSetValueExW(hConsoleKey, L"FontSize", 0, REG_DWORD,
                         (BYTE*)&TestFontSize, sizeof(TestFontSize));

          restoreConsole = true;
          forceNewConsole = true;
        }
      } else {
        std::cerr << "RegGetValueW(FontFamily) failed!" << std::endl;
      }
      RegCloseKey(hConsoleKey);
    } else {
      std::cerr << "RegOpenKeyExW(HKEY_CURRENT_USER\\Console) failed!"
                << std::endl;
    }
  }
  if (forceNewConsole || GetConsoleMode(parentOut, &consoleMode) == 0) {
    // Not a real console, let's create new one.
    FreeConsole();
    if (!AllocConsole()) {
      std::cerr << "AllocConsole failed!" << std::endl;
      return didFail;
    }
    SECURITY_ATTRIBUTES securityAttributes;
    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.bInheritHandle = TRUE;
    securityAttributes.lpSecurityDescriptor = NULL;
    hIn = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, &securityAttributes,
                      OPEN_EXISTING, 0, NULL);
    if (hIn == INVALID_HANDLE_VALUE) {
      DWORD lastError = GetLastError();
      std::cerr << "CreateFile(CONIN$)" << std::endl;
      displayError(lastError);
    }
    hOut = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, &securityAttributes,
                       OPEN_EXISTING, 0, NULL);
    if (hOut == INVALID_HANDLE_VALUE) {
      DWORD lastError = GetLastError();
      std::cerr << "CreateFile(CONOUT$)" << std::endl;
      displayError(lastError);
    }
    SetStdHandle(STD_INPUT_HANDLE, hIn);
    SetStdHandle(STD_OUTPUT_HANDLE, hOut);
    SetStdHandle(STD_ERROR_HANDLE, hOut);
    newConsole = true;
  }

#  if _WIN32_WINNT >= _WIN32_WINNT_VISTA
  if (isVistaOrGreater) {
    CONSOLE_FONT_INFOEX consoleFont;
    memset(&consoleFont, 0, sizeof(consoleFont));
    consoleFont.cbSize = sizeof(consoleFont);
    HMODULE kernel32 = LoadLibraryW(L"kernel32.dll");
    typedef BOOL(WINAPI * GetCurrentConsoleFontExFunc)(
      HANDLE hConsoleOutput, BOOL bMaximumWindow,
      PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);
    typedef BOOL(WINAPI * SetCurrentConsoleFontExFunc)(
      HANDLE hConsoleOutput, BOOL bMaximumWindow,
      PCONSOLE_FONT_INFOEX lpConsoleCurrentFontEx);
    GetCurrentConsoleFontExFunc getConsoleFont =
      (GetCurrentConsoleFontExFunc)GetProcAddress(kernel32,
                                                  "GetCurrentConsoleFontEx");
    SetCurrentConsoleFontExFunc setConsoleFont =
      (SetCurrentConsoleFontExFunc)GetProcAddress(kernel32,
                                                  "SetCurrentConsoleFontEx");
    if (getConsoleFont(hOut, FALSE, &consoleFont)) {
      if (consoleFont.FontFamily != TestFontFamily) {
        consoleFont.FontFamily = TestFontFamily;
        wcscpy(consoleFont.FaceName, TestFaceName);
        if (!setConsoleFont(hOut, FALSE, &consoleFont)) {
          std::cerr << "SetCurrentConsoleFontEx failed!" << std::endl;
        }
      }
    } else {
      std::cerr << "GetCurrentConsoleFontEx failed!" << std::endl;
    }
  } else {
#  endif
    if (restoreConsole &&
        RegOpenKeyExW(HKEY_CURRENT_USER, L"Console", 0, KEY_WRITE,
                      &hConsoleKey) == ERROR_SUCCESS) {
      RegSetValueExW(hConsoleKey, L"FontFamily", 0, REG_DWORD,
                     (BYTE*)&FontFamily, sizeof(FontFamily));
      if (FaceName[0] != 0) {
        RegSetValueExW(hConsoleKey, L"FaceName", 0, REG_SZ, (BYTE*)FaceName,
                       FaceNameSize);
      } else {
        RegDeleteValueW(hConsoleKey, L"FaceName");
      }
      RegSetValueExW(hConsoleKey, L"FontSize", 0, REG_DWORD, (BYTE*)&FontSize,
                     sizeof(FontSize));
      RegCloseKey(hConsoleKey);
    }
#  if _WIN32_WINNT >= _WIN32_WINNT_VISTA
  }
#  endif

  if (createProcess(NULL, NULL, NULL)) {
    try {
      DWORD status;
      if ((status = WaitForSingleObject(beforeInputEvent, waitTimeout)) !=
          WAIT_OBJECT_0) {
        std::cerr.setf(std::ios::hex, std::ios::basefield);
        std::cerr << "WaitForSingleObject returned unexpected status 0x"
                  << status << std::endl;
        std::cerr.unsetf(std::ios::hex);
        throw std::runtime_error("WaitForSingleObject#1 failed!");
      }
      INPUT_RECORD inputBuffer[(sizeof(UnicodeInputTestString) /
                                sizeof(UnicodeInputTestString[0])) *
                               2];
      memset(&inputBuffer, 0, sizeof(inputBuffer));
      unsigned int i;
      for (i = 0; i < (sizeof(UnicodeInputTestString) /
                         sizeof(UnicodeInputTestString[0]) -
                       1);
           i++) {
        writeInputKeyEvent(&inputBuffer[i * 2], UnicodeInputTestString[i]);
      }
      writeInputKeyEvent(&inputBuffer[i * 2], VK_RETURN);
      DWORD eventsWritten = 0;
      // We need to wait a bit before writing to console so child process have
      // started waiting for input on stdin.
      Sleep(300);
      if (!WriteConsoleInputW(hIn, inputBuffer,
                              sizeof(inputBuffer) / sizeof(inputBuffer[0]),
                              &eventsWritten) ||
          eventsWritten == 0) {
        throw std::runtime_error("WriteConsoleInput failed!");
      }
      if ((status = WaitForSingleObject(afterOutputEvent, waitTimeout)) !=
          WAIT_OBJECT_0) {
        std::cerr.setf(std::ios::hex, std::ios::basefield);
        std::cerr << "WaitForSingleObject returned unexpected status 0x"
                  << status << std::endl;
        std::cerr.unsetf(std::ios::hex);
        throw std::runtime_error("WaitForSingleObject#2 failed!");
      }
      CONSOLE_SCREEN_BUFFER_INFO screenBufferInfo;
      if (!GetConsoleScreenBufferInfo(hOut, &screenBufferInfo)) {
        throw std::runtime_error("GetConsoleScreenBufferInfo failed!");
      }

      COORD coord;
      DWORD charsRead = 0;
      coord.X = 0;
      coord.Y = screenBufferInfo.dwCursorPosition.Y - 4;
      WCHAR* outputBuffer = new WCHAR[screenBufferInfo.dwSize.X * 4];
      if (!ReadConsoleOutputCharacterW(hOut, outputBuffer,
                                       screenBufferInfo.dwSize.X * 4, coord,
                                       &charsRead) ||
          charsRead == 0) {
        delete[] outputBuffer;
        throw std::runtime_error("ReadConsoleOutputCharacter failed!");
      }
      std::wstring wideTestString = kwsys::Encoding::ToWide(encodedTestString);
      std::replace(wideTestString.begin(), wideTestString.end(), '\0', ' ');
      std::wstring wideInputTestString =
        kwsys::Encoding::ToWide(encodedInputTestString);
      if (memcmp(outputBuffer, wideTestString.c_str(),
                 wideTestString.size() * sizeof(wchar_t)) == 0 &&
          memcmp(outputBuffer + screenBufferInfo.dwSize.X * 1,
                 wideTestString.c_str(),
                 wideTestString.size() * sizeof(wchar_t)) == 0 &&
          memcmp(outputBuffer + screenBufferInfo.dwSize.X * 2,
                 UnicodeInputTestString,
                 sizeof(UnicodeInputTestString) - sizeof(WCHAR)) == 0 &&
          memcmp(outputBuffer + screenBufferInfo.dwSize.X * 3,
                 wideInputTestString.c_str(),
                 (wideInputTestString.size() - 1) * sizeof(wchar_t)) == 0) {
        didFail = 0;
      } else {
        std::cerr << "Console's output didn't match expected output!"
                  << std::endl;
        dumpBuffers<wchar_t>(wideTestString.c_str(), outputBuffer,
                             wideTestString.size());
        dumpBuffers<wchar_t>(wideTestString.c_str(),
                             outputBuffer + screenBufferInfo.dwSize.X * 1,
                             wideTestString.size());
        dumpBuffers<wchar_t>(
          UnicodeInputTestString, outputBuffer + screenBufferInfo.dwSize.X * 2,
          (sizeof(UnicodeInputTestString) - 1) / sizeof(WCHAR));
        dumpBuffers<wchar_t>(wideInputTestString.c_str(),
                             outputBuffer + screenBufferInfo.dwSize.X * 3,
                             wideInputTestString.size() - 1);
      }
      delete[] outputBuffer;
    } catch (const std::runtime_error& ex) {
      DWORD lastError = GetLastError();
      std::cerr << "In function testConsole, line " << __LINE__ << ": "
                << ex.what() << std::endl;
      displayError(lastError);
    }
    finishProcess(didFail == 0);
  }
  if (newConsole) {
    SetStdHandle(STD_INPUT_HANDLE, parentIn);
    SetStdHandle(STD_OUTPUT_HANDLE, parentOut);
    SetStdHandle(STD_ERROR_HANDLE, parentErr);
    CloseHandle(hIn);
    CloseHandle(hOut);
    FreeConsole();
  }
  return didFail;
}

#endif

int testConsoleBuf(int, char* [])
{
  int ret = 0;

#if defined(_WIN32)
  beforeInputEvent = CreateEventW(NULL,
                                  FALSE, // auto-reset event
                                  FALSE, // initial state is nonsignaled
                                  BeforeInputEventName); // object name
  if (!beforeInputEvent) {
    std::cerr << "CreateEvent#1 failed " << GetLastError() << std::endl;
    return 1;
  }

  afterOutputEvent = CreateEventW(NULL, FALSE, FALSE, AfterOutputEventName);
  if (!afterOutputEvent) {
    std::cerr << "CreateEvent#2 failed " << GetLastError() << std::endl;
    return 1;
  }

  encodedTestString = kwsys::Encoding::ToNarrow(std::wstring(
    UnicodeTestString, sizeof(UnicodeTestString) / sizeof(wchar_t) - 1));
  encodedInputTestString = kwsys::Encoding::ToNarrow(
    std::wstring(UnicodeInputTestString,
                 sizeof(UnicodeInputTestString) / sizeof(wchar_t) - 1));
  encodedInputTestString += "\n";

  ret |= testPipe();
  ret |= testFile();
  ret |= testConsole();

  CloseHandle(beforeInputEvent);
  CloseHandle(afterOutputEvent);
#endif

  return ret;
}
