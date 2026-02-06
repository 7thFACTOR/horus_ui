#include <stdlib.h>
#include <algorithm>
#include "context.h"
#include "theme.h"
#include "atlas.h"
#include "util.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font_cache.h"
#include "docking.h"

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
	return
	{
		(f32)rand() / (f32)RAND_MAX,
		(f32)rand() / (f32)RAND_MAX,
		(f32)rand() / (f32)RAND_MAX,
		1
	};
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

HContext createContext(struct Settings& settings)
{
	Context* context = new Context();

	HORUS_ASSERT(context);
	context->settings = settings;
	context->providers = &settings.providers;

	if (context->settings.dockingStyle == DockingGuidesStyle::Auto)
	{
#ifdef _WINDOWS
		context->settings.dockingStyle = DockingGuidesStyle::NativeWindows;
#elif _LINUX
		context->settings.dockingStyle = DockingGuidesStyle::InsideNativeWindows;
#endif
	}

	return context;
}

void setContext(HContext context)
{
	HORUS_ASSERT(context);
	ctx = (Context*)context;
}

HContext getContext()
{
	return ctx;
}

void deleteContext(HContext context)
{
	HORUS_ASSERT(context);
	delete (Context*)context;
}

Settings& getSettings()
{
	return ctx->settings;
}

void initializeRenderer()
{
	ctx->initializeRenderer();
}

void addRenderCallback(RenderCallback callback)
{
	ctx->renderer->cmdCallback(callback);
}

void clearBackground(const Color& color)
{
	ctx->renderer->cmdClearBackground(color);
}

void setNextDisabled()
{
	ctx->widget.nextDisabled = true;
}

void setNextFocused()
{
	ctx->widget.hovered = true;
	ctx->widget.pressed = true;
	ctx->widget.focused = true;
	ctx->focusChanged = true;
}

void addWidget(f32 height)
{
	ctx->widget.changeEnded = false;
	height = round((height + getWidgetPadding().y * 2.0f) * ctx->scale);

	// next width has priority over custom width
	if (ctx->widget.hasNextWidth)
	{
		ctx->widget.width = (ctx->widget.nextWidth + getWidgetPadding().x * 2.0f) * ctx->scale;
	}
	else
	{
		ctx->widget.width = ctx->widget.hasCustomWidth ? ctx->widget.customWidth * ctx->scale : ctx->layout.width;
	}

	// if width is under 1, then it's a percentage of the layout width
	// otherwise it's a fixed pixel width
	auto pixelWidth = ctx->widget.width > 1 ? ctx->widget.width : ctx->widget.width * ctx->layout.width;
	f32 spacing = ctx->spacing * ctx->scale;

	if (!ctx->sameLine.enabled)
	{
		auto oldY = ctx->position.y;

		// new line after same line
		if (ctx->sameLine.wasEnabled)
		{
			ctx->position.x = ctx->sameLine.currentX;
			ctx->sameLine.wasEnabled = false;
			// add the previous line max height
			ctx->position.y += ctx->sameLine.maxHeight;
			ctx->sameLine.maxHeight = 0;
			ctx->sameLine.currentY = ctx->position.y + (ctx->layout.firstWidgetInLayout ? 0 : spacing);
		}
		else
		{
			ctx->sameLine.currentX = ctx->position.x;
			ctx->sameLine.currentY = oldY + (ctx->layout.firstWidgetInLayout ? 0 : spacing);
		}

		if (!ctx->layout.firstWidgetInLayout)
		{
			ctx->position.y += spacing;
		}
		else
		{
			ctx->layout.firstWidgetInLayout = false;
		}

		ctx->position.y = round(ctx->position.y);
	}
	else
	{
		ctx->position.y = ctx->sameLine.currentY;

		if (!ctx->sameLine.wasEnabled)
		{
			ctx->position.x += ctx->widget.rect.width + ctx->sameLine.spacing * ctx->scale;
		}
	}

	ctx->widget.width = pixelWidth;
	ctx->widget.rect.set(
		round(ctx->position.x),
		round(ctx->position.y),
		pixelWidth,
		height);

	if (!ctx->sameLine.enabled)
	{
		ctx->position.y += height;
		ctx->position.y = round(ctx->position.y);
		ctx->position.x = ctx->sameLine.currentX;
	}
	else
	{
		ctx->position.x += pixelWidth + ctx->sameLine.spacing * ctx->scale;
		ctx->sameLine.wasEnabled = true;
		ctx->sameLine.maxHeight = std::max(ctx->sameLine.maxHeight, height);
	}

	// Track maximum X position for horizontal scrolling content width
	if (ctx->scrollViewDepth > 0 && ctx->layout.type == LayoutType::ScrollView)
	{
		f32 widgetRightEdge = ctx->widget.rect.right();

		if (widgetRightEdge > ctx->scrollViewStack[ctx->scrollViewDepth - 1].maxContentX)
		{
			ctx->scrollViewStack[ctx->scrollViewDepth - 1].maxContentX = widgetRightEdge;
		}
	}

	ctx->widget.hasNextWidth = false;
	ctx->widget.hasCustomWidth = false;
	ctx->sameLine.enabled = false;
}

void setFocusable()
{
	if (ctx->widget.focusedId == ctx->id)
	{
		ctx->widget.focusedWidgetRect = ctx->widget.rect;
	}

	//TODO: not working since widget id not incremental
	//if (!ctx->widget.nextFocusableId
	//	&& ctx->id > ctx->widget.focusedId)
	//{
	//	ctx->widget.nextFocusableId = ctx->id;
	//}
}

