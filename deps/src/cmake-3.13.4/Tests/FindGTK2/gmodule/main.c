#include <gmodule.h>

int main(int argc, char* argv[])
{
  gboolean b = g_module_supported();
  return 0;
}
