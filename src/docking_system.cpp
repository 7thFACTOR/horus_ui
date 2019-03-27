#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "ui_font.h"
#include "ui_theme.h"
#include "ui_context.h"
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
void updateDockingSystemInternal(bool isLastEvent, ViewHandler* handler)
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
		handler->onBeforeFrameRender(wnd);
		hui::clearBackground();

		if ((hui::getInputEvent().type == InputEvent::Type::WindowResize
			|| hui::getInputEvent().type == InputEvent::Type::WindowGotFocus)
			&& hui::getInputEvent().window == wnd)
		{
			updateViewContainerLayout(viewContainer);
			hui::forceRepaint();
		}

		std::vector<UiViewPane*> panes;

		viewContainer->rootCell->fillViewPanes(panes);
		dockingData.currentViewContainer = viewContainer;
		dockingData.closeWindow = false;

		beginContainer(wndRect);
		f32 oldY = 0;

		// top area
		handler->onTopAreaRender(wnd);
		viewContainer->sideSpacing[UiViewContainer::SideSpacingTop] = ctx->penPosition.y;
		oldY = ctx->penPosition.y;

		// left area
		ctx->penPosition.x = 0;
		beginContainer({ ctx->penPosition.x, ctx->penPosition.y, viewContainer->sideSpacing[UiViewContainer::SideSpacingLeft], wndRect.height - oldY - viewContainer->sideSpacing[UiViewContainer::SideSpacingBottom] });
		handler->onLeftAreaRender(wnd);
		endContainer();

		// right area
		ctx->penPosition.x = wndRect.width - viewContainer->sideSpacing[UiViewContainer::SideSpacingRight];
		ctx->penPosition.y = oldY;
		beginContainer({ ctx->penPosition.x, ctx->penPosition.y, viewContainer->sideSpacing[UiViewContainer::SideSpacingRight], wndRect.height - oldY - viewContainer->sideSpacing[UiViewContainer::SideSpacingBottom] });
		handler->onRightAreaRender(wnd);
		endContainer();

		// bottom area
		ctx->penPosition.x = 0;
		ctx->penPosition.y = wndRect.height - viewContainer->sideSpacing[UiViewContainer::SideSpacingBottom];
		beginContainer({ ctx->penPosition.x, ctx->penPosition.y, wndRect.width, viewContainer->sideSpacing[UiViewContainer::SideSpacingBottom] });
		handler->onBottomAreaRender(wnd);
		endContainer();

		for (size_t j = 0; j < panes.size(); j++)
		{
			auto tab = panes[j]->getSelectedViewTab();

			if (tab)
			{
				hui::ViewId crtViewId = beginViewPane(panes[j]);
				handler->onViewRender(
					wnd, panes[j], crtViewId,
					tab->userDataId);
				endViewPane();
			}
		}

		endContainer();
		endWindow();
		handler->onAfterFrameRender(wnd);
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

void updateDockingSystem(ViewHandler* handler)
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

	auto doLogicAndRender = [handler](bool isLastEvent)
	{
		ctx->renderer->disableRendering = !isLastEvent && !ctx->skipRenderAndInput;
		hui::updateDockingSystemInternal(isLastEvent, handler);
	};

	if (ctx->events.size() == 0)
	{
		doLogicAndRender(true);
	}

	for (int i = 0; i < ctx->events.size(); i++)
	{
		ctx->event = ctx->events[i];
		doLogicAndRender(i == ctx->events.size() - 1);
	}
}

