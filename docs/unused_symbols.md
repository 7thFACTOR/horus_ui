# Unused symbols (dead code)

Scope: `src/`, `include/horus.h`, `backends/`, and the demo `examples/example_*/main.cpp`.
Third-party `examples/libs` excluded. "Dead" = only its own declaration/definition exists.

## A. Public API - `include/horus.h`

### Types / macros
- `u16`, `i8`, `i16` typedefs
- `HThemeWidgetElement`, `HOverlayToolbar`, `HOverlayToolbarWidget` opaque handles
- `WindowsDockingState` struct + nested `WindowInfo`, `DockNodeInfo`
- Macros `HUI_STRUCT_API`, `HUI_BIT`, `HUI_ENUM_AS_FLAGS`, `HUI_ENUM_AS_FLAGS_EX`

### Unused enum values
- `WidgetType::Custom`, `WidgetType::Count`
- `WidgetElementId::None`
- `MouseButton::AuxButton1..5`, `MouseButton::None`, `MouseButton::Count`
- `WindowFlags::CanClose/CanMove/CanResize/CanMinimize/Disabled`
- `TextInputFlags::Custom`, `TreeNodeFlags::Normal`, `TableFlags::FixedFit/ScrollX`
- `TintColorOpType::Count`
- `KeyCode::Subtract/Decimal/Control/Shift/Alt/LButton/MButton/RButton/LWin/RWin/Apostrophe/Grave/Count`
- `KeyModifiers::CapsLock`
- `MouseCursorType::Wait/CrossHair/ArrowWait/SizeNWSE/SizeNESW/Count`
- `DockingGuidesStyle::MouseCursorOnly`
- `MessageBoxImage::Custom/Count`, `MessageBoxButtons::OkCancel/YesNo/YesNoCancel`
- `ContextMenuFlags::None`, `ColorPickerFlags::NoAlpha`, `CircularSliderFlags::Normal`
- `SplineControlPoint::NodeType::Cusp/Smooth`, `KeywordInfo::Type::Keyword`

### Unused struct members
- `Point`: `makeAbsolute`, `getCos`, `normalizeTo`, `getNegated`, `negate`, `negateTo`, `isOnLine`, `isAlmosEqual`, `getSquaredLength`, `setLength`
- `Rect::expand`
- `Color`: `setFromRgba`, `getArgb`, `random`
- `InputEvent::MouseData::wheelDelta`, `InputEvent::TextData::maxTextBufferSize`, `InputEvent::OsDragDropData::timestamp`
- `LineStyle::stipplePatternMaxCount`, `FontGlyph::pixelX/pixelY`
- `Services`: `allInputFunctionsSet`, `allGfxFunctionsSet`, `allFontFunctionsSet`, `allTextEncodingFunctionsSet`, `allFileIoFunctionsSet`, `allFunctionsSet`
- `OverlayToolbarSettings::insertionHintColor`
- `Settings`: `defaultWidgetWidth`, `sameLineHeight`, `scaleLayouts`

### Unused free functions (defined, never called)
`contextGet`, `renderCallbackAdd`, `mouseCursorCreate`, `mouseCursorDestroy`,
`dockNodeDeleteChildren`, `windowUndock`, `windowIsMouseOver`, `windowGetClientRect`,
`widgetSetDefaultStyle`, `widgetSetDefaultElementStyle`, `widgetSetUserElementStyle`,
`themeGetWidgetElementParameterString`, `themeGetWidgetElementParameterColor`,
`themeGetUserWidgetElementParameterString`, `themeGetUserWidgetElementParameterFloat`,
`themeGetUserWidgetElementParameterColor`, `themeFontDestroy`, `tableRowSetColor`,
`tableCellGetRect`, `tableCellSetColor`, `layoutGetRemainingWidth`, `spacingGet`,
`boxBeginUserElement`, `popupMouseOutside`, `widgetIsFocused`, `widgetIsVisible`,
`widgetGetId`, `widgetGetPosition` (declared twice: horus.h:3396 and horus.h:3469), `widgetGetRect`, `mouseSetPosition`,
`dragDropSetMouseCursor`, `dragDropDisallow`, `renderDrawImage`, `renderDrawBorderedImage`,
`renderDrawArrow`, `stringFromI32`, `colorIntFromHex`, `colorIntToHex`

### Declared but never implemented nor called
`dockingStateSave`, `dockingStateLoad`, `messageBox`

