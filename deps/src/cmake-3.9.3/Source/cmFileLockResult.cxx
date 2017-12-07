/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFileLockResult.h"

#include <errno.h>
#include <string.h>

cmFileLockResult cmFileLockResult::MakeOk()
{
  return cmFileLockResult(OK, 0);
}

cmFileLockResult cmFileLockResult::MakeSystem()
{
#if defined(_WIN32)
  const Error lastError = GetLastError();
#else
  const Error lastError = errno;
#endif
  return cmFileLockResult(SYSTEM, lastError);
}

cmFileLockResult cmFileLockResult::MakeTimeout()
{
  return cmFileLockResult(TIMEOUT, 0);
}

cmFileLockResult cmFileLockResult::MakeAlreadyLocked()
{
  return cmFileLockResult(ALREADY_LOCKED, 0);
}

cmFileLockResult cmFileLockResult::MakeInternal()
{
  return cmFileLockResult(INTERNAL, 0);
}

cmFileLockResult cmFileLockResult::MakeNoFunction()
{
  return cmFileLockResult(NO_FUNCTION, 0);
}

bool cmFileLockResult::IsOk() const
{
  return this->Type == OK;
}

std::string cmFileLockResult::GetOutputMessage() const
{
  switch (this->Type) {
    case OK:
      return "0";
    case SYSTEM:
#if defined(_WIN32)
    {
      char* errorText = NULL;

      // http://stackoverflow.com/a/455533/2288008
      DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS;
      ::FormatMessageA(flags, NULL, this->ErrorValue,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPSTR)&errorText, 0, NULL);

      if (errorText != NULL) {
        const std::string message = errorText;
        ::LocalFree(errorText);
        return message;
      } else {
        return "Internal error (FormatMessageA failed)";
      }
    }
#else
      return strerror(this->ErrorValue);
#endif
    case TIMEOUT:
      return "Timeout reached";
    case ALREADY_LOCKED:
      return "File already locked";
    case NO_FUNCTION:
      return "'GUARD FUNCTION' not used in function definition";
    case INTERNAL:
    default:
      return "Internal error";
  }
}

cmFileLockResult::cmFileLockResult(ErrorType typeValue, Error errorValue)
  : Type(typeValue)
  , ErrorValue(errorValue)
{
}
