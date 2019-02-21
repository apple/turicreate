#include <gdk-pixbuf/gdk-pixbuf.h>

int main(int argc, char* argv[])
{
  const char* version = gdk_pixbuf_version;
  const guint major = gdk_pixbuf_major_version;
  const guint minor = gdk_pixbuf_minor_version;
  const guint micro = gdk_pixbuf_micro_version;
  return 0;
}
