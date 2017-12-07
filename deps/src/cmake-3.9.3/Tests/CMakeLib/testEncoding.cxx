#include "cmsys/FStream.hxx"
#include <iostream>
#include <string>

#ifdef _WIN32
#include "cmsys/ConsoleBuf.hxx"
#endif

#ifdef _WIN32
void setEncoding(cmsys::ConsoleBuf::Manager& buf, UINT codepage)
{
  cmsys::ConsoleBuf* cb = buf.GetConsoleBuf();
  if (cb) {
    cb->input_pipe_codepage = codepage;
    cb->output_pipe_codepage = codepage;
    cb->input_file_codepage = codepage;
    cb->output_file_codepage = codepage;
    cb->activateCodepageChange();
  }
}
#endif

int main(int argc, char* argv[])
{
#ifdef _WIN32
  cmsys::ConsoleBuf::Manager consoleOut(std::cout);
#endif
  if (argc <= 2) {
    std::cout << "Usage: testEncoding <encoding> <file>" << std::endl;
    return 1;
  }
  const std::string encoding(argv[1]);
#ifdef _WIN32
  if (encoding == "UTF8") {
    setEncoding(consoleOut, CP_UTF8);
  } else if (encoding == "ANSI") {
    setEncoding(consoleOut, CP_ACP);
  } else if (encoding == "OEM") {
    setEncoding(consoleOut, CP_OEMCP);
  } // else AUTO
#endif
  cmsys::ifstream file(argv[2]);
  if (!file.is_open()) {
    std::cout << "Failed to open file: " << argv[2] << std::endl;
    return 2;
  }
  std::string text((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());
  std::cout << text;
  return 0;
}
