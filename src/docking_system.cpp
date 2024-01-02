#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "theme.h"
#include "context.h"
#include "docking_system.h"
#include "docking.h"
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
/*
void updateDockingSystemInternal(bool isLastEvent)
{
	// if we close a dockable native window
	if (hui::getInputEvent().type == InputEvent::Type::WindowClose
		&& hui::getInputEvent().window != HORUS_INPUT->getMainWindow())
	{
		auto node = getRootDockNode(hui::getInputEvent().window);		
		deleteRootDockNode(hui::getInputEvent().window);
		destroyOsWindow(hui::getInputEvent().window);
	}

	std::vector<DockNode*> nodes;

	for (auto& nodeIter : ctx->dockingState.rootOsWindowDockNodes)
	{
		auto rootNode = nodeIter.second;
		auto osWnd = rootNode->osWindow;

		if (!osWnd)
		{
			continue;
		}

		auto osWndRect = ctx->providers->input->getWindowRect(osWnd);

		osWndRect.x = 0;
		osWndRect.y = 0;

		if ((hui::getInputEvent().type == InputEvent::Type::WindowResize
			|| hui::getInputEvent().type == InputEvent::Type::WindowGotFocus)
			&& hui::getInputEvent().window == osWnd)
		{
			hui::forceRepaint();
		}

		ctx->dockingState.currentDockNode = rootNode;
		ctx->dockingState.closeWindow = false;
		handleDockNodeResize(rootNode);

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
}

void updateDockingSystem(bool isLastEvent)
{
	if (hui::hasNothingToDo())
	{
		std::this_thread::yield();
		return;
	}

	ctx->mustRedraw = false;
	hui::updateDockingSystemInternal(isLastEvent);

	//auto doLogicAndRender = [](bool isLastEvent)
	//{
	//	ctx->renderer->disableRendering = !isLastEvent && !ctx->skipRenderAndInput;
	//};

	//if (ctx->events.size() == 0)
	//{
	//	doLogicAndRender(true);
	//}

	//// execute logic and render for all events that might be in the queue
	//for (int i = 0; i < ctx->events.size(); i++)
	//{
	//	ctx->event = ctx->events[i];
	//	doLogicAndRender(i == ctx->events.size() - 1);
	//}
	//
}*/

