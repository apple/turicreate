/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"

#include KWSYS_HEADER(ConsoleBuf.hxx)
#include KWSYS_HEADER(Encoding.hxx)

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#include "ConsoleBuf.hxx.in"
#include "Encoding.hxx.in"
#endif

#include <iostream>

#include "testConsoleBuf.hxx"

int main(int argc, const char* argv[])
{
#if defined(_WIN32)
  kwsys::ConsoleBuf::Manager out(std::cout);
  kwsys::ConsoleBuf::Manager err(std::cerr, true);
  kwsys::ConsoleBuf::Manager in(std::cin);

  if (argc > 1) {
    std::cout << argv[1] << std::endl;
    std::cerr << argv[1] << std::endl;
  } else {
    std::string str = kwsys::Encoding::ToNarrow(std::wstring(
      UnicodeTestString, sizeof(UnicodeTestString) / sizeof(wchar_t) - 1));
    std::cout << str << std::endl;
    std::cerr << str << std::endl;
  }

  std::string input;
  HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, BeforeInputEventName);
  if (event) {
    SetEvent(event);
    CloseHandle(event);
  }

  std::cin >> input;
  std::cout << input << std::endl;
  event = OpenEventW(EVENT_MODIFY_STATE, FALSE, AfterOutputEventName);
  if (event) {
    SetEvent(event);
    CloseHandle(event);
  }
#else
  static_cast<void>(argc);
  static_cast<void>(argv);
#endif
  return 0;
}
