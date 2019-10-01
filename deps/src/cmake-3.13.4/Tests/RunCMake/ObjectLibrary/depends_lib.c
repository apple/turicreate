#include <stdlib.h>
extern int myobj_foo(void);

void mylib_foo(void)
{
  exit(myobj_foo());
}
