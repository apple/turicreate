#ifdef LIB_DBG
/* We are building in testLib4libdbg.  Provide the correct symbol.  */
int testLib4libdbg(void)
{
  return 0;
}
#else
/* We are not building in testLib4libdbg.  Poison the symbol.  */
extern int testLib4libdbg_noexist(void);
int testLib4libdbg(void)
{
  return testLib4libdbg_noexist();
}
#endif
