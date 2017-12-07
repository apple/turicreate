/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmRST.h"
#include "cmSystemTools.h"

#include "cmsys/FStream.hxx"
#include <iostream>
#include <string>

void reportLine(std::ostream& os, bool ret, std::string const& line, bool eol)
{
  if (ret) {
    os << "\"" << line << "\" (" << (eol ? "with EOL" : "without EOL") << ")";
  } else {
    os << "EOF";
  }
}

int testRST(int argc, char* argv[])
{
  if (argc != 2) {
    std::cerr << "Usage: testRST <dir>" << std::endl;
    return 1;
  }
  std::string dir = argv[1];
  if (dir.empty()) {
    dir = ".";
  }
  std::string a_name = "testRST.actual";
  std::string e_name = dir + "/testRST.expect";

  // Process the test RST file.
  {
    std::string fname = dir + "/testRST.rst";
    std::ofstream fout(a_name.c_str());
    if (!fout) {
      std::cerr << "Could not open output " << a_name << std::endl;
      return 1;
    }

    cmRST r(fout, dir);
    if (!r.ProcessFile(fname)) {
      std::cerr << "Could not open input " << fname << std::endl;
      return 1;
    }
  }

  // Compare expected and actual outputs.
  cmsys::ifstream e_fin(e_name.c_str());
  cmsys::ifstream a_fin(a_name.c_str());
  if (!e_fin) {
    std::cerr << "Could not open input " << e_name << std::endl;
    return 1;
  }
  if (!a_fin) {
    std::cerr << "Could not open input " << a_name << std::endl;
    return 1;
  }
  int lineno = 0;
  bool e_ret;
  bool a_ret;
  do {
    std::string e_line;
    std::string a_line;
    bool e_eol;
    bool a_eol;
    e_ret = cmSystemTools::GetLineFromStream(e_fin, e_line, &e_eol);
    a_ret = cmSystemTools::GetLineFromStream(a_fin, a_line, &a_eol);
    ++lineno;
    if (e_ret != a_ret || e_line != a_line || e_eol != a_eol) {
      a_fin.seekg(0, std::ios::beg);
      std::cerr << "Actual output does not match that expected on line "
                << lineno << "." << std::endl
                << "Expected ";
      reportLine(std::cerr, e_ret, e_line, e_eol);
      std::cerr << " but got ";
      reportLine(std::cerr, a_ret, a_line, a_eol);
      std::cerr << "." << std::endl
                << "Actual output:" << std::endl
                << a_fin.rdbuf();
      return 1;
    }
  } while (e_ret && a_ret);
  return 0;
}
