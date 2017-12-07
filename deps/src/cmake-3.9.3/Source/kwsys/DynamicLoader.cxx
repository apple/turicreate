/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"
#include KWSYS_HEADER(DynamicLoader.hxx)

#include KWSYS_HEADER(Configure.hxx)

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#include "Configure.hxx.in"
#include "DynamicLoader.hxx.in"
#endif

// This file actually contains several different implementations:
// * NOOP for environments without dynamic libs
// * HP machines which uses shl_load
// * Mac OS X 10.2.x and earlier which uses NSLinkModule
// * Windows which uses LoadLibrary
// * BeOS / Haiku
// * FreeMiNT for Atari
// * Default implementation for *NIX systems (including Mac OS X 10.3 and
//   later) which use dlopen
//
// Each part of the ifdef contains a complete implementation for
// the static methods of DynamicLoader.

#if !KWSYS_SUPPORTS_SHARED_LIBS
// Implementation for environments without dynamic libs
#include <string.h> // for strerror()

namespace KWSYS_NAMESPACE {

DynamicLoader::LibraryHandle DynamicLoader::OpenLibrary(
  const std::string& libname)
{
  return 0;
}

int DynamicLoader::CloseLibrary(DynamicLoader::LibraryHandle lib)
{
  if (!lib) {
    return 0;
  }

  return 1;
}

DynamicLoader::SymbolPointer DynamicLoader::GetSymbolAddress(
  DynamicLoader::LibraryHandle lib, const std::string& sym)
{
  return 0;
}

const char* DynamicLoader::LastError()
{
  return "General error";
}

} // namespace KWSYS_NAMESPACE

#elif defined(__hpux)
// Implementation for HPUX  machines
#include <dl.h>
#include <errno.h>

namespace KWSYS_NAMESPACE {

DynamicLoader::LibraryHandle DynamicLoader::OpenLibrary(
  const std::string& libname)
{
  return shl_load(libname.c_str(), BIND_DEFERRED | DYNAMIC_PATH, 0L);
}

int DynamicLoader::CloseLibrary(DynamicLoader::LibraryHandle lib)
{
  if (!lib) {
    return 0;
  }
  return !shl_unload(lib);
}

DynamicLoader::SymbolPointer DynamicLoader::GetSymbolAddress(
  DynamicLoader::LibraryHandle lib, const std::string& sym)
{
  void* addr;
  int status;

  /* TYPE_PROCEDURE Look for a function or procedure. (This used to be default)
   * TYPE_DATA      Look for a symbol in the data segment (for example,
   * variables).
   * TYPE_UNDEFINED Look for any symbol.
   */
  status = shl_findsym(&lib, sym.c_str(), TYPE_UNDEFINED, &addr);
  void* result = (status < 0) ? (void*)0 : addr;

  // Hack to cast pointer-to-data to pointer-to-function.
  return *reinterpret_cast<DynamicLoader::SymbolPointer*>(&result);
}

const char* DynamicLoader::LastError()
{
  // TODO: Need implementation with errno/strerror
  /* If successful, shl_findsym returns an integer (int) value zero. If
   * shl_findsym cannot find sym, it returns -1 and sets errno to zero.
   * If any other errors occur, shl_findsym returns -1 and sets errno to one
   * of these values (defined in <errno.h>):
   * ENOEXEC
   * A format error was detected in the specified library.
   * ENOSYM
   * A symbol on which sym depends could not be found.
   * EINVAL
   * The specified handle is invalid.
   */

  if (errno == ENOEXEC || errno == ENOSYM || errno == EINVAL) {
    return strerror(errno);
  }
  // else
  return 0;
}

} // namespace KWSYS_NAMESPACE

#elif defined(__APPLE__) && (MAC_OS_X_VERSION_MAX_ALLOWED < 1030)
// Implementation for Mac OS X 10.2.x and earlier
#include <mach-o/dyld.h>
#include <string.h> // for strlen

