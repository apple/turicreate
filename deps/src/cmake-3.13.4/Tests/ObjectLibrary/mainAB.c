
#include "b.h"

extern IMPORT_B int b1(void);
extern IMPORT_B int b2(void);
#ifndef NO_A
extern int a1(void);
extern int a2(void);
#endif
int main(void)
{
  return 0
#ifndef NO_A
    + a1() + a2()
#endif
    + b1() + b2();
}
