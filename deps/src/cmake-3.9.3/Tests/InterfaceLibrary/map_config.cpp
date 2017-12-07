
#ifdef DEBUG_MODE
#ifndef SPECIAL_MODE
#error Special configuration should be mapped to debug configuration.
#endif
#else
#ifdef SPECIAL_MODE
#error Special configuration should not be enabled if not debug configuration
#endif
#endif

int main(int, char**)
{
  return 0;
}
