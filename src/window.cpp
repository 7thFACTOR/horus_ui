#include "context.h"
#include "docking.h"
#include "renderer.h"
#include "theme.h"

namespace hui
{
bool windowBegin(const char* id, const char* title, Rect* initialRect, HImage img)
{
	Window* wnd = nullptr;
	auto iterWnd = ctx->docking.windows.find(id);
	auto iterClosed = ctx->docking.closedWindowsRects.find(id);
	bool wasClosed = iterClosed != ctx->docking.closedWindowsRects.end();
	auto flags = ctx->nextWindowFlags;

	ctx->nextWindowFlags = WindowFlags::None;

	// if there is no window created, create one
	if (iterWnd == ctx->docking.windows.end() && !wasClosed)
	{
		// find docking info if there is anything there yet
		auto iter = ctx->docking.windowsDockNodeAssignments.find(id);
		DockNode* parentNode = nullptr;

		if (iter != ctx->docking.windowsDockNodeAssignments.end())
		{
			parentNode = ctx->docking.dockNodeIdsMap[iter->second];
		}

		wnd = windowCreateInternal(id, parentNode, parentNode ? DockType::AsTab : DockType::None, title, initialRect, 0, img);
		wnd->id = id;
	}
	else
	{
		if (wasClosed)
		{
			return false;
		}

		wnd = ctx->docking.windows[id];
		wnd->image = img;
	}

	if (ctx->event.type == InputEvent::Type::WindowClose)
	{
		// close the OS window if there is just one window inside
		if (ctx->event.window == wnd->dockNode->nativeWindow
			&& !wnd->dockNode->parent
			&& wnd->dockNode->children.empty())
		{
			ctx->docking.nativeWindowsToDelete.insert(wnd->dockNode->nativeWindow);
			ctx->docking.windowsToDelete.insert(wnd);
			ctx->docking.dockNodesToDelete.insert(wnd->dockNode);
			ctx->docking.closedWindowsRects[id] = wnd->dockNode->rect;

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
	nativeWindowCurrentSet(wnd->dockNode->nativeWindow);	
	ctx->renderer.begin();
	auto rc = wnd->clientRect;

	if (!(flags & WindowFlags::Transparent))
	{
		auto& windowElem = ctx->theme->getElement(WidgetElementId::WindowBody).normalState();

		ctx->renderer.cmdSetColor(windowElem.color);
		ctx->renderer.cmdDrawImageBordered(windowElem.image, windowElem.border, rc, ctx->scale);
	}
	
	auto style = ctx->theme->getElement(WidgetElementId::WindowBody).currentStyle;
	
	Point padding = {
		style->getParameter("paddingX", 10),
		style->getParameter("paddingY", 10) };

	pushPadding(PaddingType::Layout, padding);
	layoutBegin(rc);
	pushId((void*)wnd);

	return true;
}

void windowEnd()
{
	popId();
	layoutEnd();
	popPadding(PaddingType::Layout);
	ctx->renderer.end();
}

void windowVisibleSet(const char* windowId, bool visible)
{
	if (!visible)
	{
		auto iter = ctx->docking.windows.find(windowId);

		if (iter == ctx->docking.windows.end())
			return;

		ctx->docking.windowsToDelete.insert(iter->second);
		ctx->docking.closedWindowsRects[windowId] = iter->second->dockNode->rect;
	}
	else
	{
		auto iter = ctx->docking.closedWindowsRects.find(windowId);

		if (iter != ctx->docking.closedWindowsRects.end())
		{
			ctx->docking.closedWindowsRects.erase(iter);
		}
	}
}

void windowNextFlagsSet(WindowFlags flags)
{
	ctx->nextWindowFlags = flags;
}

void windowFocus(const char* windowId)
{
	auto wndIter = ctx->docking.windows.find(windowId);

	if (wndIter == ctx->docking.windows.end())
	{
		return;
	}

	ctx->docking.focusedWindow = wndIter->second;

	// change the title of the native window to the window tab title, but only if the native window was created automatically by the docking system, do not change title of a native window created by the user

	auto iter = ctx->docking.rootNativeWindowDockNodes.find(ctx->docking.focusedWindow->dockNode->nativeWindow);

	if (iter != ctx->docking.rootNativeWindowDockNodes.end())
	{
		if (iter->second->createdByDockingSystem)
		{
			ctx->settings.services.setWindowTitle(ctx->docking.focusedWindow->dockNode->nativeWindow, ctx->docking.focusedWindow->title.c_str());
		}
	}
}

void windowDock(const char* windowId, const char* targetWindowId, DockType dockType, const Point* undockedWindowPos)
{
	Window* wnd1 = nullptr;
	Window* wnd2 = nullptr;

	auto iterWnd = ctx->docking.windows.find(windowId);

	if (iterWnd != ctx->docking.windows.end())
	{
		wnd1 = iterWnd->second;
	}

	if (targetWindowId)
	{
		iterWnd = ctx->docking.windows.find(targetWindowId);
		
		if (iterWnd != ctx->docking.windows.end())
		{
			wnd2 = iterWnd->second;
		}
	}

	HORUS_ASSERT(wnd1);

	if (wnd1)
	{
		windowDockInternal(wnd1, wnd2 ? wnd2->dockNode : nullptr, dockType, 0, undockedWindowPos);
	}
}

void windowDock(const char* windowId, const char* targetWindowId, DockType dockType)
{
	windowDock(windowId, targetWindowId, dockType, nullptr);
}

void windowUndock(const char* windowId, const Point& windowPos)
{
	windowDock(windowId, nullptr, DockType::Floating, &windowPos);
}

bool windowIsMouseOver()
{
	if (ctx->currentWindow)
	{
		return ctx->currentWindow->dockNode->nativeWindow == ctx->lastHoveredNativeWindow;
	}

	return false;
}

void windowCaptureSet()
{
	ctx->settings.services.setCapture(ctx->currentWindow ? ctx->currentWindow->dockNode->nativeWindow : 0);
}

void windowCaptureRelease()
{
	ctx->settings.services.releaseCapture();
}

Rect windowClientRectGet()
{
	return ctx->currentWindow->clientRect;
}	

Rect windowClientRectGetById(const char* windowId)
{
	auto iter = ctx->docking.windows.find(windowId);

	if (iter == ctx->docking.windows.end()) return {};

	return (*iter).second->clientRect;
}

}