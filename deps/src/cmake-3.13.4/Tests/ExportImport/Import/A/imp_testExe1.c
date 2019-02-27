extern int generated_by_testExe1(void);
extern int generated_by_testExe3(void);
extern int generated_by_testExe4(void);
extern int testLib2(void);
extern int testLib3(void);
extern int testLib4(void);
extern int testLib4lib(void);
extern int testLib5(void);
extern int testLib6(void);
extern int testLib7(void);
extern int testLibCycleA1(void);
extern int testLibPerConfigDest(void);

/* Switch a symbol between debug and optimized builds to make sure the
   proper library is found from the testLib4 link interface.  */
#ifdef EXE_DBG
#  define testLib4libcfg testLib4libdbg
#else
#  define testLib4libcfg testLib4libopt
#endif
extern testLib4libcfg(void);

int main()
{
  return (testLib2() + generated_by_testExe1() + testLib3() + testLib4() +
          testLib5() + testLib6() + testLib7() + testLibCycleA1() +
          testLibPerConfigDest() + generated_by_testExe3() +
          generated_by_testExe4() + testLib4lib() + testLib4libcfg());
}
