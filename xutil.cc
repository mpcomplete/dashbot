// Written by Pioz.
// hacked to pieces by mpcomplete

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// defined in main.cc
extern Display* g_display;

// Simulate mouse click
void mouse_click(int button)
{
  // Create and setting up the event
  XEvent event;
  memset (&event, 0, sizeof (event));
  event.xbutton.button = button;
  event.xbutton.same_screen = True;
  event.xbutton.subwindow = DefaultRootWindow(g_display);
  while (event.xbutton.subwindow) {
    event.xbutton.window = event.xbutton.subwindow;
    XQueryPointer(g_display, event.xbutton.window,
                  &event.xbutton.root, &event.xbutton.subwindow,
                  &event.xbutton.x_root, &event.xbutton.y_root,
                  &event.xbutton.x, &event.xbutton.y,
                  &event.xbutton.state);
  }
  // Press
  event.type = ButtonPress;
  if (XSendEvent (g_display, PointerWindow, True, ButtonPressMask, &event) == 0)
    fprintf (stderr, "Error to send the event!\n");
  XFlush (g_display);
  usleep (1);
  // Release
  event.type = ButtonRelease;
  if (XSendEvent (g_display, PointerWindow, True, ButtonReleaseMask, &event) == 0)
    fprintf (stderr, "Error to send the event!\n");
  XFlush (g_display);
  usleep (1);
}

// Move mouse pointer
void mouse_move(int x, int y)
{
  XWarpPointer(g_display, None, DefaultRootWindow(g_display), 0,0,0,0, x, y);
  XFlush(g_display);
  usleep(1);
}

// Get mouse coordinates
void mouse_getpos(int *x, int *y)
{
  XEvent event;
  XQueryPointer(g_display, DefaultRootWindow (g_display),
                &event.xbutton.root, &event.xbutton.window,
                &event.xbutton.x_root, &event.xbutton.y_root,
                &event.xbutton.x, &event.xbutton.y,
                &event.xbutton.state);
  *x = event.xbutton.x;
  *y = event.xbutton.y;
}
