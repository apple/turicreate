#ifndef DEF_SubDirLinkAImportedForImport
#  error "DEF_SubDirLinkAImportedForImport is not defined but should be!"
#endif
#ifndef DEF_SubDirLinkBImportedForImport
#  error "DEF_SubDirLinkBImportedForImport is not defined but should be!"
#endif

extern int testTopDirLib(void);
extern int testSubDirLinkA(void);

int main(void)
{
  return (testTopDirLib() + testSubDirLinkA() + 0);
}
