#include "horus.h"
#include <stdlib.h>
#include <assert.h>
#include "types.h"
#include "3rdparty/utf8/source/utf8.h"
#include "3rdparty/stb_image/stb_image.h"
#include "ui_theme.h"
#include "ui_atlas.h"
#include "ui_context.h"
#include "util.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font_cache.h"
#include "3rdparty/jsoncpp/include/json/json.h"
#include "3rdparty/jsoncpp/include/json/reader.h"
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace hui
{
u32 Color::getRgba() const
{
	u32 col;
	u8 *color = (u8*)&col;

	color[0] = (r > 1.0f ? 1.0f : r) * 255;
	color[1] = (g > 1.0f ? 1.0f : g) * 255;
	color[2] = (b > 1.0f ? 1.0f : b) * 255;
	color[3] = (a > 1.0f ? 1.0f : a) * 255.0f;

	return col;
}

u32 Color::getArgb() const
{
	u32 col;
	u8 *color = (u8*)&col;

	color[1] = (r > 1.0f ? 1.0f : r) * 255;
	color[2] = (g > 1.0f ? 1.0f : g) * 255;
	color[3] = (b > 1.0f ? 1.0f : b) * 255;
	color[0] = (a > 1.0f ? 1.0f : a) * 255;

	return col;
}

Color Color::random()
{
	return { (f32)rand() / (f32)RAND_MAX, (f32)rand() / (f32)RAND_MAX, (f32)rand() / (f32)RAND_MAX, 1 };
}

const Color Color::transparent(0, 0, 0, 0);
const Color Color::white(1, 1, 1, 1);
const Color Color::black(0, 0, 0, 1);
const Color Color::red(1, 0, 0, 1);
const Color Color::darkRed(.7f, 0, 0, 1);
const Color Color::veryDarkRed(.5f, 0, 0, 1);
const Color Color::green(0, 1, 0, 1);
const Color Color::darkGreen(0, 0.7f, 0, 1);
const Color Color::veryDarkGreen(0, 0.5f, 0, 1);
const Color Color::blue(0, 0, 1, 1);
const Color Color::darkBlue(0, 0, 0.7f, 1);
const Color Color::veryDarkBlue(0, 0, 0.5f, 1);
const Color Color::yellow(1, 1, 0, 1);
const Color Color::darkYellow(0.7f, 0.7f, 0, 1);
const Color Color::veryDarkYellow(0.5f, 0.5f, 0, 1);
const Color Color::magenta(1, 0, 1, 1);
const Color Color::cyan(0, 1, 1, 1);
const Color Color::darkCyan(0, .7, .7, 1);
const Color Color::veryDarkCyan(0, .5, .5, 1);
const Color Color::orange(1, 0.5f, 0, 1);
const Color Color::darkOrange(0.5f, 0.2f, 0, 1);
const Color Color::darkGray(0.3f, 0.3f, 0.3f, 1);
const Color Color::gray(0.5f, 0.5f, 0.5f, 1);
const Color Color::lightGray(0.7f, 0.7f, 0.7f, 1);
const Color Color::sky(0.f, 0.682f, 0.937f, 1);

Context createContext(InputProvider* customInputProvider, GraphicsProvider* customGfxProvider)
{
	UiContext* context = new UiContext();

	setContext((Context)context);

	if (customInputProvider)
		setInputProvider(customInputProvider);

	if (customGfxProvider)
		setGraphicsProvider(customGfxProvider);

#ifdef _WIN32
	SetProcessDPIAware();
#endif

	return context;
}

void initializeContext(Context context)
{
	UiContext* ctxPtr = (UiContext*)context;
	ctxPtr->initializeGraphics();
}

void setContext(Context context)
{
	ctx = (UiContext*)context;
}

Context getContext()
{
	return ctx;
}

void deleteContext(Context context)
{
	delete (Context*)context;
}

ContextSettings& getContextSettings()
{
	return ctx->settings;
}

void clearBackground()
{
	auto windowElemState = ctx->theme->getElement(WidgetElementId::WindowBody).normalState();

	ctx->renderer->clear(windowElemState.color);
}

void setEnabled(bool enabled)
{
	ctx->widget.enabled = enabled;
}

void setFocused()
{
	ctx->widget.focusedWidgetPressed = true;
	ctx->widget.hoveredWidgetId = ctx->currentWidgetId;
	ctx->widget.focusedWidgetId = ctx->currentWidgetId;
	ctx->widget.hovered = true;
	ctx->widget.pressed = true;
	ctx->widget.focused = true;
	ctx->focusChanged = true;
}

void addWidgetItem(f32 height)
{
	ctx->widget.changeEnded = false;
	height = round(height);
	ctx->widget.rect.set(
		round(ctx->penPosition.x + ctx->padding * ctx->globalScale),
		round(ctx->penPosition.y),
		ctx->layoutStack.back().width - ctx->padding * 2.0f * ctx->globalScale,
		height);
	ctx->penPosition.y += ctx->spacing * ctx->globalScale + height;
	ctx->penPosition.y = round(ctx->penPosition.y);
}

void setAsFocusable()
{
	if (ctx->widget.focusedWidgetId == ctx->currentWidgetId)
	{
		ctx->widget.focusedWidgetRect = ctx->widget.rect;
	}

	if (!ctx->widget.nextFocusableWidgetId
		&& ctx->currentWidgetId > ctx->widget.focusedWidgetId)
	{
		ctx->widget.nextFocusableWidgetId = ctx->currentWidgetId;
	}
}

bool viewportImageFitSize(
	f32 imageWidth, f32 imageHeight,
	f32 viewWidth, f32 viewHeight,
	f32& newWidth, f32& newHeight,
	bool ignoreHeight, bool ignoreWidth)
{
	f32 aspectRatio = 1.0f;

	newWidth = imageWidth;
	newHeight = imageHeight;

	if (imageWidth <= viewWidth
		&& imageHeight <= viewHeight)
	{
		return false;
	}

	if (newWidth >= viewWidth && !ignoreWidth)
	{
		if (newWidth < 0.0001f)
			newWidth = 0.0001f;

		aspectRatio = (f32)viewWidth / newWidth;
		newWidth = viewWidth;
		newHeight *= aspectRatio;
	}

	if (newHeight >= viewHeight && !ignoreHeight)
	{
		if (newHeight < 0.0001f)
			newHeight = 0.0001f;

		aspectRatio = (f32)viewHeight / newHeight;
		newHeight = viewHeight;
		newWidth *= aspectRatio;
	}

	return true;
}

void beginFrame()
{
	if (ctx->textInput.widgetId)
	{
		ctx->textInput.processEvent(ctx->event);
	}

	if (ctx->event.type == InputEvent::Type::Key
		&& ctx->event.key.code == KeyCode::Tab
		&& !!(ctx->event.key.modifiers, KeyModifiers::Shift)
		&& ctx->event.key.down)
	{
		ctx->widget.focusedWidgetId--;
		ctx->focusChanged = true;

		if (ctx->widget.focusedWidgetId < 0)
		{
			ctx->widget.focusedWidgetId = 0;
		}
	}
	else if (ctx->event.type == InputEvent::Type::Key
		&& ctx->event.key.code == KeyCode::Tab
		&& ctx->event.key.down)
	{
		ctx->widget.focusedWidgetId = ctx->widget.nextFocusableWidgetId;
		ctx->focusChanged = true;

		if (ctx->widget.focusedWidgetId > ctx->maxWidgetId)
		{
			ctx->widget.focusedWidgetId = 1;
		}
	}

	ctx->mustRedraw = false;
	ctx->skipRenderAndInput = false;
	ctx->currentWidgetId = 1;
	ctx->widget.enabled = true;
	ctx->currentWindowIndex = 0;
	ctx->layerIndex = 0;
	ctx->widget.nextFocusableWidgetId = 0;
	ctx->mouseCursor = MouseCursorType::Arrow;
	ctx->menuDepth = 0;
	ctx->popupIndex = 0;
	ctx->menuItemChosen = false;
	ctx->dragDropState.foundDropTarget = false;
	ctx->widget.hoveredWidgetId = 0;
	ctx->widget.hoveredWidgetType = WidgetType::None;
	ctx->frameCount++;
	ctx->totalTime += ctx->deltaTime;
	ctx->pruneUnusedTextTime += ctx->deltaTime;

	if (ctx->pruneUnusedTextTime >= ctx->settings.textCachePruneIntervalSec)
	{
		ctx->textCache->pruneUnusedTexts();
		ctx->pruneUnusedTextTime = 0;
	}
}

void endFrame()
{
	ctx->maxWidgetId = ctx->currentWidgetId;
	ctx->focusChanged = false;
	ctx->mouseMoved = false;

	if (ctx->dragDropState.begunDragging
		&& ctx->event.type == InputEvent::Type::MouseUp)
	{
		ctx->dragDropState.begunDragging = false;
	}

	if (ctx->dragDropState.begunDragging)
	{
		if (ctx->dragDropState.foundDropTarget)
		{
			setMouseCursor(ctx->dragDropState.dropAllowedCursor);
		}
		else
		{
			setMouseCursor(MouseCursorType::No);
		}
	}

	if (ctx->mouseCursor != MouseCursorType::Custom)
	{
		ctx->inputProvider->setCursor(ctx->mouseCursor);
	}
	else if (ctx->customMouseCursor)
	{
		ctx->inputProvider->setCustomCursor(ctx->customMouseCursor);
	}
}

void update(f32 deltaTime)
{
	if (ctx->widget.hoveredWidgetId && !ctx->tooltip.show)
	{
		ctx->tooltip.timer += deltaTime;
	}

	// tooltip handling
	if (ctx->widget.hoveredWidgetId
		&& ctx->widget.hoveredWidgetId != ctx->tooltip.widgetId
		&& ctx->tooltip.timer >= ctx->tooltip.delayToShow)
	{
		ctx->tooltip.widgetId = ctx->widget.hoveredWidgetId;
		ctx->tooltip.show = true;
		ctx->mustRedraw = true;
		ctx->tooltip.timer = 0;
	}
	else if (!ctx->widget.hoveredWidgetId)
	{
		ctx->tooltip.show = false;
		ctx->tooltip.widgetId = 0;
	}

	if (ctx->tooltip.show)
	{
		// track mouse pos
		ctx->tooltip.position = ctx->inputProvider->getMousePosition();
	}
}

bool hasNothingToDo()
{
	return !ctx->mustRedraw
		&& !ctx->mouseMoved
		&& !ctx->events.size()
		&& !ctx->dockingTabPane;
}

void setDisableRendering(bool disable)
{
	ctx->renderer->disableRendering = disable;
}

void forceRepaint()
{
	ctx->mustRedraw = true;
}

void skipThisFrame()
{
	ctx->setSkipRenderAndInput(true);
}

bool copyToClipboard(const char* text)
{
	return ctx->inputProvider->copyToClipboard(text);
}

bool pasteFromClipboard(char* outText, u32 maxTextSize)
{
	return ctx->inputProvider->pasteFromClipboard(outText, maxTextSize);
}

const InputEvent& getInputEvent()
{
	return ctx->event;
}

void setMouseCursor(MouseCursorType type)
{
	ctx->mouseCursor = type;
}

MouseCursor createMouseCursor(Rgba32* pixels, u32 width, u32 height, u32 hotSpotX, u32 hotSpotY)
{
	return ctx->inputProvider->createCustomCursor(pixels, width, height, hotSpotX, hotSpotY);
}

MouseCursor createMouseCursor(const char* imageFilename, u32 hotSpotX, u32 hotSpotY)
{
	auto img = hui::loadRawImage(imageFilename);
	auto cur = hui::createMouseCursor((Rgba32*)img.pixels, img.width, img.height, hotSpotX, hotSpotY);
	deleteRawImage(img);

	return cur;
}

void deleteMouseCursor(MouseCursor cursor)
{
	ctx->inputProvider->deleteCustomCursor(cursor);
}

void setMouseCursor(MouseCursor cursor)
{
	ctx->mouseCursor = MouseCursorType::Custom;
	ctx->customMouseCursor = cursor;
}

void setWindow(Window window)
{
	ctx->inputProvider->setCurrentWindow(window);

	auto rect = getWindowRect(window);

	ctx->renderer->setWindowSize({ rect.width, rect.height });
	ctx->gfx->setViewport(
		{ rect.width, rect.height },
		{ 0, 0, rect.width, rect.height });
}

Window getWindow()
{
	return ctx->inputProvider->getCurrentWindow();
}

Window getFocusedWindow()
{
	return ctx->inputProvider->getFocusedWindow();
}

Window getHoveredWindow()
{
	return ctx->inputProvider->getHoveredWindow();
}

Window getMainWindow()
{
	return ctx->inputProvider->getMainWindow();
}

Window createWindow(
	const char* title, u32 width, u32 height,
	WindowBorder border,
	WindowPositionType positionType,
	Point customPosition,
	bool showInTaskBar)
{
	auto wnd = ctx->inputProvider->createWindow(title, width, height, border, positionType, customPosition, showInTaskBar);

	if (!ctx->renderer)
	{
		ctx->initializeGraphics();
	}

	return wnd;
}

void setWindowTitle(Window window, const char* title)
{
	ctx->inputProvider->setWindowTitle(window, title);
}

void setWindowRect(Window window, const Rect& rect)
{
	ctx->inputProvider->setWindowRect(window, rect);
}

Rect getWindowRect(Window window)
{
	return ctx->inputProvider->getWindowRect(window);
}

Rect getWindowClientRect(Window window)
{
	auto rect = ctx->inputProvider->getWindowRect(window);

	rect.x = 0;
	rect.y = 0;

	return rect;
}

void presentWindow(Window window)
{
	ctx->inputProvider->presentWindow(window);
}

void destroyWindow(Window window)
{
	ctx->inputProvider->destroyWindow(window);
}

void showWindow(Window window)
{
	ctx->inputProvider->showWindow(window);
}

void hideWindow(Window window)
{
	ctx->inputProvider->hideWindow(window);
}

void riseWindow(Window window)
{
	ctx->inputProvider->raiseWindow(window);
}

void maximizeWindow(Window window)
{
	ctx->inputProvider->maximizeWindow(window);
}

void minimizeWindow(Window window)
{
	ctx->inputProvider->minimizeWindow(window);
}

WindowState getWindowState(Window window)
{
	return ctx->inputProvider->getWindowState(window);
}

void setCapture(Window window)
{
	ctx->inputProvider->setCapture(window);
}

void releaseCapture()
{
	ctx->inputProvider->releaseCapture();
}

void cancelEvent()
{
	ctx->event.type = InputEvent::Type::None;
}

void addInputEvent(const InputEvent& event)
{
	ctx->events.push_back(event);
}

void clearInputEventQueue()
{
	ctx->event.type = InputEvent::Type::None;
	ctx->events.clear();
}

void setMouseMoved(bool moved)
{
	ctx->mouseMoved = moved;
}

u32 getInputEventCount()
{
	return ctx->events.size();
}

InputEvent getInputEventAt(u32 index)
{
	return ctx->events[index];
}

void setInputEvent(const InputEvent& event)
{
	ctx->event = event;
}

bool mustQuit()
{
	return ctx->inputProvider->mustQuit();
}

bool wantsToQuit()
{
	return ctx->inputProvider->wantsToQuit();
}

void cancelQuitApplication()
{
	ctx->inputProvider->cancelQuitApplication();
}

void quitApplication()
{
	ctx->inputProvider->quitApplication();
}

void shutdown()
{
	assert(ctx);

	if (ctx->inputProvider)
		ctx->inputProvider->shutdown();
}

Image loadImage(const char* filename)
{
	int width = 0;
	int height = 0;
	int comp;
	stbi_uc* data = stbi_load(filename, &width, &height, &comp, 4);

	if (!data)
		return nullptr;

	Image img = createImage((Rgba32*)data, width, height);

	delete[] data;

	return img;
}

Image createImage(Rgba32* pixels, u32 width, u32 height)
{
	auto img = ctx->theme->addImage(pixels, width, height);
	ctx->theme->packAtlas();
	return img;
}

Point getImageSize(Image image)
{
	UiImage* img = (UiImage*)image;

	return { img->rect.width, img->rect.height };
}

void updateImagePixels(Image image, Rgba32* pixels)
{
	UiImage* img = (UiImage*)image;

	//TODO: check if image is rotated
	img->atlasTexture->textureArray->updateRectData(img->atlasTexture->textureIndex, img->rect, pixels);
}

RawImage loadRawImage(const char* filename)
{
	int width = 0;
	int height = 0;
	int comp;
	stbi_uc* data = stbi_load(filename, &width, &height, &comp, 4);

	if (!data)
		return RawImage();

	RawImage imgdata;

	imgdata.width = width;
	imgdata.height = height;
	imgdata.pixels = data;
	imgdata.bpp = 32;
	//TODO: load 8bbp or other formats?

	return imgdata;
}

void deleteImage(Image image)
{
	UiImage* img = (UiImage*)image;
	img->atlas->deleteImage(img);
}

void deleteRawImage(RawImage& image)
{
	delete[] image.pixels;
	image.pixels = nullptr;
	image.width = 0;
	image.height = 0;
	image.bpp = 0;
}

Atlas createAtlas(u32 width, u32 height)
{
	return new UiAtlas(width, height);
}

void deleteAtlas(Atlas atlas)
{
	delete (UiAtlas*)atlas;
}

Image addAtlasImage(Atlas atlas, const RawImage& img)
{
	UiAtlas* atlasPtr = (UiAtlas*)atlas;

	return atlasPtr->addImage((const Rgba32*)img.pixels, img.width, img.height);
}

bool packAtlas(Atlas atlas)
{
	UiAtlas* atlasPtr = (UiAtlas*)atlas;

	constexpr u32 border = 2;

	return atlasPtr->pack(border);
}

void setInputProvider(InputProvider* provider)
{
	ctx->inputProvider = provider;
}

void setGraphicsProvider(GraphicsProvider* provider)
{
	ctx->gfx = provider;
}

void processInputEvents()
{
	ctx->event.type = InputEvent::Type::None;
	clearInputEventQueue();
	ctx->inputProvider->processEvents();
	hui::update(getFrameDeltaTime());
}

void setFrameDeltaTime(f32 dt)
{
	ctx->deltaTime = dt;
}

f32 getFrameDeltaTime()
{
	return ctx->deltaTime;
}

Theme createTheme(u32 atlasTextureSize)
{
	UiTheme* theme = new UiTheme(atlasTextureSize);

	ctx->themes.push_back(theme);

	return theme;
}

Image addThemeImage(Theme theme, const RawImage& img)
{
	UiTheme* themePtr = (UiTheme*)theme;

	return themePtr->addImage((const Rgba32*)img.pixels, img.width, img.height);
}

void setWidgetStyle(WidgetType widgetType, const char* styleName)
{
	//TODO: more automatic correlation between widget type and its element types, to avoid manual switch
	// To not force using map to search for the current style for all widgets, this might be the only way
	switch (widgetType)
	{
	case WidgetType::Window:
		ctx->theme->elements[(u32)WidgetElementId::WindowBody].setStyle(styleName);
		break;
	case WidgetType::Layout:
		break;
	case WidgetType::Compound:
		break;
	case WidgetType::Tooltip:
		ctx->theme->elements[(u32)WidgetElementId::TooltipBody].setStyle(styleName);
		break;
	case WidgetType::Button:
		ctx->theme->elements[(u32)WidgetElementId::ButtonBody].setStyle(styleName);
		break;
	case WidgetType::IconButton:
		break;
	case WidgetType::TextInput:
		ctx->theme->elements[(u32)WidgetElementId::TextInputBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::TextInputCaret].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::TextInputSelection].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::TextInputDefaultText].setStyle(styleName);
		break;
	case WidgetType::Slider:
		ctx->theme->elements[(u32)WidgetElementId::SliderBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::SliderBodyFilled].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::SliderKnob].setStyle(styleName);
		break;
	case WidgetType::Progress:
		ctx->theme->elements[(u32)WidgetElementId::ProgressBack].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ProgressFill].setStyle(styleName);
		break;
	case WidgetType::Image:
		break;
	case WidgetType::Check:
		ctx->theme->elements[(u32)WidgetElementId::CheckBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::CheckMark].setStyle(styleName);
		break;
	case WidgetType::Radio:
		ctx->theme->elements[(u32)WidgetElementId::RadioBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::RadioMark].setStyle(styleName);
		break;
	case WidgetType::Label:
		ctx->theme->elements[(u32)WidgetElementId::LabelBody].setStyle(styleName);
		break;
	case WidgetType::Panel:
		ctx->theme->elements[(u32)WidgetElementId::PanelBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::PanelCollapsedArrow].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::PanelExpandedArrow].setStyle(styleName);
		break;
	case WidgetType::Popup:
		ctx->theme->elements[(u32)WidgetElementId::PopupBody].setStyle(styleName);
		break;
	case WidgetType::Dropdown:
		ctx->theme->elements[(u32)WidgetElementId::DropdownBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::DropdownArrow].setStyle(styleName);
		break;
	case WidgetType::List:
		break;
	case WidgetType::Selectable:
		ctx->theme->elements[(u32)WidgetElementId::SelectableBody].setStyle(styleName);
		break;
	case WidgetType::ResizeGrip:
		break;
	case WidgetType::Line:
		ctx->theme->elements[(u32)WidgetElementId::LineBody].setStyle(styleName);
		break;
	case WidgetType::Space:
		break;
	case WidgetType::ScrollView:
		ctx->theme->elements[(u32)WidgetElementId::ScrollViewBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ScrollViewScrollBar].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ScrollViewScrollThumb].setStyle(styleName);
		break;
	case WidgetType::MenuBar:
		ctx->theme->elements[(u32)WidgetElementId::MenuBarBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MenuBarItem].setStyle(styleName);
		break;
	case WidgetType::Menu:
		ctx->theme->elements[(u32)WidgetElementId::MenuBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MenuItemBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MenuItemCheckMark].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MenuItemNoCheckMark].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MenuItemSeparator].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MenuItemShortcut].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::SubMenuItemArrow].setStyle(styleName);
		break;
	case WidgetType::TabGroup:
		ctx->theme->elements[(u32)WidgetElementId::TabGroupBody].setStyle(styleName);
		break;
	case WidgetType::Tab:
		ctx->theme->elements[(u32)WidgetElementId::TabBodyActive].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::TabBodyInactive].setStyle(styleName);
		break;
	case WidgetType::Viewport:
		break;
	case WidgetType::ViewPane:
		ctx->theme->elements[(u32)WidgetElementId::ViewPaneDockRect].setStyle(styleName);
		break;
	case WidgetType::MsgBox:
		ctx->theme->elements[(u32)WidgetElementId::MessageBoxIconError].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MessageBoxIconWarning].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MessageBoxIconInfo].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MessageBoxIconQuestion].setStyle(styleName);
		break;
	case WidgetType::Box:
		ctx->theme->elements[(u32)WidgetElementId::BoxBody].setStyle(styleName);
		break;
	case WidgetType::Toolbar:
		ctx->theme->elements[(u32)WidgetElementId::ToolbarBody].setStyle(styleName);
		break;
	case WidgetType::ToolbarButton:
		ctx->theme->elements[(u32)WidgetElementId::ToolbarButtonBody].setStyle(styleName);
		break;
	case WidgetType::ToolbarDropdown:
		ctx->theme->elements[(u32)WidgetElementId::ToolbarDropdownBody].setStyle(styleName);
		break;
	case WidgetType::ToolbarSeparator:
		ctx->theme->elements[(u32)WidgetElementId::ToolbarSeparatorBody].setStyle(styleName);
		break;
	case WidgetType::ColumnsHeader:
		ctx->theme->elements[(u32)WidgetElementId::ColumnsHeaderBody].setStyle(styleName);
		break;
	case WidgetType::ComboSlider:
		ctx->theme->elements[(u32)WidgetElementId::ComboSliderBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ComboSliderLeftArrow].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ComboSliderRangeBar].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ComboSliderRightArrow].setStyle(styleName);
		break;
	case WidgetType::RotarySlider:
		ctx->theme->elements[(u32)WidgetElementId::RotarySliderBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::RotarySliderMark].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::RotarySliderValueDot].setStyle(styleName);
		break;
	}
}

