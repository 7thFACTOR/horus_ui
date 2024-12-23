#include "types.h"
#include "horus.h"
#include "horus_interfaces.h"
#include "docking.h"
#include "context.h"
#include "renderer.h"
#include "dock_node.h"

namespace hui
{

void dockWindow_DEPRECATED(const char* id, const char* dockTo, DockType dockType)
{
	DockNode* dockToNode = nullptr;
	auto iter = ctx->dockingState.windows.find(dockTo ? dockTo : "");
	
	if (dockTo && iter != ctx->dockingState.windows.end())
	{
		dockToNode = ctx->dockingState.windows[dockTo]->dockNode;
	}

	auto wnd = createWindow(id, dockToNode, dockType, false, "", nullptr, 0, 0);
	wnd->id = id;
}

bool beginWindow(const char* id, const char* title, Rect* initialRect, HImage icon)
{
	Window* wnd = nullptr;
	auto iterWnd = ctx->dockingState.windows.find(id);
	auto iterClosed = ctx->dockingState.closedWindowsRects.find(id);
	bool wasClosed = iterClosed != ctx->dockingState.closedWindowsRects.end();

	// if there is no window created, create one
	if (iterWnd == ctx->dockingState.windows.end() && !wasClosed)
	{
		// find docking info if there is anything there yet
		auto iter = ctx->dockingState.windowsDockNodeAssignments.find(id);
		DockNode* parentNode = nullptr;

		if (iter != ctx->dockingState.windowsDockNodeAssignments.end())
		{
			parentNode = ctx->dockingState.dockNodeIdsMap[iter->second];
		}

		wnd = createWindow(id, parentNode, parentNode ? DockType::AsTab : DockType::None, parentNode != nullptr, title, initialRect, 0, icon);
		wnd->id = id;
	}
	else
	{
		if (wasClosed)
		{
			return false;
		}

		wnd = ctx->dockingState.windows[id];
		wnd->icon = icon;
	}

	if (ctx->event.type == InputEvent::Type::WindowClose)
	{
		// close the OS window if there is just one window inside
		if (ctx->event.window == wnd->dockNode->osWindow
			&& !wnd->dockNode->parent
			&& wnd->dockNode->children.empty())
		{
			ctx->dockingState.osWindowsToDelete.insert(wnd->dockNode->osWindow);
			ctx->dockingState.windowsToDelete.insert(wnd);
			ctx->dockingState.dockNodesToDelete.insert(wnd->dockNode);
			ctx->dockingState.closedWindowsRects[id] = wnd->dockNode->rect;

			return false;
		}
	}

	if (wnd->dockNode->type == DockNode::Type::Tabs 
		&& wnd->dockNode->getWindowIndex(wnd) != wnd->dockNode->selectedTabIndex)
	{
		return false;
	}
	
	ctx->currentWindow = wnd;
	ctx->hoveringThisWindow = wnd->dockNode->osWindow == ctx->lastHoveredOsWindow;
	ctx->renderer->setOsWindow(wnd->dockNode->osWindow);
	ctx->renderer->begin();
	auto rc = wnd->clientRect;

	ctx->renderer->cmdSetColor(ctx->theme->getElement(WidgetElementId::WindowBody).normalState().color);
	ctx->renderer->cmdDrawSolidRectangle(rc);

	rc.y += ctx->theme->getElement(WidgetElementId::TabGroupBody).normalState().height;

	beginContainer(rc);

	return true;
}

void endWindow()
{
	endContainer();
	ctx->renderer->end();
	ctx->currentWindowIndex++;
	//TODO: make scroll struct stack
}

void setWindowVisibility(const char* windowId, bool visible)
{
	if (!visible)
	{
		auto iter = ctx->dockingState.windows.find(windowId);

		if (iter == ctx->dockingState.windows.end())
			return;

		ctx->dockingState.windowsToDelete.insert(iter->second);
		ctx->dockingState.closedWindowsRects[windowId] = iter->second->dockNode->rect;
	}
	else
	{
		auto iter = ctx->dockingState.closedWindowsRects.find(windowId);

		if (iter != ctx->dockingState.closedWindowsRects.end())
		{
			ctx->dockingState.closedWindowsRects.erase(iter);
		}
	}
}

void dockWindow(const char* windowId, const char* targetWindowId, DockType dockType, const Point* undockedWindowPos)
{
	Window* wnd1 = nullptr, * wnd2 = nullptr;

	wnd1 = ctx->dockingState.windows[windowId];

	if (targetWindowId)
	{
		wnd2 = ctx->dockingState.windows[targetWindowId];
	}

	dockWindow(wnd1, wnd2 ? wnd2->dockNode : nullptr, dockType, 0, undockedWindowPos);
}

void dockWindow(const char* windowId, const char* targetWindowId, DockType dockType)
{
	dockWindow(windowId, targetWindowId, dockType, nullptr);
}

void undockWindow(const char* windowId, const Point& windowPos)
{
	dockWindow(windowId, nullptr, DockType::Floating, &windowPos);
}

bool isMouseOverWindow()
{
	if (ctx->currentWindow)
	{
		return ctx->currentWindow->dockNode->osWindow == ctx->lastHoveredOsWindow;
	}

	return false;
}

void setCapture()
{
	HORUS_INPUT->setCapture(ctx->currentWindow);
}

void releaseCapture()
{
	HORUS_INPUT->releaseCapture();
}

Rect getWindowClientRect()
{
	return ctx->currentWindow->clientRect;
}	
	
}