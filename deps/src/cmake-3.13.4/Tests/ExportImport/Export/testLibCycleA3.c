extern int testLibCycleB3(void);
int testLibCycleA3(void)
{
  return testLibCycleB3();
}