void dockingSystemLoop()
{
	/*
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
	*/
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

void handleDockingMouseDown(const InputEvent& event, DockNode* node)
{
	auto& ds = ctx->dockingState;
	const Point& mousePos = event.mouse.point;

	ds.lastMousePos = mousePos;
	ds.resizingNode = ds.hoveredNode;

	if (ds.resizingNode)
	{
		return;
	}

	ds.dragWindow = nullptr;

	for (auto& wnd : ds.windows)
	{
		// return if the widget is not visible, that is outside current clip rect
		if (wnd.second->tabRect.outside(wnd.second->dockNode->rect))
		{
			continue;
		}

		Rect clippedRect = wnd.second->tabRect.clipInside(wnd.second->dockNode->rect);

		if (clippedRect.contains(mousePos.x, mousePos.y))
		{
			ds.dragWindow = wnd.second;
			break;
		}
	}
}

void handleDockingMouseUp(const InputEvent& event, DockNode* node)
{
	auto& ds = ctx->dockingState;
	bool moved = fabs(ds.lastMousePos.x - event.mouse.point.x) > ctx->settings.dragStartDistance || fabs(ds.lastMousePos.y - event.mouse.point.y) > ctx->settings.dragStartDistance;

	if (ds.dockToNode && moved)
	{
		if (dockWindow(ds.dragWindow, ds.dockToNode, ds.dockType, 0))
		{
			ds.dragWindow = nullptr;
			ds.dockToNode = nullptr;
		}

		hui::forceRepaint();
	}
	// we undock to a new native window
	else if (ds.dragWindow && moved)
	{
		// undock the window if there is more than one in the dock node
		// and if the dock node is not a root node of the window
		if (ds.dragWindow->dockNode->windows.size() > 1
			&& ds.dragWindow->dockNode->parent
			&& ctx->settings.allowUndockingToNewOsWindow)
		{
			auto& rc = ds.dragWindow->dockNode->rect;
			Point pt = {
				event.mouse.point.x + rc.x - rc.width / 2.0f,
				event.mouse.point.y + rc.y
			};

			undockWindow(ds.dragWindow->id.c_str(), pt);
		}

		hui::forceRepaint();
	}

	ds.dockToNode = nullptr;
	ds.dragWindow = nullptr;
	ds.resizingNode = nullptr;
}

void handleDockingMouseMove(const InputEvent& event, DockNode* node)
{
	auto& ds = ctx->dockingState;
	ds.hoveredNode = node->findResizeDockNode(event.mouse.point);

	if (ds.hoveredNode)
	{
		switch (ds.hoveredNode->parent->type)
		{
		case DockNode::Type::Horizontal:
		{
			ctx->mouseCursor = MouseCursorType::SizeWE;
			break;
		}
		case DockNode::Type::Vertical:
		{
			ctx->mouseCursor = MouseCursorType::SizeNS;
			break;
		}
		default:
			break;
		}
	}

	// if we drag a tab or a node splitter
	if (ds.dragWindow
		|| ds.hoveredNode
		|| ds.resizingNode)
	{
		auto& mousePos = event.mouse.point;
		bool moved = fabs(ds.lastMousePos.x - mousePos.x) > ctx->settings.dragStartDistance || abs(ds.lastMousePos.y - mousePos.y) > ctx->settings.dragStartDistance;
		auto mouseDelta = mousePos - ds.lastMousePos;
#ifdef false
		if (ds.dragWindow)
		{
			// draw tab rect
			auto& dockingRectElem = ctx->theme->getElement(WidgetElementId::WindowDockRect);
			auto& dockingDialRectElem = ctx->theme->getElement(WidgetElementId::WindowDockDialRect);
			auto& dockingDialRectVSplitElem = ctx->theme->getElement(WidgetElementId::WindowDockDialVSplitRect);
			auto& dockingDialRectHSplitElem = ctx->theme->getElement(WidgetElementId::WindowDockDialHSplitRect);
			auto& tabGroupElem = ctx->theme->getElement(WidgetElementId::TabGroupBody);
			auto zorder = ctx->renderer->getZOrder();

			if (ctx->mouseMoved || moved)
			{
				ds.dragOntoWindow = nullptr;

				for (auto& wnd : ds.windows)
				{
					if (wnd.second->tabRect.contains(mousePos.x, mousePos.y))
					{
						ds.dragOntoWindow = wnd.second;
						break;
					}
				}

				//TODO: visualize better the tab insertion
				// if we dragged on some other tab
				if (ds.dragOntoWindow != ds.dragWindow
					&& ds.dragWindow
					&& ds.dragOntoWindow)
				{
					// same node drag, switch tab places
					if (ds.dragWindow->dockNode == ds.dragOntoWindow->dockNode)
					{
						size_t index1 = ds.dragWindow->dockNode->getWindowIndex(ds.dragWindow);
						size_t index2 = ds.dragWindow->dockNode->getWindowIndex(ds.dragOntoWindow);

						ds.dragWindow->dockNode->windows[index1] = ds.dragOntoWindow;
						ds.dragWindow->dockNode->windows[index2] = ds.dragWindow;
						ds.dragWindow->dockNode->selectedTabIndex = index2;
					}
				}

				// also find the node we're hovering
				ds.dockToNode = node->findDockNode(mousePos);
			}

			bool isSameNode = ds.dockToNode == ds.dragWindow->dockNode;
			bool isSingleWindow = ds.dragWindow->dockNode->windows.size() == 1;

			if (!ds.dockToNode)
			{
				// we want to undock to new window
				if (ds.dragOntoWindow && !isSameNode)
				{
					ds.draggedRect = ds.dragWindow->tabRect;
					ds.draggedRect.x = mousePos.x - ds.draggedRect.width / 2;
					ds.draggedRect.y = mousePos.y - ds.draggedRect.height / 2;
					ds.draggedRect.height = ds.dragWindow->tabRect.height * 2; // we have two lines of text when undocking "Undock\nTabname"
					ds.dockToNode = ds.dragOntoWindow->dockNode;
				}
				else
				{
					// docking not allowed
					ds.dockToNode = nullptr;
				}
			}

			// draw docking rects
			if (ds.dragWindow != ds.dragOntoWindow)
			{
				ctx->renderer->setOsWindow(node->osWindow);
				ctx->renderer->setWindowSize({ node->rect.width, node->rect.height });
				ctx->renderer->begin();
				ctx->renderer->cmdSetColor(dockingRectElem.normalState().color);

				if (ds.dockToNode)
				{
					// draw the locations where we can dock the pane
					const f32 smallRectSize = 64 * ctx->globalScale; //TODO: make it settings
					const f32 smallRectGap = 1 * ctx->globalScale; //TODO: make it settings
					auto parentRect = ds.dockToNode->rect;

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

					auto& rootRect = node->rect;

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
						ds.dockType = DockType::Left;
						ds.draggedRect = parentRect;
						ds.draggedRect.width /= 2.0f;
					}

					if (isSmallRectRightHovered)
					{
						ds.dockType = DockType::Right;
						ds.draggedRect = parentRect;
						ds.draggedRect.x += ds.draggedRect.width / 2.0f;
						ds.draggedRect.width /= 2.0f;
					}

					if (isSmallRectTopHovered)
					{
						ds.dockType = DockType::Top;
						ds.draggedRect = parentRect;
						ds.draggedRect.height /= 2.0f;
					}

					if (isSmallRectBottomHovered)
					{
						ds.dockType = DockType::Bottom;
						ds.draggedRect = parentRect;
						ds.draggedRect.y += ds.draggedRect.height / 2.0f;
						ds.draggedRect.height /= 2.0f;
					}

					if (isSmallRectMiddleHovered)
					{
						ds.dockType = DockType::AsTab;
						ds.draggedRect = parentRect;
						ds.draggedRect.height = tabGroupElem.normalState().height;
					}

					if (isSmallRectRootLeftHovered)
					{
						ds.dockToNode = node;
						ds.dockType = DockType::Left;
						ds.draggedRect = ds.dockToNode->rect;
						ds.draggedRect.width /= 2.0f;
					}

					if (isSmallRectRootRightHovered)
					{
						ds.dockToNode = node;
						ds.dockType = DockType::Right;
						ds.draggedRect = ds.dockToNode->rect;
						ds.draggedRect.x += ds.draggedRect.width / 2.0f;
						ds.draggedRect.width /= 2.0f;
					}

					if (isSmallRectRootTopHovered)
					{
						ds.dockToNode = node;
						ds.dockType = DockType::Top;
						ds.draggedRect = ds.dockToNode->rect;
						ds.draggedRect.height /= 2.0f;
					}

					if (isSmallRectRootBottomHovered)
					{
						ds.dockToNode = node;
						ds.dockType = DockType::Bottom;
						ds.draggedRect = ds.dockToNode->rect;
						ds.draggedRect.y += ds.draggedRect.height / 2.0f;
						ds.draggedRect.height /= 2.0f;
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

				ctx->renderer->cmdDrawImageBordered(dockingRectElem.normalState().image, dockingRectElem.normalState().border, ds.draggedRect, ctx->globalScale);
				ctx->renderer->cmdSetFont(dockingRectElem.normalState().font);
				ctx->renderer->cmdSetColor(dockingRectElem.normalState().textColor);
				ctx->drawMultilineText(ds.dragWindow->title.c_str(), ds.draggedRect, HAlignType::Center, VAlignType::Center);
				ctx->renderer->end();
			}
		}
#endif
		if (ds.resizingNode)
		{
			
			switch (ds.resizingNode->parent->type)
			{
			case DockNode::Type::Vertical:
			{
				
				break;
			}
			case DockNode::Type::Horizontal:
			{
				f32 pushAmount = 0;
				f32 moveDelta = mousePos.x - ds.lastMousePos.x;
				pushAmount = moveDelta;

				ds.resizingNode->computeMinSize();

				//if (moveDelta != 0.0f)
				//	ds.resizingNode->rect.width = mousePos.x - ds.resizingNode->rect.x;

				//if (ds.resizingNode->rect.width < ds.resizingNode->minSize.x)
				//{
				//	ds.resizingNode->rect.width = ds.resizingNode->minSize.x;
				//	pushAmount = moveDelta;
				//	
				//	if (ds.resizingNode != ds.resizingNode->parent->children[0] && pushAmount < 0)
				//		ds.resizingNode->rect.x += pushAmount;

				//	ds.resizingNode->computeRect();
				//}
				//else
				//{
				//	//if (moveDelta > 0)
				//		pushAmount = moveDelta;
				//}

				//auto iterNxt = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);

				//if (ds.resizingNode->rect.right() >= (*iterNxt)->rect.x)
				//{
				//	ds.resizingNode->rect.width = (*iterNxt)->rect.x - ctx->settings.dockNodeSpacing - ds.resizingNode->rect.x;
				//}
				
				if (pushAmount < 0)
				{
					auto iterPrev = ds.resizingNode->parent->getReverseIteratorOf(ds.resizingNode);
					while (iterPrev != ds.resizingNode->parent->children.rend())
					{
						if (*iterPrev != ds.resizingNode->parent->children[0]
							&& (*iterPrev)->rect.width + pushAmount <= ctx->settings.dockNodeMinSize)
						{
							(*iterPrev)->rect.x += pushAmount;
							(*iterPrev)->rect.width = ctx->settings.dockNodeMinSize;
						}
						else
						{
							(*iterPrev)->rect.width += pushAmount;

							// if first one, stop all from moving if its min size
							if (*iterPrev == ds.resizingNode->parent->children[0]
								&& (*iterPrev)->rect.width <= ctx->settings.dockNodeMinSize)
							{
								auto iter = ds.resizingNode->parent->children.begin();
								auto crtX = (*iter)->rect.x;
								auto iterLast = std::find(ds.resizingNode->parent->children.begin(),
									ds.resizingNode->parent->children.end(), ds.resizingNode);
								++iterLast; // need to advance to next

								do {

									(*iter)->rect.x = crtX;
									(*iter)->rect.width = ctx->settings.dockNodeMinSize;
									(*iter)->computeRect();
									++iter;
									crtX += ctx->settings.dockNodeMinSize + ctx->settings.dockNodeSpacing;
									if (iter == iterLast) break;
								} while (true);
							}
							(*iterPrev)->computeRect();
							break;
						}

						(*iterPrev)->computeRect();

						++iterPrev;
					}

					{
						auto iterNext = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);
						auto oldRight = (*iterNext)->rect.right();

						(*iterNext)->rect.x = ds.resizingNode->rect.right() + ctx->settings.dockNodeSpacing;
						(*iterNext)->rect.width = oldRight - (*iterNext)->rect.x;
						(*iterNext)->computeRect();
					}
				}

			

			/*	*/

				if (pushAmount > 0)
				{
					auto iterPrev = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);
					

					while (iterPrev != ds.resizingNode->parent->children.end())
					{
						if (*iterPrev != ds.resizingNode->parent->children.back()
							&& (*iterPrev)->rect.width - pushAmount < ctx->settings.dockNodeMinSize)
						{
							(*iterPrev)->rect.x += pushAmount;
							(*iterPrev)->rect.width = ctx->settings.dockNodeMinSize;
						}
						else
						{
							(*iterPrev)->rect.x += pushAmount;
							(*iterPrev)->rect.width -= pushAmount;

							// if last one, stop all from moving if min size
							if (*iterPrev == ds.resizingNode->parent->children.back()
								&& (*iterPrev)->rect.width < ctx->settings.dockNodeMinSize)
							{
								(*iterPrev)->rect.width = ctx->settings.dockNodeMinSize;
								(*iterPrev)->rect.x = (*iterPrev)->parent->rect.right() - ctx->settings.dockNodeMinSize;
								
								auto iter = ds.resizingNode->parent->children.rbegin();
								auto crtX = (*iter)->rect.x;
								auto iterLast = std::find(ds.resizingNode->parent->children.rbegin(),
									ds.resizingNode->parent->children.rend(), ds.resizingNode);

								do {

									(*iter)->rect.x = crtX;
									(*iter)->rect.width = ctx->settings.dockNodeMinSize;
									(*iter)->computeRect();
									++iter;
									crtX -= ctx->settings.dockNodeMinSize + ctx->settings.dockNodeSpacing;
									if (iter == iterLast) break;
								} while (true);

								auto iterNext = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);
								
								ds.resizingNode->rect.width = (*iterNext)->rect.x - ds.resizingNode->rect.x - ctx->settings.dockNodeSpacing;
						
							}
							(*iterPrev)->computeRect();
							break;
						}

						(*iterPrev)->computeRect();
						++iterPrev;
					}
				}

				break;
			}
			}
			ds.lastMousePos = mousePos;
			ds.resizingNode->computeRect();
		}
	}
}

