#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "theme.h"
#include "context.h"
#include "docking_system.h"
#include "view.h"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <thread>

#ifdef HORUS_TIMING_DEBUG
#include <ctime>
#include <chrono>
#include <ratio>
#endif

namespace hui
{
void setCurrentViewHandler(ViewHandler* handler)
{
	ctx->currentViewHandler = handler;
}

ViewHandler* getCurrentViewHandler()
{
	return ctx->currentViewHandler;
}

void updateDockingSystemInternal(bool isLastEvent)
{
	hui::beginFrame();

	if (hui::getInputEvent().type == InputEvent::Type::WindowClose
		&& hui::getInputEvent().window != hui::getMainWindow())
	{
		//auto node = (DockNode*)getRootDockNode(hui::getInputEvent().window);
		hui::destroyWindow(hui::getInputEvent().window);
	}

	std::vector<DockNode*> viewTabsNodes;

	for (auto& nodeIter : ctx->dockingState.rootWindowDockNodes)
	{
		auto rootNode = nodeIter.second;
		auto wnd = rootNode->window;

		if (!wnd)
		{
			continue;
		}

		auto wndRect = ctx->providers->input->getWindowRect(wnd);

		wndRect.x = 0;
		wndRect.y = 0;

		hui::setWindow(wnd);
		hui::beginWindow(wnd);
		ctx->currentViewHandler->onBeforeFrameRender(wnd);
		hui::clearBackground();

		if ((hui::getInputEvent().type == InputEvent::Type::WindowResize
			|| hui::getInputEvent().type == InputEvent::Type::WindowGotFocus)
			&& hui::getInputEvent().window == wnd)
		{
			hui::forceRepaint();
		}

		//TODO: cache the nodes for each root node window, so its not every frame
		viewTabsNodes.clear();
		rootNode->gatherViewTabsNodes(viewTabsNodes);
		ctx->dockingState.currentDockNode = rootNode;
		ctx->dockingState.closeWindow = false;

		beginContainer(wndRect);
		f32 oldY = 0;

		/*
		// top area
		ctx->currentViewHandler->onTopAreaRender(wnd);
		//node->sideSpacing[(int)ViewContainer::SideSpacing::SideSpacingTop] = ctx->penPosition.y;
		oldY = ctx->penPosition.y;

		// left area
		ctx->penPosition.x = 0;
		beginContainer({ ctx->penPosition.x, ctx->penPosition.y, 0, wndRect.height - oldY });
		ctx->currentViewHandler->onLeftAreaRender(wnd);
		endContainer();

		// right area
		ctx->penPosition.x = wndRect.width - viewCtr->sideSpacing[(int)ViewContainer::SideSpacing::SideSpacingRight];
		ctx->penPosition.y = oldY;
		beginContainer({ ctx->penPosition.x, ctx->penPosition.y, viewCtr->sideSpacing[(int)ViewContainer::SideSpacing::SideSpacingRight], wndRect.height - oldY - viewCtr->sideSpacing[(int)ViewContainer::SideSpacing::SideSpacingBottom] });
		ctx->currentViewHandler->onRightAreaRender(wnd);
		endContainer();

		// bottom area
		ctx->penPosition.x = 0;
		ctx->penPosition.y = wndRect.height - viewCtr->sideSpacing[(int)ViewContainer::SideSpacing::SideSpacingBottom];
		beginContainer({ ctx->penPosition.x, ctx->penPosition.y, wndRect.width, viewCtr->sideSpacing[(int)ViewContainer::SideSpacing::SideSpacingBottom] });
		ctx->currentViewHandler->onBottomAreaRender(wnd);
		endContainer();
		*/

		for (auto& viewTabsNode : viewTabsNodes)
		{
			for (auto& v : viewTabsNode->views)
			{
				beginView(v);
				ctx->currentViewHandler->onViewRender(wnd, v, v->viewType, v->userData);
				endView();
			}
		}

		endContainer();
		endWindow();
		ctx->currentViewHandler->onAfterFrameRender(wnd);
		ctx->dockingState.currentDockNode = nullptr;
		//handleDockNodeResize(v);

		if ((isLastEvent
			&& !ctx->skipRenderAndInput
			&& !ctx->renderer->disableRendering
			&& !ctx->renderer->skipRender)
			|| ctx->dockingTabPane)
		{
			hui::presentWindow(wnd);
		}

		if (ctx->dockingState.closeWindow)
		{
			hui::deleteRootDockNode(wnd);
			hui::destroyWindow(wnd);
		}
	}

	hui::endFrame();
}

void updateDockingSystem()
{
	hui::processInputEvents();

	if (hui::hasNothingToDo())
	{
		std::this_thread::yield();
		return;
	}

	ctx->mustRedraw = false;

	auto doLogicAndRender = [](bool isLastEvent)
	{
		ctx->renderer->disableRendering = !isLastEvent && !ctx->skipRenderAndInput;
		hui::updateDockingSystemInternal(isLastEvent);
	};

	if (ctx->events.size() == 0)
	{
		doLogicAndRender(true);
	}

	// execute logic and render for all events that might be in the queue
	for (int i = 0; i < ctx->events.size(); i++)
	{
		ctx->event = ctx->events[i];
		doLogicAndRender(i == ctx->events.size() - 1);
	}
}

void dockingSystemLoop()
{
#ifdef HORUS_TIMING_DEBUG
	using namespace std;
	using namespace std::chrono;

	high_resolution_clock::time_point t1, t2;
	duration<double, std::milli> total;
#endif
	while (!hui::mustQuit())
	{
#ifdef HORUS_TIMING_DEBUG
		t1 = high_resolution_clock::now();
#endif

		updateDockingSystem();

#ifdef HORUS_TIMING_DEBUG
		t2 = high_resolution_clock::now();
		total = t2 - t1;

		printf("%fms\n", total.count());
#endif
	}
}

//void resizeRootViewPaneToSides(ViewPane* viewPane)
//{
//	auto rect = getWindowRect(viewPane->window);
//
//	viewPane->rect.x = viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingLeft];
//	viewPane->rect.y = viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingTop];
//	viewPane->rect.width = rect.width - (viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingLeft] + viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingRight]);
//	viewPane->rect.height = rect.height - (viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingTop] + viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingBottom]);
//}

void handleDockNodeResize(DockNode* node)
{
	//TODO: maybe put all these in the current context
	const i32 gripSize = 8;
	const f32 dockBorderSizePercent = 0.5f;
	static bool draggingDockNodeBorder = false;
	static bool draggingView = false;
	static DockNode* draggingNodeSource = nullptr;
	static DockNode* resizingNode = nullptr;
	static View* dragView = nullptr;
	static View* dragOntoTab = nullptr;
	static Rect dockRect;
	static DockType dockType = DockType::Left;
	static DockNode* dockToNode = nullptr;
	static Rect resizePaneRect;
	static Rect resizePaneSiblingRect;
	static Point lastMousePos;
	static Rect rectDragged;

	auto& rect = node->rect;
	auto& event = hui::getInputEvent();

	//TODO: find current window index better
	// find if the current window of the view pane had a layer index > 0
	// if so, then we must be having popups or menus
	if (ctx->maxLayerIndex)
	{
		return;
	}

	// is the event for this window ?
	if (event.window != node->window)
	{
		return;
	}

	if (event.type == InputEvent::Type::MouseDown)
	{
		const Point& mousePos = event.mouse.point;
		lastMousePos = mousePos;
		resizingNode = node->findResizeNode(mousePos);
		std::vector<DockNode*> viewTabsNodes;

		node->gatherViewTabsNodes(viewTabsNodes);
		dragView = nullptr;
		draggingNodeSource = node;

		for (auto& viewTabsNode : viewTabsNodes)
		{
			for (auto& view : viewTabsNode->views)
			{
				// return if the widget is not visible, that is outside current clip rect
				if (view->tabRect.outside(viewTabsNode->rect))
				{
					continue;
				}

				Rect clippedRect = view->tabRect.clipInside(viewTabsNode->rect);

				if (clippedRect.contains(mousePos.x, mousePos.y))
				{
					dragView = view;
					break;
				}
			}
		}

		if (resizingNode)
		{
			draggingDockNodeBorder = true;
		}
	}

	if (event.type == InputEvent::Type::MouseUp)
	{
		const f32 moveTriggerDelta = 3;
		bool moved = fabs(lastMousePos.x - event.mouse.point.x) > moveTriggerDelta || fabs(lastMousePos.y - event.mouse.point.y) > moveTriggerDelta;

		// we have a pane to dock to
		if (dockToNode && moved)
		{
			dockView(dragView, dockToNode, dockType);

			//if (draggingNodeSource->children.empty()
			//	&& draggingNodeSource->splitMode == ViewPane::SplitMode::None
			//	&& !draggingNodeSource)
			//{
			//	if (hui::getMainWindow() != draggingNodeSource->window)
			//	{
			//		destroyWindow(draggingNodeSource->window);
			//		auto iter = std::find(ctx->dockingState.rootViewPanes.begin(), ctx->dockingState.rootViewPanes.end(), draggingNodeSource);

			//		ctx->dockingState.rootViewPanes.erase(iter);
			//		//TODO: crashes LLVM on MacOS, check for dangling ptrs
			//		draggingNodeSource->destroy();
			//		delete draggingNodeSource;
			//		draggingNodeSource = nullptr;
			//	}
			//}

			dockToNode = nullptr;

			hui::forceRepaint();
			//}
		}
		//else if (dragTab && !dragOntoTab && moved)
		//{
		//	// do not undock if the source window is the main window and there is just one tab left!
		//	if (!(dragTab->viewPane->viewTabs.size() == 1
		//		&& draggingNodeSource == dragTab->viewPane)
		//		&& ctx->settings.allowUndockingToNewWindow)
		//	{
		//		Rect rc = hui::getWindowRect(draggingNodeSource->window);

		//		auto paneWnd = hui::createWindow(
		//			"", //TODO: title 
		//			dragTab->viewPane->rect.width,
		//			dragTab->viewPane->rect.height,
		//			WindowFlags::Resizable | WindowFlags::NoTaskbarButton | WindowFlags::CustomPosition,
		//			{
		//				crtEvent.mouse.point.x + rc.x - dragTab->viewPane->rect.width / 2,
		//				crtEvent.mouse.point.y + rc.y
		//			});

		//		auto newViewPane = createRootViewPane(paneWnd);

		//		// we don't have to create a new view pane if it holds just one view tab
		//		if (dragTab->viewPane->viewTabs.size() == 1)
		//		{
		//			viewPane->removeChild(dragTab->viewPane);
		//			//hui::dockViewPane(dragTab->viewPane, newViewPane, DockType::TopAsViewTab);
		//		}
		//		else
		//		{
		//			// remove from old pane
		//			dragTab->viewPane->removeViewTab(dragTab);
		//			auto newPane = (ViewPane*)hui::createEmptyViewPane(newViewPane, DockType::TopAsViewTab);
		//			newPane->viewTabs.push_back(dragTab);
		//			dockViewTab(newPane, );
		//			dragTab->viewPane = newPane;
		//		}
		//	}

		//	hui::forceRepaint();
		//}

		draggingNodeSource = nullptr;
		draggingDockNodeBorder = false;
		draggingView = false;
		dragView = nullptr;
		resizingNode = nullptr;
	}

}

void handleDockNodeResize2(DockNode* node)
{
	auto& rect = node->rect;
	auto& crtEvent = hui::getInputEvent();

	//TODO: find current window index better
	// find if the current window of the view pane had a layer index > 0
	// if so, then we must be having popups or menus
	if (ctx->maxLayerIndex)
	{
		return;
	}

	// is the event for this window ?
	if (crtEvent.window != node->window)
	{
		return;
	}

	//TODO: maybe put all these in the current context
	const i32 gripSize = 8;
	const f32 dockBorderSizePercent = 0.5f;
	static bool draggingDockNodeBorder = false;
	static bool draggingView = false;
	static DockNode* draggingNodeSource = nullptr;
	static DockNode* resizingNode = nullptr;
	static View* dragView = nullptr;
	static View* dragOntoTab = nullptr;
	static Rect dockRect;
	static DockType dockType = DockType::Left;
	static DockNode* dockToNode = nullptr;
	static Rect resizePaneRect;
	static Rect resizePaneSiblingRect;
	static Point lastMousePos;
	static Rect rectDragged;

	if (crtEvent.type == InputEvent::Type::MouseDown)
	{
		const Point& mousePos = crtEvent.mouse.point;
		lastMousePos = mousePos;
		resizingNode = node->findResizeNode(mousePos);
		std::vector<DockNode*> viewTabsNodes;

		node->gatherViewTabsNodes(viewTabsNodes);
		dragView = nullptr;
		draggingNodeSource = node;

		for (auto& viewTabsNode : viewTabsNodes)
		{
			for (auto& view : viewTabsNode->views)
			{
				// return if the widget is not visible, that is outside current clip rect
				if (view->tabRect.outside(viewTabsNode->rect))
				{
					continue;
				}

				Rect clippedRect = view->tabRect.clipInside(viewTabsNode->rect);

				if (clippedRect.contains(mousePos.x, mousePos.y))
				{
					dragView = view;
					draggingView = true;
					break;
				}
			}
		}

		if (resizingNode)
		{
			draggingDockNodeBorder = true;
		}
	}

	// execute docking or undocking
	if (crtEvent.type == InputEvent::Type::MouseUp)
	{
		const f32 moveTriggerDelta = 3;
		bool moved = fabs(lastMousePos.x - crtEvent.mouse.point.x) > moveTriggerDelta || fabs(lastMousePos.y - crtEvent.mouse.point.y) > moveTriggerDelta;

		// we have a pane to dock to
		if (dockToNode && moved)
		{
			dockView(dragView, dockToNode, dockType);

				//if (draggingNodeSource->children.empty()
				//	&& draggingNodeSource->splitMode == ViewPane::SplitMode::None
				//	&& !draggingNodeSource)
				//{
				//	if (hui::getMainWindow() != draggingNodeSource->window)
				//	{
				//		destroyWindow(draggingNodeSource->window);
				//		auto iter = std::find(ctx->dockingState.rootViewPanes.begin(), ctx->dockingState.rootViewPanes.end(), draggingNodeSource);

				//		ctx->dockingState.rootViewPanes.erase(iter);
				//		//TODO: crashes LLVM on MacOS, check for dangling ptrs
				//		draggingNodeSource->destroy();
				//		delete draggingNodeSource;
				//		draggingNodeSource = nullptr;
				//	}
				//}

				dockToNode = nullptr;

				hui::forceRepaint();
			//}
		}
		//else if (dragTab && !dragOntoTab && moved)
		//{
		//	// do not undock if the source window is the main window and there is just one tab left!
		//	if (!(dragTab->viewPane->viewTabs.size() == 1
		//		&& draggingNodeSource == dragTab->viewPane)
		//		&& ctx->settings.allowUndockingToNewWindow)
		//	{
		//		Rect rc = hui::getWindowRect(draggingNodeSource->window);

		//		auto paneWnd = hui::createWindow(
		//			"", //TODO: title 
		//			dragTab->viewPane->rect.width,
		//			dragTab->viewPane->rect.height,
		//			WindowFlags::Resizable | WindowFlags::NoTaskbarButton | WindowFlags::CustomPosition,
		//			{
		//				crtEvent.mouse.point.x + rc.x - dragTab->viewPane->rect.width / 2,
		//				crtEvent.mouse.point.y + rc.y
		//			});

		//		auto newViewPane = createRootViewPane(paneWnd);

		//		// we don't have to create a new view pane if it holds just one view tab
		//		if (dragTab->viewPane->viewTabs.size() == 1)
		//		{
		//			viewPane->removeChild(dragTab->viewPane);
		//			//hui::dockViewPane(dragTab->viewPane, newViewPane, DockType::TopAsViewTab);
		//		}
		//		else
		//		{
		//			// remove from old pane
		//			dragTab->viewPane->removeViewTab(dragTab);
		//			auto newPane = (ViewPane*)hui::createEmptyViewPane(newViewPane, DockType::TopAsViewTab);
		//			newPane->viewTabs.push_back(dragTab);
		//			dockViewTab(newPane, );
		//			dragTab->viewPane = newPane;
		//		}
		//	}

		//	hui::forceRepaint();
		//}

		draggingNodeSource = nullptr;
		draggingDockNodeBorder = false;
		draggingView = false;
		dragView = nullptr;
		resizingNode = nullptr;
	}

	const auto mousePos = ctx->providers->input->getMousePosition();
	auto overPaneToResize = viewPane->findResizeViewPane(mousePos, gripSize);

	// if we drag a tab or a pane splitter
	if (crtEvent.type == InputEvent::Type::MouseDown
		|| dragView
		|| overPaneToResize
		|| resizingNode)
	{
		auto paneToResize = overPaneToResize;
		const int moveOffsetTriggerSize = 3;
		bool moved = fabs(lastMousePos.x - mousePos.x) > moveOffsetTriggerSize || abs(lastMousePos.y - mousePos.y) > moveOffsetTriggerSize;

		if (dragView)
		{
			// draw tab rect
			auto& dockingRectElem = ctx->theme->getElement(WidgetElementId::ViewPaneDockRect);
			auto& dockingDialRectElem = ctx->theme->getElement(WidgetElementId::ViewPaneDockDialRect);
			auto& dockingDialRectVSplitElem = ctx->theme->getElement(WidgetElementId::ViewPaneDockDialVSplitRect);
			auto& dockingDialRectHSplitElem = ctx->theme->getElement(WidgetElementId::ViewPaneDockDialHSplitRect);
			auto& tabGroupElem = ctx->theme->getElement(WidgetElementId::TabGroupBody);
			auto zorder = ctx->renderer->getZOrder();

			if (crtEvent.type == InputEvent::Type::MouseDown || ctx->mouseMoved || moved)
			{
				std::vector<ViewTab*> tabs;

				viewPane->gatherViewTabs(tabs);
				dragOntoTab = nullptr;

				for (auto tab : tabs)
				{
					if (tab->rect.contains(mousePos.x, mousePos.y))
					{
						dragOntoTab = tab;
						break;
					}
				}

				//TODO: visualize better the tab insertion
				// if we dragged on some other tab
				if (dragOntoTab != dragView
					&& dragView
					&& dragOntoTab)
				{
					if (dragView->viewPane == dragOntoTab->viewPane)
					{
						// same pane
						size_t index1 = dragView->viewPane->getViewTabIndex(dragView);
						size_t index2 = dragView->viewPane->getViewTabIndex(dragOntoTab);

						dragView->viewPane->viewTabs[index1] = dragOntoTab;
						dragView->viewPane->viewTabs[index2] = dragView;
						dragView->viewPane->selectedTabIndex = index2;
					}
				}

				// check for docking on pane views
				dockToNode = viewPane->findDockViewPane(mousePos);
			}

			bool isSamePane = dockToNode == dragView->viewPane;
			bool isPaneSingleTab = dragView->viewPane->viewTabs.size() == 1;

			if (!dockToNode)
			{
				if (dragOntoTab && !isSamePane)
				{
					rectDragged = dragView->rect;
					rectDragged.x = mousePos.x - rectDragged.width / 2;
					rectDragged.y = mousePos.y - rectDragged.height / 2;
					rectDragged.height = dragView->rect.height * 2; // we have two lines of text when undocking "Undock\nTabname"
					dockToNode = dragOntoTab->viewPane;
				}
				else
				{
					// docking not allowed
					dockToNode = nullptr;
				}
			}
			
			if (dragView != dragOntoTab)
			{
				ctx->renderer->setWindowSize({ rect.width, rect.height });
				ctx->renderer->beginFrame();
				ctx->renderer->cmdSetColor(dockingRectElem.normalState().color);

				if (dockToNode)
				{
					// draw the locations where we can dock the pane
					const f32 smallRectSize = 64 * ctx->globalScale;
					const f32 smallRectGap = 1 * ctx->globalScale;
					auto parentRect = dockToNode->rect;
		
					auto smallRectLeft = Rect(
						parentRect.x + parentRect.width / 2.0f - smallRectSize / 2.0f - smallRectGap - smallRectSize,
						parentRect.y + parentRect.height / 2.0f - smallRectSize / 2.0f,
						smallRectSize, smallRectSize);
					
					auto smallRectRight = Rect(
						parentRect.x + parentRect.width / 2.0f + smallRectSize / 2.0f + smallRectGap,
						parentRect.y + parentRect.height / 2.0f - smallRectSize / 2.0f,
						smallRectSize, smallRectSize);
					
					auto smallRectTop = Rect(
						parentRect.x + parentRect.width / 2.0f - smallRectSize / 2.0f,
						parentRect.y + parentRect.height / 2.0f - smallRectSize / 2.0f - smallRectSize - smallRectGap,
						smallRectSize, smallRectSize);
					
					auto smallRectBottom = Rect(
						parentRect.x + parentRect.width / 2.0f - smallRectSize / 2.0f,
						parentRect.y + parentRect.height / 2 + smallRectSize / 2 + smallRectGap,
						smallRectSize, smallRectSize);
					
					auto smallRectMiddle = Rect(
						parentRect.x + parentRect.width / 2.0f - smallRectSize / 2.0f,
						parentRect.y + parentRect.height / 2.0f - smallRectSize / 2.0f,
						smallRectSize, smallRectSize);

					auto rootRect = viewPane->rect;

					auto smallRectRootLeft = Rect(
						rootRect.x + smallRectGap,
						rootRect.y + rootRect.height / 2.0f - smallRectSize / 2.0f,
						smallRectSize, smallRectSize);

					auto smallRectRootRight = Rect(
						rootRect.right() - smallRectGap - smallRectSize,
						rootRect.y + rootRect.height / 2.0f - smallRectSize / 2.0f,
						smallRectSize, smallRectSize);

					auto smallRectRootTop = Rect(
						rootRect.x + rootRect.width / 2.0f - smallRectSize / 2.0f,
						rootRect.y + smallRectGap,
						smallRectSize, smallRectSize);

					auto smallRectRootBottom = Rect(
						rootRect.x + rootRect.width / 2.0f - smallRectSize / 2.0f,
						rootRect.bottom() - smallRectSize / 2.0f - smallRectGap,
						smallRectSize, smallRectSize);

					auto isSmallRectLeftHovered = smallRectLeft.contains(mousePos);
					auto isSmallRectRightHovered = smallRectRight.contains(mousePos);
					auto isSmallRectTopHovered = smallRectTop.contains(mousePos);
					auto isSmallRectBottomHovered = smallRectBottom.contains(mousePos);
					auto isSmallRectMiddleHovered = smallRectMiddle.contains(mousePos);
					auto isSmallRectRootLeftHovered = smallRectRootLeft.contains(mousePos);
					auto isSmallRectRootRightHovered = smallRectRootRight.contains(mousePos);
					auto isSmallRectRootTopHovered = smallRectRootTop.contains(mousePos);
					auto isSmallRectRootBottomHovered = smallRectRootBottom.contains(mousePos);

					if (isSmallRectLeftHovered)
					{
						dockType = DockType::Left;
						rectDragged = parentRect;
						rectDragged.width /= 2;
					}

					if (isSmallRectRightHovered)
					{
						dockType = DockType::Right;
						rectDragged = parentRect;
						rectDragged.x += rectDragged.width / 2;
						rectDragged.width /= 2;
					}

					if (isSmallRectTopHovered)
					{
						dockType = DockType::Top;
						rectDragged = parentRect;
						rectDragged.height /= 2;
					}

					if (isSmallRectBottomHovered)
					{
						dockType = DockType::Bottom;
						rectDragged = parentRect;
						rectDragged.y += rectDragged.height / 2;
						rectDragged.height /= 2;
					}

					if (isSmallRectMiddleHovered)
					{
						dockType = DockType::AsTab;
						rectDragged = parentRect;
						rectDragged.height = tabGroupElem.normalState().height;
					}

					if (isSmallRectRootLeftHovered)
					{
						dockToNode = viewPane;
						dockType = DockType::RootLeft;
						rectDragged = dockToNode->rect;
						rectDragged.width /= 2.0f;
					}
					
					if (isSmallRectRootRightHovered)
					{
						dockToNode = viewPane;
						dockType = DockType::RootRight;
						rectDragged = dockToNode->rect;
						rectDragged.x += rectDragged.width / 2.0f;
						rectDragged.width /= 2.0f;
					}
					
					if (isSmallRectRootTopHovered)
					{
						dockToNode = viewPane;
						dockType = DockType::RootTop;
						rectDragged = dockToNode->rect;
						rectDragged.height /= 2.0f;
					}
					
					if (isSmallRectRootBottomHovered)
					{
						dockToNode = viewPane;
						dockType = DockType::RootBottom;
						rectDragged = dockToNode->rect;
						rectDragged.y += rectDragged.height / 2.0f;
						rectDragged.height /= 2.0f;
					}

					ctx->renderer->cmdSetColor(isSmallRectLeftHovered ? dockingDialRectVSplitElem.hoveredState().color : dockingDialRectVSplitElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectVSplitElem.normalState().image,
						dockingDialRectVSplitElem.normalState().border, smallRectLeft, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectRightHovered ? dockingDialRectVSplitElem.hoveredState().color : dockingDialRectVSplitElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectVSplitElem.normalState().image,
						dockingDialRectVSplitElem.normalState().border, smallRectRight, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectTopHovered ? dockingDialRectHSplitElem.hoveredState().color : dockingDialRectHSplitElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectHSplitElem.normalState().image,
						dockingDialRectHSplitElem.normalState().border, smallRectTop, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectBottomHovered ? dockingDialRectHSplitElem.hoveredState().color : dockingDialRectHSplitElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectHSplitElem.normalState().image,
						dockingDialRectHSplitElem.normalState().border, smallRectBottom, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectMiddleHovered ? dockingDialRectElem.hoveredState().color : dockingDialRectElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectElem.normalState().image,
						dockingDialRectElem.normalState().border, smallRectMiddle, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectRootLeftHovered ? dockingDialRectVSplitElem.hoveredState().color : dockingDialRectVSplitElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectVSplitElem.normalState().image,
						dockingDialRectVSplitElem.normalState().border, smallRectRootLeft, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectRootRightHovered ? dockingDialRectVSplitElem.hoveredState().color : dockingDialRectVSplitElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectVSplitElem.normalState().image,
						dockingDialRectVSplitElem.normalState().border, smallRectRootRight, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectRootTopHovered ? dockingDialRectHSplitElem.hoveredState().color : dockingDialRectHSplitElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectHSplitElem.normalState().image,
						dockingDialRectHSplitElem.normalState().border, smallRectRootTop, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectRootBottomHovered ? dockingDialRectHSplitElem.hoveredState().color : dockingDialRectHSplitElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectHSplitElem.normalState().image,
						dockingDialRectHSplitElem.normalState().border, smallRectRootBottom, ctx->globalScale);
				}

				ctx->renderer->cmdDrawImageBordered(dockingRectElem.normalState().image, dockingRectElem.normalState().border, rectDragged, ctx->globalScale);
				ctx->renderer->cmdSetFont(dockingRectElem.normalState().font);
				ctx->renderer->cmdSetColor(dockingRectElem.normalState().textColor);
				ctx->drawMultilineText(dragView->title.c_str(), rectDragged, HAlignType::Center, VAlignType::Center);
				ctx->renderer->endFrame();
			}
		}

		// are we resizing a pane ?
		if ((paneToResize || resizingNode) && !dragView)
		{
			ViewPane::SplitMode splitType = ViewPane::SplitMode::None;

			if (paneToResize)
			{
				if (paneToResize->parent)
				{
					splitType = paneToResize->parent->splitMode;
				}
			}

			if (resizingNode)
			{
				if (resizingNode->parent)
				{
					splitType = resizingNode->parent->splitMode;
				}

				if (crtEvent.type == InputEvent::Type::MouseDown)
				{
					resizePaneRect = resizingNode->rect;
					auto iterNextPane = std::find(resizingNode->parent->children.begin(), resizingNode->parent->children.end(), resizingNode);

					iterNextPane++;

					if (iterNextPane != resizingNode->parent->children.end())
					{
						resizePaneSiblingRect = (*iterNextPane)->rect;
					}
				}
			}

			switch (splitType)
			{
			case ViewPane::SplitMode::Horizontal:
			{
				ctx->mouseCursor = MouseCursorType::SizeWE;
				break;
			}
			case ViewPane::SplitMode::Vertical:
			{
				ctx->mouseCursor = MouseCursorType::SizeNS;
				break;
			}
			default:
				ctx->mouseCursor = MouseCursorType::Arrow;
				break;
			}
		}
	}

	if (draggingDockNodeBorder
		&& resizingNode
		&& resizingNode->parent)
	{
		switch (resizingNode->parent->splitMode)
		{
		case ViewPane::SplitMode::Horizontal:
		{
			f32 normalizedSizeBothSiblings = (resizePaneRect.width + resizePaneSiblingRect.width) / resizingNode->parent->rect.width;
			f32 normalizedSize1 = (mousePos.x - resizePaneRect.x) / resizingNode->parent->rect.width;
			f32 normalizedSize2 = (resizePaneSiblingRect.right() - mousePos.x) / resizingNode->parent->rect.width;
			auto iterNextPane = std::find(resizingNode->parent->children.begin(), resizingNode->parent->children.end(), resizingNode);

			iterNextPane++;
			//resizingNode->normalizedSize.x = normalizedSize1;

			//if (resizingNode->normalizedSize.x < resizingNode->minNormalizedSize.x)
			//{
			//	resizingNode->normalizedSize.x = resizingNode->minNormalizedSize.y;
			//}

			if (iterNextPane != resizingNode->parent->children.end())
			{
				//if (resizingNode->normalizedSize.x + normalizedSize2 > normalizedSizeBothSiblings)
				//{
				//	normalizedSize2 = normalizedSizeBothSiblings - resizingNode->normalizedSize.x;
				//}

				//(*iterNextPane)->normalizedSize.x = normalizedSize2;

				//if ((*iterNextPane)->normalizedSize.x < resizingNode->minNormalizedSize.x)
				//{
				//	(*iterNextPane)->normalizedSize.x = resizingNode->minNormalizedSize.x;
				//	resizingNode->normalizedSize.x = normalizedSizeBothSiblings - (*iterNextPane)->normalizedSize.x;
				//}
			}

			break;
		}

		case ViewPane::SplitMode::Vertical:
		{
			f32 normalizedSizeBothSiblings = (resizePaneRect.height + resizePaneSiblingRect.height) / resizingNode->parent->rect.height;
			f32 normalizedSize1 = (mousePos.y - resizePaneRect.y) / resizingNode->parent->rect.height;
			f32 normalizedSize2 = (resizePaneSiblingRect.bottom() - mousePos.y) / resizingNode->parent->rect.height;
			auto iterNextPane = std::find(resizingNode->parent->children.begin(), resizingNode->parent->children.end(), resizingNode);

			iterNextPane++;
			//resizingNode->normalizedSize.y = normalizedSize1;

			//if (resizingNode->normalizedSize.y < resizingNode->minNormalizedSize.x)
			//{
			//	resizingNode->normalizedSize.y = resizingNode->minNormalizedSize.y;
			//}

			//if (iterNextPane != resizingNode->parent->children.end())
			//{
			//	if (resizingNode->normalizedSize.y + normalizedSize2 > normalizedSizeBothSiblings)
			//	{
			//		normalizedSize2 = normalizedSizeBothSiblings - resizingNode->normalizedSize.y;
			//	}

			//	(*iterNextPane)->normalizedSize.y = normalizedSize2;

			//	if ((*iterNextPane)->normalizedSize.y < resizingNode->minNormalizedSize.y)
			//	{
			//		(*iterNextPane)->normalizedSize.y = resizingNode->minNormalizedSize.y;
			//		resizingNode->normalizedSize.y = normalizedSizeBothSiblings - (*iterNextPane)->normalizedSize.y;
			//	}
			//}

			break;
		}
		}

		hui::forceRepaint();
	}

	ctx->dockingTabPane = draggingView;
}

}