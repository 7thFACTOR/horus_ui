#include "context.h"
#include "docking.h"
#include "renderer.h"
#include "theme.h"
#include <algorithm>

namespace hui
{
// recreates the dock node a closed window was in, returning the node the window
// should be docked into, nullptr if the slot cannot be restored
static DockNode* windowRestoreDockSlot(const DockingState::HiddenWindowSlot& slot)
{
	auto parentIter = ctx->docking.dockNodeIdsMap.find(slot.parentNodeId);

	if (parentIter == ctx->docking.dockNodeIdsMap.end() || !parentIter->second)
		return nullptr;

	DockNode* parent = parentIter->second;

	// never split a chrome window (menu bar, toolbars), the slot is stale
	if (dockNodeHasChromeWindow(parent))
		return nullptr;

	DockNode* node = new DockNode();
	f32 fraction = std::clamp(slot.fraction, 0.05f, 0.95f);

	node->type = DockNode::Type::Tabs;
	node->parent = parent;
	node->nativeWindow = parent->nativeWindow;
	node->rect = parent->rect;
	ctx->docking.dockNodeIdsMap[node->id] = node;

	// the parent is still a split, put the node back at its index
	if (parent->type == DockNode::Type::Vertical || parent->type == DockNode::Type::Horizontal)
	{
		bool vertical = parent->type == DockNode::Type::Vertical;

		// make room by shrinking the siblings, then insert the node
		for (auto child : parent->children)
		{
			if (vertical)
				child->rect.height *= 1.0f - fraction;
			else
				child->rect.width *= 1.0f - fraction;
		}

		if (vertical)
			node->rect.height = parent->rect.height * fraction;
		else
			node->rect.width = parent->rect.width * fraction;

		u32 index = std::min<u32>(slot.index, (u32)parent->children.size());

		parent->children.insert(parent->children.begin() + index, node);

		return node;
	}

	// the parent turned into a leaf when the node was removed, split it again around
	// its remaining content
	parent->type = slot.verticalParent ? DockNode::Type::Vertical : DockNode::Type::Horizontal;

	if (parent->type == DockNode::Type::Vertical)
		node->rect.height = parent->rect.height * fraction;
	else
		node->rect.width = parent->rect.width * fraction;

	if (!parent->windows.empty())
	{
		DockNode* relocated = new DockNode();

		relocated->type = DockNode::Type::Tabs;
		relocated->parent = parent;
		relocated->nativeWindow = parent->nativeWindow;
		relocated->rect = parent->rect;

		if (parent->type == DockNode::Type::Vertical)
			relocated->rect.height = parent->rect.height - node->rect.height;
		else
			relocated->rect.width = parent->rect.width - node->rect.width;

		relocated->windows = parent->windows;
		ctx->docking.dockNodeIdsMap[relocated->id] = relocated;

		for (auto wnd : relocated->windows)
			wnd->dockNode = relocated;

		parent->windows.clear();
		parent->children.push_back(relocated);
	}

	if (slot.index == 0)
		parent->children.insert(parent->children.begin(), node);
	else
		parent->children.push_back(node);

	return node;
}

bool windowBegin(const char* id, const char* title, Rect* initialRect, HImage img)
{
	Window* wnd = nullptr;
	auto iterWnd = ctx->docking.windows.find(id);
	bool wasClosed = ctx->docking.hiddenWindows.count(id) > 0;
	auto flags = ctx->nextWindowFlags;
	auto fixedSize = ctx->nextWindowSize;

	ctx->nextWindowFlags = WindowFlags::None;
	ctx->nextWindowSize = { 0, 0 };

	// if there is no window created, create one
	if (iterWnd == ctx->docking.windows.end() && !wasClosed)
	{
		// find docking info if there is anything there yet
		auto iter = ctx->docking.windowsDockNodeAssignments.find(id);
		DockNode* parentNode = nullptr;

		if (iter != ctx->docking.windowsDockNodeAssignments.end())
		{
			auto mapIter = ctx->docking.dockNodeIdsMap.find(iter->second);
			if (mapIter != ctx->docking.dockNodeIdsMap.end())
				parentNode = mapIter->second;
		} else {
			parentNode = nullptr;
		}

		// the dock node of the window was removed when the window was closed, recreate
		// it in the remembered slot so the window docks back where it was
		auto slotIter = ctx->docking.hiddenWindowSlots.find(id);

		if (!parentNode && slotIter != ctx->docking.hiddenWindowSlots.end())
			parentNode = windowRestoreDockSlot(slotIter->second);

		ctx->docking.hiddenWindowSlots.erase(id);

		// a window with no remembered place docks into the dock area by default
		if (!parentNode)
			parentNode = dockNodeGetDefaultDockTarget();

		if (parentNode)
			ctx->docking.windowsDockNodeAssignments[id] = parentNode->id;

		// reopen the window where it was when it was closed
		Rect rememberedRect;
		auto iterRect = ctx->docking.closedWindowsRects.find(id);

		if (!initialRect && iterRect != ctx->docking.closedWindowsRects.end())
		{
			rememberedRect = iterRect->second;
			initialRect = &rememberedRect;
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

	auto oldFlags = wnd->flags;
	auto oldFixedSize = wnd->fixedSize;
	wnd->flags = flags;
	wnd->fixedSize = fixedSize;

	// when tab visibility or the fixed size changes, recompute the layout so the client rect and dock node sizes update
	if (!!(flags & WindowFlags::NoTab) != !!(oldFlags & WindowFlags::NoTab)
		|| fixedSize.x != oldFixedSize.x
		|| fixedSize.y != oldFixedSize.y)
	{
		dockNodeLayoutRecalculate();
	}

	if (ctx->event.type == InputEvent::Type::WindowClose)
	{
		// close the OS window if there is just one window inside
		if (ctx->event.window == wnd->dockNode->nativeWindow
			&& !wnd->dockNode->parent
			&& wnd->dockNode->children.empty())
		{
			windowHideInternal(wnd);
			ctx->docking.nativeWindowsToDelete.insert(wnd->dockNode->nativeWindow);
			ctx->docking.windowsToDelete.insert(wnd);
			ctx->docking.dockNodesToDelete.insert(wnd->dockNode);

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
	nativeWindowSetCurrent(wnd->dockNode->nativeWindow);	
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
		style->getParameter("paddingX", 5),
		style->getParameter("paddingY", 5) };

	if (!!(flags & WindowFlags::NoPadding))
	{
		padding = { 0, 0 };
	}

	paddingPush(PaddingType::Layout, padding);
	layoutBegin(rc);
	idPush((void*)wnd);

	return true;
}

void windowEnd()
{
	idPop();
	layoutEnd();
	paddingPop(PaddingType::Layout);
	ctx->renderer.end();
}

void windowSetVisible(const char* windowId, bool visible)
{
	if (!visible)
	{
		auto iter = ctx->docking.windows.find(windowId);

		if (iter == ctx->docking.windows.end())
		{
			ctx->docking.hiddenWindows.insert(windowId);
			return;
		}

		Window* wnd = iter->second;
		DockNode* node = wnd->dockNode;

		windowHideInternal(wnd);

		if (node)
		{
			// detach the window from the docking tree before it is deleted, otherwise
			// dockNodeTabs would later dereference a freed window
			node->removeWindow(wnd);

			if (node->windows.empty() && node->id != ctx->docking.dockAreaNodeId)
			{
				if (!node->parent && node->createdByDockingSystem)
				{
					ctx->docking.dockNodesToDelete.insert(node);
					ctx->docking.nativeWindowsToDelete.insert(node->nativeWindow);
				}
				else
				{
					DockNode* removedNode = node->removeFromParent();
					ctx->docking.dockNodesToDelete.insert(removedNode);
				}
			}

			if (ctx->docking.focusedWindow == wnd)
				ctx->docking.focusedWindow = nullptr;

			dockNodeLayoutRecalculate();
		}

		ctx->docking.windowsToDelete.insert(wnd);
	}
	else
	{
		auto iter = ctx->docking.hiddenWindows.find(windowId);

		if (iter != ctx->docking.hiddenWindows.end())
		{
			ctx->docking.hiddenWindows.erase(iter);
		}
	}
}

bool windowIsVisible(const char* windowId)
{
	if (!windowId)
		return false;

	return ctx->docking.hiddenWindows.count(windowId) == 0;
}

void windowSetNextFlags(WindowFlags flags)
{
	ctx->nextWindowFlags = flags;
}

void windowSetNextSize(f32 width, f32 height)
{
	ctx->nextWindowSize = { width, height };
}

void windowSetFocus(const char* windowId)
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

	HUI_ASSERT(wnd1);

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

void windowSetCapture()
{
	ctx->settings.services.setCapture(ctx->currentWindow ? ctx->currentWindow->dockNode->nativeWindow : 0);
}

void windowReleaseCapture()
{
	ctx->settings.services.releaseCapture();
}

Rect windowGetClientRect()
{
	if (!ctx->currentWindow) return {};
	return ctx->currentWindow->clientRect;
}	

Rect windowGetClientRectByWindowId(const char* windowId)
{
	auto iter = ctx->docking.windows.find(windowId);

	if (iter == ctx->docking.windows.end()) return {};

	return (*iter).second->clientRect;
}

}