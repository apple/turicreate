// This header should not be compiled directly but through inclusion
// in A.cxx through A.hh.
extern int A();
int A()
{
  return 10;
}
