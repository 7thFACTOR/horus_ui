#include <stdlib.h>
#include <assert.h>
#include <algorithm>
#include "horus.h"
#include "types.h"
#include "theme.h"
#include "atlas.h"
#include "context.h"
#include "util.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font_cache.h"

#ifdef _WINDOWS
#include <windows.h>
#endif

namespace hui
{
u32 Color::getRgba() const
{
	u32 col = 0;
	u8 *color = (u8*)&col;

	color[0] = (r > 1.0f ? 1.0f : r) * 255;
	color[1] = (g > 1.0f ? 1.0f : g) * 255;
	color[2] = (b > 1.0f ? 1.0f : b) * 255;
	color[3] = (a > 1.0f ? 1.0f : a) * 255.0f;

	return col;
}

u32 Color::getArgb() const
{
	u32 col = 0;
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

HContext createContext(struct ContextSettings& settings)
{
	UiContext* context = new UiContext();

	context->settings = settings;
	context->providers = &settings.providers;
	setContext((HContext)context);

#ifdef _WIN32
	SetProcessDPIAware();
#endif

	return context;
}

void setContext(HContext context)
{
	ctx = (UiContext*)context;
}

HContext getContext()
{
	return ctx;
}

void deleteContext(HContext context)
{
	delete (HContext*)context;
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

	f32 width = (ctx->widget.sameLine ? ctx->widget.width : (ctx->widget.width != 0 ? ctx->widget.width : ctx->layoutStack.back().width)) - ctx->padding * 2.0f * ctx->globalScale;
	f32 verticalOffset = 0;
	const f32 totalHeight = ctx->spacing * ctx->globalScale + height;

	if (!ctx->widget.sameLine)
	{
		ctx->penPosition.x = ctx->layoutStack.back().position.x;
	}
	else
	{
		if (ctx->sameLineInfo[ctx->sameLineInfoIndex].computeHeight)
		{
			ctx->sameLineInfo[ctx->sameLineInfoIndex].lineHeight = round(fmax(totalHeight, ctx->sameLineInfo[ctx->sameLineInfoIndex].lineHeight));
		}

		verticalOffset = (ctx->sameLineInfo[ctx->sameLineInfoIndex].lineHeight - totalHeight) / 2.0f;
	}

	ctx->widget.rect.set(
		round(ctx->penPosition.x + ctx->padding * ctx->globalScale),
		round(ctx->penPosition.y + verticalOffset),
		width,
		height);

	if (!ctx->widget.sameLine)
	{
		ctx->penPosition.y += totalHeight;
		ctx->penPosition.y = round(ctx->penPosition.y);
	}
	else
	{
		ctx->penPosition.x += width + ctx->padding * ctx->globalScale;
	}
}

void setFocusable()
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
	ctx->sameLineInfoIndex = 0;
	ctx->sameLineInfoCount = 0;

	if (ctx->pruneUnusedTextTime >= ctx->settings.textCachePruneIntervalSec)
	{
		ctx->textCache->pruneUnusedText();
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
		ctx->providers->input->setCursor(ctx->mouseCursor);
	}
	else if (ctx->customMouseCursor)
	{
		ctx->providers->input->setCustomCursor(ctx->customMouseCursor);
	}

	for (u32 i = 0; i < ctx->sameLineInfoCount; i++)
	{
		ctx->sameLineInfo[i].computeHeight = false;
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
		ctx->tooltip.closeTooltipPopup = false;
	}
	else if (!ctx->widget.hoveredWidgetId && ctx->tooltip.show && ctx->tooltip.widgetId)
	{
		ctx->tooltip.show = false;
		ctx->tooltip.widgetId = 0;
		ctx->tooltip.closeTooltipPopup = true;
	}

	if (ctx->tooltip.show)
	{
		// track mouse pos
		ctx->tooltip.position = ctx->providers->input->getMousePosition();
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
	return ctx->providers->input->copyToClipboard(text);
}

bool pasteFromClipboard(char* outText, u32 maxTextSize)
{
	return ctx->providers->input->pasteFromClipboard(outText, maxTextSize);
}

const InputEvent& getInputEvent()
{
	return ctx->event;
}

void setMouseCursor(MouseCursorType type)
{
	ctx->mouseCursor = type;
}

HMouseCursor createMouseCursor(Rgba32* pixels, u32 width, u32 height, u32 hotSpotX, u32 hotSpotY)
{
	return ctx->providers->input->createCustomCursor(pixels, width, height, hotSpotX, hotSpotY);
}

HMouseCursor loadMouseCursor(const char* imageFilename, u32 hotSpotX, u32 hotSpotY)
{
	auto img = hui::loadImageData(imageFilename);
	auto cur = hui::createMouseCursor((Rgba32*)img.pixels, img.width, img.height, hotSpotX, hotSpotY);
	deleteImageData(img);

	return cur;
}

void deleteMouseCursor(HMouseCursor cursor)
{
	ctx->providers->input->deleteCustomCursor(cursor);
}

void setMouseCursor(HMouseCursor cursor)
{
	ctx->mouseCursor = MouseCursorType::Custom;
	ctx->customMouseCursor = cursor;
}

void setWindow(HWindow window)
{
	ctx->providers->input->setCurrentWindow(window);

	auto rect = getWindowRect(window);

	ctx->renderer->setWindowSize({ rect.width, rect.height });
	ctx->providers->gfx->setViewport(
		{ rect.width, rect.height },
		{ 0, 0, rect.width, rect.height });
}

HWindow getWindow()
{
	return ctx->providers->input->getCurrentWindow();
}

HWindow getFocusedWindow()
{
	return ctx->providers->input->getFocusedWindow();
}

HWindow getHoveredWindow()
{
	return ctx->providers->input->getHoveredWindow();
}

HWindow getMainWindow()
{
	return ctx->providers->input->getMainWindow();
}

HWindow createWindow(
	const char* title, u32 width, u32 height,
	WindowFlags flags,
	Point customPosition)
{
	auto wnd = ctx->providers->input->createWindow(title, width, height, flags, customPosition);

	if (!ctx->renderer)
	{
		ctx->initializeGraphics();
	}

	return wnd;
}

void setWindowTitle(HWindow window, const char* title)
{
	ctx->providers->input->setWindowTitle(window, title);
}

void setWindowRect(HWindow window, const Rect& rect)
{
	ctx->providers->input->setWindowRect(window, rect);
}

Rect getWindowRect(HWindow window)
{
	return ctx->providers->input->getWindowRect(window);
}

Rect getWindowClientRect(HWindow window)
{
	auto rect = ctx->providers->input->getWindowRect(window);

	rect.x = 0;
	rect.y = 0;

	return rect;
}

void presentWindow(HWindow window)
{
	ctx->providers->input->presentWindow(window);
}

void destroyWindow(HWindow window)
{
	ctx->providers->input->destroyWindow(window);
}

void showWindow(HWindow window)
{
	ctx->providers->input->showWindow(window);
}

void hideWindow(HWindow window)
{
	ctx->providers->input->hideWindow(window);
}

void riseWindow(HWindow window)
{
	ctx->providers->input->raiseWindow(window);
}

void maximizeWindow(HWindow window)
{
	ctx->providers->input->maximizeWindow(window);
}

void minimizeWindow(HWindow window)
{
	ctx->providers->input->minimizeWindow(window);
}

WindowState getWindowState(HWindow window)
{
	return ctx->providers->input->getWindowState(window);
}

void setCapture(HWindow window)
{
	ctx->providers->input->setCapture(window);
}

void releaseCapture()
{
	ctx->providers->input->releaseCapture();
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
	return ctx->providers->input->mustQuit();
}

bool wantsToQuit()
{
	return ctx->providers->input->wantsToQuit();
}

void cancelQuitApplication()
{
	ctx->providers->input->cancelQuitApplication();
}

void quitApplication()
{
	ctx->providers->input->quitApplication();
}

void shutdown()
{
	assert(ctx);

	if (ctx->providers->gfx)
		ctx->providers->gfx->shutdown();

	if (ctx->providers->input)
		ctx->providers->input->shutdown();
}

HImage loadImage(const char* filename)
{
	ImageData imgData = loadImageData(filename);

	if (!imgData.pixels)
		return nullptr;

	if (imgData.bpp != 32)
	{
		printf("ERROR: Only 32bpp images allowed (%s)\n", filename);
		return nullptr;
	}

	HImage img = createImage((Rgba32*)imgData.pixels, imgData.width, imgData.height);

	deleteImageData(imgData);

	return img;
}

HImage createImage(Rgba32* pixels, u32 width, u32 height)
{
	auto img = ctx->theme->addImage(pixels, width, height);
	ctx->theme->packAtlas();
	return img;
}

Point getImageSize(HImage image)
{
	UiImage* img = (UiImage*)image;

	return { img->rect.width, img->rect.height };
}

void updateImagePixels(HImage image, Rgba32* pixels)
{
	UiImage* img = (UiImage*)image;

	//TODO: check if image is rotated
	img->atlasTexture->textureArray->updateRectData(img->atlasTexture->textureIndex, img->rect, pixels);
}

ImageData loadImageData(const char* filename)
{
	ImageData imgData;

	if (!ctx->providers->image->loadImage(filename, imgData))
	{
		return ImageData();
	}

	return imgData;
}

void deleteImage(HImage image)
{
	UiImage* img = (UiImage*)image;
	img->atlas->deleteImage(img);
}

void deleteImageData(ImageData& image)
{
	delete[] image.pixels;
	image.pixels = nullptr;
	image.width = 0;
	image.height = 0;
	image.bpp = 0;
}

HAtlas createAtlas(u32 width, u32 height)
{
	return new UiAtlas(width, height);
}

void deleteAtlas(HAtlas atlas)
{
	delete (UiAtlas*)atlas;
}

HImage addAtlasImage(HAtlas atlas, const ImageData& img)
{
	UiAtlas* atlasPtr = (UiAtlas*)atlas;

	return atlasPtr->addImage((const Rgba32*)img.pixels, img.width, img.height);
}

bool packAtlas(HAtlas atlas, u32 border)
{
	UiAtlas* atlasPtr = (UiAtlas*)atlas;

	return atlasPtr->pack(border);
}

void processInputEvents()
{
	ctx->event.type = InputEvent::Type::None;
	clearInputEventQueue();
	ctx->providers->input->processEvents();
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

HTheme createTheme(u32 atlasTextureSize)
{
	UiTheme* theme = new UiTheme(atlasTextureSize);

	ctx->themes.push_back(theme);

	return theme;
}

void setThemeUserSetting(HTheme theme, const char* name, const char* value)
{
	((UiTheme*)theme)->userSettings[name] = value;
}

const char* getThemeUserSetting(HTheme theme, const char* name)
{
	auto iter = ((UiTheme*)theme)->userSettings.find(name);

	if (iter == ((UiTheme*)theme)->userSettings.end())
		return "";

	return iter->second.c_str();
}

HImage addThemeImage(HTheme theme, const ImageData& img)
{
	UiTheme* themePtr = (UiTheme*)theme;

	return themePtr->addImage((const Rgba32*)img.pixels, img.width, img.height);
}

HImage getThemeImage(HTheme theme, const char* imageName)
{
	UiTheme* themePtr = (UiTheme*)theme;

	auto iter = themePtr->images.find(imageName);

	if (iter != themePtr->images.end())
		return iter->second;

	return nullptr;
}

void setThemeImage(HTheme theme, const char* imageName, HImage image)
{
	UiTheme* themePtr = (UiTheme*)theme;

	auto iter = themePtr->images.find(imageName);

	if (iter != themePtr->images.end())
	{
		iter->second = (UiImage*)image;
		return;
	}

	themePtr->images[imageName] = (UiImage*)image;
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
		ctx->theme->elements[(u32)WidgetElementId::PopupBehind].setStyle(styleName);
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
		ctx->theme->elements[(u32)WidgetElementId::ViewPaneDockDialRect].setStyle(styleName);
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
		ctx->theme->elements[(u32)WidgetElementId::ToolbarSeparatorVerticalBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ToolbarSeparatorHorizontalBody].setStyle(styleName);
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

void setDefaultWidgetStyle(WidgetType widgetType)
{
	setWidgetStyle(widgetType, "default");
}

void setUserWidgetElementStyle(const char* elementName, const char* styleName)
{
	ctx->theme->userElements[elementName]->setStyle(styleName);
}

void buildTheme(HTheme theme)
{
	UiTheme* themePtr = (UiTheme*)theme;

	themePtr->packAtlas();
	themePtr->setDefaultWidgetStyle();
}

void setThemeWidgetElement(
	HTheme theme,
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
	HTheme theme,
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

void setTheme(HTheme theme)
{
	ctx->theme = (UiTheme*)theme;
}

HTheme getTheme()
{
	return ctx->theme;
}

void deleteTheme(HTheme theme)
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

void setThemeWidgetElementParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const char* paramValue)
{
	((UiTheme*)theme)->elements[(int)elementId].styles[styleName].parameters[paramName] = paramValue;
}

const char* getThemeWidgetElementStringParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const char* defaultValue)
{
	auto& style = ((UiTheme*)theme)->elements[(int)elementId].styles[styleName];

	auto iter = style.parameters.find(paramName);

	if (iter == style.parameters.end())
		return defaultValue;

	return iter->second.c_str();
}

f32 getThemeWidgetElementFloatParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, f32 defaultValue)
{
	auto& style = ((UiTheme*)theme)->elements[(int)elementId].styles[styleName];

	return style.getParameterValue(paramName, defaultValue);
}

const Color& getThemeWidgetElementColorParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const Color& defaultValue)
{
	auto& style = ((UiTheme*)theme)->elements[(int)elementId].styles[styleName];

	return style.getParameterValue(paramName, defaultValue);
}

void setThemeUserWidgetElementParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const char* paramValue)
{
	((UiTheme*)theme)->userElements[userElementName]->styles[styleName].parameters[paramName] = paramValue;
}

const char* getThemeUserWidgetElementStringParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const char* defaultValue)
{
	auto& style = ((UiTheme*)theme)->userElements[userElementName]->styles[styleName];

	auto iter = style.parameters.find(paramName);

	if (iter == style.parameters.end())
		return defaultValue;

	return iter->second.c_str();
}

f32 getThemeUserWidgetElementFloatParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, f32 defaultValue)
{
	auto& style = ((UiTheme*)theme)->userElements[userElementName]->styles[styleName];

	return style.getParameterValue(paramName, defaultValue);
}

const Color& getThemeUserWidgetElementColorParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const Color& defaultValue)
{
	auto& style = ((UiTheme*)theme)->userElements[userElementName]->styles[styleName];

	return style.getParameterValue(paramName, defaultValue);
}


HFont createThemeFont(HTheme theme, const char* name, const char* fontFilename, u32 faceSize)
{
	return (HFont)((UiTheme*)theme)->fontCache->createFont(name, fontFilename, faceSize * ctx->globalScale, false);
}

void releaseThemeFont(HTheme theme, HFont font)
{
	((UiTheme*)theme)->fontCache->releaseFont((UiFont*)font);
}

HFont getThemeFont(HTheme theme, const char* themeFontName)
{
	UiTheme* themeObj = (UiTheme*)theme;

	return themeObj->fonts[themeFontName];
}

HFont getFont(const char* themeFontName)
{
	return getThemeFont(getTheme(), themeFontName);
}

void beginWindow(HWindow window)
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
	ctx->widget.sameLine = false;
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

void pushWidgetLoop(u32 loopMaxCount)
{
	UiContext::WidgetLoopInfo li;

	li.previousId = ctx->currentWidgetId;
	li.startId = ctx->widgetLoopStack.size() ? ctx->widgetLoopStack.back().startId + ctx->widgetLoopStack.back().maxCount : ctx->settings.widgetLoopStartId;
	li.maxCount = loopMaxCount == ~0 ? ctx->settings.widgetLoopMaxCount : loopMaxCount;
	ctx->currentWidgetId = li.startId;
	ctx->widgetLoopStack.push_back(li);
}

void popWidgetLoop()
{
	if (ctx->widgetLoopStack.size())
	{
		ctx->currentWidgetId = ctx->widgetLoopStack.back().previousId;
		ctx->widgetLoopStack.pop_back();
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

void setDragDropMouseCursor(HMouseCursor dropAllowedCursor)
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
		ctx->providers->input->raiseWindow(getWindow());
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
