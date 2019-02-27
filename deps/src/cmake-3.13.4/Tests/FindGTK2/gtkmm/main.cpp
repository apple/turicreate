#include "helloworld.h"
#include <gtkmm.h>

int main(int argc, char* argv[])
{
  Gtk::Main kit(argc, argv);

  HelloWorld helloworld;
  // Shows the window and returns when it is closed.
  Gtk::Main::run(helloworld);

  return 0;
}