void setWidgetDefaultStyle(WidgetType widgetType)
{
	setWidgetStyle(widgetType, "default");
}

void setUserWidgetElementStyle(const char* elementName, const char* styleName)
{
	ctx->theme->userElements[elementName]->setStyle(styleName);
}

void buildTheme(Theme theme)
{
	UiTheme* themePtr = (UiTheme*)theme;

	themePtr->packAtlas();
	themePtr->setDefaultWidgetStyle();
}

void setThemeWidgetElement(
	Theme theme,
	WidgetElementId elementId,
	WidgetStateType widgetStateType,
	const WidgetElementInfo& elementInfo,
	const char* styleName)
{
	UiTheme* themePtr = (UiTheme*)theme;
	u32 stateIndex = (u32)widgetStateType;

	auto& state = themePtr->elements[(u32)elementId].styles[styleName].states[stateIndex];

	state.border = elementInfo.border;
	state.color = elementInfo.color;
	state.textColor = elementInfo.textColor;
	state.font = (UiFont*)elementInfo.font;
	state.width = elementInfo.width;
	state.height = elementInfo.height;
	state.image = (UiImage*)elementInfo.image;
}

void setThemeUserWidgetElement(
	Theme theme,
	const char* userElementName,
	WidgetStateType widgetStateType,
	const WidgetElementInfo& elementInfo,
	const char* styleName)
{
	UiTheme* themePtr = (UiTheme*)theme;
	u32 stateIndex = (u32)widgetStateType;

	if (themePtr->userElements.find(userElementName) == themePtr->userElements.end())
	{
		themePtr->userElements.insert(std::make_pair(userElementName, new UiThemeElement()));
	}

	auto& state = themePtr->userElements[userElementName]->styles[styleName].states[stateIndex];

	state.border = elementInfo.border;
	state.color = elementInfo.color;
	state.textColor = elementInfo.textColor;
	state.font = (UiFont*)elementInfo.font;
	state.width = elementInfo.width;
	state.height = elementInfo.height;
	state.image = (UiImage*)elementInfo.image;
}

