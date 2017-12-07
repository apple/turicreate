
#ifndef MY_PRIVATE_DEFINE
#error Expected MY_PRIVATE_DEFINE
#endif

#ifndef MY_PUBLIC_DEFINE
#error Expected MY_PUBLIC_DEFINE
#endif

#ifdef MY_INTERFACE_DEFINE
#error Unexpected MY_INTERFACE_DEFINE
#endif

int main()
{
  return 0;
}
