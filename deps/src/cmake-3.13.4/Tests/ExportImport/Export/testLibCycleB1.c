extern int testLibCycleA2(void);
int testLibCycleB1(void)
{
  return testLibCycleA2();
}
