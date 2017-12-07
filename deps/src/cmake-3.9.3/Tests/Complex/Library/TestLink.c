int TestLinkGetType()
{
#ifdef CMakeTestLinkShared_EXPORTS
  return 0;
#else
  return 1;
#endif
}