namespace KWSYS_NAMESPACE {

DynamicLoader::LibraryHandle DynamicLoader::OpenLibrary(
  const std::string& libname)
{
  NSObjectFileImageReturnCode rc;
  NSObjectFileImage image = 0;

  rc = NSCreateObjectFileImageFromFile(libname.c_str(), &image);
  // rc == NSObjectFileImageInappropriateFile when trying to load a dylib file
  if (rc != NSObjectFileImageSuccess) {
    return 0;
  }
  NSModule handle =
    NSLinkModule(image, libname.c_str(), NSLINKMODULE_OPTION_BINDNOW |
                   NSLINKMODULE_OPTION_RETURN_ON_ERROR);
  NSDestroyObjectFileImage(image);
  return handle;
}

int DynamicLoader::CloseLibrary(DynamicLoader::LibraryHandle lib)
{
  // NSUNLINKMODULE_OPTION_KEEP_MEMORY_MAPPED
  // With  this  option  the memory for the module is not deallocated
  // allowing pointers into the module to still be valid.
  // You should use this option instead if your code experience some problems
  // reported against Panther 10.3.9 (fixed in Tiger 10.4.2 and up)
  bool success = NSUnLinkModule(lib, NSUNLINKMODULE_OPTION_NONE);
  return success;
}

DynamicLoader::SymbolPointer DynamicLoader::GetSymbolAddress(
  DynamicLoader::LibraryHandle lib, const std::string& sym)
{
  void* result = 0;
  // Need to prepend symbols with '_' on Apple-gcc compilers
  size_t len = sym.size();
  char* rsym = new char[len + 1 + 1];
  strcpy(rsym, "_");
  strcat(rsym + 1, sym.c_str());

  NSSymbol symbol = NSLookupSymbolInModule(lib, rsym);
  if (symbol) {
    result = NSAddressOfSymbol(symbol);
  }

  delete[] rsym;
  // Hack to cast pointer-to-data to pointer-to-function.
  return *reinterpret_cast<DynamicLoader::SymbolPointer*>(&result);
}

const char* DynamicLoader::LastError()
{
  return 0;
}

} // namespace KWSYS_NAMESPACE

#elif defined(_WIN32) && !defined(__CYGWIN__)
// Implementation for Windows win32 code but not cygwin
#include <windows.h>

namespace KWSYS_NAMESPACE {

DynamicLoader::LibraryHandle DynamicLoader::OpenLibrary(
  const std::string& libname)
{
  DynamicLoader::LibraryHandle lh;
  int length = MultiByteToWideChar(CP_UTF8, 0, libname.c_str(), -1, NULL, 0);
  wchar_t* wchars = new wchar_t[length + 1];
  wchars[0] = '\0';
  MultiByteToWideChar(CP_UTF8, 0, libname.c_str(), -1, wchars, length);
  lh = LoadLibraryW(wchars);
  delete[] wchars;
  return lh;
}

int DynamicLoader::CloseLibrary(DynamicLoader::LibraryHandle lib)
{
  return (int)FreeLibrary(lib);
}

DynamicLoader::SymbolPointer DynamicLoader::GetSymbolAddress(
  DynamicLoader::LibraryHandle lib, const std::string& sym)
{
  // TODO: The calling convention affects the name of the symbol.  We
  // should have a tool to help get the symbol with the desired
  // calling convention.  Currently we assume cdecl.
  //
  // Borland:
  //   __cdecl    = "_func" (default)
  //   __fastcall = "@_func"
  //   __stdcall  = "func"
  //
  // Watcom:
  //   __cdecl    = "_func"
  //   __fastcall = "@_func@X"
  //   __stdcall  = "_func@X"
  //   __watcall  = "func_" (default)
  //
  // MSVC:
  //   __cdecl    = "func" (default)
  //   __fastcall = "@_func@X"
  //   __stdcall  = "_func@X"
  //
  // Note that the "@X" part of the name above is the total size (in
  // bytes) of the arguments on the stack.
  void* result;
#if defined(__BORLANDC__) || defined(__WATCOMC__)
  // Need to prepend symbols with '_'
  size_t len = sym.size();
  char* rsym = new char[len + 1 + 1];
  strcpy(rsym, "_");
  strcat(rsym, sym.c_str());
#else
  const char* rsym = sym.c_str();
#endif
  result = (void*)GetProcAddress(lib, rsym);
#if defined(__BORLANDC__) || defined(__WATCOMC__)
  delete[] rsym;
#endif
// Hack to cast pointer-to-data to pointer-to-function.
#ifdef __WATCOMC__
  return *(DynamicLoader::SymbolPointer*)(&result);
#else
  return *reinterpret_cast<DynamicLoader::SymbolPointer*>(&result);
#endif
}

const char* DynamicLoader::LastError()
{
  LPVOID lpMsgBuf = NULL;

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR)&lpMsgBuf, 0, NULL);

  if (!lpMsgBuf) {
    return NULL;
  }

  static char* str = 0;
  delete[] str;
  str = strcpy(new char[strlen((char*)lpMsgBuf) + 1], (char*)lpMsgBuf);
  // Free the buffer.
  LocalFree(lpMsgBuf);
  return str;
}

} // namespace KWSYS_NAMESPACE

#elif defined(__BEOS__)
// Implementation for BeOS / Haiku
#include <string.h> // for strerror()

