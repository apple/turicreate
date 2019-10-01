#include "pcShared.h"
extern const char* pcStatic(void);
int main()
{
  pcStatic();
  pcShared();
  return 0;
}
