extern int notInAllLibFunc();

int main()
{
  return notInAllLibFunc();
}

#if 1
#  error "This target should not be compiled by ALL."
#endif