void setTheme(Theme theme)
{
	ctx->theme = (UiTheme*)theme;
}

Theme getTheme()
{
	return ctx->theme;
}

void deleteTheme(Theme theme)
{
	auto iter = std::find(ctx->themes.begin(), ctx->themes.end(), (UiTheme*)theme);

	if (iter == ctx->themes.end())
		return;

	delete *iter;
	ctx->themes.erase(iter);
	ctx->theme = nullptr;
}

void getThemeWidgetElementInfo(WidgetElementId elementId, WidgetStateType state, WidgetElementInfo& outInfo,
	const char* styleName)
{
	auto& elemState = ctx->theme->elements[(u32)elementId].getStyleState(styleName, state);

	outInfo.border = elemState.border;
	outInfo.color = elemState.color;
	outInfo.font = elemState.font;
	outInfo.textColor = elemState.textColor;
	outInfo.image = elemState.image;
	outInfo.width = elemState.width;
	outInfo.height = elemState.height;
}

void getThemeUserWidgetElementInfo(const char* userElementName, WidgetStateType state, WidgetElementInfo& outInfo, const char* styleName)
{
	outInfo = {};
	auto iter = ctx->theme->userElements.find(userElementName);

	if (iter == ctx->theme->userElements.end())
		return;

	auto elemState = iter->second->getStyleState(styleName, state);

	outInfo.border = elemState.border;
	outInfo.color = elemState.color;
	outInfo.font = elemState.font;
	outInfo.textColor = elemState.textColor;
	outInfo.image = elemState.image;
	outInfo.width = elemState.width;
	outInfo.height = elemState.height;
}

