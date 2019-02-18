extern int testLibCycleB2(void);
int testLibCycleA2(void)
{
  return testLibCycleB2();
}