bool viewportImageFitSize(
	f32 imageWidth, f32 imageHeight,
	f32 viewWidth, f32 viewHeight,
	f32& outNewWidth, f32& outNewHeight,
	bool ignoreHeight, bool ignoreWidth)
{
	f32 aspectRatio = 1.0f;

	outNewWidth = imageWidth;
	outNewHeight = imageHeight;

	if (imageWidth <= viewWidth
		&& imageHeight <= viewHeight)
	{
		return false;
	}

	if (outNewWidth >= viewWidth && !ignoreWidth)
	{
		if (outNewWidth < 0.0001f)
			outNewWidth = 0.0001f;

		aspectRatio = (f32)viewWidth / outNewWidth;
		outNewWidth = viewWidth;
		outNewHeight *= aspectRatio;
	}

	if (outNewHeight >= viewHeight && !ignoreHeight)
	{
		if (outNewHeight < 0.0001f)
			outNewHeight = 0.0001f;

		aspectRatio = (f32)viewHeight / outNewHeight;
		outNewHeight = viewHeight;
		outNewWidth *= aspectRatio;
	}

	return true;
}

void beginFrame()
{
	if (ctx->textInput.id)
	{
		ctx->textInput.textChanged = false;
		ctx->textInput.processEvent(ctx->event);
	}

	if (ctx->event.window)
	{
		if (ctx->event.type == InputEvent::Type::WindowMouseEnter && ctx->event.window != ctx->docking.dragIndicatorNativeWindow)
			ctx->lastHoveredNativeWindow = ctx->event.window;

		if (ctx->event.type == InputEvent::Type::WindowMouseLeave && ctx->event.window != ctx->docking.dragIndicatorNativeWindow)
			ctx->lastHoveredNativeWindow = 0;

		if (ctx->event.type == InputEvent::Type::MouseDown
			|| ctx->event.type == InputEvent::Type::MouseUp
			|| ctx->event.type == InputEvent::Type::MouseMove
			|| ctx->event.type == InputEvent::Type::MouseWheel)
		{
			if (ctx->event.window && ctx->event.window != ctx->docking.dragIndicatorNativeWindow)
			{
				ctx->lastHoveredNativeWindow = ctx->event.window;
				ctx->mousePosition = ctx->event.mouse.point;
			}
		}
	}
	else
	{
		// we need mouse position updated even if we're out of the window
		// use last hovered window as reference for the global position to subtract from
		if (ctx->lastHoveredNativeWindow)
		{
			auto wndPos = HORUS_INPUT->getWindowPosition(ctx->lastHoveredNativeWindow);
			auto absMousePos = HORUS_INPUT->getAbsoluteMousePosition();
			ctx->mousePosition = absMousePos - wndPos;
		}
	}

	if (ctx->event.type == InputEvent::Type::Key
		&& ctx->event.key.code == KeyCode::Tab
		&& !!(ctx->event.key.modifiers, KeyModifiers::Shift)
		&& ctx->event.key.down)
	{
		//TODO: wont work now
		//ctx->widget.focusedId--;
		ctx->focusChanged = true;

		//if (ctx->widget.focusedId < 0)
		//{
		//	ctx->widget.focusedId = 0;
		//}
	}
	else if (ctx->event.type == InputEvent::Type::Key
		&& ctx->event.key.code == KeyCode::Tab
		&& ctx->event.key.down)
	{
		ctx->widget.focusedId = ctx->widget.nextFocusableId;
		ctx->focusChanged = true;

		//TODO: doesnt work anymore
		//if (ctx->widget.focusedId > ctx->maxWidgetId)
		//{
		//	ctx->widget.focusedId = 1;
		//}
	}

	ctx->mustRedraw = false;
	ctx->skipRenderAndInput = false;
	ctx->widget.disabled = false;
	ctx->layerIndex = 0;
	ctx->widget.nextFocusableId = 0;
	ctx->menuDepth = 0;
	ctx->popupIndex = 0;
	ctx->menuItemChosen = false;
	ctx->dragDrop.foundDropTarget = false;
	ctx->widget.hoveredId = 0;
	ctx->widget.hoveredType = WidgetType::None;
	ctx->widget.changeEnded = false;
	ctx->frameCount++;
	ctx->totalTime += ctx->deltaTime;
	ctx->pruneUnusedTextTime += ctx->deltaTime;
	ctx->sameLine.wasEnabled = false;
	//ctx->sameLineInfoIndex = 0;
	//ctx->sameLineInfoCount = 0;
	ctx->fontStack.clear();

	ctx->padding[(i32)PaddingType::Layout] = ctx->settings.defaultLayoutPadding;
	ctx->padding[(i32)PaddingType::ScrollView] = ctx->settings.defaultScrollViewPadding;
	ctx->padding[(i32)PaddingType::Widget] = ctx->settings.defaultWidgetPadding;

	if (ctx->pruneUnusedTextTime >= ctx->settings.textCachePruneIntervalSec)
	{
		ctx->textCache->pruneUnusedText();
		ctx->pruneUnusedTextTime = 0;
	}

	ctx->savedEventType = ctx->event.type;

	for (auto& popup : ctx->popupStack)
	{
		popup.alreadyClosedWithEscape = false;
		popup.alreadyClickedOnSomething = false;
	}

	ctx->alreadyClickedOnSomething = false;
	setMouseCursor(MouseCursorType::Arrow);
	updateDockingSystem();
}

