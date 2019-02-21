#include <pango/pango.h>

int main(int argc, char* argv[])
{
  gboolean ret = pango_color_parse(NULL, "#ffffff");
  return 0;
}
