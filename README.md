![HUI Logo](docs/images/hui_logo.png)
# Immediate Mode Graphical User Interface for Tools

![HUI](docs/images/hui.png)

## OVERVIEW
The HorusUI library allows you to quickly develop GUIs for your applications by leveraging the ease of use provided by immediate mode GUI concepts. No need to design your GUI layout and writing many lines of boilerplate GUI preparation, imgui takes care of layouting and making sure every widget you add to the system has an unique ID, gets drawn and responds to events.

## PREREQUISITES
**Windows:**
	Microsoft Visual Studio 2017

**Linux:**
	g++

	GTK3 dev, needed for the nativefiledialog lib, use:
		```sudo apt-get install libgtk-3-dev```

	GLU/GLUT dev, needed by GLEW, use:
		```sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev```
	
	libudev-dev, needed by SFML

## BUILDING
**Windows:**
- Execute: ```generate.bat```
- Open and compile: ```build_vs2017/horus.sln```
- Run generated files from ```./bin``` folder

**Linux:**
- Execute: ```sh ./generate.sh``` to generate makefiles
- Execute: ```sh ./build.sh``` to compile and generate the lib and executables for the examples
- Run generated files from the ```./bin``` folder

## FEATURES
- Immediate mode GUI, imgui (no state kept per widget, user provides the state)
- Docking OS native windows and tab panes system
- UTF8 text support 
- DPI aware, scaling of the whole UI elements, useful for high DPI screens
- Fully customizable through themes specified in a JSON file with 9-cell resizable elements and PNG images
- Dynamic font atlas for unlimited unicode glyphs and font sizes
- Widgets: text input (with hint text), tooltip, box, popup, messagebox, progress, button, icon button, dropdown, menu, context menu, tab, panel, radio, check, slider, toolbar etc.
- MegaWidgets: color picker, XYZ/XY editor, object reference editor (with dragdrop support)
- Widgets color tinting override
- Virtual list view support (huge number of items)
- Automatic vertical layouting of widgets (no need to position them by hand)
- Multi-column layout with custom sizes (preferred, fill, max size, percentage based or pixel based)
- Padding (left-right) and spacing for widgets
- Native Open/Save/Pick folder dialogs API
- Inter-widget drag and drop
- OS file/text drag and drop
- Currently using OpenGL and SDL as backends for rendering and window/input
- Customizable render/input/file i/o backends
- Custom user widgets API
- 2D primitive drawing, lines, polylines, hermite splines, elipses, rectangles, with thickness
- User viewport widget (rendering your scene/document view with the current rendering API)
- Custom mouse cursor API
- Clipboard API
- Undo/Redo system
- Keyboard shortcuts system
- Single header API
- C-like API

## Roadmap
- OSX proper support
- Vulkan rendering backend
- Better unicode input (show IME suggestion box for Chinese etc.)
- DPI theme elements based on current UI scale, choose the proper element bitmap size
- Multiline text input editor with highlighting, advanced text operations
- Hyperlink widget
- Rich text widget (multi-font family, style and size, multi-color, insert images and hyperlinks)
- Customizable Object Inspector
- Customizable Node Editor