Font createFont(Theme theme, const char* name, const char* fontFilename, u32 faceSize)
{
	return (Font)((UiTheme*)theme)->fontCache->createFont(name, fontFilename, faceSize * ctx->globalScale, false);
}

void releaseFont(Font font)
{
	ctx->theme->fontCache->releaseFont((UiFont*)font);
}

Font getFont(Theme theme, const char* themeFontName)
{
	UiTheme* themeObj = (UiTheme*)theme;

	return themeObj->fonts[themeFontName];
}

Font getFont(const char* themeFontName)
{
	return getFont(getTheme(), themeFontName);
}

static std::string readTextFile(const char* path)
{
	FILE* file = fopen(path, "rb");

	if (!file)
		return std::string("");

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	std::string text;

	if (size != -1)
	{
		fseek(file, 0, SEEK_SET);

		char* buffer = new char[size + 1];
		buffer[size] = 0;

		if (fread(buffer, 1, size, file) == (unsigned long)size)
			text = buffer;

		delete[] buffer;
	}

	fclose(file);

	return text;
}

WidgetType getWidgetTypeFromName(std::string name)
{
	if (name == "window") return WidgetType::Window;
	if (name == "tooltip") return WidgetType::Tooltip;
	if (name == "button") return WidgetType::Button;
	if (name == "iconButton") return WidgetType::IconButton;
	if (name == "textInput") return WidgetType::TextInput;
	if (name == "slider") return WidgetType::Slider;
	if (name == "progress") return WidgetType::Progress;
	if (name == "image") return WidgetType::Image;
	if (name == "check") return WidgetType::Check;
	if (name == "radio") return WidgetType::Radio;
	if (name == "label") return WidgetType::Label;
	if (name == "panel") return WidgetType::Panel;
	if (name == "popup") return WidgetType::Popup;
	if (name == "dropdown") return WidgetType::Dropdown;
	if (name == "list") return WidgetType::List;
	if (name == "resizeGrip") return WidgetType::ResizeGrip;
	if (name == "line") return WidgetType::Line;
	if (name == "space") return WidgetType::Space;
	if (name == "scrollView") return WidgetType::ScrollView;
	if (name == "menuBar") return WidgetType::MenuBar;
	if (name == "menu") return WidgetType::Menu;
	if (name == "tabGroup") return WidgetType::TabGroup;
	if (name == "tab") return WidgetType::Tab;
	if (name == "viewport") return WidgetType::Viewport;
	if (name == "viewPane") return WidgetType::ViewPane;
	if (name == "messageBox") return WidgetType::MsgBox;
	if (name == "selectable") return WidgetType::Selectable;
	if (name == "box") return WidgetType::Box;
	if (name == "toolbar") return WidgetType::Toolbar;
	if (name == "toolbarButton") return WidgetType::ToolbarButton;
	if (name == "columnsHeader") return WidgetType::ColumnsHeader;
	if (name == "comboSlider") return WidgetType::ComboSlider;
	if (name == "rotarySlider") return WidgetType::RotarySlider;

	return WidgetType::None;
}

