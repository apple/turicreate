#ifdef IS_LIB

#  ifdef _WIN32
__declspec(dllexport)
#  endif
  int internal_empty_1()
{
  return 0;
}

#else

#  ifdef _WIN32
__declspec(dllexport)
#  endif
  int empty_1()
{
  return 0;
}

#endif
