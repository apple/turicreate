#ifdef _WIN32
#  include <windows.h>
#endif
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  include <GL/gl.h>
#endif

#include <stdio.h>

int main()
{
  /* Reference a GL symbol without requiring a context at runtime.  */
  printf("&glGetString = %p\n", &glGetString);
  return 0;
}
