#include "types.h"
#include "horus.h"
#include "horus_interfaces.h"
#include "docking.h"
#include "context.h"
#include "renderer.h"
#include "theme.h"
#include <assert.h>

namespace hui
{
bool beginWindow(const char* id, const char* title, Rect* initialRect, HImage icon)
{
	Window* wnd = nullptr;
	auto iterWnd = ctx->dockingState.windows.find(id);
	auto iterClosed = ctx->dockingState.closedWindowsRects.find(id);
	bool wasClosed = iterClosed != ctx->dockingState.closedWindowsRects.end();
	auto flags = ctx->nextWindowFlags;

	ctx->nextWindowFlags = WindowFlags::None;

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

		wnd = createWindow(id, parentNode, parentNode ? DockType::AsTab : DockType::None, title, initialRect, 0, icon);
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
		if (ctx->event.window == wnd->dockNode->nativeWindow
			&& !wnd->dockNode->parent
			&& wnd->dockNode->children.empty())
		{
			ctx->dockingState.nativeWindowsToDelete.insert(wnd->dockNode->nativeWindow);
			ctx->dockingState.windowsToDelete.insert(wnd);
			ctx->dockingState.dockNodesToDelete.insert(wnd->dockNode);
			ctx->dockingState.closedWindowsRects[id] = wnd->dockNode->rect;

			return false;
		}
	}

	if (wnd->dockNode->type == DockNode::Type::Tabs 
		&& wnd->dockNode->selectedTabIndex != wnd->dockNode->getWindowIndex(wnd)
		&& !wnd->dockingNow)
	{
		return false;
	}
	
	ctx->currentWindow = wnd;
	ctx->hoveringThisWindow = wnd->dockNode->nativeWindow == ctx->lastHoveredNativeWindow;
	ctx->renderer->setCurrentNativeWindow(wnd->dockNode->nativeWindow);
	ctx->renderer->setWindowSize(HORUS_INPUT->getWindowClientSize(wnd->dockNode->nativeWindow));
	ctx->renderer->begin();
	auto rc = wnd->clientRect;

	if (!(flags & WindowFlags::Transparent))
	{
		auto& windowElem = ctx->theme->getElement(WidgetElementId::WindowBody).normalState();

		ctx->renderer->cmdSetColor(windowElem.color);
		ctx->renderer->cmdDrawImageBordered(windowElem.image, windowElem.border, rc, ctx->globalScale);
	}

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

void setWindowVisible(const char* windowId, bool visible)
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

void setNextWindowFlags(WindowFlags flags)
{
	ctx->nextWindowFlags = flags;
}

void dockWindow(const char* windowId, const char* targetWindowId, DockType dockType, const Point* undockedWindowPos)
{
	Window* wnd1 = nullptr, * wnd2 = nullptr;

	auto iterWnd = ctx->dockingState.windows.find(windowId);

	if (iterWnd != ctx->dockingState.windows.end())
	{
		wnd1 = iterWnd->second;
	}

	if (targetWindowId)
	{
		iterWnd = ctx->dockingState.windows.find(targetWindowId);
		
		if (iterWnd != ctx->dockingState.windows.end())
		{
			wnd2 = iterWnd->second;
		}
	}

	assert(wnd1);

	if (wnd1)
	{
		dockWindow(wnd1, wnd2 ? wnd2->dockNode : nullptr, dockType, 0, undockedWindowPos);
	}
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
		return ctx->currentWindow->dockNode->nativeWindow == ctx->lastHoveredNativeWindow;
	}

	return false;
}

void setCapture()
{
	HORUS_INPUT->setCapture(ctx->currentWindow ? ctx->currentWindow->dockNode->nativeWindow : 0);
}

void releaseCapture()
{
	HORUS_INPUT->releaseCapture();
}

Rect getCurrentWindowClientRect()
{
	return ctx->currentWindow->clientRect;
}	

Rect getWindowClientRect(const char* windowId)
{
	auto iter = ctx->dockingState.windows.find(windowId);

	if (iter == ctx->dockingState.windows.end()) return {};

	return (*iter).second->clientRect;
}

}