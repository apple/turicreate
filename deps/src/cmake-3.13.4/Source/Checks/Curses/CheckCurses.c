#if defined(CURSES_HAVE_NCURSES_H)
#  include <ncurses.h>
#elif defined(CURSES_HAVE_NCURSES_NCURSES_H)
#  include <ncurses/ncurses.h>
#elif defined(CURSES_HAVE_NCURSES_CURSES_H)
#  include <ncurses/curses.h>
#else
#  include <curses.h>
#endif

int main()
{
  curses_version();
  return 0;
}
