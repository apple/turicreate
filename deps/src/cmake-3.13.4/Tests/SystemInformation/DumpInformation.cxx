#include "DumpInformation.h"
#include <stdio.h>
#include <sys/stat.h>

void cmDumpInformationPrintFile(const char* name, FILE* fout)
{
  fprintf(fout, "Avoid ctest truncation of output: CTEST_FULL_OUTPUT\n");
  fprintf(
    fout,
    "================================================================\n");
  struct stat fs;
  if (stat(name, &fs) != 0) {
    fprintf(fout, "The file \"%s\" does not exist.\n", name);
    fflush(fout);
    return;
  }

  FILE* fin = fopen(name, "r");
  if (fin) {
    fprintf(
      fout,
      "Contents of \"%s\":\n"
      "----------------------------------------------------------------\n",
      name);
    const int bufferSize = 4096;
    char buffer[bufferSize];
    int n;
    while ((n = fread(buffer, 1, bufferSize, fin)) > 0) {
      for (char* c = buffer; c < buffer + n; ++c) {
        switch (*c) {
          case '<':
            fprintf(fout, "&lt;");
            break;
          case '>':
            fprintf(fout, "&gt;");
            break;
          case '&':
            fprintf(fout, "&amp;");
            break;
          default:
            putc(*c, fout);
            break;
        }
      }
      fflush(fout);
    }
    fclose(fin);
  } else {
    fprintf(fout, "Error opening \"%s\" for reading.\n", name);
    fflush(fout);
  }
}

int main(int, char* [])
{
  const char* files[] = {
    DumpInformation_BINARY_DIR "/SystemInformation.out",
    DumpInformation_BINARY_DIR "/AllVariables.txt",
    DumpInformation_BINARY_DIR "/AllCommands.txt",
    DumpInformation_BINARY_DIR "/AllMacros.txt",
    DumpInformation_BINARY_DIR "/OtherProperties.txt",
    DumpInformation_BINARY_DIR "/../../Source/cmConfigure.h",
    DumpInformation_BINARY_DIR "/../../CMakeCache.txt",
    DumpInformation_BINARY_DIR "/../../CMakeFiles/CMakeOutput.log",
    DumpInformation_BINARY_DIR "/../../CMakeFiles/CMakeError.log",
    DumpInformation_BINARY_DIR "/../../Bootstrap.cmk/cmake_bootstrap.log",
    DumpInformation_BINARY_DIR "/../../Source/cmsys/Configure.hxx",
    DumpInformation_BINARY_DIR "/../../Source/cmsys/Configure.h",
    DumpInformation_BINARY_DIR "/CMakeFiles/CMakeOutput.log",
    DumpInformation_BINARY_DIR "/CMakeFiles/CMakeError.log",
    0
  };

  const char** f;
  for (f = files; *f; ++f) {
    cmDumpInformationPrintFile(*f, stdout);
  }

  return 0;
}
