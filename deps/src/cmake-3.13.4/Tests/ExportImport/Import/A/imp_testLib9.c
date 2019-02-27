#ifndef testLib9ObjPub_USED
#  error "testLib9ObjPub_USED not defined!"
#endif
#ifdef testLib9ObjPriv_USED
#  error "testLib9ObjPriv_USED defined but should not be!"
#endif
#ifndef testLib9ObjIface_USED
#  error "testLib9ObjIface_USED not defined!"
#endif

int testLib9(void);

int main()
{
  return testLib9();
}