WidgetElementId getWidgetElementFromName(std::string name)
{
	if (name == "windowBody") return WidgetElementId::WindowBody;
	if (name == "buttonBody") return WidgetElementId::ButtonBody;
	if (name == "checkBody") return WidgetElementId::CheckBody;
	if (name == "checkMark") return WidgetElementId::CheckMark;
	if (name == "radioBody") return WidgetElementId::RadioBody;
	if (name == "radioMark") return WidgetElementId::RadioMark;
	if (name == "lineBody") return WidgetElementId::LineBody;
	if (name == "labelBody") return WidgetElementId::LabelBody;
	if (name == "panelBody") return WidgetElementId::PanelBody;
	if (name == "panelCollapsedArrow") return WidgetElementId::PanelCollapsedArrow;
	if (name == "panelExpandedArrow") return WidgetElementId::PanelExpandedArrow;
	if (name == "textInputBody") return WidgetElementId::TextInputBody;
	if (name == "textInputCaret") return WidgetElementId::TextInputCaret;
	if (name == "textInputSelection") return WidgetElementId::TextInputSelection;
	if (name == "textInputDefaultText") return WidgetElementId::TextInputDefaultText;
	if (name == "sliderBody") return WidgetElementId::SliderBody;
	if (name == "sliderBodyFilled") return WidgetElementId::SliderBodyFilled;
	if (name == "sliderKnob") return WidgetElementId::SliderKnob;
	if (name == "progressBack") return WidgetElementId::ProgressBack;
	if (name == "progressFill") return WidgetElementId::ProgressFill;
	if (name == "tooltipBody") return WidgetElementId::TooltipBody;
	if (name == "popupBody") return WidgetElementId::PopupBody;
	if (name == "dropdownBody") return WidgetElementId::DropdownBody;
	if (name == "dropdownArrow") return WidgetElementId::DropdownArrow;
	if (name == "scrollViewBody") return WidgetElementId::ScrollViewBody;
	if (name == "scrollViewScrollBar") return WidgetElementId::ScrollViewScrollBar;
	if (name == "scrollViewScrollThumb") return WidgetElementId::ScrollViewScrollThumb;
	if (name == "tabGroupBody") return WidgetElementId::TabGroupBody;
	if (name == "tabBodyActive") return WidgetElementId::TabBodyActive;
	if (name == "tabBodyInactive") return WidgetElementId::TabBodyInactive;
	if (name == "viewPaneDockRect") return WidgetElementId::ViewPaneDockRect;
	if (name == "menuBarBody") return WidgetElementId::MenuBarBody;
	if (name == "menuBarItem") return WidgetElementId::MenuBarItem;
	if (name == "menuBody") return WidgetElementId::MenuBody;
	if (name == "menuItemSeparator") return WidgetElementId::MenuItemSeparator;
	if (name == "menuItemBody") return WidgetElementId::MenuItemBody;
	if (name == "menuItemShortcut") return WidgetElementId::MenuItemShortcut;
	if (name == "menuItemCheckMark") return WidgetElementId::MenuItemCheckMark;
	if (name == "menuItemNoCheckMark") return WidgetElementId::MenuItemNoCheckMark;
	if (name == "subMenuItemArrow") return WidgetElementId::SubMenuItemArrow;
	if (name == "errorIcon") return WidgetElementId::MessageBoxIconError;
	if (name == "infoIcon") return WidgetElementId::MessageBoxIconInfo;
	if (name == "questionIcon") return WidgetElementId::MessageBoxIconQuestion;
	if (name == "warningIcon") return WidgetElementId::MessageBoxIconWarning;
	if (name == "selectableBody") return WidgetElementId::SelectableBody;
	if (name == "boxBody") return WidgetElementId::BoxBody;
	if (name == "toolbarBody") return WidgetElementId::ToolbarBody;
	if (name == "toolbarButtonBody") return WidgetElementId::ToolbarButtonBody;
	if (name == "columnsHeaderBody") return WidgetElementId::ColumnsHeaderBody;
	if (name == "comboSliderBody") return WidgetElementId::ComboSliderBody;
	if (name == "comboSliderLeftArrow") return WidgetElementId::ComboSliderLeftArrow;
	if (name == "comboSliderRightArrow") return WidgetElementId::ComboSliderRightArrow;
	if (name == "comboSliderRangeBar") return WidgetElementId::ComboSliderRangeBar;
	if (name == "rotarySliderBody") return WidgetElementId::RotarySliderBody;
	if (name == "rotarySliderMark") return WidgetElementId::RotarySliderMark;
	if (name == "rotarySliderValueDot") return WidgetElementId::RotarySliderValueDot;

	return WidgetElementId::Custom;
}

std::string getPath(const std::string& fname)
{
	size_t pos = fname.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : fname.substr(0, pos);
}

bool getColorFromText(std::string colorText, Color& color)
{
	if (colorText == "white") { color = Color::white; return true; }
	if (colorText == "black") { color = Color::black; return true; }
	if (colorText == "red") { color = Color::red; return true; }
	if (colorText == "darkRed") { color = Color::darkRed; return true; }
	if (colorText == "veryDarkRed") { color = Color::veryDarkRed; return true; }
	if (colorText == "green") { color = Color::green; return true; }
	if (colorText == "darkGreen") { color = Color::darkGreen; return true; }
	if (colorText == "veryDarkGreen") { color = Color::veryDarkGreen; return true; }
	if (colorText == "blue") { color = Color::blue; return true; }
	if (colorText == "yellow") { color = Color::yellow; return true; }
	if (colorText == "magenta") { color = Color::magenta; return true; }
	if (colorText == "cyan") { color = Color::cyan; return true; }
	if (colorText == "darkCyan") { color = Color::darkCyan; return true; }
	if (colorText == "veryDarkCyan") { color = Color::veryDarkCyan; return true; }
	if (colorText == "orange") { color = Color::orange; return true; }
	if (colorText == "darkOrange") { color = Color::darkOrange; return true; }
	if (colorText == "lightGray") { color = Color::lightGray; return true; }
	if (colorText == "gray") { color = Color::gray; return true; }
	if (colorText == "darkGray") { color = Color::darkGray; return true; }
	if (colorText == "sky") { color = Color::sky; return true; }

	return false;
}

bool getColorFromText(const char* colorText, Color& color)
{
	return getColorFromText(std::string(colorText), color);
}

void setThemeElement(
	UiTheme* theme,
	const std::string& themePath,
	const char* styleName,
	WidgetType widgetType,
	WidgetElementId elemId,
	WidgetStateType widgetStateType,
	Json::Value state,
	i32 width, i32 height)
{
	auto imageName = state.get("image", "").asString();
	auto border = state.get("border", 0).asInt();
	auto color = state.get("color", "white").asString();
	auto textColor = state.get("textColor", "white").asString();
	auto fontName = state.get("font", "").asString();
	auto imageFilename = themePath + imageName + ".png";
	auto iter = theme->images.find(imageFilename);
	Image image = 0;
	width = state.get("width", width).asInt();
	height = state.get("height", height).asInt();

	if (iter == theme->images.end())
	{
		auto rawImage = loadRawImage(imageFilename.c_str());
		image = addThemeImage(theme, rawImage);
		deleteRawImage(rawImage);
		theme->images[imageFilename] = (UiImage*)image;
	}
	else
	{
		image = (Image)iter->second;
	}

	auto iterFnt = theme->fonts.find(fontName);
	Font font = 0;

	if (iterFnt != theme->fonts.end())
	{
		font = iterFnt->second;
	}

	u32 r = 0, g = 0, b = 0, a = 255;
	Color bgColor;
	Color txtColor;

	if (!getColorFromText(color, bgColor))
	{
		sscanf(color.c_str(), "%d,%d,%d,%d", &r, &g, &b, &a);
		bgColor = Color((f32)r / 255.0f, (f32)g / 255.0f, (f32)b / 255.0f, (f32)a / 255.0f);
	}

	if (!getColorFromText(textColor, txtColor))
	{
		sscanf(textColor.c_str(), "%d,%d,%d,%d", &r, &g, &b, &a);
		txtColor = Color((f32)r / 255.0f, (f32)g / 255.0f, (f32)b / 255.0f, (f32)a / 255.0f);
	}

	auto& elemState = theme->elements[(u32)elemId].styles[styleName].states[(u32)widgetStateType];
	elemState.image = (UiImage*)image;
	elemState.border = border;
	elemState.color = bgColor;
	elemState.textColor = txtColor;
	elemState.font = (UiFont*)font;
	elemState.width = width;
	elemState.height = height;
}

