#ifdef DEF_SubDirLinkAImportedForExport
#  error "DEF_SubDirLinkAImportedForExport is defined but should not be!"
#endif
#ifndef DEF_SubDirLinkBImportedForExport
#  error "DEF_SubDirLinkBImportedForExport is not defined but should be!"
#endif

int testSubDirLinkA(void)
{
  return 0;
}
