#ifndef DEF_SubDirLinkAImportedForExport
#  error "DEF_SubDirLinkAImportedForExport is not defined but should be!"
#endif
#ifdef DEF_SubDirLinkBImportedForExport
#  error "DEF_SubDirLinkBImportedForExport is defined but should not be!"
#endif

int testTopDirLib(void)
{
  return 0;
}
