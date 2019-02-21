int main(int argc, char* argv[])
{
  int i = 3;
  switch (argc) {
    case 1:
      i = 0;
      [[fallthrough]];
    default:
      return i;
  }
}
