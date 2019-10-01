// causes a segfault
int main()
{
  int* ptr = 0;
  *ptr = 1;
}
