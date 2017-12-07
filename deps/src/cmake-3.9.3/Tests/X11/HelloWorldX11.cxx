
/*** START MAIN.H ***/
/* http://www.geocities.com/jeff_louie/x11/helloworld.htm* */
/*
 *  main.h
 *  TestX
 *
 *  Created by Jeff Louie on Tue Feb 03 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef MAIN_H
#define MAIN_H 1

#include <iostream>
#include <stdlib.h>

/* include the X library headers */
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>

class Main
{

public:
  // constructor
  Main(int argc, char* const argv[]);
  // virtual ~Main();

private:
  /* here are our X variables */
  Display* dis;
  int screen;
  Window win;
  GC gc;

  /* here are our X routines declared! */
  void init_x();
  void close_x();
  void redraw();
  int delay(int i);
};

#endif

/*** END MAIN.H ***/

/*** START MAIN.CPP ***/

// modified from Brian Hammond's Howdy program at
// http://www.insanityengine.com/doc/x11/xintro.html
// jeff louie 02.05.2004

int main(int argc, char* const argv[])
{
  Main m(argc, argv);
  return 0;
}

// Main::~Main() {;};
Main::Main(int argc, char* const argv[])
{
  XEvent event;   // XEvent declaration
  KeySym key;     // KeyPress Events
  char text[255]; // char buffer for KeyPress Events

  init_x();

  // event loop
  while (1) {
    // get the next event and stuff it into our event variable.
    // Note:  only events we set the mask for are detected!
    XNextEvent(dis, &event);

    switch (event.type) {
      int x;
      int y;
      case Expose:
        if (event.xexpose.count == 0) {
          redraw();
        }
        break;
      case KeyPress:
        if (XLookupString(&event.xkey, text, 255, &key, 0) == 1) {
          // use the XLookupString routine to convert the invent
          // KeyPress data into regular text.  Weird but necessary...
          if ((text[0] == 'q') || (text[0] == 'Q')) {
            close_x();
          } else {
            // echo key press
            printf("You pressed the %c key!\n", text[0]);
          }
        }
        break;
      case ButtonPress:
        // get cursor position
        x = event.xbutton.x;
        y = event.xbutton.y;
        strcpy(text, "X is FUN!");
        XSetForeground(dis, gc, rand() % event.xbutton.x % 255);
        // draw text at cursor
        XDrawString(dis, win, gc, x, y, text, strlen(text));
        break;
      default:
        printf("Unhandled event.\n");
    }
  }
}

void Main::init_x()
{
  unsigned long black, white;

  dis = XOpenDisplay(NULL);
  screen = DefaultScreen(dis);
  black = BlackPixel(dis, screen), white = WhitePixel(dis, screen);
  win = XCreateSimpleWindow(dis, DefaultRootWindow(dis), 0, 0, 300, 300, 5,
                            black, white);
  XSetStandardProperties(dis, win, "Hello World", "Hi", None, NULL, 0, NULL);
  XSelectInput(dis, win, ExposureMask | ButtonPressMask | KeyPressMask);
  // get Graphics Context
  gc = XCreateGC(dis, win, 0, 0);
  XSetBackground(dis, gc, white);
  XSetForeground(dis, gc, black);
  XClearWindow(dis, win);
  XMapRaised(dis, win);
};

void Main::close_x()
{
  XFreeGC(dis, gc);
  XDestroyWindow(dis, win);
  XCloseDisplay(dis);
  exit(1);
};

void Main::redraw()
{
  XClearWindow(dis, win);
};
