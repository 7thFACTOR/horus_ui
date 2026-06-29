#include <stdlib.h>
#include <algorithm>
#include "context.h"
#include "theme.h"
#include "atlas.h"
#include "util.h"
#include "renderer.h"
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

HContext contextCreate(const Settings& settings)
{
	Context* context = new Context();

	HUI_ASSERT(context);
	context->settings = settings;

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

void contextSet(HContext context)
{
	HUI_ASSERT(context);
	ctx = (Context*)context;
}

HContext contextGet()
{
	return ctx;
}

void contextDestroy(HContext context)
{
	HUI_ASSERT(context);
	delete (Context*)context;
}

Settings& contextGetSettings()
{
	return ctx->settings;
}

void renderCallbackAdd(RenderCallback callback)
{
	ctx->renderer.cmdCallback(callback);
}

void clearBackground(const Color& color)
{
	ctx->renderer.cmdClearBackground(color);
}

void widgetSetNextDisabled(bool disabled)
{
	ctx->widget.nextDisabled = disabled;
}

void widgetPushDisabled(bool disabled)
{
	if (disabled)
		ctx->disabledNesting++;
}

void widgetPopDisabled()
{
	if (ctx->disabledNesting > 0)
		ctx->disabledNesting--;
}

bool widgetGetDisabled()
{
	return ctx->widget.nextDisabled || (ctx->disabledNesting > 0);
}

void widgetSetNextFocused()
{
	ctx->widget.hovered = true;
	ctx->widget.pressed = true;
	ctx->widget.focused = true;
	ctx->focusChanged = true;
}

void addWidget(f32 height)
{
	ctx->widget.disabled = ctx->widget.nextDisabled || (ctx->disabledNesting > 0);
	ctx->widget.nextDisabled = false;
	ctx->widget.changeEnded = false;
	height = (height + widgetGetPadding().y * 2.0f) * ctx->scale;

	// next width has priority over custom width
	if (ctx->widget.hasNextWidth)
	{
		// nextWidth specifies the TOTAL widget width (including padding)
		ctx->widget.width = ctx->widget.nextWidth * ctx->scale;
	}
	else
	{
		ctx->widget.width = ctx->widget.hasCustomWidth ? ctx->widget.customWidth * ctx->scale : ctx->layout.width;
	}

	// if width is under 1, then it's a percentage of the layout width
	// otherwise it's a fixed pixel width
	auto pixelWidth = ctx->widget.width > 1 ? ctx->widget.width : ctx->widget.width * ctx->layout.width;
	f32 spacing = ctx->spacing * ctx->scale;

	// Handle transition from sameLine back to normal layout
	if (!ctx->sameLine.enabled && ctx->sameLine.wasEnabled)
	{
		// End of same-line group - move to next line
		ctx->position.x = ctx->sameLine.currentPosition.x;
		ctx->position.y += ctx->sameLine.maxHeight + spacing;
		ctx->sameLine.wasEnabled = false;
		ctx->sameLine.maxHeight = 0;
		ctx->sameLine.lastLineWidth = 0; // Reset to prevent stale values
	}

	if (!ctx->sameLine.enabled && !ctx->sameLine.wasEnabled)
	{
		ctx->sameLine.currentPosition = ctx->position;
		ctx->sameLine.lastLineWidth = 0; // Reset for new line
	}

	// if in sameLine mode, align Y to the same line and advance X from previous widget
	if (ctx->sameLine.enabled)
	{
		ctx->position.y = ctx->sameLine.currentPosition.y;

		// if first widget in sameLine, advance X by the previous normal widget's width
		if (!ctx->sameLine.wasEnabled)
		{
			ctx->position.x += ctx->sameLine.lastLineWidth + ctx->sameLine.nextSpacing * ctx->scale;
		}
	}

	ctx->widget.width = pixelWidth;
	ctx->widget.rect.set(
		ctx->position.x,
		ctx->position.y,
		pixelWidth,
		height);

	// advance cursor after placing widget
	if (!ctx->sameLine.enabled)
	{
		// normal mode: advance Y (vertical)
		ctx->position.y += height + spacing;
		ctx->sameLine.lastLineWidth = pixelWidth; // track for sameLine transition
	}
	else
	{
		// sameLine mode: advance X (horizontal)
		ctx->position.x += pixelWidth + ctx->sameLine.nextSpacing * ctx->scale;
		ctx->sameLine.wasEnabled = true;
		ctx->sameLine.maxHeight = std::max(ctx->sameLine.maxHeight, height);
	}

	// track maximum X position for horizontal scrolling content width
	if (ctx->layout.type == LayoutType::ScrollView)
	{
		f32 widgetRightEdge = ctx->widget.rect.right();

		if (widgetRightEdge > ctx->maxContentWidth)
		{
			ctx->maxContentWidth = widgetRightEdge;
		}
	}

	ctx->widget.hasNextWidth = false;
	ctx->widget.hasCustomWidth = false;
	ctx->sameLine.enabled = false;
}

void widgetSetFocusable()
{
	if (ctx->widget.disabled)
		return;

	if (ctx->widget.focusedId == ctx->id)
	{
		ctx->widget.focusedWidgetRect = ctx->widget.rect;
	}

	ctx->focusableWidgets.push_back(ctx->id);
}

bool viewportImageSizeFit(
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

void frameBegin()
{
	ctx->frameStartTime = std::chrono::high_resolution_clock::now();

	// swap focusable widgets lists
	std::swap(ctx->focusableWidgets, ctx->lastFrameFocusableWidgets);
	ctx->focusableWidgets.clear();
	// reserve some space to avoid allocations
	if (ctx->focusableWidgets.capacity() < ctx->lastFrameFocusableWidgets.size())
		ctx->focusableWidgets.reserve(ctx->lastFrameFocusableWidgets.size());

	if (ctx->textInput.id)
	{
		ctx->textInput.textChanged = false;
		ctx->textInput.processEvent(ctx->event);
	}

	if (ctx->activeMultilineInputId && ctx->textMultilineInput.count(ctx->activeMultilineInputId))
	{
		ctx->textMultilineInput[ctx->activeMultilineInputId].textChanged = false;
		ctx->textMultilineInput[ctx->activeMultilineInputId].processEvent(ctx->event);
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
			auto wndPos = ctx->settings.services.getWindowPosition(ctx->lastHoveredNativeWindow);
			auto absMousePos = ctx->settings.services.getAbsoluteMousePosition();
			ctx->mousePosition = absMousePos - wndPos;
		}
	}

	if (ctx->event.type == InputEvent::Type::Key
		&& ctx->event.key.code == KeyCode::Tab
		&& ctx->event.key.down
		&& !ctx->activeMultilineInputId // Don't switch focus if editing multiline text
		&& !ctx->lastFrameFocusableWidgets.empty())
	{
		bool shift = has(ctx->event.key.modifiers, KeyModifiers::Shift);
		size_t currentIndex = ~0;

		// find current focused widget index
		for (size_t i = 0; i < ctx->lastFrameFocusableWidgets.size(); i++)
		{
			if (ctx->lastFrameFocusableWidgets[i] == ctx->widget.focusedId)
			{
				currentIndex = i;
				break;
			}
		}

		if (currentIndex == ~0)
		{
			// if nothing focused, start from beginning (or end if shift)
			if (shift)
				ctx->widget.focusedId = ctx->lastFrameFocusableWidgets.back();
			else
				ctx->widget.focusedId = ctx->lastFrameFocusableWidgets.front();
		}
		else
		{
			if (shift)
			{
				if (currentIndex > 0)
					ctx->widget.focusedId = ctx->lastFrameFocusableWidgets[currentIndex - 1];
				else
					ctx->widget.focusedId = ctx->lastFrameFocusableWidgets.back(); // wrap to end
			}
			else
			{
				if (currentIndex < ctx->lastFrameFocusableWidgets.size() - 1)
					ctx->widget.focusedId = ctx->lastFrameFocusableWidgets[currentIndex + 1];
				else
					ctx->widget.focusedId = ctx->lastFrameFocusableWidgets.front(); // wrap to start
			}
		}

		ctx->focusChanged = true;
	}

	ctx->mustRedraw = false;
	ctx->skipRenderAndInput = false;
	ctx->widget.disabled = false;
	ctx->widget.nextDisabled = false;
	ctx->disabledNesting = 0;
	ctx->layerIndex = 0;
	ctx->maxLayerIndex = 0;
	ctx->widget.nextFocusableId = 0;
	ctx->menuDepth = 0;
	ctx->popupIndex = 0;
	ctx->menuItemChosen = false;
	ctx->dragDrop.foundDropTarget = false;
	ctx->widget.hoveredId = 0;
	ctx->widget.hoveredType = WidgetType::None;
	ctx->widget.changeEnded = false;
	ctx->frameCount++;
	ctx->totalTime += ctx->settings.deltaTime;
	ctx->pruneUnusedTextTime += ctx->settings.deltaTime;
	ctx->sameLine.wasEnabled = false;
	ctx->fontStack.clear();

	ctx->padding[(i32)PaddingType::Layout] = ctx->settings.defaultLayoutPadding;
	ctx->padding[(i32)PaddingType::ScrollView] = ctx->settings.defaultScrollViewPadding;
	ctx->padding[(i32)PaddingType::Widget] = ctx->settings.defaultWidgetPadding;
	ctx->savedEventType = ctx->event.type;

	for (auto& popup : ctx->popupStack)
	{
		popup.alreadyClosedWithEscape = false;
		popup.alreadyClickedOnSomething = false;
	}

	ctx->alreadyClickedOnSomething = false;
	mouseCursorSetType(MouseCursorType::Arrow);
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

		ctx->settings.services.destroyWindow(wnd);
	}

	ctx->docking.dockNodesToDelete.clear();
	ctx->docking.windowsToDelete.clear();
	ctx->docking.nativeWindowsToDelete.clear();
}

