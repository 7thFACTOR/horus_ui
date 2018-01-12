# HorusUI
Advanced Immediate Mode Graphical User Interface

Features:
- Docking OS native windows and tab panes system
- UTF8 text support 
- DPI aware, scaling of the whole UI elements, useful for high DPI screens
- Fully customizable through themes specified in a JSON file with 9-cell resizable elements and PNG images
- Dynamic font atlas for unlimited unicode glyphs and font sizes
- Widgets: text input (with hint text), tooltip, box, popup, messagebox, progress, button, icon button, dropdown, menu, context menu, tab, panel, radio, check, slider, etc.
- MegaWidgets: color picker, XYZ/XY editor, object reference editor (with dragdrop support)
- Widgets color tinting override
- Virtual list view support (huge number of items)
- Automatic vertical layouting of widgets (no need to position them by hand)
- Multi-column layout with custom sizes (preferred, fill, max size, percentage based or pixel based)
- Native Open/Save/Pick folder dialogs API
- inter-widget drag and drop
- OS file/text drag and drop
- Currently using OpenGL and SDL as backends for rendering and window/input
- Customizable render/input/file i/o backends
- Custom user widgets API
- 2D primitive drawing, lines, polylines, hermite splines, elipses, rectangles, with thickness
- User viewport widget (rendering your scene/document view with the current rendering API)
- Custom mouse cursor API
- Clipboard API
- Single header API
- Simple functions API
