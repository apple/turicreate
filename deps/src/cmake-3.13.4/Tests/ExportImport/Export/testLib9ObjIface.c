/* Duplicate symbols from other sources to verify that this source
   is not included when the object library is used.  */
int testLib9ObjMissing(void);
int testLib9ObjPub(void)
{
  return testLib9ObjMissing();
}
int testLib9ObjPriv(void)
{
  return testLib9ObjMissing();
}