void setUserElement(
	UiTheme* theme,
	const std::string& themePath,
	const char* styleName,
	const std::string& widgetName,
	const std::string& elemName,
	WidgetStateType widgetStateType,
	Json::Value state,
	i32 width, i32 height)
{
	auto imageName = state.get("image", "").asString();
	auto border = state.get("border", 0).asInt();
	auto color = state.get("color", "white").asString();
	auto textColor = state.get("textColor", "white").asString();
	auto fontName = state.get("font", "").asString();
	auto imageFilename = themePath + imageName + ".png";
	auto iter = theme->images.find(imageFilename);
	Image image = 0;
	width = state.get("width", width).asInt();
	height = state.get("height", height).asInt();

	if (iter == theme->images.end())
	{
		auto rawImage = loadRawImage(imageFilename.c_str());
		image = addThemeImage(theme, rawImage);
		deleteRawImage(rawImage);
		theme->images[imageFilename] = (UiImage*)image;
	}
	else
	{
		image = (Image)iter->second;
	}

	auto iterFnt = theme->fonts.find(fontName);
	Font font = 0;

	if (iterFnt != theme->fonts.end())
	{
		font = iterFnt->second;
	}

	u32 r = 0, g = 0, b = 0, a = 255;
	Color bgColor;
	Color txtColor;

	if (!getColorFromText(color, bgColor))
	{
		sscanf(color.c_str(), "%d,%d,%d,%d", &r, &g, &b, &a);
		bgColor = Color((f32)r / 255.0f, (f32)g / 255.0f, (f32)b / 255.0f, (f32)a / 255.0f);
	}

	if (!getColorFromText(textColor, txtColor))
	{
		sscanf(textColor.c_str(), "%d,%d,%d,%d", &r, &g, &b, &a);
		txtColor = Color((f32)r / 255.0f, (f32)g / 255.0f, (f32)b / 255.0f, (f32)a / 255.0f);
	}

	WidgetElementInfo inf;
	
	inf.image = image;
	inf.border = border;
	inf.color = bgColor;
	inf.textColor = txtColor;
	inf.font = font;
	inf.width = width,
	inf.height = height;

	setThemeUserWidgetElement(
		theme,
		elemName.c_str(),
		widgetStateType,
		inf,
		styleName);
}

Theme loadTheme(const char* filename)
{
	assert(ctx);

	UiTheme* theme = new UiTheme(ctx->settings.defaultAtlasSize);

	ctx->themes.push_back(theme);

	Json::Reader reader;
	Json::Value root;
	auto json = readTextFile(filename);
	bool ok = reader.parse(json, root);
	std::string themePath = getPath(filename) + "/";

	if (!ok)
	{
		printf(reader.getFormatedErrorMessages().c_str());
		deleteTheme(theme);
		return 0;
	}

	Json::Value fonts = root.get("fonts", Json::Value());
	auto fontNames = fonts.getMemberNames();
	
	for (size_t i = 0; i < fontNames.size(); i++)
	{
		auto name = fontNames[i];
		auto fnt = fonts.get(name.c_str(), Json::Value());

		std::string fontFilename = fnt.get("file", "").asString();

		if (fontFilename.find_first_of(':') == std::string::npos)
		{
			fontFilename = themePath + fontFilename;
		}

		auto newFont = createFont(theme, name.c_str(), fontFilename.c_str(), fnt.get("size", 0).asInt());
		theme->fonts[name] = (UiFont*)newFont;
	}

	Json::Value settings = root.get("settings", Json::Value());
	auto settingNames = settings.getMemberNames();

	for (size_t i = 0; i < settingNames.size(); i++)
	{
		auto name = settingNames[i];
		auto val = settings.get(name.c_str(), Json::Value());
		theme->userSettings[name] = val.asString();
	}

	Json::Value widgets = root.get("widgets", Json::Value());
	auto widgetNames = widgets.getMemberNames();

	for (size_t i = 0; i < widgetNames.size(); i++)
	{
		auto widgetName = widgetNames[i];
		auto widget = widgets.get(widgetName.c_str(), Json::Value());
		WidgetType widgetType = getWidgetTypeFromName(widgetName);
		auto widgetMemberNames = widget.getMemberNames();
		auto styles = widget.get("styles", Json::Value());

		auto readElements = [theme, themePath, widgetType](const std::string& styleName, Json::Value& parentElem)
		{
			auto elementNames = parentElem.getMemberNames();
			// read all widget elements
			for (size_t l = 0; l < elementNames.size(); l++)
			{
				auto elemType = getWidgetElementFromName(elementNames[l]);
				auto elem = parentElem.get(elementNames[l], Json::Value());
				auto width = elem.get("width", 0).asInt();
				auto height = elem.get("height", 0).asInt();
				auto elemStates = elem.getMemberNames();

				for (size_t m = 0; m < elemStates.size(); m++)
				{
					auto& stateName = elemStates[m];
					auto elemState = elem.get(stateName, Json::Value());
					auto widgetStateType = WidgetStateType::None;

					if (stateName == "normal")
						widgetStateType = WidgetStateType::Normal;
					else if (stateName == "focused")
						widgetStateType = WidgetStateType::Focused;
					else if (stateName == "pressed")
						widgetStateType = WidgetStateType::Pressed;
					else if (stateName == "hovered")
						widgetStateType = WidgetStateType::Hovered;
					else if (stateName == "disabled")
						widgetStateType = WidgetStateType::Disabled;

					if (widgetStateType != WidgetStateType::None)
						setThemeElement(theme, themePath, styleName.c_str(), widgetType, elemType, widgetStateType, elemState, width, height);
					else
						theme->elements[(u32)elemType].styles[styleName].parameters[stateName] = elemState.asString();
				}
			}
		};

		auto readUserElements = [theme, themePath, widgetName](const std::string& styleName, Json::Value& parentElem)
		{
			auto elementNames = parentElem.getMemberNames();

			// read all widget elements
			for (size_t l = 0; l < elementNames.size(); l++)
			{
				auto& elementName = elementNames[l];
				auto elem = parentElem.get(elementName, Json::Value());
				auto width = elem.get("width", 0).asInt();
				auto height = elem.get("height", 0).asInt();
				auto elemStates = elem.getMemberNames();

				for (size_t m = 0; m < elemStates.size(); m++)
				{
					auto& stateName = elemStates[m];
					auto elemState = elem.get(stateName, Json::Value());
					auto widgetStateType = WidgetStateType::None;

					if (stateName == "normal")
						widgetStateType = WidgetStateType::Normal;
					else if (stateName == "focused")
						widgetStateType = WidgetStateType::Focused;
					else if (stateName == "pressed")
						widgetStateType = WidgetStateType::Pressed;
					else if (stateName == "hovered")
						widgetStateType = WidgetStateType::Hovered;
					else if (stateName == "disabled")
						widgetStateType = WidgetStateType::Disabled;

					if (!theme->userElements[elementName])
						theme->userElements[elementName] = new UiThemeElement();

					if (widgetStateType != WidgetStateType::None)
						setUserElement(theme, themePath, styleName.c_str(), widgetName, elementName, widgetStateType, elemState, width, height);
					else if (elemState.isDouble())
						theme->userElements[elementName]->styles[styleName].parameters[stateName] = elemState.asFloat();
					else if (elemState.isInt())
						theme->userElements[elementName]->styles[styleName].parameters[stateName] = elemState.asInt();
				}
			}
		};

		if (styles.isObject())
		{
			auto styleNames = styles.getMemberNames();

			// read all styles
			for (size_t k = 0; k < styleNames.size(); k++)
			{
				auto styleName = styleNames[k];
				auto styleElem = styles.get(styleName, Json::Value());

				if (widgetType != WidgetType::None)
					readElements(styleName, styleElem);
				else
					readUserElements(styleName, styleElem);
			}
		}
		else
		{
			if (widgetType != WidgetType::None)
				readElements("default", widget);
			else
				readUserElements("default", widget);
		}
	}

	buildTheme(theme);

	return theme;
}

void beginWindow(Window window)
{
	ctx->savedEventType = ctx->event.type;
	ctx->renderer->setZOrder(0);

	for (auto& popup : ctx->popupStack)
	{
		popup.alreadyClosedWithEscape = false;
		popup.alreadyClickedOnSomething = false;
	}

	ctx->hoveringThisWindow = getHoveredWindow() == getWindow();
	ctx->renderer->beginFrame();
}

void endWindow()
{
	ctx->renderer->endFrame();
	ctx->currentWindowIndex++;
	ctx->event.type = ctx->savedEventType;
	ctx->penStack.clear();
	//TODO: make scroll struct stack
}

void beginContainer(const Rect& rect)
{
	ctx->layoutStack.push_back(LayoutState(LayoutType::Container));
	ctx->layoutStack.back().position = rect.topLeft();
	ctx->layoutStack.back().width = rect.width;
	ctx->layoutStack.back().height = rect.height;

	ctx->renderer->pushClipRect(rect);
	ctx->penPosition = { rect.x, rect.y };
	ctx->containerRect = rect;
}

