#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "theme.h"
#include "context.h"
#include "docking_system.h"
#include "layout_cell.h"
#include <vector>
#include <unordered_map>
#include <algorithm>

#ifdef HORUS_TIMING_DEBUG
#include <ctime>
#include <chrono>
#include <ratio>
#endif

#ifdef _WIN32
#include <windows.h>
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
		hui::deleteViewContainerFromWindow(hui::getInputEvent().window);
		hui::destroyWindow(hui::getInputEvent().window);
	}

	for (size_t i = 0; i < dockingData.viewContainers.size(); i++)
	{
		auto viewContainer = dockingData.viewContainers[i];
		auto wnd = viewContainer->window;

		if (!wnd)
		{
			continue;
		}

		auto wndRect = ctx->inputProvider->getWindowRect(wnd);

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
			updateViewContainerLayout(viewContainer);
			hui::forceRepaint();
		}

		std::vector<UiViewPane*> panes;

		viewContainer->rootCell->gatherViewPanes(panes);
		dockingData.currentViewContainer = viewContainer;
		dockingData.closeWindow = false;

		beginContainer(wndRect);
		f32 oldY = 0;

		// top area
		ctx->currentViewHandler->onTopAreaRender(wnd);
		viewContainer->sideSpacing[UiViewContainer::SideSpacingTop] = ctx->penPosition.y;
		oldY = ctx->penPosition.y;

		// left area
		ctx->penPosition.x = 0;
		beginContainer({ ctx->penPosition.x, ctx->penPosition.y, viewContainer->sideSpacing[UiViewContainer::SideSpacingLeft], wndRect.height - oldY - viewContainer->sideSpacing[UiViewContainer::SideSpacingBottom] });
		ctx->currentViewHandler->onLeftAreaRender(wnd);
		endContainer();

		// right area
		ctx->penPosition.x = wndRect.width - viewContainer->sideSpacing[UiViewContainer::SideSpacingRight];
		ctx->penPosition.y = oldY;
		beginContainer({ ctx->penPosition.x, ctx->penPosition.y, viewContainer->sideSpacing[UiViewContainer::SideSpacingRight], wndRect.height - oldY - viewContainer->sideSpacing[UiViewContainer::SideSpacingBottom] });
		ctx->currentViewHandler->onRightAreaRender(wnd);
		endContainer();

		// bottom area
		ctx->penPosition.x = 0;
		ctx->penPosition.y = wndRect.height - viewContainer->sideSpacing[UiViewContainer::SideSpacingBottom];
		beginContainer({ ctx->penPosition.x, ctx->penPosition.y, wndRect.width, viewContainer->sideSpacing[UiViewContainer::SideSpacingBottom] });
		ctx->currentViewHandler->onBottomAreaRender(wnd);
		endContainer();

		for (size_t j = 0; j < panes.size(); j++)
		{
			auto tab = panes[j]->getSelectedViewTab();

			if (tab)
			{
				hui::ViewId crtViewId = beginViewPane(panes[j]);
				ctx->currentViewHandler->onViewRender(
					wnd, panes[j], crtViewId,
					tab->userDataId);
				endViewPane();
			}
		}

		endContainer();
		endWindow();
		ctx->currentViewHandler->onAfterFrameRender(wnd);
		dockingData.currentViewContainer = nullptr;
		handleViewContainerResize(viewContainer);

		if ((isLastEvent
			&& !ctx->skipRenderAndInput
			&& !ctx->renderer->disableRendering
			&& !ctx->renderer->skipRender)
			|| ctx->dockingTabPane)
		{
			hui::presentWindow(wnd);
		}

		if (dockingData.closeWindow)
		{
			hui::deleteViewContainerFromWindow(wnd);
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
#ifdef _WIN32
		Sleep(1);
#endif
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

void updateViewContainerLayout(UiViewContainer* viewContainer)
{
	auto rect = getWindowRect(viewContainer->window);

	viewContainer->rootCell->normalizedSize.x = 1;
	viewContainer->rootCell->normalizedSize.y = 1;
	viewContainer->rootCell->rect.x = viewContainer->sideSpacing[UiViewContainer::SideSpacingLeft];
	viewContainer->rootCell->rect.y = viewContainer->sideSpacing[UiViewContainer::SideSpacingTop];
	viewContainer->rootCell->rect.width = rect.width - (viewContainer->sideSpacing[UiViewContainer::SideSpacingLeft] + viewContainer->sideSpacing[UiViewContainer::SideSpacingRight]);
	viewContainer->rootCell->rect.height = rect.height - (viewContainer->sideSpacing[UiViewContainer::SideSpacingTop] + viewContainer->sideSpacing[UiViewContainer::SideSpacingBottom]);
	viewContainer->rootCell->computeSize();
	viewContainer->rootCell->fixNormalizedSizes();
}

void handleViewContainerResize(UiViewContainer* viewContainer)
{
	auto rect = getWindowRect(viewContainer->window);
	auto& crtEvent = hui::getInputEvent();

	//TODO: find current window index better
	// find if the current window of the view container had a layer index > 0
	// if so, then we must be having popups or menus
	if (ctx->maxLayerIndex)
	{
		return;
	}

	// is the event for this window ?
	if (crtEvent.window != viewContainer->window)
	{
		return;
	}

	const i32 gripSize = 8;
	const f32 dockBorderSizePercent = 0.5f;

	static bool draggingViewPaneBorder = false;
	static bool draggingViewPaneTab = false;
	static UiViewContainer* draggingContainerSource = nullptr;
	static LayoutCell* resizingCell = nullptr;
	static UiViewTab* dragTab = nullptr;
	static UiViewTab* dragOntoTab = nullptr;
	static Rect dockRect;
	static DockType dockType;
	static LayoutCell* dockToCell = nullptr;
	static Rect resizeCellRect;
	static Rect resizeCellSiblingRect;
	static Point lastMousePos;
	static Rect rectDragged;

	if (crtEvent.type == InputEvent::Type::MouseDown)
	{
		const Point& mousePos = crtEvent.mouse.point;
		lastMousePos = mousePos;
		resizingCell = viewContainer->rootCell->findResizeCell(mousePos, gripSize);
		std::vector<UiViewTab*> tabs;

		viewContainer->rootCell->gatherViewTabs(tabs);
		dragTab = nullptr;
		draggingContainerSource = viewContainer;

		for (auto tab : tabs)
		{
			Rect paneRect = tab->parentViewPane->rect;

			// return if the widget is not visible, that is outside current clip rect
			if (tab->rect.outside(paneRect))
			{
				continue;
			}

			Rect clippedRect = tab->rect.clipInside(paneRect);

			if (clippedRect.contains(mousePos.x, mousePos.y))
			{
				dragTab = tab;
				draggingViewPaneTab = true;
				break;
			}
		}

		if (resizingCell)
		{
			draggingViewPaneBorder = true;
		}
	}

	if (crtEvent.type == InputEvent::Type::MouseUp)
	{
		const f32 moveTriggerDelta = 3;
		bool moved = fabs(lastMousePos.x - crtEvent.mouse.point.x) > moveTriggerDelta || fabs(lastMousePos.y - crtEvent.mouse.point.y) > moveTriggerDelta;

		// we have a cell to dock to
		if (dockToCell)
		{
			UiViewPane* newPane = nullptr;
			auto dockToViewPane = dockToCell->viewPane;
			bool executeDocking = true;

			// we don't have to create a new view pane if it holds just one view tab
			if (dragTab && dragTab->parentViewPane->viewTabs.size() == 1)
			{
				newPane = dragTab->parentViewPane;

				if (newPane != dockToViewPane)
				{
					draggingContainerSource->rootCell->removeViewPaneCell(newPane);

					if (draggingContainerSource == viewContainer)
					{
						dockToCell = draggingContainerSource->rootCell->findCellOfViewPane(dockToViewPane);
					}
				}
				else
				{
					executeDocking = false;
				}
			}

			if (!moved)
			{
				executeDocking = false;
			}

			// dock now
			if (dragTab && executeDocking)
			{
				if (dockType == DockType::TopAsViewTab)
				{
					newPane = dockToViewPane;
				}
				else
				{
					if (!newPane)
					{
						newPane = new UiViewPane();
					}
				}

				newPane->viewTabs.push_back(dragTab);
				newPane->selectedTabIndex = 0;
				newPane->rect = dockRect;
				// remove from old pane
				if (dragTab->parentViewPane)
				{
					dragTab->parentViewPane->removeViewTab(dragTab);
				}
				// re-parent to new pane
				dragTab->parentViewPane = newPane;

				if (dockToCell && dockType != DockType::TopAsViewTab)
				{
					// dock the new pane
					dockToCell->dockViewPane(newPane, dockType);
				}

				if (draggingContainerSource->rootCell->children.empty()
					&& draggingContainerSource->rootCell->splitMode == LayoutCell::CellSplitMode::None
					&& !draggingContainerSource->rootCell->viewPane)
				{
					if (hui::getMainWindow() != draggingContainerSource->window)
					{
						destroyWindow(draggingContainerSource->window);
						auto iter = std::find(dockingData.viewContainers.begin(), dockingData.viewContainers.end(), draggingContainerSource);

						dockingData.viewContainers.erase(iter);
						//TODO: crashes LLVM on MacOS, check for dangling ptrs
						draggingContainerSource->destroy();
						delete draggingContainerSource;
						draggingContainerSource = nullptr;
					}
				}

				dockToCell = nullptr;

				if (draggingContainerSource)
				{
					hui::updateViewContainerLayout(draggingContainerSource);
				}

				hui::updateViewContainerLayout(viewContainer);
				hui::forceRepaint();
			}
		}
		else if (dragTab && !dragOntoTab && moved)
		{
			// do not undock if the source window is the main window and there is just one tab left!
			if (!(dragTab->parentViewPane->viewTabs.size() == 1
				&& draggingContainerSource->rootCell->viewPane == dragTab->parentViewPane)
				&& ctx->settings.allowUndockingToNewWindow)
			{
				Rect rc = hui::getWindowRect(draggingContainerSource->window);

				auto paneWnd = hui::createWindow(
					"",
					dragTab->parentViewPane->rect.width,
					dragTab->parentViewPane->rect.height,
					WindowFlags::Resizable | WindowFlags::NoTaskbarButton | WindowFlags::CustomPosition,
					{
						crtEvent.mouse.point.x + rc.x - dragTab->parentViewPane->rect.width / 2,
						crtEvent.mouse.point.y + rc.y
					});

				auto newViewContainer = createViewContainer(paneWnd);

				// we don't have to create a new view pane if it holds just one view tab
				if (dragTab->parentViewPane->viewTabs.size() == 1)
				{
					viewContainer->rootCell->removeViewPaneCell(dragTab->parentViewPane);
					hui::dockViewPane(dragTab->parentViewPane, newViewContainer, DockType::TopAsViewTab);
				}
				else
				{
					// remove from old pane
					dragTab->parentViewPane->removeViewTab(dragTab);
					auto newPane = (UiViewPane*)hui::createViewPane(newViewContainer, DockType::TopAsViewTab);
					newPane->viewTabs.push_back(dragTab);
					dragTab->parentViewPane = newPane;
				}

				updateViewContainerLayout((UiViewContainer*)newViewContainer);
			}

			updateViewContainerLayout(viewContainer);
			hui::forceRepaint();
		}

		if (resizingCell)
		{
			resizingCell->parent->fixNormalizedSizes();
		}

		draggingContainerSource = nullptr;
		draggingViewPaneBorder = false;
		draggingViewPaneTab = false;
		dragTab = nullptr;
		resizingCell = nullptr;
	}

	const auto mousePos = ctx->inputProvider->getMousePosition();
	auto overCellToResize = viewContainer->rootCell->findResizeCell(mousePos, gripSize);

	// if we drag a tab or a pane splitter
	if (crtEvent.type == InputEvent::Type::MouseDown
		|| dragTab
		|| overCellToResize
		|| resizingCell)
	{
		auto cellToResize = overCellToResize;
		const int moveOffsetTriggerSize = 3;
		bool moved = fabs(lastMousePos.x - mousePos.x) > moveOffsetTriggerSize || abs(lastMousePos.y - mousePos.y) > moveOffsetTriggerSize;

		if (dragTab)
		{
			// draw tab rect
			auto dockingRectElem = ctx->theme->getElement(WidgetElementId::ViewPaneDockRect);
			auto dockingDialRectElem = ctx->theme->getElement(WidgetElementId::ViewPaneDockDialRect);
			auto tabGroupElem = ctx->theme->getElement(WidgetElementId::TabGroupBody);
			auto zorder = ctx->renderer->getZOrder();

			if (crtEvent.type == InputEvent::Type::MouseDown || ctx->mouseMoved || moved)
			{
				std::vector<UiViewTab*> tabs;

				viewContainer->rootCell->gatherViewTabs(tabs);
				dragOntoTab = nullptr;

				for (auto tab : tabs)
				{
					if (tab->rect.contains(mousePos.x, mousePos.y))
					{
						dragOntoTab = tab;
						break;
					}
				}

				// if we dragged on some other tab
				if (dragOntoTab != dragTab
					&& dragTab
					&& dragOntoTab)
				{
					if (dragTab->parentViewPane == dragOntoTab->parentViewPane)
					{
						// same pane
						size_t index1 = dragTab->parentViewPane->getViewTabIndex(dragTab);
						size_t index2 = dragTab->parentViewPane->getViewTabIndex(dragOntoTab);

						dragTab->parentViewPane->viewTabs[index1] = dragOntoTab;
						dragTab->parentViewPane->viewTabs[index2] = dragTab;
						dragTab->parentViewPane->selectedTabIndex = index2;
					}
				}

				// check for docking on pane views
				dockToCell = viewContainer->rootCell->findDockCell(mousePos);
			}

			bool isSamePane = dockToCell == viewContainer->rootCell->findCellOfViewPane(dragTab->parentViewPane);
			bool isPaneSingleTab = dragTab->parentViewPane->viewTabs.size() == 1;

			if (!dockToCell)
			{
				if (dragOntoTab && !isSamePane)
				{
					rectDragged = dockRect;
					dockToCell = viewContainer->rootCell->findCellOfViewPane(dragOntoTab->parentViewPane);
				}
				else
				{
					// docking not allowed
					dockToCell = nullptr;
				}
			}

			rectDragged = dragTab->rect;
			rectDragged.x = mousePos.x - rectDragged.width / 2;
			rectDragged.y = mousePos.y - rectDragged.height / 2;
			rectDragged.height = dragTab->rect.height * 2; // we have two lines of text when undocking "Undock\nTabname"
			
			if (dragTab != dragOntoTab)
			{
				ctx->renderer->setWindowSize({ rect.width, rect.height });
				ctx->renderer->beginFrame();
				ctx->renderer->cmdSetColor(dockingRectElem.normalState().color);

				if (dockToCell)
				{
					// draw the 5 locations where we can dock the pane
					const f32 smallRectSize = 64 * ctx->globalScale;
					const f32 smallRectGap = 1 * ctx->globalScale;
					auto parentRect = dockToCell->rect;
		
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

					auto rootRect = viewContainer->rootCell->rect;

					auto smallRectRootLeft = Rect(
						smallRectGap,
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

					if (isSmallRectLeftHovered) dockType = DockType::Left;
					if (isSmallRectRightHovered) dockType = DockType::Right;
					if (isSmallRectTopHovered) dockType = DockType::Top;
					if (isSmallRectBottomHovered) dockType = DockType::Bottom;
					if (isSmallRectMiddleHovered) dockType = DockType::TopAsViewTab;
					
					if (isSmallRectRootLeftHovered)
					{
						dockToCell = viewContainer->rootCell; dockType = DockType::RootLeft;
					}
					
					if (isSmallRectRootRightHovered)
					{
						dockToCell = viewContainer->rootCell; dockType = DockType::RootRight;
					}
					
					if (isSmallRectRootTopHovered)
					{
						dockToCell = viewContainer->rootCell; dockType = DockType::RootTop;
					}
					
					if (isSmallRectRootBottomHovered)
					{
						dockToCell = viewContainer->rootCell; dockType = DockType::RootBottom;
					}

					ctx->renderer->cmdSetColor(isSmallRectLeftHovered ? dockingDialRectElem.hoveredState().color : dockingDialRectElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectElem.normalState().image,
						dockingDialRectElem.normalState().border, smallRectLeft, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectRightHovered ? dockingDialRectElem.hoveredState().color : dockingDialRectElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectElem.normalState().image,
						dockingDialRectElem.normalState().border, smallRectRight, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectTopHovered ? dockingDialRectElem.hoveredState().color : dockingDialRectElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectElem.normalState().image,
						dockingDialRectElem.normalState().border, smallRectTop, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectBottomHovered ? dockingDialRectElem.hoveredState().color : dockingDialRectElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectElem.normalState().image,
						dockingDialRectElem.normalState().border, smallRectBottom, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectMiddleHovered ? dockingDialRectElem.hoveredState().color : dockingDialRectElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectElem.normalState().image,
						dockingDialRectElem.normalState().border, smallRectMiddle, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectRootLeftHovered ? dockingDialRectElem.hoveredState().color : dockingDialRectElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectElem.normalState().image,
						dockingDialRectElem.normalState().border, smallRectRootLeft, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectRootRightHovered ? dockingDialRectElem.hoveredState().color : dockingDialRectElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectElem.normalState().image,
						dockingDialRectElem.normalState().border, smallRectRootRight, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectRootTopHovered ? dockingDialRectElem.hoveredState().color : dockingDialRectElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectElem.normalState().image,
						dockingDialRectElem.normalState().border, smallRectRootTop, ctx->globalScale);

					ctx->renderer->cmdSetColor(isSmallRectRootBottomHovered ? dockingDialRectElem.hoveredState().color : dockingDialRectElem.normalState().color);
					ctx->renderer->cmdDrawImageBordered(
						dockingDialRectElem.normalState().image,
						dockingDialRectElem.normalState().border, smallRectRootBottom, ctx->globalScale);
				}

				ctx->renderer->cmdDrawImageBordered(dockingRectElem.normalState().image, dockingRectElem.normalState().border, rectDragged, ctx->globalScale);
				ctx->renderer->cmdSetFont(dockingRectElem.normalState().font);
				ctx->renderer->cmdSetColor(dockingRectElem.normalState().textColor);
				ctx->drawMultilineText(dragTab->title, rectDragged, HAlignType::Center, VAlignType::Center);
				ctx->renderer->endFrame();
			}
		}

		// do we resize a cell ?
		if ((cellToResize || resizingCell) && !dragTab)
		{
			LayoutCell::CellSplitMode splitType = LayoutCell::CellSplitMode::None;

			if (cellToResize)
			{
				if (cellToResize->parent)
				{
					splitType = cellToResize->parent->splitMode;
				}
			}

			if (resizingCell)
			{
				if (resizingCell->parent)
				{
					splitType = resizingCell->parent->splitMode;
				}

				if (crtEvent.type == InputEvent::Type::MouseDown)
				{
					resizeCellRect = resizingCell->rect;
					auto iterNextCell = std::find(resizingCell->parent->children.begin(), resizingCell->parent->children.end(), resizingCell);

					iterNextCell++;

					if (iterNextCell != resizingCell->parent->children.end())
					{
						resizeCellSiblingRect = (*iterNextCell)->rect;
					}
				}
			}

			switch (splitType)
			{
			case LayoutCell::CellSplitMode::Horizontal:
			{
				ctx->mouseCursor = MouseCursorType::SizeWE;
				break;
			}
			case LayoutCell::CellSplitMode::Vertical:
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

	if (draggingViewPaneBorder
		&& resizingCell
		&& resizingCell->parent)
	{
		switch (resizingCell->parent->splitMode)
		{
		case LayoutCell::CellSplitMode::Horizontal:
		{
			f32 normalizedSizeBothSiblings = (resizeCellRect.width + resizeCellSiblingRect.width) / resizingCell->parent->rect.width;
			f32 normalizedSize1 = (mousePos.x - resizeCellRect.x) / resizingCell->parent->rect.width;
			f32 normalizedSize2 = (resizeCellSiblingRect.right() - mousePos.x) / resizingCell->parent->rect.width;
			auto iterNextCell = std::find(resizingCell->parent->children.begin(), resizingCell->parent->children.end(), resizingCell);

			iterNextCell++;
			resizingCell->normalizedSize.x = normalizedSize1;

			//TODO: externalize to user
			const f32 minPaneNormalizedSize = 0.1f;

			if (resizingCell->normalizedSize.x < minPaneNormalizedSize)
			{
				resizingCell->normalizedSize.x = minPaneNormalizedSize;
			}

			if (iterNextCell != resizingCell->parent->children.end())
			{
				if (resizingCell->normalizedSize.x + normalizedSize2 > normalizedSizeBothSiblings)
				{
					normalizedSize2 = normalizedSizeBothSiblings - resizingCell->normalizedSize.x;
				}

				(*iterNextCell)->normalizedSize.x = normalizedSize2;

				if ((*iterNextCell)->normalizedSize.x < minPaneNormalizedSize)
				{
					(*iterNextCell)->normalizedSize.x = minPaneNormalizedSize;
					resizingCell->normalizedSize.x = normalizedSizeBothSiblings - (*iterNextCell)->normalizedSize.x;
				}
			}

			break;
		}

		case LayoutCell::CellSplitMode::Vertical:
		{
			f32 normalizedSizeBothSiblings = (resizeCellRect.height + resizeCellSiblingRect.height) / resizingCell->parent->rect.height;
			f32 normalizedSize1 = (mousePos.y - resizeCellRect.y) / resizingCell->parent->rect.height;
			f32 normalizedSize2 = (resizeCellSiblingRect.bottom() - mousePos.y) / resizingCell->parent->rect.height;
			auto iterNextCell = std::find(resizingCell->parent->children.begin(), resizingCell->parent->children.end(), resizingCell);

			iterNextCell++;
			resizingCell->normalizedSize.y = normalizedSize1;

			//TODO: externalize to user
			const f32 minPaneNormalizedSize = 0.1f;

			if (resizingCell->normalizedSize.y < minPaneNormalizedSize)
			{
				resizingCell->normalizedSize.y = minPaneNormalizedSize;
			}

			if (iterNextCell != resizingCell->parent->children.end())
			{
				if (resizingCell->normalizedSize.y + normalizedSize2 > normalizedSizeBothSiblings)
				{
					normalizedSize2 = normalizedSizeBothSiblings - resizingCell->normalizedSize.y;
				}

				(*iterNextCell)->normalizedSize.y = normalizedSize2;

				if ((*iterNextCell)->normalizedSize.y < minPaneNormalizedSize)
				{
					(*iterNextCell)->normalizedSize.y = minPaneNormalizedSize;
					resizingCell->normalizedSize.y = normalizedSizeBothSiblings - (*iterNextCell)->normalizedSize.y;
				}
			}

			break;
		}
		default:
			break;
		}

		resizingCell->parent->fixNormalizedSizes();
		updateViewContainerLayout(viewContainer);
		hui::forceRepaint();
	}

	ctx->dockingTabPane = draggingViewPaneTab;
}

}