
#ifndef NO_NEED_TO_CALL
extern int icasetest();
#endif

int main(int argc, char** argv)
{
#ifdef NO_NEED_TO_CALL
  return 0;
#else
  return icasetest();
#endif
}
