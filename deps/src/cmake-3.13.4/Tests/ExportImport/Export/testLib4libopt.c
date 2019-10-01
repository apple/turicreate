#ifdef LIB_OPT
/* We are building in testLib4libopt.  Provide the correct symbol.  */
int testLib4libopt(void)
{
  return 0;
}
#else
/* We are not building in testLib4libopt.  Poison the symbol.  */
extern int testLib4libopt_noexist(void);
int testLib4libopt(void)
{
  return testLib4libopt_noexist();
}
#endif