## B. `src/types.h`
- Whole structs: `ImageSizingPolicy` (enum), `TextMarker`, `MemoryStream`, `CmdData` (union type name)
- Unused enumerators: `LayoutType::Columns`, `LayoutType::Column`
- Members: `DrawCommand::CmdDrawRect::textureIndex/wire`, `SameLineGroupState::startX/startY`,
  `WidgetState::prevFocusableId`, `MenuWidgetState::startPosition/lastItemRect/activatedNow/hoveredItemIndex/selectedItemIndex`,
  `TableColumnState::maxWidth/isResizable`, `TableState::rowDrawCmdIndex`,
  `PopupState::oldZOrder/opened/ownerWindow/savedSameLineInfoIndexStack`, `CircularSliderState::isFine`,
  `TextLineState::start/length`, `DockingState::closeWindow`, `DragDropState::dropped`,
  all `TextMarker` / `MemoryStream` members
- Methods: `ThemeElement::pressedState`, `ThemeElement::styleNormalState`,
  `MemoryStream::beginWrite/beginRead/writeData/readData/write/read`
- Gray area (write-only): `VectorEditorState::colWidthsPRS`

## C. `src/context.h`
- Static consts: `maxLayerCount`, `maxNestingIndex`, `maxBoxDepth`, `maxSameLineInfoIndex`
  (`maxPopupIndex` only used internally by the ctor)
- Members: `textLines`, `verticalToolbar`, `verticalToolbarStack`, `contextMenuNameId`,
  `lastColumnRect`, `columnSpacingStack`, `columnSpacing`, `treeNodeDepthLevel`, `drawCmdIndexStack`

## D. Internal headers
- `renderer.h`: methods `cmdSetTextUnderline`, `cmdSetTextBackfill`, `cmdSetTextBackfillColor`, `cmdDrawQuad`
- `font.h`: member `ellipsisSize`; methods `precacheGlyphs` (both overloads)
- `docking.h`: `dockNodeRootDelete`, `dockNodeRootGet`, `windowDeleteInternal`, `windowCloseInternal`
- `undo.h`: `UndoStack::current`, `UndoStack::clear`, `UndoStack::historySize`, `UndoStack::maxHistory()`, private `npos`
- `theme.h` / `atlas.h` / `util.h`: none found
- `renderer.cpp`: static `clipLineToRect`

## E. State headers
- `text_input_state.h`: nested struct `EditKeyShiftInfo` (+`charCode`/`shiftedCharCode`),
  member `TextInputState::decimalPlaces`; write-only `defaultText`, `themeElement`, `mouseDownSelectionBegin`
- `multiline_text_input_state.h`: member `firstVisibleLine`; write-only `themeElement`

## F. Static / file-local functions in `src/*.cpp`
`rendererFontPush`, `rendererFontPop`, `rendererColorSet`, `rendererLineColorSet`,
`rendererFillColorSet`, `rendererTextAtDraw` (custom_widget.cpp), `toHexString` (util.cpp),
`closeMenu` (menu_widget.cpp), `getRemainingDockNodeClientHeight`, `saveDockingState`,
`dockingSaveStateToMemory`, `loadDockingState`, `dockingLoadStateFromMemory` (docking.cpp),
`demoOnToolbarRotate`, `demoOnToolbarScale` (demo.cpp)

## G. Backends
- **dx11**: whole `Dx11VertexBuffer` struct (all ctors/methods/members);
  `Dx11Texture(u32,u32,Rgba32*)`, `Dx11Texture(u32,u32)`, `updateRectData`, `getWidth`, `getHeight`
- **dx12**: `Dx12Texture(u32,u32,Rgba32*)`, `updateRectData`, `getWidth`, `getHeight`,
  `Dx12VertexBuffer(u32,Vertex*)`, `Dx12VertexBuffer::create`
- **opengl**: `OpenGLTexture(u32,u32,Rgba32*)`, `OpenGLTexture(u32,u32)`, `updateRectData`,
  `getWidth`, `getHeight`, `OpenGLVertexBuffer(u32,Vertex*)`; static `setIntValueInGpuProgram`;
  `checkErrorGL` / `errStr` (release-only, debug macro)
- **vulkan**: whole `VulkanVertexBuffer` struct; `VulkanTexture::updateRectData`, `getImage`,
  `getWidth`, `getHeight`; `setDefaultWhiteTextureVk`
- **sdl3_input**: `Sdl3InputContext::sizeChanged`, static `hitTestCallback`, write-only `SdlWindowProxy::dx12RTV`
- **json_theme_loader**: `savePngImage`
- **native_file_dialogs**: `OpenMultipleFileSet` struct, `openMultipleFileDialog`, `saveFileDialog`, `pickFolderDialog`
- **utfcpp**: `utf32To8NoAlloc(const Utf32String&, const char*, size_t)` overload
- **freetype_fonts / stb_rectpack / stdio_fileio**: none

> Note: many section A items are public API and may be intentionally reserved for library
> consumers; everything else is internal dead code.