void deferredDeleteObjects()
{
	for (auto& wnd : ctx->docking.windowsToDelete)
	{
		auto iter = ctx->docking.windows.find(wnd->id);

		if (iter != ctx->docking.windows.end())
		{
			ctx->docking.windows.erase(iter);
		}

		ctx->docking.closedWindowsRects[wnd->id] = wnd->dockNode->rect;

		auto iterWnd = ctx->docking.windowsDockNodeAssignments.find(wnd->id);

		if (iterWnd != ctx->docking.windowsDockNodeAssignments.end())
		{
			ctx->docking.windowsDockNodeAssignments.erase(iterWnd);
		}

		delete wnd;
	}

	for (auto& dn : ctx->docking.dockNodesToDelete)
	{
		delete dn;
	}

	for (auto& wnd : ctx->docking.nativeWindowsToDelete)
	{
		auto iter = ctx->docking.rootNativeWindowDockNodes.find(wnd);

		if (iter != ctx->docking.rootNativeWindowDockNodes.end())
		{
			ctx->docking.rootNativeWindowDockNodes.erase(iter);
		}

		auto iter2 = std::find(ctx->nativeWindows.begin(), ctx->nativeWindows.end(), wnd);

		if (iter2 != ctx->nativeWindows.end())
		{
			ctx->nativeWindows.erase(iter2);
		}

		HORUS_INPUT->destroyWindow(wnd);
	}

	ctx->docking.dockNodesToDelete.clear();
	ctx->docking.windowsToDelete.clear();
	ctx->docking.nativeWindowsToDelete.clear();
}

