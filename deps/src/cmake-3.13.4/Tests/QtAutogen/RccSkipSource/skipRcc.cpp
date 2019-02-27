
extern int qInitResources_skipRccGood();

int main(int, char**)
{
  // Fails to link if the symbol is not present.
  qInitResources_skipRccGood();
  return 0;
}
