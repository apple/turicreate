
extern int qInitResources_rccEmptyRes();

int main(int, char**)
{
  // Fails to link if the symbol is not present.
  qInitResources_rccEmptyRes();
  return 0;
}
