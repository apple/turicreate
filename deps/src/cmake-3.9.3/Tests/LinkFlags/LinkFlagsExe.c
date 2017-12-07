int main(void)
{
  return 0;
}

/* Intel compiler does not reject bad flags or objects!  */
#if defined(__INTEL_COMPILER)
#error BADFLAG
#endif
