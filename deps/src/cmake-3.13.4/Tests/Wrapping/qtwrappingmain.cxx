#include "qtwrapping.h"
#include <qapplication.h>

#ifndef _WIN32
#  include <stdio.h>
#  include <stdlib.h>
#endif

int main(int argc, char* argv[])
{
#ifndef _WIN32
  const char* display = getenv("DISPLAY");
  if (display && strlen(display) > 0) {
#endif
    QApplication app(argc, argv);

    qtwrapping qtw;
    app.setMainWidget(&qtw);
#ifndef _WIN32
  } else {
    printf("Environment variable DISPLAY is not set. I will pretend like the "
           "test passed, but you should really set it.\n");
  }
#endif

  return 0;
}
