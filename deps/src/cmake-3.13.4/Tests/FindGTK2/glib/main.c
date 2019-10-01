#include <glib.h>

int main(int argc, char* argv[])
{
  if (!g_file_test("file", G_FILE_TEST_EXISTS)) {
    g_print("File not found. \n");
  } else {
    g_print("File found. \n");
  }
  return 0;
}
