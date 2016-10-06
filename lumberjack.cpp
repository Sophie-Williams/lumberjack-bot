#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <cassert>
#include <cstdio>
#include <cstring>

#include <iostream>

class X11WindowControl
{
	Display *_display = nullptr;
	Window _win_root = 0;

	Window _win = 0;

	size_t _width = 0;
	size_t _height = 0;

public:
	struct RGB {
		size_t red, green, blue;
		bool operator==(const RGB &op) { return red == op.red && green == op.green && blue == op.blue; }
	};

	X11WindowControl(Window w = 0)
	    : _win(w)
	{
		_display = XOpenDisplay(0);
		assert(_display != nullptr);

		// Get the root window for the current display.
		_win_root = XDefaultRootWindow(_display);
		assert(_win_root);
	}

	~X11WindowControl()
	{
		XCloseDisplay(_display);
	}

	size_t width() { return _width; }
	size_t height() { return _height; }

	void selectWindow()
	{
		FILE *f = popen("xwininfo  | egrep '(Window id:|Width:|Height:)' | perl -ne 'm/(0x[0-9a-fA-F]+|[0-9]+)/; print \"$1\n\";'", "r");

		_win = 0xffffffff;

		if (f == NULL)
			return;

		if (!ferror(f)) {
			fscanf(f, "%x\n", &_win);
			fscanf(f, "%d\n", &_width);
			fscanf(f, "%d\n", &_height);
		}

		pclose(f);
	}

	void sendEv(int k, bool p)
	{
		// Send a fake key press event to the window.
		XKeyEvent event;

		event.display = _display;
		event.window = _win;
		event.root = _win_root;
		event.subwindow = None;
		event.time = CurrentTime;
		event.x = 1;
		event.y = 1;
		event.x_root = 1;
		event.y_root = 1;
		event.same_screen = True;
		event.keycode = XKeysymToKeycode(_display, k);
		if (k >= XK_A && k <= XK_Z)
			event.state = ShiftMask; // modifiers;
		else
			event.state = 0; // modifiers;

		event.type = p ? KeyPress : KeyRelease;

		XSendEvent(_display, _win, True, p ? KeyPressMask : KeyReleaseMask, (XEvent *) &event);
		XFlush(_display);
	}

	RGB pixelColor(size_t x, size_t y)
	{
		std::cerr << x << " " << y << "\n";
		XImage *image = XGetImage(_display, _win, x, y, 1, 1, AllPlanes, XYPixmap);
		if (image == nullptr)
			return {0, 0, 0};

		XColor c;
		c.pixel = XGetPixel(image, 0, 0);
		XFree(image);

		XQueryColor(_display, DefaultColormap(_display, DefaultScreen(_display)), &c);

		return RGB{c.red / 256, c.green / 256, c.blue / 256};
	}
};

int main()
{
	X11WindowControl x;

	x.selectWindow();

	/* wait some time for starting - to the user focus the window */
	usleep(1000000);

	int horizontal_offset = 50; /* from the horizontal center */
	int bottom_up_offset = 400; /* from the bottom - if window is high enough */
	int count = 25;             /* how much to score */

	/* start on the left side */
	int side = XK_Left;
	horizontal_offset *= -1;

	while (count--) {
		bool branch_is_close =
		    x.pixelColor(x.width() / 2 + horizontal_offset,
						 x.height() - bottom_up_offset) == X11WindowControl::RGB{136, 99, 50};

		/* switch sides if needed */
		if (branch_is_close) {
			std::cerr << "uh uh, switching sides\n";
			side = side == XK_Left ? XK_Right
			                       : XK_Left;
			horizontal_offset *= -1;
		} else
			std::cerr << "hack, hack, hack\n";

		x.sendEv(side, true);
		x.sendEv(side, false);
		usleep(200000);
	}

	return 0;
}