void dockingSystemLoop(ViewHandler* handler)
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

		updateDockingSystem(handler);

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
	const f32 dockBorderSizePercent = 0.2f;

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

		viewContainer->rootCell->fillViewTabs(tabs);
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
				dragTab->parentViewPane->removeViewTab(dragTab);
				// re-parent to new pane
				dragTab->parentViewPane = newPane;

				if (dockToCell && dockType != DockType::TopAsViewTab)
				{
					// dock the new pane
					dockToCell->dockViewPane(newPane, dockType);
				}

				if (draggingContainerSource->rootCell->children.empty()
					&& draggingContainerSource->rootCell->tileType == LayoutCell::CellTileType::None
					&& !draggingContainerSource->rootCell->viewPane)
				{
					if (hui::getMainWindow() != draggingContainerSource->window)
					{
						destroyWindow(draggingContainerSource->window);
						auto iter = std::find(dockingData.viewContainers.begin(), dockingData.viewContainers.end(), draggingContainerSource);

						dockingData.viewContainers.erase(iter);
						//crashes LLVM on MacOS	
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
					WindowFlags::Resizable | WindowFlags::NoTaskbarButton,
					WindowPositionType::Custom,
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

	// if we draw a tab or a pane splitter
	if (crtEvent.type == InputEvent::Type::MouseDown
		|| dragTab
		|| overCellToResize
		|| resizingCell)
	{
		auto cellToResize = overCellToResize;
		bool moved = fabs(lastMousePos.x - mousePos.x) > 3 || abs(lastMousePos.y - mousePos.y) > 3;

		if (dragTab)
		{
			// draw tab rect
			auto dockingRectElem = ctx->theme->getElement(WidgetElementId::ViewPaneDockRect);
			auto tabGroupElem = ctx->theme->getElement(WidgetElementId::TabGroupBody);
			auto zorder = ctx->renderer->getZOrder();

			if (ctx->mouseMoved
				|| crtEvent.type == InputEvent::Type::MouseDown
				|| moved)
			{
				std::vector<UiViewTab*> tabs;

				viewContainer->rootCell->fillViewTabs(tabs);
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
					else
					{
					}
				}

				rectDragged = dragTab->rect;

				// probably user wants to undock it
				if (!dragOntoTab)
				{
					rectDragged.x = mousePos.x - rectDragged.width / 2;
					rectDragged.y = mousePos.y - rectDragged.height / 2;
					rectDragged.height = dragTab->rect.height * 2; // we have two lines of text when undocking "Undock\nTabname"
				}

				// check for docking on pane views
				dockToCell = viewContainer->rootCell->findDockToCell(
					mousePos,
					dockType,
					dockRect,
					dockBorderSizePercent,
					tabGroupElem.normalState().height * ctx->globalScale);
				bool isSamePane = dockToCell == viewContainer->rootCell->findCellOfViewPane(dragTab->parentViewPane);
				bool isPaneSingleTab = dragTab->parentViewPane->viewTabs.size() == 1;

				// drag onto other view panes, aside itself
				if (!dragOntoTab && dockToCell)
				{
					rectDragged = { (f32)dockRect.x, (f32)dockRect.y, (f32)dockRect.width, (f32)dockRect.height };
				}
				else
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
			}

			if (dragTab != dragOntoTab)
			{
				ctx->renderer->setWindowSize({ rect.width, rect.height });
				ctx->renderer->beginFrame();
				ctx->renderer->cmdSetColor(dockingRectElem.normalState().color);
				ctx->renderer->cmdDrawImageBordered(dockingRectElem.normalState().image, dockingRectElem.normalState().border, rectDragged, ctx->globalScale);
				ctx->renderer->cmdSetFont(dockingRectElem.normalState().font);
				ctx->renderer->cmdSetColor(dockingRectElem.normalState().textColor);

				const char* str = nullptr;
				std::string title = "";

				if (dragTab)
					str = dragTab->title;

				if (!dragOntoTab && !dockToCell && ctx->settings.allowUndockingToNewWindow)
				{
					title = std::string("Undock\n" + std::string(dragTab->title));
					str = title.c_str();
				}
				else if (!ctx->settings.allowUndockingToNewWindow)
				{
					str = dragTab->title;
				}

				if (str)
				{
					ctx->drawMultilineText(str, rectDragged, HAlignType::Center, VAlignType::Center);
				}

				ctx->renderer->endFrame();
			}
		}

		// do we resize a cell ?
		if ((cellToResize || resizingCell) && !dragTab)
		{
			LayoutCell::CellTileType splitType = LayoutCell::CellTileType::None;

			if (cellToResize)
			{
				if (cellToResize->parent)
				{
					splitType = cellToResize->parent->tileType;
				}
			}

			if (resizingCell)
			{
				if (resizingCell->parent)
				{
					splitType = resizingCell->parent->tileType;
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
			case LayoutCell::CellTileType::Horizontal:
			{
				ctx->mouseCursor = MouseCursorType::SizeWE;
				break;
			}
			case LayoutCell::CellTileType::Vertical:
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
		switch (resizingCell->parent->tileType)
		{
		case LayoutCell::CellTileType::Horizontal:
		{
			f32 normalizedSizeBothSiblings = (resizeCellRect.width + resizeCellSiblingRect.width) / resizingCell->parent->rect.width;
			f32 normalizedSize1 = (mousePos.x - resizeCellRect.x) / resizingCell->parent->rect.width;
			f32 normalizedSize2 = (resizeCellSiblingRect.right() - mousePos.x) / resizingCell->parent->rect.width;
			auto iterNextCell = std::find(resizingCell->parent->children.begin(), resizingCell->parent->children.end(), resizingCell);

			iterNextCell++;
			resizingCell->normalizedSize.x = normalizedSize1;

			if (resizingCell->normalizedSize.x < 0.1f)
			{
				//TODO: externalize to user
				resizingCell->normalizedSize.x = 0.1f;
			}

			if (iterNextCell != resizingCell->parent->children.end())
			{
				if (resizingCell->normalizedSize.x + normalizedSize2 > normalizedSizeBothSiblings)
				{
					normalizedSize2 = normalizedSizeBothSiblings - resizingCell->normalizedSize.x;
				}

				(*iterNextCell)->normalizedSize.x = normalizedSize2;

				if ((*iterNextCell)->normalizedSize.x < 0.1f)
				{
					(*iterNextCell)->normalizedSize.x = 0.1f;
					resizingCell->normalizedSize.x = normalizedSizeBothSiblings - (*iterNextCell)->normalizedSize.x;
				}
			}

			break;
		}

		case LayoutCell::CellTileType::Vertical:
		{
			f32 normalizedSizeBothSiblings = (resizeCellRect.height + resizeCellSiblingRect.height) / resizingCell->parent->rect.height;
			f32 normalizedSize1 = (mousePos.y - resizeCellRect.y) / resizingCell->parent->rect.height;
			f32 normalizedSize2 = (resizeCellSiblingRect.bottom() - mousePos.y) / resizingCell->parent->rect.height;
			auto iterNextCell = std::find(resizingCell->parent->children.begin(), resizingCell->parent->children.end(), resizingCell);

			iterNextCell++;
			resizingCell->normalizedSize.y = normalizedSize1;

			if (resizingCell->normalizedSize.y < 0.1f)
			{
				resizingCell->normalizedSize.y = 0.1f;
			}

			if (iterNextCell != resizingCell->parent->children.end())
			{
				if (resizingCell->normalizedSize.y + normalizedSize2 > normalizedSizeBothSiblings)
				{
					normalizedSize2 = normalizedSizeBothSiblings - resizingCell->normalizedSize.y;
				}

				(*iterNextCell)->normalizedSize.y = normalizedSize2;

				if ((*iterNextCell)->normalizedSize.y < 0.1f)
				{
					(*iterNextCell)->normalizedSize.y = 0.1f;
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