#include <be/kernel/image.h>
#include <be/support/Errors.h>

namespace KWSYS_NAMESPACE {

static image_id last_dynamic_err = B_OK;

DynamicLoader::LibraryHandle DynamicLoader::OpenLibrary(
  const std::string& libname)
{
  // image_id's are integers, errors are negative. Add one just in case we
  //  get a valid image_id of zero (is that even possible?).
  image_id rc = load_add_on(libname.c_str());
  if (rc < 0) {
    last_dynamic_err = rc;
    return 0;
  }

  return rc + 1;
}

int DynamicLoader::CloseLibrary(DynamicLoader::LibraryHandle lib)
{
  if (!lib) {
    last_dynamic_err = B_BAD_VALUE;
    return 0;
  } else {
    // The function dlclose() returns 0 on success, and non-zero on error.
    status_t rc = unload_add_on(lib - 1);
    if (rc != B_OK) {
      last_dynamic_err = rc;
      return 0;
    }
  }

  return 1;
}

DynamicLoader::SymbolPointer DynamicLoader::GetSymbolAddress(
  DynamicLoader::LibraryHandle lib, const std::string& sym)
{
  // Hack to cast pointer-to-data to pointer-to-function.
  union
  {
    void* pvoid;
    DynamicLoader::SymbolPointer psym;
  } result;

  result.psym = NULL;

  if (!lib) {
    last_dynamic_err = B_BAD_VALUE;
  } else {
    // !!! FIXME: BeOS can do function-only lookups...does this ever
    // !!! FIXME:  actually _want_ a data symbol lookup, or was this union
    // !!! FIXME:  a leftover of dlsym()? (s/ANY/TEXT for functions only).
    status_t rc =
      get_image_symbol(lib - 1, sym.c_str(), B_SYMBOL_TYPE_ANY, &result.pvoid);
    if (rc != B_OK) {
      last_dynamic_err = rc;
      result.psym = NULL;
    }
  }
  return result.psym;
}

const char* DynamicLoader::LastError()
{
  const char* retval = strerror(last_dynamic_err);
  last_dynamic_err = B_OK;
  return retval;
}

} // namespace KWSYS_NAMESPACE

#elif defined(__MINT__)
// Implementation for FreeMiNT on Atari
#define _GNU_SOURCE /* for program_invocation_name */
#include <dld.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>

namespace KWSYS_NAMESPACE {

DynamicLoader::LibraryHandle DynamicLoader::OpenLibrary(
  const std::string& libname)
{
  char* name = (char*)calloc(1, libname.size() + 1);
  dld_init(program_invocation_name);
  strncpy(name, libname.c_str(), libname.size());
  dld_link(libname.c_str());
  return (void*)name;
}

int DynamicLoader::CloseLibrary(DynamicLoader::LibraryHandle lib)
{
  dld_unlink_by_file((char*)lib, 0);
  free(lib);
  return 0;
}

DynamicLoader::SymbolPointer DynamicLoader::GetSymbolAddress(
  DynamicLoader::LibraryHandle lib, const std::string& sym)
{
  // Hack to cast pointer-to-data to pointer-to-function.
  union
  {
    void* pvoid;
    DynamicLoader::SymbolPointer psym;
  } result;
  result.pvoid = dld_get_symbol(sym.c_str());
  return result.psym;
}

const char* DynamicLoader::LastError()
{
  return dld_strerror(dld_errno);
}

} // namespace KWSYS_NAMESPACE

#else
// Default implementation for *NIX systems (including Mac OS X 10.3 and
// later) which use dlopen
#include <dlfcn.h>

namespace KWSYS_NAMESPACE {

DynamicLoader::LibraryHandle DynamicLoader::OpenLibrary(
  const std::string& libname)
{
  return dlopen(libname.c_str(), RTLD_LAZY);
}

int DynamicLoader::CloseLibrary(DynamicLoader::LibraryHandle lib)
{
  if (lib) {
    // The function dlclose() returns 0 on success, and non-zero on error.
    return !dlclose(lib);
  }
  // else
  return 0;
}

DynamicLoader::SymbolPointer DynamicLoader::GetSymbolAddress(
  DynamicLoader::LibraryHandle lib, const std::string& sym)
{
  // Hack to cast pointer-to-data to pointer-to-function.
  union
  {
    void* pvoid;
    DynamicLoader::SymbolPointer psym;
  } result;
  result.pvoid = dlsym(lib, sym.c_str());
  return result.psym;
}

const char* DynamicLoader::LastError()
{
  return dlerror();
}

} // namespace KWSYS_NAMESPACE
#endif