void handleDockNodeEvents(DockNode* node)
{
	auto& rect = node->rect;
	auto& event = hui::getInputEvent();

	//TODO: find current window index better
	// find if the current window of the view pane had a layer index > 0
	// if so, then we must be having popups or menus
	if (ctx->maxLayerIndex)
	{
		// node is disabled for input
		return;
	}

	// is the event for this window ?
	if (event.window != node->osWindow)
	{
		return;
	}

	switch (event.type)
	{
	case InputEvent::Type::MouseDown: handleDockingMouseDown(event, node); break;
	case InputEvent::Type::MouseUp: handleDockingMouseUp(event, node); break;
	default:
		break;
	};

	handleDockingMouseMove(event, node);
}

/*
void handleDockNodeResize(DockNode* node)
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
	if (crtEvent.window != node->osWindow)
	{
		return;
	}

	auto& ds = ctx->dockingState;

	if (crtEvent.type == InputEvent::Type::MouseDown)
	{
		const Point& mousePos = crtEvent.mouse.point;
		ds.lastMousePos = mousePos;
		ds.resizingNode = node->findResizeDockNode(mousePos);
		ds.dragWindow = nullptr;
		ds.draggingNodeSource = node;

		for (auto& wnd : ds.windows)
		{
			// return if the widget is not visible, that is outside current clip rect
			if (wnd.second->tabRect.outside(node->rect))
			{
				continue;
			}

			Rect clippedRect = wnd.second->tabRect.clipInside(node->rect);

			if (clippedRect.contains(mousePos.x, mousePos.y))
			{
				ds.dragWindow = wnd.second;
				ds.draggingWindow = true;
				break;
			}
		}

		if (ds.resizingNode)
		{
			ds.draggingDockNodeBorder = true;
		}
	}

	// execute docking or undocking
	if (crtEvent.type == InputEvent::Type::MouseUp)
	{
		const f32 moveTriggerDelta = 3;
		bool moved = fabs(ds.lastMousePos.x - crtEvent.mouse.point.x) > moveTriggerDelta || fabs(ds.lastMousePos.y - crtEvent.mouse.point.y) > moveTriggerDelta;

		// we have a node to dock to
		if (ds.dockToNode && moved)
		{
			dockWindow(ds.dragWindow, ds.dockToNode, ds.dockType, 0);

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

				ds.dockToNode = nullptr;

				hui::forceRepaint();
			//}
		}
		//else if (dragTab && !dragOntoView && moved)
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

		ds.draggingNodeSource = nullptr;
		ds.draggingDockNodeBorder = false;
		ds.draggingWindow = false;
		ds.dragWindow = nullptr;
		ds.resizingNode = nullptr;
	}

	const auto mousePos = ctx->providers->input->getMousePosition();
	auto overNodeToResize = node->findResizeDockNode(mousePos);

	// if we drag a tab or a node splitter
	if (crtEvent.type == InputEvent::Type::MouseDown
		|| ds.dragWindow
		|| overNodeToResize
		|| ds.resizingNode)
	{
		auto nodeToResize = overNodeToResize;
		const int moveOffsetTriggerSize = 3;
		bool moved = fabs(ds.lastMousePos.x - mousePos.x) > moveOffsetTriggerSize || abs(ds.lastMousePos.y - mousePos.y) > moveOffsetTriggerSize;

		if (ds.dragWindow)
		{
			// draw tab rect
			auto& dockingRectElem = ctx->theme->getElement(WidgetElementId::WindowDockRect);
			auto& dockingDialRectElem = ctx->theme->getElement(WidgetElementId::WindowDockDialRect);
			auto& dockingDialRectVSplitElem = ctx->theme->getElement(WidgetElementId::WindowDockDialVSplitRect);
			auto& dockingDialRectHSplitElem = ctx->theme->getElement(WidgetElementId::WindowDockDialHSplitRect);
			auto& tabGroupElem = ctx->theme->getElement(WidgetElementId::TabGroupBody);
			auto zorder = ctx->renderer->getZOrder();

			if (crtEvent.type == InputEvent::Type::MouseDown || ctx->mouseMoved || moved)
			{
				ds.dragOntoWindow = nullptr;

				for (auto& wnd : ds.windows)
				{
					if (wnd.second->tabRect.contains(mousePos.x, mousePos.y))
					{
						ds.dragOntoWindow = wnd.second;
						break;
					}
				}

				//TODO: visualize better the tab insertion
				// if we dragged on some other tab
				if (ds.dragOntoWindow != ds.dragWindow
					&& ds.dragWindow
					&& ds.dragOntoWindow)
				{
					if (ds.dragWindow->dockNode == ds.dragOntoWindow->dockNode)
					{
						// same node
						size_t index1 = ds.dragWindow->dockNode->getWindowIndex(ds.dragWindow);
						size_t index2 = ds.dragWindow->dockNode->getWindowIndex(ds.dragOntoWindow);

						ds.dragWindow->dockNode->windows[index1] = ds.dragOntoWindow;
						ds.dragWindow->dockNode->windows[index2] = ds.dragWindow;
						ds.dragWindow->dockNode->selectedTabIndex = index2;
					}
				}

				// check for docking on windows
				ds.dockToNode = node->findTargetDockNode(mousePos);
			}

			bool isSameNode = ds.dockToNode == ds.dragWindow->dockNode;
			bool isNodeSingleTab = ds.dragWindow->dockNode->windows.size() == 1;

			if (!ds.dockToNode)
			{
				if (ds.dragOntoWindow && !isSameNode)
				{
					ds.draggedRect = ds.dragWindow->clientRect;
					ds.draggedRect.x = mousePos.x - ds.draggedRect.width / 2;
					ds.draggedRect.y = mousePos.y - ds.draggedRect.height / 2;
					ds.draggedRect.height = ds.dragWindow->clientRect.height * 2; // we have two lines of text when undocking "Undock\nTabname"
					ds.dockToNode = ds.dragOntoWindow->dockNode;
				}
				else
				{
					// docking not allowed
					ds.dockToNode = nullptr;
				}
			}
			
			if (ds.dragWindow != ds.dragOntoWindow)
			{
				ctx->renderer->setOsWindow(node->osWindow);
				ctx->renderer->setWindowSize({ rect.width, rect.height });
				ctx->renderer->begin();
				ctx->renderer->cmdSetColor(dockingRectElem.normalState().color);

				if (ds.dockToNode)
				{
					// draw the locations where we can dock the pane
					const f32 smallRectSize = 64 * ctx->globalScale; //TODO: make settings
					const f32 smallRectGap = 1 * ctx->globalScale; //TODO: make settings
					auto parentRect = ds.dockToNode->rect;
		
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

					auto rootRect = node->rect;

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
						ds.dockType = DockType::Left;
						ds.draggedRect = parentRect;
						ds.draggedRect.width /= 2;
					}

					if (isSmallRectRightHovered)
					{
						ds.dockType = DockType::Right;
						ds.draggedRect = parentRect;
						ds.draggedRect.x += ds.draggedRect.width / 2;
						ds.draggedRect.width /= 2;
					}

					if (isSmallRectTopHovered)
					{
						ds.dockType = DockType::Top;
						ds.draggedRect = parentRect;
						ds.draggedRect.height /= 2;
					}

					if (isSmallRectBottomHovered)
					{
						ds.dockType = DockType::Bottom;
						ds.draggedRect = parentRect;
						ds.draggedRect.y += ds.draggedRect.height / 2;
						ds.draggedRect.height /= 2;
					}

					if (isSmallRectMiddleHovered)
					{
						ds.dockType = DockType::AsTab;
						ds.draggedRect = parentRect;
						ds.draggedRect.height = tabGroupElem.normalState().height;
					}

					if (isSmallRectRootLeftHovered)
					{
						ds.dockToNode = node;
						ds.dockType = DockType::Left;
						ds.draggedRect = ds.dockToNode->rect;
						ds.draggedRect.width /= 2.0f;
					}
					
					if (isSmallRectRootRightHovered)
					{
						ds.dockToNode = node;
						ds.dockType = DockType::Right;
						ds.draggedRect = ds.dockToNode->rect;
						ds.draggedRect.x += ds.draggedRect.width / 2.0f;
						ds.draggedRect.width /= 2.0f;
					}
					
					if (isSmallRectRootTopHovered)
					{
						ds.dockToNode = node;
						ds.dockType = DockType::Top;
						ds.draggedRect = ds.dockToNode->rect;
						ds.draggedRect.height /= 2.0f;
					}
					
					if (isSmallRectRootBottomHovered)
					{
						ds.dockToNode = node;
						ds.dockType = DockType::Bottom;
						ds.draggedRect = ds.dockToNode->rect;
						ds.draggedRect.y += ds.draggedRect.height / 2.0f;
						ds.draggedRect.height /= 2.0f;
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
				ctx->drawMultilineText(ds.dragWindow->title.c_str(), rectDragged, HAlignType::Center, VAlignType::Center);
				ctx->renderer->end();
			}
		}

		// are we resizing a node ?
		if ((ds.nodeToResize || ds.resizingNode) && !ds.dragWindow)
		{
			DockNode::Type dockType = DockNode::Type::None;

			if (ds.nodeToResize)
			{
				if (ds.nodeToResize->parent)
				{
					dockType = ds.nodeToResize->parent->type;
				}
			}

			if (ds.resizingNode)
			{
				if (ds.resizingNode->parent)
				{
					dockType = ds.resizingNode->parent->type;
				}

				if (crtEvent.type == InputEvent::Type::MouseDown)
				{
					ds.resizeNodeRect = ds.resizingNode->rect;
					auto iterNextPane = std::find(ds.resizingNode->parent->children.begin(), ds.resizingNode->parent->children.end(), ds.resizingNode);

					iterNextPane++;

					if (iterNextPane != ds.resizingNode->parent->children.end())
					{
						ds.resizeNodeSiblingRect = (*iterNextPane)->rect;
					}
				}
			}

			switch (dockType)
			{
			case DockNode::Type::Horizontal:
			{
				ctx->mouseCursor = MouseCursorType::SizeWE;
				break;
			}
			case DockNode::Type::Vertical:
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

	if (ds.draggingDockNodeBorder
		&& ds.resizingNode
		&& ds.resizingNode->parent)
	{
		switch (ds.resizingNode->parent->type)
		{
		case DockNode::Type::Horizontal:
		{
			f32 normalizedSizeBothSiblings = (ds.resizeNodeRect.width + ds.resizeNodeSiblingRect.width) / ds.resizingNode->parent->rect.width;
			f32 normalizedSize1 = (mousePos.x - ds.resizeNodeRect.x) / ds.resizingNode->parent->rect.width;
			f32 normalizedSize2 = (ds.resizeNodeSiblingRect.right() - mousePos.x) / ds.resizingNode->parent->rect.width;
			auto iterNextPane = std::find(ds.resizingNode->parent->children.begin(), ds.resizingNode->parent->children.end(), ds.resizingNode);

			iterNextPane++;
			//resizingNode->normalizedSize.x = normalizedSize1;

			//if (resizingNode->normalizedSize.x < resizingNode->minNormalizedSize.x)
			//{
			//	resizingNode->normalizedSize.x = resizingNode->minNormalizedSize.y;
			//}

			if (iterNextPane != ds.resizingNode->parent->children.end())
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

		case DockNode::Type::Vertical:
		{
			f32 normalizedSizeBothSiblings = (ds.resizeNodeRect.height + ds.resizeNodeSiblingRect.height) / ds.resizingNode->parent->rect.height;
			f32 normalizedSize1 = (mousePos.y - ds.resizeNodeRect.y) / ds.resizingNode->parent->rect.height;
			f32 normalizedSize2 = (ds.resizeNodeSiblingRect.bottom() - mousePos.y) / ds.resizingNode->parent->rect.height;
			auto iterNextPane = std::find(ds.resizingNode->parent->children.begin(), ds.resizingNode->parent->children.end(), ds.resizingNode);

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

	//ds.dockingNode = ds.draggingWindow;
}
*/
}