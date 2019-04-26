#include <stdio.h>

extern int foo(void);
extern int bar(void);
extern int foobar(void);
extern int barbar(void);
extern int baz(void);
extern int tree_prefix_foo(void);
extern int tree_prefix_bar(void);
extern int tree_empty_prefix_foo(void);
extern int tree_empty_prefix_bar(void);
extern int tree_bar(void);
extern int tree_foobar(void);
extern int tree_baz(void);

int main()
{
  printf("foo: %d bar: %d foobar: %d barbar: %d baz: %d\n", foo(), bar(),
         foobar(), barbar(), baz());

  printf("tree_prefix_foo: %d tree_prefix_bar: %d tree_bar: %d tree_foobar: "
         "%d tree_baz: %d  tree_empty_prefix_foo: %d "
         "tree_empty_prefix_bar: %d\n",
         tree_prefix_foo(), tree_prefix_bar(), tree_bar(), tree_foobar(),
         tree_baz(), tree_empty_prefix_foo(), tree_empty_prefix_bar());
  return 0;
}
