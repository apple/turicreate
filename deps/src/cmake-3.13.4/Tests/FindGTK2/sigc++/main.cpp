// Taken from https://developer.gnome.org/libsigc++-tutorial/stable/ch02.html

#include <iostream>
#include <sigc++/sigc++.h>

class AlienDetector
{
public:
  AlienDetector() {}

  void run() {}

  sigc::signal<void> signal_detected;
};

void warn_people()
{
  std::cout << "There are aliens in the carpark!" << std::endl;
}

int main()
{
  AlienDetector mydetector;
  mydetector.signal_detected.connect(sigc::ptr_fun(warn_people));

  mydetector.run();

  return 0;
}