void frameEnd()
{
	//TODO: check stacks to see if there is are items on them
	// the stacks should be empty, otherwise push/pop count not matching
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
			mouseCursorSet(ctx->dragDrop.dropAllowedCursor);
		}
		else
		{
			mouseCursorSetType(MouseCursorType::No);
		}
	}

	if (ctx->mouseCursor != MouseCursorType::Custom)
	{
		ctx->settings.services.setCursor(ctx->mouseCursor);
	}
	else if (ctx->customMouseCursor)
	{
		ctx->settings.services.setCustomCursor(ctx->customMouseCursor);
	}

	ctx->event.type = ctx->savedEventType;
	ctx->positionStack.clear();

	auto frameEndTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> frameDuration = frameEndTime - ctx->frameStartTime;
	ctx->lastFrameTimeMs = (f32)frameDuration.count();

	// contextUpdate peak
	if (ctx->lastFrameTimeMs < 5 && ctx->lastFrameTimeMs > ctx->peakFrameTimeMs)
		ctx->peakFrameTimeMs = ctx->lastFrameTimeMs;

	// contextUpdate rolling average
	ctx->frameTimes[ctx->frameTimeIndex] = ctx->lastFrameTimeMs;
	ctx->frameTimeIndex = (ctx->frameTimeIndex + 1) % 60;

	f32 sum = 0.0f;
	u32 count = ctx->frameCount < 60 ? ctx->frameCount : 60;
	for (u32 i = 0; i < count; i++)
		sum += ctx->frameTimes[i];
	ctx->avgFrameTimeMs = count > 0 ? sum / (f32)count : 0.0f;
}

