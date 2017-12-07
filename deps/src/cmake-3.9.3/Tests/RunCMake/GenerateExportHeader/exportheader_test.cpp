
#include "libshared.h"

#include "libstatic.h"

#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>

void compare(const char* refName, const char* testName)
{
  std::ifstream ref;
  ref.open(refName);
  if (!ref.is_open()) {
    std::cout << "Could not open \"" << refName << "\"." << std::endl;
    exit(1);
  }
  std::ifstream test;
  test.open(testName);
  if (!test.is_open()) {
    std::cout << "Could not open \"" << testName << "\"." << std::endl;
    exit(1);
  }

  while (!ref.eof() && !test.eof()) {
    std::string refLine;
    std::string testLine;
    std::getline(ref, refLine);
    std::getline(test, testLine);
    // Some very old Borland runtimes (C++ Builder 5 WITHOUT Update 1) add a
    // trailing null to the string that we need to strip before testing for a
    // trailing space.
    if (refLine.size() && refLine[refLine.size() - 1] == 0) {
      refLine = refLine.substr(0, refLine.size() - 1);
    }
    if (testLine.size() && testLine[testLine.size() - 1] == 0) {
      testLine = testLine.substr(0, testLine.size() - 1);
    }
    // The reference files never have trailing spaces:
    if (testLine.size() && testLine[testLine.size() - 1] == ' ') {
      testLine = testLine.substr(0, testLine.size() - 1);
    }
    if (refLine != testLine) {
      std::cout << "Ref and test are not the same:\n  Ref:  \"" << refLine
                << "\"\n  Test: \"" << testLine << "\"\n";
      exit(1);
    }
  }
  if (!ref.eof() || !test.eof()) {
    std::cout << "Ref and test have differing numbers of lines.";
    exit(1);
  }
}

int main()
{
  {
    libshared::Class l;
    // l.method(); LINK ERROR
    l.method_exported();
    // l.method_deprecated(); LINK ERROR
    l.method_deprecated_exported();
    // l.method_excluded(); LINK ERROR

    // use_int(l.data); LINK ERROR
    use_int(l.data_exported);
    // use_int(l.data_excluded); LINK ERROR
  }

  {
    libshared::ExportedClass l;
    l.method();
    l.method_deprecated();
#if defined(_WIN32) || defined(__CYGWIN__)
    l.method_excluded();
#else
// l.method_excluded(); LINK ERROR (NOT WIN32 AND NOT CYGWIN)
#endif

    use_int(l.data);
#if defined(_WIN32) || defined(__CYGWIN__)
    use_int(l.data_excluded);
#else
// use_int(l.data_excluded); LINK ERROR (NOT WIN32 AND NOT CYGWIN)
#endif
  }

  {
    libshared::ExcludedClass l;
    // l.method(); LINK ERROR
    l.method_exported();
    // l.method_deprecated(); LINK ERROR
    l.method_deprecated_exported();
    // l.method_excluded(); LINK ERROR

    // use_int(l.data); LINK ERROR
    use_int(l.data_exported);
    // use_int(l.data_excluded); LINK ERROR
  }

  // libshared::function(); LINK ERROR
  libshared::function_exported();
  // libshared::function_deprecated(); LINK ERROR
  libshared::function_deprecated_exported();
  // libshared::function_excluded(); LINK ERROR

  // use_int(libshared::data); LINK ERROR
  use_int(libshared::data_exported);
  // use_int(libshared::data_excluded); LINK ERROR

  {
    libstatic::Class l;
    l.method();
    l.method_exported();
    l.method_deprecated();
    l.method_deprecated_exported();
    l.method_excluded();

    use_int(l.data);
    use_int(l.data_exported);
    use_int(l.data_excluded);
  }

  {
    libstatic::ExportedClass l;
    l.method();
    l.method_exported();
    l.method_deprecated();
    l.method_deprecated_exported();
    l.method_excluded();

    use_int(l.data);
    use_int(l.data_exported);
    use_int(l.data_excluded);
  }

  {
    libstatic::ExcludedClass l;
    l.method();
    l.method_exported();
    l.method_deprecated();
    l.method_deprecated_exported();
    l.method_excluded();

    use_int(l.data);
    use_int(l.data_exported);
    use_int(l.data_excluded);
  }

  libstatic::function();
  libstatic::function_exported();
  libstatic::function_deprecated();
  libstatic::function_deprecated_exported();
  libstatic::function_excluded();

  use_int(libstatic::data);
  use_int(libstatic::data_exported);
  use_int(libstatic::data_excluded);

#if defined(SRC_DIR) && defined(BIN_DIR)
  compare(SRC_DIR "/libshared_export.h",
          BIN_DIR "/libshared/libshared_export.h");
  compare(SRC_DIR "/libstatic_export.h",
          BIN_DIR "/libstatic/libstatic_export.h");
#endif

  return 0;
}
