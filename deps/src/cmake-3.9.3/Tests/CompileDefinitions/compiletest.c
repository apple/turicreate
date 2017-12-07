
#ifndef LINK_C_DEFINE
#error Expected LINK_C_DEFINE
#endif
#ifndef LINK_LANGUAGE_IS_C
#error Expected LINK_LANGUAGE_IS_C
#endif

#ifdef LINK_CXX_DEFINE
#error Unexpected LINK_CXX_DEFINE
#endif
#ifdef LINK_LANGUAGE_IS_CXX
#error Unexpected LINK_LANGUAGE_IS_CXX
#endif

#ifdef DEBUG_MODE
#error Unexpected DEBUG_MODE
#endif

int main(void)
{
  return 0;
}
