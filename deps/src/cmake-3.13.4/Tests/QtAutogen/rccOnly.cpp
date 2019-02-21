
extern int qInitResources_rccOnlyRes();

int main(int, char**)
{
  // Fails to link if the symbol is not present.
  qInitResources_rccOnlyRes();
  return 0;
}
