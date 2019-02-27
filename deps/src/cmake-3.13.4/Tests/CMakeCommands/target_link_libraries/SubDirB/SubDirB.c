#ifdef DEF_SameNameImportedSubDirA
#  error "DEF_SameNameImportedSubDirA is defined but should not be!"
#endif
#ifdef DEF_SameNameImportedSubDirB
#  error "DEF_SameNameImportedSubDirB is defined but should not be!"
#endif
#ifndef DEF_TopDirImported
#  error "DEF_TopDirImported is not defined but should be!"
#endif

int main(void)
{
  return 0;
}
