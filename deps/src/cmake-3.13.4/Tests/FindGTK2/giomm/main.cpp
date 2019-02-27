#include <giomm.h>

int main(int argc, char* argv[])
{
  Glib::RefPtr<Gio::File> f = Gio::File::create_for_path("path");
  return 0;
}