void endFrame()
{
	//TODO: check stacks to see if there is are items on them
	// the stacks should be empty, otherwise push/pop count not matching

	if (ctx->theme->atlas->packWithLastUsedParams())
		skipThisFrame();

	ctx->focusChanged = false;
	ctx->mouseMoved = false;

	if (ctx->dragDrop.begunDragging
		&& ctx->event.type == InputEvent::Type::MouseUp)
	{
		ctx->dragDrop.begunDragging = false;
	}

	if (ctx->dragDrop.begunDragging)
	{
		if (ctx->dragDrop.foundDropTarget)
		{
			setMouseCursor(ctx->dragDrop.dropAllowedCursor);
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

	//for (u32 i = 0; i < ctx->sameLineInfoCount; i++)
	//{
	//	ctx->sameLineInfo[i].computeHeight = false;
	//}

	ctx->event.type = ctx->savedEventType;
	ctx->positionStack.clear();
}

void update()
{
	clearInputEventQueue();
	ctx->providers->input->processEvents();

	// tooltip handling
	//TODO: move to own func
	if (ctx->tooltip.id && ctx->tooltip.id != ctx->tooltip.lastId && !ctx->tooltip.show)
	{
		ctx->tooltip.timer += ctx->deltaTime;
	}

	if (!ctx->tooltip.wasShown)
	{
		ctx->tooltip.resetTimer += ctx->deltaTime;
	}

	if (!ctx->tooltip.show && ctx->tooltip.id
		&& (ctx->tooltip.timer >= ctx->tooltip.delayToShow
			|| ctx->tooltip.resetTimer < ctx->tooltip.delayToShowConsecutive))
	{
		ctx->tooltip.show = true;
		ctx->mustRedraw = true;
		ctx->tooltip.timer = 0;
		ctx->tooltip.lastId = 0;
		ctx->tooltip.closeTooltipPopup = false;
	}
	else if (ctx->tooltip.show && !ctx->tooltip.wasShown)
	{
		ctx->tooltip.timer = 0;
		ctx->tooltip.resetTimer = 0;
		ctx->tooltip.show = false;
		ctx->tooltip.id = 0;
		ctx->tooltip.closeTooltipPopup = true;
	}

	if (ctx->tooltip.show)
	{
		// track mouse pos
		ctx->tooltip.position = ctx->mousePosition;
	}

	ctx->tooltip.wasShown = false;
}

bool hasNothingToDo()
{
	return !ctx->mustRedraw
		&& !ctx->mouseMoved
		&& !ctx->events.size()
		&& !ctx->docking.dragStarted;
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

void setCurrentNativeWindow(HNativeWindow wnd)
{
	ctx->providers->input->setCurrentWindow(wnd);
	auto size = HORUS_INPUT->getWindowSize(wnd);
	ctx->renderer->setCurrentNativeWindow(wnd);
	ctx->renderer->setWindowSize(size);
	ctx->hoveringThisWindow = ctx->lastHoveredNativeWindow == wnd;
}

void beginRendering()
{
	ctx->renderer->begin();
}

void endRendering()
{
	ctx->renderer->end();
}

static void presentWindow(HNativeWindow wnd)
{
	HORUS_INPUT->setCurrentWindow(wnd);
	ctx->renderer->setCurrentNativeWindow(wnd);
	ctx->renderer->setWindowSize(HORUS_INPUT->getWindowSize(wnd));
	ctx->hoveringThisWindow = ctx->lastHoveredNativeWindow == wnd;

	auto iterWnd = ctx->docking.rootNativeWindowDockNodes.find(wnd);

	if (iterWnd != ctx->docking.rootNativeWindowDockNodes.end())
	{
		ctx->renderer->begin();
		dockNodeTabs(iterWnd->second);
		ctx->renderer->end();
	}

	ctx->renderer->executeDrawCommands(wnd);
	HORUS_INPUT->presentWindow(wnd);
}

void present()
{
	// first, delete pending objects so we dont access them
	deferredDeleteObjects();

	if (ctx->renderer->allowRendering())
	{
		for (auto& wnd : ctx->nativeWindows)
		{
			presentWindow(wnd);
		}
	}

	ctx->renderer->resetWindowContexts();
	ctx->renderer->skipRender = false;
	ctx->renderer->disableRendering = false;
}

void presentNativeWindow(HNativeWindow nativeWnd)
{
	// first, delete pending objects so we dont access them
	deferredDeleteObjects();

	if (ctx->renderer->allowRendering())
	{
		presentWindow(nativeWnd);
	}

	ctx->renderer->resetWindowContexts();
	ctx->renderer->skipRender = false;
	ctx->renderer->disableRendering = false;
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
	ctx->event = InputEvent();
	ctx->events.clear();
}

void setMouseMoved(bool moved)
{
	ctx->mouseMoved = moved;
}

size_t getInputEventCount()
{
	return ctx->events.size();
}

InputEvent getInputEventAt(size_t index)
{
	return ctx->events[index];
}

void setInputEvent(const InputEvent& event)
{
	ctx->event = event;
}

void shutdown()
{
	HORUS_ASSERT(ctx);

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
		return nullptr;
	}

	HImage img = createImage((Rgba32*)imgData.pixels, imgData.width, imgData.height);

	deleteImageData(imgData);

	return img;
}

HImage createImage(Rgba32* pixels, u32 width, u32 height)
{
	HORUS_ASSERT(ctx->theme);
	auto img = ctx->theme->addImage(pixels, width, height);

	return img;
}

Point getImageSize(HImage image)
{
	HORUS_ASSERT(image);
	Image* img = (Image*)image;

	return { img->rect.width, img->rect.height };
}

void updateImagePixels(HImage image, Rgba32* pixels)
{
	HORUS_ASSERT(image);
	HORUS_ASSERT(pixels);
	Image* img = (Image*)image;

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
	HORUS_ASSERT(image);
	Image* img = (Image*)image;
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
	return new Atlas(width, height);
}

void deleteAtlas(HAtlas atlas)
{
	delete (Atlas*)atlas;
}

HImage addAtlasImage(HAtlas atlas, const ImageData& img)
{
	HORUS_ASSERT(atlas);
	Atlas* atlasPtr = (Atlas*)atlas;

	return atlasPtr->addImage((const Rgba32*)img.pixels, img.width, img.height);
}

bool packAtlas(HAtlas atlas, u32 border)
{
	HORUS_ASSERT(atlas);
	Atlas* atlasPtr = (Atlas*)atlas;

	return atlasPtr->pack(border);
}

DockNodeId createRootDockNode(HNativeWindow nativeWnd)
{
	HORUS_ASSERT(nativeWnd);
	auto node = createNativeWindowRootDockNode(nativeWnd);
	HORUS_ASSERT(node);

	ctx->nativeWindows.push_back(nativeWnd);
	ctx->docking.dockNodeIdsMap[node->id] = node;

	return node->id;
}

void dockLayoutDeleteChildren(DockNodeId rootNodeId)
{
	DockNode* node = (DockNode*)ctx->docking.dockNodeIdsMap[rootNodeId];

	if (node)
	{
		node->removeWindowsAndDeleteChildrenRecursive();
	}
}

void dockLayoutSplit(DockNodeId nodeId, DockNodeSplitType splitType, f32 firstNodeSizeUnitPercent, DockNodeId* outNodeId1, DockNodeId* outNodeId2)
{
	DockNode* nodeToSplit = ctx->docking.dockNodeIdsMap[nodeId];
	DockNode* newNode1 = new DockNode();
	DockNode* newNode2 = new DockNode();

	ctx->docking.dockNodeIdsMap[newNode1->id] = newNode1;
	ctx->docking.dockNodeIdsMap[newNode2->id] = newNode2;

	// find if there is a window assigned to the node to be split, if so, reassign to the new node that has the parent's content
	auto iterWindow = ctx->docking.windowsDockNodeAssignments.begin();

	while (iterWindow != ctx->docking.windowsDockNodeAssignments.end())
	{
		if (iterWindow->second == nodeToSplit->id)
		{
			iterWindow->second = newNode1->id;
			break;
		}

		++iterWindow;
	}

	auto node1Id = newNode1->id;
	*newNode1 = *nodeToSplit;
	newNode1->id = node1Id;
	newNode1->parent = nodeToSplit;

	newNode2->nativeWindow = nodeToSplit->nativeWindow;
	newNode2->parent = nodeToSplit;

	for (auto& child : newNode1->children) child->parent = newNode1;

	nodeToSplit->children.clear();

	switch (splitType)
	{
	case DockNodeSplitType::Top:
	case DockNodeSplitType::Left:
		if (splitType == DockNodeSplitType::Top)
		{
			newNode1->rect.height = firstNodeSizeUnitPercent;
			newNode2->rect.height = 1.0f - firstNodeSizeUnitPercent;
		}
		else
		{
			newNode1->rect.width = firstNodeSizeUnitPercent;
			newNode2->rect.width = 1.0f - firstNodeSizeUnitPercent;
		}
		nodeToSplit->children.push_back(newNode1);
		nodeToSplit->children.push_back(newNode2);
		if (outNodeId1) *outNodeId1 = newNode1->id;
		if (outNodeId2) *outNodeId2 = newNode2->id;
		break;
	case DockNodeSplitType::Bottom:
	case DockNodeSplitType::Right:
		if (splitType == DockNodeSplitType::Bottom)
		{
			newNode2->rect.height = firstNodeSizeUnitPercent;
			newNode1->rect.height = 1.0f - firstNodeSizeUnitPercent;
		}
		else
		{
			newNode2->rect.width = firstNodeSizeUnitPercent;
			newNode1->rect.width = 1.0f - firstNodeSizeUnitPercent;
		}

		nodeToSplit->children.push_back(newNode2);
		nodeToSplit->children.push_back(newNode1);
		if (outNodeId1) *outNodeId1 = newNode2->id;
		if (outNodeId2) *outNodeId2 = newNode1->id;
		break;
	}

	switch (splitType)
	{
	case DockNodeSplitType::Top:
	case DockNodeSplitType::Bottom:
		nodeToSplit->type = DockNode::Type::Vertical;
		break;
	case DockNodeSplitType::Left:
	case DockNodeSplitType::Right:
		nodeToSplit->type = DockNode::Type::Horizontal;
		break;
	}
}

void dockLayoutSetNodeWindow(DockNodeId parentNodeId, const char* windowId)
{
	DockNode* node = ctx->docking.dockNodeIdsMap[parentNodeId];

	ctx->docking.windowsDockNodeAssignments[windowId] = node->id;
}

void dockLayoutRecalculate()
{
	for (auto& pair : ctx->docking.rootNativeWindowDockNodes)
	{
		pair.second->checkRedundancy();
		pair.second->computeRect();
	}
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
	Theme* theme = new Theme(atlasTextureSize);

	ctx->themes.push_back(theme);

	return theme;
}

void setThemeUserSetting(HTheme theme, const char* name, const char* value)
{
	((Theme*)theme)->userSettings[name] = value;
}

const char* getThemeUserSetting(HTheme theme, const char* name)
{
	auto iter = ((Theme*)theme)->userSettings.find(name);

	if (iter == ((Theme*)theme)->userSettings.end())
		return "";

	return iter->second.c_str();
}

HImage addThemeImage(HTheme theme, const ImageData& img)
{
	Theme* themePtr = (Theme*)theme;

	return themePtr->addImage((const Rgba32*)img.pixels, img.width, img.height);
}

HImage getThemeImage(HTheme theme, const char* imageName)
{
	Theme* themePtr = (Theme*)theme;

	auto iter = themePtr->images.find(imageName);

	if (iter != themePtr->images.end())
		return iter->second;

	return nullptr;
}

void setThemeImage(HTheme theme, const char* imageName, HImage image)
{
	Theme* themePtr = (Theme*)theme;

	auto iter = themePtr->images.find(imageName);

	if (iter != themePtr->images.end())
	{
		iter->second = (Image*)image;
		return;
	}

	themePtr->images[imageName] = (Image*)image;
}

void setWidgetStyle(WidgetType widgetType, const char* styleName)
{
	//TODO: more automatic correlation between widget type and its element types, to avoid manual switch
	// To not force using map to search for the current style for all widgets, this might be the only way
	// switch might be faster than map tho
	switch (widgetType)
	{
	case WidgetType::Window:
		ctx->theme->elements[(u32)WidgetElementId::WindowBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::WindowHorizontalSplitter].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::WindowVerticalSplitter].setStyle(styleName);
		break;
	case WidgetType::Layout:
		break;
	case WidgetType::Tooltip:
		ctx->theme->elements[(u32)WidgetElementId::TooltipBody].setStyle(styleName);
		break;
	case WidgetType::Button:
		ctx->theme->elements[(u32)WidgetElementId::ButtonBody].setStyle(styleName);
		break;
	case WidgetType::ImageButton:
		ctx->theme->elements[(u32)WidgetElementId::ImageButtonBody].setStyle(styleName);
		break;
	case WidgetType::TextInput:
		ctx->theme->elements[(u32)WidgetElementId::TextInputBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::TextInputCaret].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::TextInputSelection].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::TextInputDefaultText].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::TextInputFilterClearImage].setStyle(styleName);
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
	case WidgetType::Expandable:
		ctx->theme->elements[(u32)WidgetElementId::ExpandableBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ExpandableCollapsedArrow].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ExpandableExpandedArrow].setStyle(styleName);
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
		ctx->theme->elements[(u32)WidgetElementId::ScrollViewScrollBarV].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ScrollViewScrollThumbV].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ScrollViewScrollBarH].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ScrollViewScrollThumbH].setStyle(styleName);
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
	case WidgetType::MsgBox:
		ctx->theme->elements[(u32)WidgetElementId::MessageBoxImageError].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MessageBoxImageWarning].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MessageBoxImageInfo].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MessageBoxImageQuestion].setStyle(styleName);
		break;
	case WidgetType::Box:
		ctx->theme->elements[(u32)WidgetElementId::BoxBody].setStyle(styleName);
		break;
	case WidgetType::ComboSlider:
		ctx->theme->elements[(u32)WidgetElementId::ComboSliderBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ComboSliderLeftArrow].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ComboSliderRangeBar].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ComboSliderRightArrow].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ComboSliderVerticalLine].setStyle(styleName);
		break;
	case WidgetType::RotarySlider:
		ctx->theme->elements[(u32)WidgetElementId::RotarySliderBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::RotarySliderMark].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::RotarySliderValueDot].setStyle(styleName);
		break;
	case WidgetType::ColorPicker:
		ctx->theme->elements[(u32)WidgetElementId::ColorPickerCheckers].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::ColorPickerBody].setStyle(styleName);
		break;
	case WidgetType::Table:
		ctx->theme->elements[(u32)WidgetElementId::TableBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::TableHeaderBody].setStyle(styleName);
		break;
	}
}

