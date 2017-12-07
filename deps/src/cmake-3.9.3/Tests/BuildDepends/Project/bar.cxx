#include <noregen.h>
#include <regen.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
  /* Make sure the noregen header was not regenerated.  */
  if (strcmp("foo", noregen_string) != 0) {
    printf("FAILED: noregen.h was regenerated!\n");
    return 1;
  }

  /* Print out the string that should have been regenerated.  */
  printf("%s\n", regen_string);
  fflush(stdout);
  return 0;
}
