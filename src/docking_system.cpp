#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "theme.h"
#include "context.h"
#include "docking_system.h"
#include "view_pane.h"
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
		auto pane = (ViewPane*)getWindowRootViewPane(hui::getInputEvent().window);
		hui::destroyWindow(hui::getInputEvent().window);
	}

	std::vector<ViewPane*> panes;

	for (size_t i = 0; i < ctx->dockingData.rootViewPanes.size(); i++)
	{
		auto viewPane = ctx->dockingData.rootViewPanes[i];
		auto wnd = viewPane->window;

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
			updateViewPaneLayout(viewPane);
			hui::forceRepaint();
		}

		panes.clear();
		viewPane->gatherViewPanes(panes);
		ctx->dockingData.currentViewPane = viewPane;
		ctx->dockingData.closeWindow = false;

		beginContainer(wndRect);
		f32 oldY = 0;

		// top area
		ctx->currentViewHandler->onTopAreaRender(wnd);
		viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingTop] = ctx->penPosition.y;
		oldY = ctx->penPosition.y;

		// left area
		ctx->penPosition.x = 0;
		beginContainer({ ctx->penPosition.x, ctx->penPosition.y, viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingLeft], wndRect.height - oldY - viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingBottom] });
		ctx->currentViewHandler->onLeftAreaRender(wnd);
		endContainer();

		// right area
		ctx->penPosition.x = wndRect.width - viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingRight];
		ctx->penPosition.y = oldY;
		beginContainer({ ctx->penPosition.x, ctx->penPosition.y, viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingRight], wndRect.height - oldY - viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingBottom] });
		ctx->currentViewHandler->onRightAreaRender(wnd);
		endContainer();

		// bottom area
		ctx->penPosition.x = 0;
		ctx->penPosition.y = wndRect.height - viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingBottom];
		beginContainer({ ctx->penPosition.x, ctx->penPosition.y, wndRect.width, viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingBottom] });
		ctx->currentViewHandler->onBottomAreaRender(wnd);
		endContainer();

		for (auto pane : panes)
		{
			auto tab = pane->getSelectedViewTab();

			if (tab)
			{
				hui::ViewId crtViewId = beginViewPane(pane);
				ctx->currentViewHandler->onViewRender(
					wnd, pane, crtViewId,
					tab->userData);
				endViewPane();
			}
		}

		endContainer();
		endWindow();
		ctx->currentViewHandler->onAfterFrameRender(wnd);
		ctx->dockingData.currentViewPane = nullptr;
		handleViewPaneResize(viewPane);

		if ((isLastEvent
			&& !ctx->skipRenderAndInput
			&& !ctx->renderer->disableRendering
			&& !ctx->renderer->skipRender)
			|| ctx->dockingTabPane)
		{
			hui::presentWindow(wnd);
		}

		if (ctx->dockingData.closeWindow)
		{
			hui::deleteWindowRootViewPane(wnd);
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

void updateViewPaneLayout(ViewPane* viewPane)
{
	auto rect = getWindowRect(viewPane->window);

	viewPane->normalizedSize.x = 1;
	viewPane->normalizedSize.y = 1;
	viewPane->rect.x = viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingLeft];
	viewPane->rect.y = viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingTop];
	viewPane->rect.width = rect.width - (viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingLeft] + viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingRight]);
	viewPane->rect.height = rect.height - (viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingTop] + viewPane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingBottom]);
	viewPane->computeSize();
	viewPane->fixNormalizedSizes();
}

