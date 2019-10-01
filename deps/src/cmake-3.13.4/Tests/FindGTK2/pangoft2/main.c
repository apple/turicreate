#include <pango/pangoft2.h>

int main(int argc, char* argv[])
{
  PangoFontMap* pfm = pango_ft2_font_map_new();
  g_object_unref(pfm);
  return 0;
}