void endContainer()
{
	ctx->renderer->popClipRect();
	ctx->layoutStack.pop_back();
	ctx->currentTabIndex = 0;
	ctx->selectedTabIndex = 0;

	if (!ctx->layoutStack.empty())
	{
		Rect rect = {
			ctx->layoutStack.back().position.x,
			ctx->layoutStack.back().position.y,
			ctx->layoutStack.back().width,
			ctx->layoutStack.back().height };
		ctx->penPosition = { rect.x, rect.y };
		ctx->containerRect = rect;
	}

	ctx->scrollViewDepth = 0;
}

void pushWidgetId(u32 id)
{
	ctx->widgetIdStack.push_back(ctx->currentWidgetId);
	ctx->currentWidgetId = id;
}

void popWidgetId()
{
	if (ctx->widgetIdStack.size())
	{
		ctx->currentWidgetId = ctx->widgetIdStack.back();
		ctx->widgetIdStack.pop_back();
	}
}

void incrementLayerIndex()
{
	ctx->layerIndex++;
	ctx->renderer->incrementZOrder();

	if (ctx->maxLayerIndex < ctx->layerIndex)
	{
		ctx->maxLayerIndex = ctx->layerIndex;
	}
}

u32 decrementLayerIndex()
{
	ctx->layerIndex--;
	ctx->renderer->decrementZOrder();

	return ctx->layerIndex;
}

void decrementWindowMaxLayerIndex()
{
	ctx->maxLayerIndex--;

	if (ctx->maxLayerIndex == ~0)
	{
		ctx->maxLayerIndex = 0;
	}
}

static void computeColumnsPixelSize(LayoutState& parentLayout, LayoutState& layout)
{
	f32 availableWidth = parentLayout.width;
	u32 fullFillerCount = 0;

	// alloc fixed pixel size columns
	for (size_t i = 0; i < layout.columnSizes.size(); i++)
	{
		// if its pixels units
		if (layout.columnSizes[i] > 1.0f)
		{
			availableWidth -= layout.columnPixelSizes[i];
		}

		// if its a column that wants to get equal part of remaining space
		if (layout.columnSizes[i] <= 0.0f)
		{
			fullFillerCount++;
		}
	}

	f32 decrementAvailableWidth = 0;
	// alloc percentage size columns
	for (size_t i = 0; i < layout.columnSizes.size(); i++)
	{
		// if the column width is a percentage (0-1 range)
		if (layout.columnSizes[i] > 0.0f && layout.columnSizes[i] <= 1.0f)
		{
			f32 pixelSize = layout.columnSizes[i] * availableWidth;

			if (!layout.columnMinSizes.empty())
			{
				if (pixelSize < layout.columnMinSizes[i] * ctx->globalScale)
				{
					pixelSize = layout.columnMinSizes[i] * ctx->globalScale;
				}
			}

			layout.columnPixelSizes[i] = round(pixelSize);
			decrementAvailableWidth += round(pixelSize);
		}
	}

	availableWidth -= decrementAvailableWidth;

	// alloc what's left for the fill up size columns
	f32 partitionForFillers = round(availableWidth / (f32)fullFillerCount);

	for (size_t i = 0; i < layout.columnSizes.size(); i++)
	{
		if (layout.columnSizes[i] <= 0.0f)
		{
			layout.columnPixelSizes[i] = partitionForFillers;
		}
	}
}

void beginColumns(u32 columnCount, const f32 widths[], const f32 minWidths[], const f32 maxWidths[])
{
	if (columnCount == 0)
	{
		return;
	}

	LayoutState columns;

	columns.currentColumn = 0;
	columns.type = LayoutType::Columns;

	for (u32 i = 0; i < columnCount; i++)
	{
		if (widths)
		{
			columns.columnSizes.push_back(widths[i]);
			columns.columnPixelSizes.push_back(round(widths[i] * ctx->globalScale));
		}
		else
		{
			columns.columnSizes.push_back(-1);
			columns.columnPixelSizes.push_back(-1);
		}

		if (minWidths)
		{
			columns.columnMinSizes.push_back(round(minWidths[i] * ctx->globalScale));
		}

		if (maxWidths)
		{
			columns.columnMaxSizes.push_back(round(maxWidths[i] * ctx->globalScale));
		}
	}

	ctx->penStack.push_back(ctx->penPosition);
	columns.position = ctx->penPosition;
	computeColumnsPixelSize(ctx->layoutStack.back(), columns);
	ctx->layoutStack.push_back(columns);
	nextColumn();
}

void beginEqualColumns(u32 columnCount, const f32 minWidths[], bool addPadding)
{
	if (columnCount == 0)
	{
		return;
	}

	ctx->penStack.push_back(ctx->penPosition);

	if (addPadding)
		ctx->penPosition.x += ctx->padding * ctx->globalScale;

	LayoutState columns;

	columns.currentColumn = 0;
	columns.type = LayoutType::Columns;
	columns.position = ctx->penPosition;
	columns.width = ctx->layoutStack.back().width;

	if (addPadding)
		columns.width -= ctx->padding * 2.0f * ctx->globalScale;

	f32 columnPercentSize = round(columns.width / (f32)columnCount);

	for (u32 i = 0; i < columnCount; i++)
	{
		columns.columnSizes.push_back(columnPercentSize);
		columns.columnPixelSizes.push_back(columnPercentSize);

		if (minWidths)
		{
			columns.columnMinSizes.push_back(minWidths[i]);
			columns.columnMaxSizes.push_back(columnPercentSize);
		}
	}

	computeColumnsPixelSize(ctx->layoutStack.back(), columns);
	ctx->layoutStack.push_back(columns);
	nextColumn();
}

void beginPaddedColumn()
{
	beginEqualColumns(1, nullptr, true);
}

void endPaddedColumn()
{
	endColumns();
}

void beginTwoColumns()
{
	beginEqualColumns(2);
}

void beginThreeColumns()
{
	beginEqualColumns(3);
}

void beginFourColumns()
{
	beginEqualColumns(4);
}

void beginFiveColumns()
{
	beginEqualColumns(5);
}

void beginSixColumns()
{
	beginEqualColumns(6);
}

void nextColumn()
{
	auto& layout = ctx->layoutStack.back();
	LayoutState newColumn;

	// if we're adding the first column, called from beginColumns()
	if (layout.type == LayoutType::Columns)
	{
		// lets just add the first column
		newColumn.type = LayoutType::Column;
		newColumn.position = ctx->penPosition;
		newColumn.width = layout.columnPixelSizes[layout.currentColumn];
		ctx->layoutStack.push_back(newColumn);
		return;
	}
	else if (layout.type == LayoutType::Column)
	{
		// take out the last column
		ctx->layoutStack.pop_back();
		auto& columnsLayout = ctx->layoutStack.back();

		// wrong placement, here must be a Columns
		if (columnsLayout.type != LayoutType::Columns)
			return;

		// now we're in the Columns layout
		columnsLayout.currentColumn++;

		// set max column Y in columns
		if (columnsLayout.maxPenPositionY < ctx->penPosition.y)
			columnsLayout.maxPenPositionY = ctx->penPosition.y;

		ctx->lastColumnRect.set(ctx->penPosition.x, ctx->penPosition.y, layout.width, ctx->penPosition.y - layout.position.y);

		// if we're done with the columns, move to the parent layout
		if (columnsLayout.currentColumn >= columnsLayout.columnSizes.size())
		{
			f32 maxY = columnsLayout.maxPenPositionY;
			// pop out the columns layout
			ctx->layoutStack.pop_back();
			auto& parentLayout = ctx->layoutStack.back();

			// set max column Y in parent
			if (parentLayout.maxPenPositionY < maxY)
				parentLayout.maxPenPositionY = maxY;

			ctx->penPosition = {
				parentLayout.position.x,
				maxY };

			ctx->penPosition.x = ctx->penStack.back().x;
			ctx->penStack.pop_back();
			return;
		}
		else
		{
			// increment the columns' current column X
			ctx->penPosition = {
				columnsLayout.position.x += columnsLayout.columnPixelSizes[columnsLayout.currentColumn - 1],
				columnsLayout.position.y };

			// lets add a new column
			newColumn.type = LayoutType::Column;
			newColumn.position = ctx->penPosition;
			newColumn.width = columnsLayout.columnPixelSizes[columnsLayout.currentColumn];
			ctx->layoutStack.push_back(newColumn);
		}
	}
}