void pushWidgetStyle(WidgetType widgetType, const char* styleName)
{
	if (ctx->widgetCurrentStyle.find(widgetType) == ctx->widgetCurrentStyle.end())
	{
		ctx->widgetCurrentStyle.insert(std::make_pair(widgetType, "default"));
	}

	ctx->widgetStyleStack.push_back(std::make_pair(widgetType, ctx->widgetCurrentStyle[widgetType]));

	setWidgetStyle(widgetType, styleName);
}

void popWidgetStyle()
{
	HORUS_ASSERT(!ctx->widgetStyleStack.empty());

	if (ctx->widgetStyleStack.empty())
		return;

	auto& top = ctx->widgetStyleStack.back();
	setWidgetStyle(top.first, top.second.c_str());
	ctx->widgetStyleStack.pop_back();
}

void setWidgetElementStyle(WidgetElementId widgetElementId, const char* styleName)
{
	HORUS_ASSERT(ctx);
	HORUS_ASSERT(ctx->theme);
	ctx->theme->elements[(u32)widgetElementId].setStyle(styleName);
}

void setDefaultWidgetStyle(WidgetType widgetType)
{
	setWidgetStyle(widgetType, "default");
}

void setDefaultWidgetElementStyle(WidgetElementId widgetElementId)
{
	setWidgetElementStyle(widgetElementId, "default");
}

