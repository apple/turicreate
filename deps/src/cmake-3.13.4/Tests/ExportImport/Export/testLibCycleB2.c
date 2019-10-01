extern int testLibCycleA3(void);
int testLibCycleB2(void)
{
  return testLibCycleA3();
}
