#include "foo.h"
extern F_test_mod_sub(void);
extern F_mysub(void);
int myc(void)
{
  F_mysub();
  F_my_sub();
#ifdef TEST_MOD
  F_test_mod_sub();
#endif
  return 0;
}
