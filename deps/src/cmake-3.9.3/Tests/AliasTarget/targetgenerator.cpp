
#include <fstream>

int main(int argc, char** argv)
{
  std::ofstream fout("targetoutput.h");
  if (!fout)
    return 1;
  fout << "#define TARGETOUTPUT_DEFINE\n";
  fout.close();
  return 0;
}
