![HUI Logo](hui_logo.png)
Advanced Immediate Mode Graphical User Interface

Features:
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

Roadmap:
- Linux and OSX proper support
- Vulkan rendering backend
- Better unicode input (suggestion box for Chinese etc.)
- DPI theme elements based on current UI scale, choose the proper element bitmap size
- Multiline text input editor with highlighting, advanced text operations
- Hyperlink widget
- Rich text widget (multi-font family, style and size, multi-color, insert images and hyperlinks)
- Customizable Object Inspector
- Customizable Node Editor

![HUI](hui.png)