void renderBegin()
{
	ctx->renderer.begin();
}

void renderEnd()
{
	ctx->renderer.end();
}

HUI_API f32 frameTimeGetLastMs()
{
	return ctx->lastFrameTimeMs;
}

HUI_API f32 frameTimeGetPeakMs()
{
	return ctx->peakFrameTimeMs;
}

HUI_API f32 frameTimeGetAvgMs()
{
	return ctx->avgFrameTimeMs;
}

void contextUpdate()
{
	u32 timeoutMs = 0;

	if (ctx->settings.fpsThrottleEnable)
	{
		if (hasNothingToDo())
		{
			ctx->idleTime += ctx->settings.deltaTime;
		}
		else
		{
			ctx->idleTime = 0;
		}

		f32 t = ctx->idleTime / ctx->settings.fpsThrottleGradualTime;
		if (t > 1.0f) t = 1.0f;

		f32 targetFps = (f32)ctx->settings.fpsThrottleMaxFps + t * ((f32)ctx->settings.fpsThrottleMinFps - (f32)ctx->settings.fpsThrottleMaxFps);
		f32 targetFrameTimeMs = 1000.0f / targetFps;

		// if we have something to do (like animations or input happened last frame), we don't want to wait
		if (!hasNothingToDo())
		{
			timeoutMs = 0;
		}
		else
		{
			timeoutMs = (u32)std::max(0.0f, targetFrameTimeMs - ctx->lastFrameTimeMs);
		}
	}

	inputEventClearQueue();
	ctx->settings.services.processWindowEvents(timeoutMs);

	// tooltip handling
	//TODO: move to own func
	if (ctx->tooltip.id && ctx->tooltip.id != ctx->tooltip.lastId && !ctx->tooltip.show)
	{
		ctx->tooltip.timer += ctx->settings.deltaTime;
	}

	if (!ctx->tooltip.wasShown)
	{
		ctx->tooltip.resetTimer += ctx->settings.deltaTime;
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

void skipRenderingThisFrame(bool disable)
{
	ctx->renderer.disableRendering = disable;
}

void forceRepaint()
{
	ctx->mustRedraw = true;
}

void skipFrame()
{
	ctx->setSkipRenderAndInput(true);
}

bool clipboardSetText(const char* text)
{
	return ctx->settings.services.clipboardSetText(text);
}

bool clipboardGetText(char* outText, u32 maxTextSize)
{
	return ctx->settings.services.clipboardGetText(outText, maxTextSize);
}

const InputEvent& inputEventGet()
{
	return ctx->event;
}

void mouseCursorSetType(MouseCursorType type)
{
	ctx->mouseCursor = type;
}

HMouseCursor mouseCursorCreate(Rgba32* pixels, u32 width, u32 height, u32 hotSpotX, u32 hotSpotY)
{
	return ctx->settings.services.createCustomCursor(pixels, width, height, hotSpotX, hotSpotY);
}

void mouseCursorDestroy(HMouseCursor cursor)
{
	ctx->settings.services.deleteCustomCursor(cursor);
}

void mouseCursorSet(HMouseCursor cursor)
{
	ctx->mouseCursor = MouseCursorType::Custom;
	ctx->customMouseCursor = cursor;
}

void nativeWindowSetCurrent(HNativeWindow wnd)
{
	ctx->settings.services.setCurrentWindow(wnd);
	auto size = ctx->settings.services.getWindowSize(wnd);
	ctx->renderer.nativeWindowSetCurrent(wnd);
	ctx->renderer.setWindowSize(size);
	ctx->hoveringThisWindow = ctx->lastHoveredNativeWindow == wnd;
}

static void presentWindow(HNativeWindow wnd)
{
	nativeWindowSetCurrent(wnd);

	auto iterWnd = ctx->docking.rootNativeWindowDockNodes.find(wnd);

	if (iterWnd != ctx->docking.rootNativeWindowDockNodes.end())
	{
		ctx->renderer.begin();
		dockNodeTabs(iterWnd->second);
		ctx->renderer.end();
	}

	ctx->renderer.executeDrawCommands(wnd);
	ctx->settings.services.presentWindow(wnd);
}

void present()
{
	// first, delete pending objects so we dont access them
	deferredDeleteObjects();

	if (ctx->renderer.allowRendering())
	{
		for (auto& wnd : ctx->nativeWindows)
		{
			presentWindow(wnd);
		}
	}

	ctx->renderer.resetWindowContexts();
	ctx->renderer.skipRender = false;
	ctx->renderer.disableRendering = false;
}

void presentNativeWindow(HNativeWindow nativeWnd)
{
	// first, delete pending objects so we dont access them
	deferredDeleteObjects();

	if (ctx->renderer.allowRendering())
	{
		presentWindow(nativeWnd);
	}

	ctx->renderer.resetWindowContexts();
	ctx->renderer.skipRender = false;
	ctx->renderer.disableRendering = false;
}

void inputEventCancel()
{
	ctx->event.type = InputEvent::Type::None;
}

void inputEventAdd(const InputEvent& event)
{
	ctx->events.push_back(event);
}

void inputEventClearQueue()
{
	ctx->event = InputEvent();
	ctx->events.clear();
}

void inputSetMouseMoved(bool moved)
{
	ctx->mouseMoved = moved;
}

size_t inputEventGetCount()
{
	return ctx->events.size();
}

InputEvent inputEventGetAtIndex(size_t index)
{
	return ctx->events[index];
}

void inputEventSet(const InputEvent& event)
{
	ctx->event = event;
}

void shutdown()
{
}

DockNodeId dockNodeCreateRoot(HNativeWindow nativeWnd)
{
	HUI_ASSERT(nativeWnd);
	auto node = dockNodeRootCreateInternal(nativeWnd);
	HUI_ASSERT(node);

	ctx->nativeWindows.push_back(nativeWnd);
	ctx->docking.dockNodeIdsMap[node->id] = node;

	return node->id;
}

void dockNodeDeleteChildren(DockNodeId rootNodeId)
{
	DockNode* node = (DockNode*)ctx->docking.dockNodeIdsMap[rootNodeId];

	if (node)
	{
		node->removeWindowsAndDeleteChildrenRecursive();
	}
}

void dockNodeSplit(DockNodeId nodeId, DockNodeSplitType splitType, f32 firstNodeSizeUnitPercent, DockNodeId* outNodeId1, DockNodeId* outNodeId2)
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

void dockNodeSetWindow(DockNodeId parentNodeId, const char* windowId)
{
	DockNode* node = ctx->docking.dockNodeIdsMap[parentNodeId];

	ctx->docking.windowsDockNodeAssignments[windowId] = node->id;
}

void dockNodeLayoutRecalculate()
{
	for (auto& pair : ctx->docking.rootNativeWindowDockNodes)
	{
		pair.second->checkRedundancy();
		pair.second->computeRect();
	}
}

HTheme themeCreate(u32 atlasTextureSize)
{
	Theme* theme = new Theme(atlasTextureSize);

	ctx->themes.push_back(theme);

	return theme;
}

void themeSetUserSetting(HTheme theme, const char* name, const char* value)
{
	((Theme*)theme)->userSettings[name] = value;
}

const char* themeGetUserSetting(HTheme theme, const char* name)
{
	auto iter = ((Theme*)theme)->userSettings.find(name);

	if (iter == ((Theme*)theme)->userSettings.end())
		return "";

	return iter->second.c_str();
}

HImage themeAddImage(HTheme theme, const char* id, const ImageData& imgData)
{
	Theme* themePtr = (Theme*)theme;
	ImageId nid = hashString(id);

	auto iter = themePtr->images.find(nid);

	if (iter != themePtr->images.end())
	{
		return iter->second;
	}

	Image* timg = new Image();

	timg->width = imgData.width;
	timg->height = imgData.height;
	timg->pixels.resize(imgData.width * imgData.height);
	memcpy(timg->pixels.data(), imgData.pixels, sizeof(Rgba32) * imgData.width * imgData.height);
	themePtr->images[nid] = timg;

	return timg;
}

HImage themeGetImage(HTheme theme, const char* id)
{
	Theme* themePtr = (Theme*)theme;

	auto iter = themePtr->images.find(hashString(id));

	if (iter != themePtr->images.end())
		return iter->second;

	return nullptr;
}

void widgetSetStyle(WidgetType widgetType, const char* styleName)
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
	case WidgetType::MultilineTextInput:
		ctx->theme->elements[(u32)WidgetElementId::MultilineTextInputBody].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::TextInputCaret].setStyle(styleName); // Reuse caret
		ctx->theme->elements[(u32)WidgetElementId::TextInputSelection].setStyle(styleName); // Reuse selection
		ctx->theme->elements[(u32)WidgetElementId::MultilineTextInputLineNumbers].setStyle(styleName);
		ctx->theme->elements[(u32)WidgetElementId::MultilineTextInputCurrentLineHighlight].setStyle(styleName);
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

void widgetPushStyle(WidgetType widgetType, const char* styleName)
{
	if (ctx->widgetCurrentStyle.find(widgetType) == ctx->widgetCurrentStyle.end())
	{
		ctx->widgetCurrentStyle.insert(std::make_pair(widgetType, "default"));
	}

	ctx->widgetStyleStack.push_back(std::make_pair(widgetType, ctx->widgetCurrentStyle[widgetType]));

	widgetSetStyle(widgetType, styleName);
}

void widgetPopStyle()
{
	HUI_ASSERT(!ctx->widgetStyleStack.empty());

	if (ctx->widgetStyleStack.empty())
		return;

	auto& top = ctx->widgetStyleStack.back();
	widgetSetStyle(top.first, top.second.c_str());
	ctx->widgetStyleStack.pop_back();
}

void widgetSetElementStyle(WidgetElementId widgetElementId, const char* styleName)
{
	HUI_ASSERT(ctx);
	HUI_ASSERT(ctx->theme);
	ctx->theme->elements[(u32)widgetElementId].setStyle(styleName);
}

void widgetSetDefaultStyle(WidgetType widgetType)
{
	widgetSetStyle(widgetType, "default");
}

void widgetSetDefaultElementStyle(WidgetElementId widgetElementId)
{
	widgetSetElementStyle(widgetElementId, "default");
}

void widgetSetUserElementStyle(const char* elementName, const char* styleName)
{
	ctx->theme->userElements[elementName]->setStyle(styleName);
}

void themeBuild(HTheme theme)
{
	HUI_ASSERT(theme);
	if (!theme)
		return;

	Theme* themePtr = (Theme*)theme;
	themePtr->build();
}

void themeSetWidgetElement(
	HTheme theme,
	WidgetElementId elementId,
	WidgetStateType widgetStateType,
	const WidgetElementInfo& elementInfo,
	const char* styleName)
{
	HUI_ASSERT(theme);
	HUI_ASSERT(styleName);

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

void themeSetUserWidgetElement(
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

void themeSet(HTheme theme)
{
	HUI_ASSERT(theme);

	if (!theme)
		return;

	ctx->theme = (Theme*)theme;
}

HTheme themeGet()
{
	return ctx->theme;
}

ImageData themeGetAtlasImageData()
{
	ImageData img;

	img.width = ctx->theme->atlas.width;
	img.height = ctx->theme->atlas.height;
	img.pixels = ctx->theme->atlas.atlasImageData.data();

	return img;
}

void themeSetAtlasTexture(HTexture texture)
{
	ctx->theme->texture = texture;
}

HTexture themeGetAtlasTexture()
{
	return ctx->theme->texture;
}

void themeDestroy(HTheme theme)
{
	auto iter = std::find(ctx->themes.begin(), ctx->themes.end(), (Theme*)theme);

	if (iter == ctx->themes.end())
		return;

	delete *iter;
	ctx->themes.erase(iter);
	ctx->theme = nullptr;
}

void themeGetWidgetElementInfo(WidgetElementId elementId, WidgetStateType state, WidgetElementInfo& outInfo, const char* styleName)
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

void themeGetUserWidgetElementInfo(const char* userElementName, WidgetStateType state, WidgetElementInfo& outInfo, const char* styleName)
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

void themeSetWidgetElementParameter(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const char* paramValue)
{
	((Theme*)theme)->elements[(int)elementId].styles[styleName].parameters[paramName] = paramValue;
}

const char* themeGetWidgetElementParameterString(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const char* defaultValue)
{
	auto& style = ((Theme*)theme)->elements[(int)elementId].styles[styleName];

	auto iter = style.parameters.find(paramName);

	if (iter == style.parameters.end())
		return defaultValue;

	return iter->second.c_str();
}

f32 themeGetWidgetElementParameterFloat(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, f32 defaultValue)
{
	auto& style = ((Theme*)theme)->elements[(int)elementId].styles[styleName];

	return style.getParameter(paramName, defaultValue);
}

const Color& themeGetWidgetElementParameterColor(HTheme theme, WidgetElementId elementId, const char* styleName, const char* paramName, const Color& defaultValue)
{
	auto& style = ((Theme*)theme)->elements[(int)elementId].styles[styleName];

	return style.getColorParameter(paramName, defaultValue);
}

void themeSetUserWidgetElementParameter(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const char* paramValue)
{
	auto themePtr = ((Theme*)theme);

	if (themePtr->userElements.find(userElementName) == themePtr->userElements.end())
	{
		themePtr->userElements.insert(std::make_pair(userElementName, new ThemeElement()));
	}

	themePtr->userElements[userElementName]->styles[styleName].parameters[paramName] = paramValue;
}

const char* themeGetUserWidgetElementParameterString(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const char* defaultValue)
{
	auto& style = ((Theme*)theme)->userElements[userElementName]->styles[styleName];

	auto iter = style.parameters.find(paramName);

	if (iter == style.parameters.end())
		return defaultValue;

	return iter->second.c_str();
}

f32 themeGetUserWidgetElementParameterFloat(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, f32 defaultValue)
{
	auto& style = ((Theme*)theme)->userElements[userElementName]->styles[styleName];

	return style.getParameter(paramName, defaultValue);
}

const Color& themeGetUserWidgetElementParameterColor(HTheme theme, const char* userElementName, const char* styleName, const char* paramName, const Color& defaultValue)
{
	auto& style = ((Theme*)theme)->userElements[userElementName]->styles[styleName];

	return style.getColorParameter(paramName, defaultValue);
}

HFont themeFontCreate(HTheme theme, const char* name, const char* fontFilename, u32 faceSize)
{
	Theme* themePtr = (Theme*)theme;
	auto fnt = (HFont)themePtr->createFont(name, fontFilename, faceSize * ctx->scale);

	return fnt;
}

void themeFontDestroy(HTheme theme, HFont font)
{
	Theme* themePtr = (Theme*)theme;

	themePtr->deleteFont((Font*)font);
}

HFont themeFontGetFromTheme(HTheme theme, const char* themeFontName)
{
	Theme* themePtr = (Theme*)theme;

	return themePtr->getFont(themeFontName);
}

HFont themeFontGet(const char* themeFontName)
{
	return themeFontGetFromTheme(themeGet(), themeFontName);
}

void layoutBegin(const Rect& rect)
{
	layoutPush();
	auto paddedRect = rect.contract(paddingGet(PaddingType::Layout));
	ctx->layout.type = LayoutType::Generic;
	ctx->layout.savedPosition = paddedRect.topLeft();
	ctx->layout.width = paddedRect.width;
	ctx->layout.height = paddedRect.height;
	ctx->renderer.pushClipRect(paddedRect);
	ctx->position = { paddedRect.x, paddedRect.y};
	ctx->sameLine.enabled = false;
}

void layoutEnd()
{
	ctx->renderer.popClipRect();
	layoutPop();
	ctx->currentTabIndex = 0;
	ctx->selectedTabIndex = 0;
}

void idPush(const char* id)
{
	ctx->idStack.push_back(genId(id));
}

void idPush(u32 id)
{
	ctx->idStack.push_back(genId(id));
}

void idPush(void* id)
{
	ctx->idStack.push_back(genId(id));
}

void idPop()
{
	if (ctx->idStack.empty())
	{
		HUI_LOG("idPop used too many times");
		return;
	}

	ctx->idStack.pop_back();
}

void layoutPush()
{
	ctx->layout.savedSameLine = ctx->sameLine;
	ctx->layout.savedSameLineGroup = ctx->sameLineGroup;
	ctx->layoutStack.push_back(ctx->layout);
	ctx->sameLine = SameLineState();
	ctx->sameLineGroup = SameLineGroupState();
}

void layoutPop()
{
	if (ctx->layoutStack.empty())
	{
		HUI_LOG("popLayout used too many times");
		return;
	}

	ctx->layout = ctx->layoutStack.back();
	ctx->layoutStack.pop_back();

	ctx->sameLine = ctx->layout.savedSameLine;
	ctx->sameLineGroup = ctx->layout.savedSameLineGroup;
}

f32 layoutGetRemainingHeight()
{
	f32 remainingHeight = ctx->layout.height - (ctx->position.y - ctx->layout.savedPosition.y);

	return remainingHeight > 0 ? remainingHeight : 0;
}

f32 layoutGetRemainingWidth()
{
	f32 remainingWidth = ctx->layout.width - (ctx->position.x - ctx->layout.savedPosition.x);

	return remainingWidth > 0 ? remainingWidth : 0;
}


Point layoutGetSize()
{
	Point pt;

	pt.x = ctx->layout.width;
	pt.y = ctx->layout.height;

	return pt;
}

void layerIndexIncrement()
{
	ctx->layerIndex++;

	if (ctx->maxLayerIndex < ctx->layerIndex)
	{
		ctx->maxLayerIndex = ctx->layerIndex;
	}
}

u32 layerIndexDecrement()
{
	ctx->layerIndex--;

	return ctx->layerIndex;
}

void layerDecrementWindowMaxLayerIndex()
{
	ctx->maxLayerIndex--;

	if (ctx->maxLayerIndex == ~0)
	{
		ctx->maxLayerIndex = 0;
	}
}

void paddingPush(PaddingType type, const Point& newPadding)
{
	ctx->paddingStack[(i32)type].push_back(ctx->padding[(i32)type]);
	ctx->padding[(i32)type] = newPadding;
}

void widgetPaddingPush(const Point& newPadding)
{
	paddingPush(PaddingType::Widget, newPadding);
}

void paddingPop(PaddingType type)
{
	if (!ctx->paddingStack[(i32)type].empty())
	{
		ctx->padding[(i32)type] = ctx->paddingStack[(i32)type].back();
		ctx->paddingStack[(i32)type].pop_back();
	}
}

void widgetPaddingPop()
{
	paddingPop(PaddingType::Widget);
}

const Point& paddingGet(PaddingType type)
{
	return ctx->padding[(i32)type];
}

const Point& widgetGetPadding()
{
	return ctx->padding[(i32)PaddingType::Widget];
}

void spacingPush(f32 newSpacing)
{
	ctx->spacingStack.push_back(ctx->spacing);
	ctx->spacing = newSpacing;
}

void spacingPop()
{
	if (!ctx->spacingStack.empty())
	{
		ctx->spacing = ctx->spacingStack.back();
		ctx->spacingStack.pop_back();
	}
}

f32 spacingGet()
{
	return ctx->spacing;
}

void scaleSet(f32 scale)
{
	ctx->scale = scale;

	if (ctx->theme)
	{
		ctx->theme->rescaleFonts(scale);
	}
}

f32 scaleGet()
{
	return ctx->scale;
}

void tintPush(const Color& color, TintColorType type, TintColorOpType opType)
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

void tintPop()
{
	if (!ctx->tintStack.empty())
	{
		ctx->tint = ctx->tintStack.back();
		ctx->tintStack.pop_back();
	}
}

Color tintApply(const Color& originalColor, TintColorType type)
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

bool widgetIsHovered()
{
	return ctx->widget.hovered;
}

bool widgetIsFocused()
{
	return ctx->widget.focused;
}

bool widgetIsPressed()
{
	return ctx->widget.pressed;
}

bool widgetIsClicked()
{
	return ctx->widget.clicked;
}

bool widgetIsVisible()
{
	return ctx->widget.visible;
}

bool widgetIsChangeEnded()
{
	return ctx->widget.changeEnded;
}

WidgetId widgetGetId()
{
	return ctx->id;
}

Point mouseGetPosition()
{
	return ctx->mousePosition;
}

Point widgetGetPosition()
{
	return ctx->position;
}

void widgetSetPosition(const Point& position)
{
	ctx->position = position;
}

void widgetPushPosition()
{
	ctx->positionStack.push_back(ctx->position);
}

void widgetPopPosition()
{
	if (!ctx->positionStack.empty())
	{
		ctx->position = ctx->positionStack.back();
		ctx->positionStack.pop_back();
	}
}

Rect widgetGetRect()
{
	return ctx->widget.rect;
}

bool dragDropWantsTo()
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
		mouseCursorSetType(MouseCursorType::Arrow);
	}

	return false;
}

void dragDropSetMouseCursor(HMouseCursor dropAllowedCursor)
{
	ctx->dragDrop.dropAllowedCursor = dropAllowedCursor;
}

void dragDropBegin(u32 dragObjectType, void* dragObject)
{
	ctx->dragDrop.dragObject = dragObject;
	ctx->dragDrop.dragObjectType = dragObjectType;
	ctx->dragDrop.begunDragging = true;
}

void dragDropEnd()
{
	ctx->dragDrop.dragging = false;
	ctx->dragDrop.begunDragging = false;
	ctx->dragDrop.draggingIntent = false;
	ctx->dragDrop.dragObject = nullptr;
	ctx->dragDrop.dragObjectType = 0;
}

void dragDropAllow()
{
	ctx->dragDrop.allowDrop = true;

	if (ctx->dragDrop.begunDragging && ctx->widget.hovered)
	{
		ctx->dragDrop.foundDropTarget = true;
	}
}

void dragDropDisallow()
{
	ctx->dragDrop.allowDrop = false;
}

bool dragDropDroppedOnWidget()
{
	if (ctx->currentWindow
		&& ctx->dragDrop.begunDragging
		&& ctx->hoveringThisWindow
		&& ctx->settings.services.getFocusedWindow() != ctx->currentWindow->dockNode->nativeWindow)
	{
		//TODO: should it raise it or not ?
		ctx->settings.services.raiseWindow(ctx->currentWindow->dockNode->nativeWindow);
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

void* dragDropGetObject()
{
	return ctx->dragDrop.dragObject;
}

u32 dragDropGetObjectType()
{
	return ctx->dragDrop.dragObjectType;
}

static Color colorFromText(std::string colorText)
{
	if (colorText == "white") { return Color::white; }
	if (colorText == "black") { return Color::black; }
	if (colorText == "red") { return Color::red; }
	if (colorText == "darkRed") { return Color::darkRed; }
	if (colorText == "veryDarkRed") { return Color::veryDarkRed; }
	if (colorText == "green") { return Color::green; }
	if (colorText == "darkGreen") { return Color::darkGreen; }
	if (colorText == "veryDarkGreen") { return Color::veryDarkGreen; }
	if (colorText == "blue") { return Color::blue; }
	if (colorText == "darkBlue") { return Color::darkBlue; }
	if (colorText == "veryDarkBlue") { return Color::veryDarkBlue; }
	if (colorText == "yellow") { return Color::yellow; }
	if (colorText == "darkYellow") { return Color::darkYellow; }
	if (colorText == "veryDarkYellow") { return Color::veryDarkYellow; }
	if (colorText == "magenta") { return Color::magenta; }
	if (colorText == "cyan") { return Color::cyan; }
	if (colorText == "darkCyan") { return Color::darkCyan; }
	if (colorText == "veryDarkCyan") { return Color::veryDarkCyan; }
	if (colorText == "orange") { return Color::orange; }
	if (colorText == "darkOrange") { return Color::darkOrange; }
	if (colorText == "lightGray") { return Color::lightGray; }
	if (colorText == "gray") { return Color::gray; }
	if (colorText == "darkGray") { return Color::darkGray; }
	if (colorText == "sky") { return Color::sky; }
	if (colorText == "transparent") { return Color::transparent; }

	u32 r, g, b, a;

#ifdef _WINDOWS
	sscanf_s(colorText.c_str(), "%d %d %d %d", &r, &g, &b, &a);
#else
	sscanf(colorText.c_str(), "%d %d %d %d", &r, &g, &b, &a);
#endif

	return Color((f32)r / 255.0f, (f32)g / 255.0f, (f32)b / 255.0f, (f32)a / 255.0f);
}

Color colorFromText(const char* colorText)
{
	return colorFromText(std::string(colorText));
}

static u8 hexByte(const char* p)
{
	auto hex = [](char c) -> u8
		{
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return 0;
		};

	return (hex(p[0]) << 4) | hex(p[1]);
}

Color colorFromHex(const char* hexText)
{
	Color out{ 1.f, 1.f, 1.f, 1.f };

	if (!hexText)
		return out;

	// Skip optional '#'
	if (hexText[0] == '#')
		hexText++;

	size_t len = std::strlen(hexText);

	if (len < 1 || len > 8)
		return out;

	char buf[9] = {};
	memcpy(buf, hexText, len);
	if (len <= 6)
	{
		memset(buf + len, '0', 6 - len);
		buf[6] = 'F';
		buf[7] = 'F';
	}
	else if (len == 7)
	{
		buf[7] = '0';
	}

	hexText = buf;
	len = 8;

	u8 r = hexByte(hexText + 0);
	u8 g = hexByte(hexText + 2);
	u8 b = hexByte(hexText + 4);
	u8 a = hexByte(hexText + 6);

	out.r = r / 255.0f;
	out.g = g / 255.0f;
	out.b = b / 255.0f;
	out.a = a / 255.0f;

	return out;
}

u32 colorIntFromHex(const char* hexText)
{
	return colorFromHex(hexText).getRgba();
}

std::string colorToHex(const Color& color)
{
	auto clampToByte = [](float v) -> u8
		{
			v = std::clamp(v, 0.0f, 1.0f);
			return static_cast<u8>(v * 255.0f + 0.5f);
		};

	u8 r = clampToByte(color.r);
	u8 g = clampToByte(color.g);
	u8 b = clampToByte(color.b);
	u8 a = clampToByte(color.a);

	char buf[9];

	std::snprintf(buf, sizeof(buf), "%02X%02X%02X%02X", r, g, b, a);

	return std::string(buf);
}

std::string colorIntToHex(const u32 color)
{
	return colorToHex(Color(color));
}

Color colorHsvToRgb(const Color& hsv)
{
	f32 h = hsv.r;
	f32 s = hsv.g;
	f32 v = hsv.b;
	f32 r = 0;
	f32 g = 0;
	f32 b = 0;

	if (s <= 0.0f)
	{
		// Gray
		r = g = b = v;
		return Color(r, g, b, hsv.a);
	}

	h = std::fmod(h, 1.0f) * 6.0f;
	i32 i = (int)std::floor(h);
	f32 f = h - i;

	f32 p = v * (1.0f - s);
	f32 q = v * (1.0f - s * f);
	f32 t = v * (1.0f - s * (1.0f - f));

	switch (i)
	{
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	default: r = v; g = p; b = q; break;
	}

	return Color(r, g, b, hsv.a);
}

Color colorRgbToHsv(const Color& rgb)
{
	f32 r = rgb.r, g = rgb.g, b = rgb.b;
	f32 h = 0, s = 0, v = 0;

	f32 max = std::max(r, std::max(g, b));
	f32 min = std::min(r, std::min(g, b));
	f32 delta = max - min;

	v = max;

	if (max <= 0.0f)
	{
		// Black
		s = 0.0f;
		h = 0.0f;

		return Color(h, s, v, rgb.a);
	}

	s = delta / max;

	if (delta <= 0.0f)
	{
		// Gray
		h = 0.0f;
		return Color(h, s, v, rgb.a);
	}

	if (max == r)
		h = (g - b) / delta;
	else if (max == g)
		h = 2.0f + (b - r) / delta;
	else
		h = 4.0f + (r - g) / delta;

	h /= 6.0f;

	if (h < 0.0f)
		h += 1.0f;

	return Color(h, s, v, rgb.a);
}

Color colorHueToRgb(f32 h, f32 alpha)
{
	h = std::fmod(h, 1.0f);
	if (h < 0.0f) h += 1.0f;

	f32 r, g, b;

	f32 i = std::floor(h * 6.0f);
	f32 f = h * 6.0f - i;

	f32 q = 1.0f - f;
	f32 t = f;

	switch (i32(i) % 6)
	{
	case 0: r = 1; g = t; b = 0; break;
	case 1: r = q; g = 1; b = 0; break;
	case 2: r = 0; g = 1; b = t; break;
	case 3: r = 0; g = q; b = 1; break;
	case 4: r = t; g = 0; b = 1; break;
	default:r = 1; g = 0; b = q; break;
	}

	return { r, g, b, alpha };
}

void sameLineGroupBegin(u32 widgetCount)
{
	if (widgetCount == 0)
		return;

	ctx->sameLineGroup.active = true;
	ctx->sameLineGroup.widgetCount = widgetCount;
	ctx->sameLineGroup.currentWidget = 0;

	// Calculate equal width for each widget, accounting for spacing between them
	f32 totalSpacing = ctx->sameLine.spacing * (f32)(widgetCount - 1);

	ctx->sameLineGroup.widgetWidth = (ctx->layout.width - totalSpacing) / (f32)widgetCount;

	// Set nextSpacing to control the spacing after the first widget
	ctx->sameLine.nextSpacing = ctx->sameLine.spacing;

	// Set width for first widget using the proper API
	widgetSetNextWidth(ctx->sameLineGroup.widgetWidth);
}

void sameLineGroupNext()
{
	if (!ctx->sameLineGroup.active)
		return;

	ctx->sameLineGroup.currentWidget++;

	if (ctx->sameLineGroup.currentWidget >= ctx->sameLineGroup.widgetCount)
		return;

	// sameLine() will set nextSpacing automatically
	sameLine();

	// Set width for next widget using the proper API
	widgetSetNextWidth(ctx->sameLineGroup.widgetWidth);
}

void sameLineGroupEnd()
{
	if (!ctx->sameLineGroup.active)
		return;

	// Disable sameLine
	ctx->sameLine.enabled = false;

	// Reset state
	ctx->sameLineGroup.active = false;
	ctx->sameLineGroup.widgetCount = 0;
	ctx->sameLineGroup.currentWidget = 0;
}

}
