extern "C" int myc(void);
extern "C" int mycxx(void)
{
  delete new int;
  return myc();
}
