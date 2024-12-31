#ifdef _LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
namespace hui
{
void makeWindowClickThrough_Linux(SDL_Window* sdlwindow)
{
	auto window = (long unsigned int)SDL_GetPointerProperty(SDL_GetWindowProperties(sdlwindow), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, NULL);
	auto display = (Display*)SDL_GetPointerProperty(SDL_GetWindowProperties(sdlwindow), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
	
    if (!window)
    {
		printf("Failed to get native X11 window handle: %s\n", SDL_GetError());
		return;
	}

	int opcode, eventBase, errorBase;
// Set the window to be non-blocking by making it pass through input
Atom opacityAtom = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", False);
unsigned long opacity = 0; // Fully transparent (not for input, just visual)
XChangeProperty(display, window, opacityAtom, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&opacity, 1);

// Set the input shape to None (completely transparent to input)
    XShapeCombineMask(display, window, ShapeInput, 0, 0, None, ShapeSet);

// Use XShape to set the input to be passed through to other windows
if (XShapeQueryExtension(display, &eventBase, &errorBase)) {
    XShapeCombineMask(display, window, ShapeInput, 0, 0, None, ShapeSet);
}

// Map the window (make it visible)
XMapWindow(display, window);
// // Grab the pointer to prevent the window from receiving mouse events
// XGrabPointer(hdisplay, hwnd, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
//              GrabModeAsync, GrabModeAsync, hwnd, None, CurrentTime);

// 	XSelectInput(hdisplay, hwnd, 0);
}

}
#endif
