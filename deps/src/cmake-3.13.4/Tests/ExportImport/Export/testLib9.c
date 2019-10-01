#ifndef testLib9ObjPub_USED
#  error "testLib9ObjPub_USED not defined!"
#endif
#ifndef testLib9ObjPriv_USED
#  error "testLib9ObjPriv_USED not defined!"
#endif
#ifdef testLib9ObjIface_USED
#  error "testLib9ObjIface_USED defined but should not be!"
#endif
int testLib9ObjPub(void);
int testLib9ObjPriv(void);
int testLib9(void)
{
  return (testLib9ObjPub() + testLib9ObjPriv());
}
