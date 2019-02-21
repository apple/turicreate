int notInAllLibFunc()
{
  return 0;
}

#if 1
#  error "This target should not be compiled by ALL."
#endif
