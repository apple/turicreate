#include <gio/gio.h>

int main(int argc, char* argv[])
{
  GFile* file = g_file_new_for_path("path");
  g_object_unref(file);
  return 0;
}