void handleViewPaneResize(ViewPane* viewPane)
{
	auto rect = getWindowRect(viewPane->window);
	auto& crtEvent = hui::getInputEvent();

	//TODO: find current window index better
	// find if the current window of the view pane had a layer index > 0
	// if so, then we must be having popups or menus
	if (ctx->maxLayerIndex)
	{
		return;
	}

	// is the event for this window ?
	if (crtEvent.window != viewPane->window)
	{
		return;
	}

	const i32 gripSize = 8;
	const f32 dockBorderSizePercent = 0.5f;

	//TODO: maybe put all these in the current context
	static bool draggingViewPaneBorder = false;
	static bool draggingViewPaneTab = false;
	static ViewPane* draggingPaneSource = nullptr;
	static ViewPane* resizingPane = nullptr;
	static ViewTab* dragTab = nullptr;
	static ViewTab* dragOntoTab = nullptr;
	static Rect dockRect;
	static DockType dockType = DockType::Left;
	static ViewPane* dockToPane = nullptr;
	static Rect resizePaneRect;
	static Rect resizePaneSiblingRect;
	static Point lastMousePos;
	static Rect rectDragged;

	if (crtEvent.type == InputEvent::Type::MouseDown)
	{
		const Point& mousePos = crtEvent.mouse.point;
		lastMousePos = mousePos;
		resizingPane = viewPane->findResizeViewPane(mousePos, gripSize);
		std::vector<ViewTab*> tabs;

		viewPane->gatherViewTabs(tabs);
		dragTab = nullptr;
		draggingPaneSource = viewPane;

		for (auto tab : tabs)
		{
			Rect paneRect = tab->viewPane->rect;

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

		if (resizingPane)
		{
			draggingViewPaneBorder = true;
		}
	}

	// execute docking or undocking
	if (crtEvent.type == InputEvent::Type::MouseUp)
	{
		const f32 moveTriggerDelta = 3;
		bool moved = fabs(lastMousePos.x - crtEvent.mouse.point.x) > moveTriggerDelta || fabs(lastMousePos.y - crtEvent.mouse.point.y) > moveTriggerDelta;

		// we have a pane to dock to
		if (dockToPane)
		{
			ViewPane* newPane = nullptr;
			auto dockToViewPane = dockToPane;
			bool executeDocking = true;

			// we don't have to create a new view pane if it holds just one view tab
			if (dragTab && dragTab->viewPane->viewTabs.size() == 1)
			{
				newPane = dragTab->viewPane;

				if (newPane != dockToViewPane)
				{
					draggingPaneSource->removeChild(newPane);

					if (draggingPaneSource == viewPane)
					{
						dockToPane = dockToViewPane;
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
						newPane = new ViewPane();
					}
				}

				newPane->viewTabs.push_back(dragTab);
				newPane->selectedTabIndex = 0;
				newPane->rect = dockRect;

				// remove from old pane
				if (dragTab->viewPane)
				{
					dragTab->viewPane->removeViewTab(dragTab);
				}
				// re-parent to new pane
				dragTab->viewPane = newPane;

				if (dockToPane && dockType != DockType::TopAsViewTab)
				{
					// dock the new pane
					//dockToPane->dockViewTab(newPane, dockType);
				}

				if (draggingPaneSource->children.empty()
					&& draggingPaneSource->splitMode == ViewPane::SplitMode::None
					&& !draggingPaneSource)
				{
					if (hui::getMainWindow() != draggingPaneSource->window)
					{
						destroyWindow(draggingPaneSource->window);
						auto iter = std::find(ctx->dockingData.rootViewPanes.begin(), ctx->dockingData.rootViewPanes.end(), draggingPaneSource);

						ctx->dockingData.rootViewPanes.erase(iter);
						//TODO: crashes LLVM on MacOS, check for dangling ptrs
						draggingPaneSource->destroy();
						delete draggingPaneSource;
						draggingPaneSource = nullptr;
					}
				}

				dockToPane = nullptr;

				if (draggingPaneSource)
				{
					hui::updateViewPaneLayout(draggingPaneSource);
				}

				hui::updateViewPaneLayout(viewPane);
				hui::forceRepaint();
			}
		}
		else if (dragTab && !dragOntoTab && moved)
		{
			// do not undock if the source window is the main window and there is just one tab left!
			if (!(dragTab->viewPane->viewTabs.size() == 1
				&& draggingPaneSource == dragTab->viewPane)
				&& ctx->settings.allowUndockingToNewWindow)
			{
				Rect rc = hui::getWindowRect(draggingPaneSource->window);

				auto paneWnd = hui::createWindow(
					"",
					dragTab->viewPane->rect.width,
					dragTab->viewPane->rect.height,
					WindowFlags::Resizable | WindowFlags::NoTaskbarButton | WindowFlags::CustomPosition,
					{
						crtEvent.mouse.point.x + rc.x - dragTab->viewPane->rect.width / 2,
						crtEvent.mouse.point.y + rc.y
					});

				auto newViewPane = createRootViewPane(paneWnd);

				// we don't have to create a new view pane if it holds just one view tab
				if (dragTab->viewPane->viewTabs.size() == 1)
				{
					viewPane->removeChild(dragTab->viewPane);
					//hui::dockViewPane(dragTab->viewPane, newViewPane, DockType::TopAsViewTab);
				}
				else
				{
					// remove from old pane
					dragTab->viewPane->removeViewTab(dragTab);
					auto newPane = (ViewPane*)hui::createViewPane(newViewPane, DockType::TopAsViewTab);
					newPane->viewTabs.push_back(dragTab);
					dragTab->viewPane = newPane;
				}

				updateViewPaneLayout((ViewPane*)newViewPane);
			}

			updateViewPaneLayout(viewPane);
			hui::forceRepaint();
		}

		if (resizingPane)
		{
			resizingPane->parent->fixNormalizedSizes();
		}

		draggingPaneSource = nullptr;
		draggingViewPaneBorder = false;
		draggingViewPaneTab = false;
		dragTab = nullptr;
		resizingPane = nullptr;
	}

	const auto mousePos = ctx->providers->input->getMousePosition();
	auto overPaneToResize = viewPane->findResizeViewPane(mousePos, gripSize);

	// if we drag a tab or a pane splitter
	if (crtEvent.type == InputEvent::Type::MouseDown
		|| dragTab
		|| overPaneToResize
		|| resizingPane)
	{
		auto paneToResize = overPaneToResize;
		const int moveOffsetTriggerSize = 3;
		bool moved = fabs(lastMousePos.x - mousePos.x) > moveOffsetTriggerSize || abs(lastMousePos.y - mousePos.y) > moveOffsetTriggerSize;

		if (dragTab)
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

				// if we dragged on some other tab
				if (dragOntoTab != dragTab
					&& dragTab
					&& dragOntoTab)
				{
					if (dragTab->viewPane == dragOntoTab->viewPane)
					{
						// same pane
						size_t index1 = dragTab->viewPane->getViewTabIndex(dragTab);
						size_t index2 = dragTab->viewPane->getViewTabIndex(dragOntoTab);

						dragTab->viewPane->viewTabs[index1] = dragOntoTab;
						dragTab->viewPane->viewTabs[index2] = dragTab;
						dragTab->viewPane->selectedTabIndex = index2;
					}
				}

				// check for docking on pane views
				dockToPane = viewPane->findDockViewPane(mousePos);
			}

			bool isSamePane = dockToPane == dragTab->viewPane;
			bool isPaneSingleTab = dragTab->viewPane->viewTabs.size() == 1;

			if (!dockToPane)
			{
				if (dragOntoTab && !isSamePane)
				{
					rectDragged = dragTab->rect;
					rectDragged.x = mousePos.x - rectDragged.width / 2;
					rectDragged.y = mousePos.y - rectDragged.height / 2;
					rectDragged.height = dragTab->rect.height * 2; // we have two lines of text when undocking "Undock\nTabname"
					dockToPane = dragOntoTab->viewPane;
				}
				else
				{
					// docking not allowed
					dockToPane = nullptr;
				}
			}
			
			if (dragTab != dragOntoTab)
			{
				ctx->renderer->setWindowSize({ rect.width, rect.height });
				ctx->renderer->beginFrame();
				ctx->renderer->cmdSetColor(dockingRectElem.normalState().color);

				if (dockToPane)
				{
					// draw the 5 locations where we can dock the pane
					const f32 smallRectSize = 64 * ctx->globalScale;
					const f32 smallRectGap = 1 * ctx->globalScale;
					auto parentRect = dockToPane->rect;
		
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
						dockType = DockType::TopAsViewTab;
						rectDragged = parentRect;
						rectDragged.height = tabGroupElem.normalState().height;
					}

					if (isSmallRectRootLeftHovered)
					{
						dockToPane = viewPane;
						dockType = DockType::RootLeft;
						rectDragged = dockToPane->rect;
						rectDragged.width /= 2.0f;
					}
					
					if (isSmallRectRootRightHovered)
					{
						dockToPane = viewPane;
						dockType = DockType::RootRight;
						rectDragged = dockToPane->rect;
						rectDragged.x += rectDragged.width / 2.0f;
						rectDragged.width /= 2.0f;
					}
					
					if (isSmallRectRootTopHovered)
					{
						dockToPane = viewPane;
						dockType = DockType::RootTop;
						rectDragged = dockToPane->rect;
						rectDragged.height /= 2.0f;
					}
					
					if (isSmallRectRootBottomHovered)
					{
						dockToPane = viewPane;
						dockType = DockType::RootBottom;
						rectDragged = dockToPane->rect;
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
				ctx->drawMultilineText(dragTab->title.c_str(), rectDragged, HAlignType::Center, VAlignType::Center);
				ctx->renderer->endFrame();
			}
		}

		// are we resizing a pane ?
		if ((paneToResize || resizingPane) && !dragTab)
		{
			ViewPane::SplitMode splitType = ViewPane::SplitMode::None;

			if (paneToResize)
			{
				if (paneToResize->parent)
				{
					splitType = paneToResize->parent->splitMode;
				}
			}

			if (resizingPane)
			{
				if (resizingPane->parent)
				{
					splitType = resizingPane->parent->splitMode;
				}

				if (crtEvent.type == InputEvent::Type::MouseDown)
				{
					resizePaneRect = resizingPane->rect;
					auto iterNextPane = std::find(resizingPane->parent->children.begin(), resizingPane->parent->children.end(), resizingPane);

					iterNextPane++;

					if (iterNextPane != resizingPane->parent->children.end())
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

	if (draggingViewPaneBorder
		&& resizingPane
		&& resizingPane->parent)
	{
		switch (resizingPane->parent->splitMode)
		{
		case ViewPane::SplitMode::Horizontal:
		{
			f32 normalizedSizeBothSiblings = (resizePaneRect.width + resizePaneSiblingRect.width) / resizingPane->parent->rect.width;
			f32 normalizedSize1 = (mousePos.x - resizePaneRect.x) / resizingPane->parent->rect.width;
			f32 normalizedSize2 = (resizePaneSiblingRect.right() - mousePos.x) / resizingPane->parent->rect.width;
			auto iterNextPane = std::find(resizingPane->parent->children.begin(), resizingPane->parent->children.end(), resizingPane);

			iterNextPane++;
			resizingPane->normalizedSize.x = normalizedSize1;

			if (resizingPane->normalizedSize.x < resizingPane->minNormalizedSize.x)
			{
				resizingPane->normalizedSize.x = resizingPane->minNormalizedSize.y;
			}

			if (iterNextPane != resizingPane->parent->children.end())
			{
				if (resizingPane->normalizedSize.x + normalizedSize2 > normalizedSizeBothSiblings)
				{
					normalizedSize2 = normalizedSizeBothSiblings - resizingPane->normalizedSize.x;
				}

				(*iterNextPane)->normalizedSize.x = normalizedSize2;

				if ((*iterNextPane)->normalizedSize.x < resizingPane->minNormalizedSize.x)
				{
					(*iterNextPane)->normalizedSize.x = resizingPane->minNormalizedSize.x;
					resizingPane->normalizedSize.x = normalizedSizeBothSiblings - (*iterNextPane)->normalizedSize.x;
				}
			}

			break;
		}

		case ViewPane::SplitMode::Vertical:
		{
			f32 normalizedSizeBothSiblings = (resizePaneRect.height + resizePaneSiblingRect.height) / resizingPane->parent->rect.height;
			f32 normalizedSize1 = (mousePos.y - resizePaneRect.y) / resizingPane->parent->rect.height;
			f32 normalizedSize2 = (resizePaneSiblingRect.bottom() - mousePos.y) / resizingPane->parent->rect.height;
			auto iterNextPane = std::find(resizingPane->parent->children.begin(), resizingPane->parent->children.end(), resizingPane);

			iterNextPane++;
			resizingPane->normalizedSize.y = normalizedSize1;

			if (resizingPane->normalizedSize.y < resizingPane->minNormalizedSize.x)
			{
				resizingPane->normalizedSize.y = resizingPane->minNormalizedSize.y;
			}

			if (iterNextPane != resizingPane->parent->children.end())
			{
				if (resizingPane->normalizedSize.y + normalizedSize2 > normalizedSizeBothSiblings)
				{
					normalizedSize2 = normalizedSizeBothSiblings - resizingPane->normalizedSize.y;
				}

				(*iterNextPane)->normalizedSize.y = normalizedSize2;

				if ((*iterNextPane)->normalizedSize.y < resizingPane->minNormalizedSize.y)
				{
					(*iterNextPane)->normalizedSize.y = resizingPane->minNormalizedSize.y;
					resizingPane->normalizedSize.y = normalizedSizeBothSiblings - (*iterNextPane)->normalizedSize.y;
				}
			}

			break;
		}
		}

		resizingPane->parent->fixNormalizedSizes();
		updateViewPaneLayout(viewPane);
		hui::forceRepaint();
	}

	ctx->dockingTabPane = draggingViewPaneTab;
}

}