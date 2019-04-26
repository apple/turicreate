
#include <fstream>

#include "object.h"

int main(int argc, char** argv)
{
  std::ofstream fout("commandoutput.h");
  if (!fout)
    return 1;
  fout << "#define COMMANDOUTPUT_DEFINE\n";
  fout.close();
  return object();
}
