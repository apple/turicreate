extern int testLibCycleB1(void);
int testLibCycleA1(void)
{
  return testLibCycleB1();
}
