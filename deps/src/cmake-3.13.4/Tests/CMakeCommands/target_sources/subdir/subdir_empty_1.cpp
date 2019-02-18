#ifdef IS_LIB

#  ifdef _WIN32
__declspec(dllexport)
#  endif
  int internal_subdir_empty_1()
{
  return 0;
}

#else

#  ifdef _WIN32
__declspec(dllexport)
#  endif
  int subdir_empty_1()
{
  return 0;
}

#endif
