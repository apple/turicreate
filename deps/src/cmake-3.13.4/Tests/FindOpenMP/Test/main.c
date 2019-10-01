#include <omp.h>
int main()
{
#ifndef _OPENMP
  breaks_on_purpose
#endif
}
