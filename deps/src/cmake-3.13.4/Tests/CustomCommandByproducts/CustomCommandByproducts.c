extern int byproduct1(void);
extern int byproduct2(void);
extern int byproduct3(void);
extern int byproduct4(void);
extern int byproduct5(void);
extern int byproduct6(void);
extern int byproduct7(void);
extern int byproduct8(void);
extern int ExternalLibrary(void);
int main(void)
{
  return (byproduct1() + byproduct2() + byproduct3() + byproduct4() +
          byproduct5() + byproduct6() + byproduct7() + byproduct8() +
          ExternalLibrary() + 0);
}
