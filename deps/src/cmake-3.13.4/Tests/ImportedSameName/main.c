#ifndef DEF_A
#  error "DEF_A not defined"
#endif
#ifndef DEF_B
#  error "DEF_B not defined"
#endif

extern void a(void);
extern void b(void);

int main(void)
{
  a();
  b();
  return 0;
}