Rect getColumnRect()
{
	return Rect(ctx->lastColumnRect.x, ctx->lastColumnRect.y, ctx->lastColumnRect.width, ctx->lastColumnRect.height);
}

void endColumns()
{
	nextColumn();
}

void columnHeader(const char* label, f32 width, f32 preferredWidth, f32 minWidth, f32 maxWidth)
{
	auto headerElemState = ctx->theme->getElement(WidgetElementId::ColumnsHeaderBody).normalState();

	ctx->widget.rect = {
		ctx->layoutStack.back().position.x + ctx->padding,
		ctx->layoutStack.back().position.y,
		ctx->layoutStack.back().width - ctx->padding * 2.0f * ctx->globalScale,
		headerElemState.height
	};

	Rect rcText = ctx->widget.rect;

	rcText.x += 5.0f * ctx->globalScale;

	ctx->renderer->cmdSetColor(headerElemState.color);
	ctx->renderer->cmdDrawImageBordered(headerElemState.image, headerElemState.border, ctx->widget.rect, ctx->globalScale);
	ctx->renderer->cmdSetColor(headerElemState.textColor);
	ctx->renderer->cmdSetFont(headerElemState.font);
	ctx->renderer->cmdDrawTextInBox(label, rcText, HAlignType::Left, VAlignType::Center);

	addWidgetItem(ctx->widget.rect.height);
	ctx->currentWidgetId++;
}

void pushPadding(f32 newPadding)
{
	ctx->paddingStack.push_back(ctx->padding);
	ctx->padding = newPadding;
}

void popPadding()
{
	if (!ctx->paddingStack.empty())
	{
		ctx->padding = ctx->paddingStack.back();
		ctx->paddingStack.pop_back();
	}
}

void pushSpacing(f32 newSpacing)
{
	ctx->spacingStack.push_back(ctx->spacing);
	ctx->spacing = newSpacing;
}

void popSpacing()
{
	if (!ctx->spacingStack.empty())
	{
		ctx->spacing = ctx->spacingStack.back();
		ctx->spacingStack.pop_back();
	}
}

f32 getSpacing()
{
	return ctx->spacing;
}

f32 getPadding()
{
	return ctx->padding;
}

void setGlobalScale(f32 scale)
{
	ctx->globalScale = scale;

	if (ctx->theme)
	{
		ctx->theme->fontCache->rescaleFonts(scale);
		ctx->theme->atlas->repackImages();
	}
}

f32 getGlobalScale()
{
	return ctx->globalScale;
}

void pushTint(const Color& color, TintColorType type)
{
	switch (type)
	{
	case hui::TintColorType::Body:
	case hui::TintColorType::Text:
		ctx->tintStack[(u32)type].push_back(color);
		ctx->tint[(u32)type] = color;
		break;
	case hui::TintColorType::All:
		for (int i = 0; i < (int)TintColorType::Count; i++)
		{
			ctx->tintStack[i].push_back(color);
			ctx->tint[i] = color;
		}
		break;
	default:
		break;
	}
}

void popTint(TintColorType type)
{
	switch (type)
	{
	case hui::TintColorType::Body:
	case hui::TintColorType::Text:
		if (ctx->tintStack[(u32)type].empty())
			return;

		ctx->tintStack[(u32)type].pop_back();

		if (!ctx->tintStack[(u32)type].empty())
			ctx->tint[(u32)type] = ctx->tintStack[(u32)type].back();
		else
			ctx->tint[(u32)type] = Color::white;
		break;
	case hui::TintColorType::All:
		for (int i = 0; i < (int)TintColorType::Count; i++)
		{
			if (ctx->tintStack[i].empty())
			{
				ctx->tint[i] = Color::white;
				continue;
			}

			ctx->tintStack[i].pop_back();

			if (!ctx->tintStack[i].empty())
				ctx->tint[i] = ctx->tintStack[i].back();
			else
				ctx->tint[i] = Color::white;
		}
		break;
	default:
		break;
	}
}

bool isHovered()
{
	return ctx->widget.hovered;
}

bool isFocused()
{
	return ctx->widget.focused;
}

bool isPressed()
{
	return ctx->widget.pressed;
}

bool isClicked()
{
	return ctx->widget.clicked;
}

bool isVisible()
{
	return ctx->widget.visible;
}

bool isChangeEnded()
{
	return ctx->widget.changeEnded;
}

u32 getWidgetId()
{
	return ctx->currentWidgetId;
}

Point getMousePosition()
{
	return ctx->event.mouse.point;
}

Point getPenPosition()
{
	return ctx->penPosition;
}

void setPenPosition(const Point& penPosition)
{
	ctx->penPosition = penPosition;
}

bool wantsToDragDrop()
{
	if (ctx->event.type == InputEvent::Type::MouseDown
		&& !ctx->dragDropState.draggingIntent
		&& !ctx->dragDropState.dragging
		&& ctx->widget.hovered)
	{
		ctx->dragDropState.draggingIntent = true;
		ctx->dragDropState.lastMousePos = ctx->event.mouse.point;
		ctx->dragDropState.widgetId = ctx->currentWidgetId;
	}

	const int dragStartPixelDistance = 4;

	if (ctx->dragDropState.draggingIntent
		&& !ctx->dragDropState.dragging
		&& ctx->currentWidgetId == ctx->dragDropState.widgetId
		&& ctx->dragDropState.lastMousePos.getDistance(ctx->event.mouse.point) >= dragStartPixelDistance)
	{
		ctx->dragDropState.dragging = true;
		return true;
	}

	if (ctx->event.type == InputEvent::Type::MouseUp
		&& (ctx->dragDropState.draggingIntent ||
			ctx->dragDropState.dragging))
	{
		ctx->dragDropState.draggingIntent = false;
		ctx->dragDropState.dragging = false;
		setMouseCursor(MouseCursorType::Arrow);
	}

	return false;
}

void setDragDropMouseCursor(MouseCursor dropAllowedCursor)
{
	ctx->dragDropState.dropAllowedCursor = dropAllowedCursor;
}

void beginDragDrop(u32 dragObjectType, void* dragObject)
{
	ctx->dragDropState.dragObject = dragObject;
	ctx->dragDropState.dragObjectType = dragObjectType;
	ctx->dragDropState.begunDragging = true;

	// we want other widgets to get hovered, so kill current one
	ctx->widget.focusedWidgetPressed = false;
}

void endDragDrop()
{
	ctx->dragDropState.dragging = false;
	ctx->dragDropState.begunDragging = false;
	ctx->dragDropState.draggingIntent = false;
	ctx->dragDropState.dragObject = nullptr;
	ctx->dragDropState.dragObjectType = 0;
}

void allowDragDrop()
{
	ctx->dragDropState.allowDrop = true;

	if (ctx->dragDropState.begunDragging)
	{
		if (ctx->widget.hovered)
		{
			ctx->dragDropState.foundDropTarget = true;
		}
	}
}

void disallowDragDrop()
{
	ctx->dragDropState.allowDrop = false;
}

bool droppedOnWidget()
{
	if (ctx->dragDropState.begunDragging
		&& ctx->hoveringThisWindow
		&& getFocusedWindow() != getWindow())
	{
		ctx->inputProvider->raiseWindow(getWindow());
	}

	if (ctx->dragDropState.begunDragging
		&& ctx->widget.hovered
		&& ctx->event.type == InputEvent::Type::MouseUp
		&& ctx->dragDropState.allowDrop)
	{
		return true;
	}

	return false;
}

void* getDragDropObject()
{
	return ctx->dragDropState.dragObject;
}

u32 getDragDropObjectType()
{
	return ctx->dragDropState.dragObjectType;
}

}