void setUserWidgetElementStyle(const char* elementName, const char* styleName)
{
	ctx->theme->userElements[elementName]->setStyle(styleName);
}

void buildTheme(HTheme theme)
{
	HORUS_ASSERT(theme);
	Theme* themePtr = (Theme*)theme;

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
	HORUS_ASSERT(theme);
	HORUS_ASSERT(styleName);

	Theme* themePtr = (Theme*)theme;
	u32 stateIndex = (u32)widgetStateType;

	auto& state = themePtr->elements[(u32)elementId].styles[styleName].states[stateIndex];

	state.border = elementInfo.border;
	state.color = elementInfo.color;
	state.textColor = elementInfo.textColor;
	state.font = (Font*)elementInfo.font;
	state.width = elementInfo.width;
	state.height = elementInfo.height;
	state.image = (Image*)elementInfo.image;
}

void setThemeUserWidgetElement(
	HTheme theme,
	const char* userElementName,
	WidgetStateType widgetStateType,
	const WidgetElementInfo& elementInfo,
	const char* styleName)
{
	Theme* themePtr = (Theme*)theme;
	u32 stateIndex = (u32)widgetStateType;

	if (themePtr->userElements.find(userElementName) == themePtr->userElements.end())
	{
		themePtr->userElements.insert(std::make_pair(userElementName, new ThemeElement()));
	}

	auto& state = themePtr->userElements[userElementName]->styles[styleName].states[stateIndex];

	state.border = elementInfo.border;
	state.color = elementInfo.color;
	state.textColor = elementInfo.textColor;
	state.font = (Font*)elementInfo.font;
	state.width = elementInfo.width;
	state.height = elementInfo.height;
	state.image = (Image*)elementInfo.image;
}

void setTheme(HTheme theme)
{
	HORUS_ASSERT(theme);
	ctx->theme = (Theme*)theme;
}

HTheme getTheme()
{
	return ctx->theme;
}

void deleteTheme(HTheme theme)
{
	auto iter = std::find(ctx->themes.begin(), ctx->themes.end(), (Theme*)theme);

	if (iter == ctx->themes.end())
		return;

	delete *iter;
	ctx->themes.erase(iter);
	ctx->theme = nullptr;
}

void getThemeWidgetElementInfo(WidgetElementId elementId, WidgetStateType state, WidgetElementInfo& outInfo, const char* styleName)
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

	auto& elemState = iter->second->getStyleState(styleName, state);

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
	((Theme*)theme)->elements[(int)elementId].styles[styleName].parameters[paramName] = paramValue;
}

const char* getThemeWidgetElementStringParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const char* defaultValue)
{
	auto& style = ((Theme*)theme)->elements[(int)elementId].styles[styleName];

	auto iter = style.parameters.find(paramName);

	if (iter == style.parameters.end())
		return defaultValue;

	return iter->second.c_str();
}

f32 getThemeWidgetElementFloatParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, f32 defaultValue)
{
	auto& style = ((Theme*)theme)->elements[(int)elementId].styles[styleName];

	return style.getParameter(paramName, defaultValue);
}

const Color& getThemeWidgetElementColorParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const Color& defaultValue)
{
	auto& style = ((Theme*)theme)->elements[(int)elementId].styles[styleName];

	return style.getColorParameter(paramName, defaultValue);
}

void setThemeUserWidgetElementParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const char* paramValue)
{
	auto themePtr = ((Theme*)theme);

	if (themePtr->userElements.find(userElementName) == themePtr->userElements.end())
	{
		themePtr->userElements.insert(std::make_pair(userElementName, new ThemeElement()));
	}

	themePtr->userElements[userElementName]->styles[styleName].parameters[paramName] = paramValue;
}

const char* getThemeUserWidgetElementStringParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const char* defaultValue)
{
	auto& style = ((Theme*)theme)->userElements[userElementName]->styles[styleName];

	auto iter = style.parameters.find(paramName);

	if (iter == style.parameters.end())
		return defaultValue;

	return iter->second.c_str();
}

f32 getThemeUserWidgetElementFloatParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, f32 defaultValue)
{
	auto& style = ((Theme*)theme)->userElements[userElementName]->styles[styleName];

	return style.getParameter(paramName, defaultValue);
}

const Color& getThemeUserWidgetElementColorParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const Color& defaultValue)
{
	auto& style = ((Theme*)theme)->userElements[userElementName]->styles[styleName];

	return style.getColorParameter(paramName, defaultValue);
}

HFont createThemeFont(HTheme theme, const char* name, const char* fontFilename, u32 faceSize)
{
	Theme* themePtr = (Theme*)theme;
	auto fnt = (HFont)themePtr->fontCache->createFont(name, fontFilename, faceSize * ctx->scale, false);

	auto fontIter = themePtr->fonts.find(name);

	if (fontIter != themePtr->fonts.end())
	{
		themePtr->fontCache->releaseFont(fontIter->second);
		themePtr->fonts.erase(fontIter);
	}

	themePtr->fonts[name] = (Font*)fnt;

	return fnt;
}

void releaseThemeFont(HTheme theme, HFont font)
{
	Theme* themePtr = (Theme*)theme;

	for (auto& fntPair : themePtr->fonts)
	{
		if (fntPair.second == (Font*)font)
		{
			themePtr->fonts.erase(fntPair.first);
			break;
		}
	}

	themePtr->fontCache->releaseFont((Font*)font);
}

HFont getThemeFont(HTheme theme, const char* themeFontName)
{
	Theme* themePtr = (Theme*)theme;

	return themePtr->fonts[themeFontName];
}

HFont getFont(const char* themeFontName)
{
	return getThemeFont(getTheme(), themeFontName);
}

void beginLayout(const Rect& rect)
{
	pushLayout();
	auto paddedRect = rect.contract(getPadding(PaddingType::Layout));
	ctx->layout.type = LayoutType::Generic;
	ctx->layout.savedPosition = paddedRect.topLeft();
	ctx->layout.width = paddedRect.width;
	ctx->layout.height = paddedRect.height;
	ctx->layout.firstWidgetInLayout = true;
	ctx->renderer->pushClipRect(paddedRect);
	ctx->position = { paddedRect.x, paddedRect.y};
	ctx->sameLine.enabled = false;
}

void endLayout()
{
	ctx->renderer->popClipRect();
	popLayout();
	ctx->currentTabIndex = 0;
	ctx->selectedTabIndex = 0;
	ctx->scrollViewDepth = 0;
}

void pushId(const char* id)
{
	ctx->idStack.push_back(genId(id));
}

void pushId(u32 id)
{
	ctx->idStack.push_back(genId(id));
}

void pushId(void* id)
{
	ctx->idStack.push_back(genId(id));
}

void popId()
{
	if (ctx->idStack.empty())
	{
		HORUS_LOG("popId used too many times");
		return;
	}

	ctx->idStack.pop_back();
}

void pushLayout()
{
	ctx->layoutStack.push_back(ctx->layout);
}

void popLayout()
{
	if (ctx->layoutStack.empty())
	{
		HORUS_LOG("popLayout used too many times");
		return;
	}

	ctx->layout = ctx->layoutStack.back();
	ctx->layoutStack.pop_back();
}

f32 getRemainingHeight()
{
	f32 remainingHeight = ctx->layout.height - (ctx->position.y - ctx->layout.savedPosition.y);

	return remainingHeight > 0 ? remainingHeight : 0;
}

f32 getRemainingWidth()
{
	f32 remainingWidth = ctx->layout.width - (ctx->position.x - ctx->layout.savedPosition.x);

	return remainingWidth > 0 ? remainingWidth : 0;
}

void incrementLayerIndex()
{
	ctx->layerIndex++;
	//ctx->renderer->setWindowDrawCmdLayer(ctx->layerIndex);

	if (ctx->maxLayerIndex < ctx->layerIndex)
	{
		ctx->maxLayerIndex = ctx->layerIndex;
	}
}

