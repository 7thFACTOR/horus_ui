#include "types.h"
#include "horus.h"
#include "horus_interfaces.h"
#include "docking.h"
#include "context.h"
#include "renderer.h"
#include "dock_node.h"

namespace hui
{

void createMainWindow(HOsWindow osWnd)
{
	auto title = HORUS_INPUT->getWindowTitle(osWnd);
	auto rc = HORUS_INPUT->getWindowRect(osWnd);
	auto wnd = createWindow(HORUS_MAIN_WINDOW_ID, nullptr, DockType::None, title, &rc, osWnd, 0);
	ctx->osWindows.push_back(osWnd);
	ctx->dockingState.mainWindow = wnd;
	ctx->currentWindow = wnd;
	ctx->hoveringThisWindow = true;
}

void closeMainWindow()
{
	deleteWindow(ctx->dockingState.mainWindow);
	ctx->dockingState.mainWindow = 0;
}

bool beginWindow(const char* id, const char* title, const char* dockTo, DockType dockType, Rect* initialRect, HImage icon)
{
	Window* wnd = nullptr;

	if (ctx->dockingState.windows.find(id) == ctx->dockingState.windows.end())
	{
		auto iter = ctx->dockingState.windows.find(dockTo ? dockTo : "");
		DockNode* dockToNode = nullptr;

		if (dockTo && iter != ctx->dockingState.windows.end())
		{
			dockToNode = ctx->dockingState.windows[dockTo]->dockNode;
		}

		wnd = createWindow(id, dockToNode, dockType, title, initialRect, 0, icon);
		wnd->id = id;
	}
	else
	{
		wnd = ctx->dockingState.windows[id];
		wnd->icon = icon;
	}

	if (wnd->dockNode->type == DockNode::Type::Tabs && wnd->dockNode->getWindowIndex(wnd) != wnd->dockNode->selectedTabIndex)
	{
		return false;
	}
	
	ctx->currentWindow = wnd;
	ctx->hoveringThisWindow = isMouseOverWindow();
	ctx->renderer->setOsWindow(wnd->dockNode->osWindow);
	ctx->renderer->begin();
	auto rc = wnd->clientRect;

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

void dockWindow(const char* windowId, const char* targetWindowId, DockType dockType)
{
	Window *wnd1 = nullptr, *wnd2 = nullptr;
	
	wnd1 = ctx->dockingState.windows[windowId];
	
	if (targetWindowId)
	{
		wnd2 = ctx->dockingState.windows[targetWindowId];
	}

	dockWindow(wnd1, wnd2 ? wnd2->dockNode : nullptr, dockType, 0);
}

void undockWindow(const char* windowId, const Point& windowPos)
{
	dockWindow(windowId, 0, DockType::Floating);
	Window* wnd = ctx->dockingState.windows[windowId];
	HORUS_INPUT->setWindowPosition(wnd->dockNode->osWindow, windowPos);
}

bool isMouseOverWindow()
{
	if (ctx->currentWindow)
	{
		return ctx->currentWindow->dockNode->osWindow == HORUS_INPUT->getHoveredWindow();
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