u32 decrementLayerIndex()
{
	ctx->layerIndex--;

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

void pushPadding(PaddingType type, const Point& newPadding)
{
	ctx->paddingStack[(i32)type].push_back(ctx->padding[(i32)type]);
	ctx->padding[(i32)type] = newPadding;
}

void pushWidgetPadding(const Point& newPadding)
{
	pushPadding(PaddingType::Widget, newPadding);
}

void popPadding(PaddingType type)
{
	if (!ctx->paddingStack[(i32)type].empty())
	{
		ctx->padding[(i32)type] = ctx->paddingStack[(i32)type].back();
		ctx->paddingStack[(i32)type].pop_back();
	}
}

void popWidgetPadding()
{
	popPadding(PaddingType::Widget);
}

const Point& getPadding(PaddingType type)
{
	return ctx->padding[(i32)type];
}

const Point& getWidgetPadding()
{
	return ctx->padding[(i32)PaddingType::Widget];
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

void changeScale(f32 scale)
{
	ctx->scale = scale;

	if (ctx->theme)
	{
		ctx->theme->fontCache->rescaleFonts(scale);
		ctx->theme->atlas->repackImages();
	}
}

f32 getScale()
{
	return ctx->scale;
}

void pushTint(const Color& color, TintColorType type, TintColorOpType opType)
{
	ctx->tintStack.push_back(ctx->tint);

	switch (type)
	{
	case hui::TintColorType::Body:
	case hui::TintColorType::Text:
		ctx->tint.color[(i32)type] = color;
		ctx->tint.op[(i32)type] = opType;
		break;
	case hui::TintColorType::All:
		for (i32 i = 0; i < (i32)TintColorType::Count; i++)
		{
			ctx->tint.color[i] = color;
			ctx->tint.op[i] = opType;
		}
		break;
	default:
		break;
	}
}

void popTint()
{
	if (!ctx->tintStack.empty())
	{
		ctx->tint = ctx->tintStack.back();
		ctx->tintStack.pop_back();
	}
}

Color applyTint(const Color& originalColor, TintColorType type)
{
	Color tintedColor;

	if (ctx->tint.op[(i32)type] == TintColorOpType::Multiply)
	{
		tintedColor = originalColor * ctx->tint.color[(i32)type];
	}
	else
	if (ctx->tint.op[(i32)type] == TintColorOpType::Add)
	{
		tintedColor = originalColor + ctx->tint.color[(i32)type];
	}
	else
	if (ctx->tint.op[(i32)type] == TintColorOpType::Subtract)
	{
		tintedColor = originalColor - ctx->tint.color[(i32)type];
	}
	else
	if (ctx->tint.op[(i32)type] == TintColorOpType::Replace)
	{
		tintedColor = ctx->tint.color[(i32)type];
	}
	else
	{
		tintedColor = originalColor;
	}

	return tintedColor;
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

WidgetId getWidgetId()
{
	return ctx->id;
}

Point getMousePosition()
{
	return ctx->mousePosition;
}

Point getPosition()
{
	return ctx->position;
}

void setPosition(const Point& position)
{
	ctx->position = position;
}

void pushPosition()
{
	ctx->positionStack.push_back(ctx->position);
}

void popPosition()
{
	if (!ctx->positionStack.empty())
	{
		ctx->position = ctx->positionStack.back();
		ctx->positionStack.pop_back();
	}
}

bool wantsToDragDrop()
{
	if (ctx->event.type == InputEvent::Type::MouseDown
		&& !ctx->dragDrop.draggingIntent
		&& !ctx->dragDrop.dragging
		&& ctx->widget.hovered)
	{
		ctx->dragDrop.draggingIntent = true;
		ctx->dragDrop.lastMousePos = ctx->mousePosition;
		ctx->dragDrop.id = ctx->id;
	}

	const u32 dragStartPixelDistance = 4;

	if (ctx->dragDrop.draggingIntent
		&& !ctx->dragDrop.dragging
		&& ctx->id == ctx->dragDrop.id
		&& ctx->dragDrop.lastMousePos.getDistance(ctx->mousePosition) >= dragStartPixelDistance)
	{
		ctx->dragDrop.dragging = true;
		return true;
	}

	if (ctx->event.type == InputEvent::Type::MouseUp
		&& (ctx->dragDrop.draggingIntent ||
			ctx->dragDrop.dragging))
	{
		ctx->dragDrop.draggingIntent = false;
		ctx->dragDrop.dragging = false;
		setMouseCursor(MouseCursorType::Arrow);
	}

	return false;
}

void setDragDropMouseCursor(HMouseCursor dropAllowedCursor)
{
	ctx->dragDrop.dropAllowedCursor = dropAllowedCursor;
}

void beginDragDrop(u32 dragObjectType, void* dragObject)
{
	ctx->dragDrop.dragObject = dragObject;
	ctx->dragDrop.dragObjectType = dragObjectType;
	ctx->dragDrop.begunDragging = true;
}

void endDragDrop()
{
	ctx->dragDrop.dragging = false;
	ctx->dragDrop.begunDragging = false;
	ctx->dragDrop.draggingIntent = false;
	ctx->dragDrop.dragObject = nullptr;
	ctx->dragDrop.dragObjectType = 0;
}

void allowDragDrop()
{
	ctx->dragDrop.allowDrop = true;

	if (ctx->dragDrop.begunDragging)
	{
		if (ctx->widget.hovered)
		{
			ctx->dragDrop.foundDropTarget = true;
		}
	}
}

void disallowDragDrop()
{
	ctx->dragDrop.allowDrop = false;
}

bool droppedOnWidget()
{
	if (ctx->dragDrop.begunDragging
		&& ctx->hoveringThisWindow
		&& HORUS_INPUT->getFocusedWindow() != ctx->currentWindow->dockNode->nativeWindow)
	{
		ctx->providers->input->raiseWindow(ctx->currentWindow->dockNode->nativeWindow);
	}

	if (ctx->dragDrop.begunDragging
		&& ctx->widget.hovered
		&& ctx->event.type == InputEvent::Type::MouseUp
		&& ctx->dragDrop.allowDrop)
	{
		return true;
	}

	return false;
}

void* getDragDropObject()
{
	return ctx->dragDrop.dragObject;
}

u32 getDragDropObjectType()
{
	return ctx->dragDrop.dragObjectType;
}

}
