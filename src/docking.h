#pragma once
#include "types.h"
#include "horus.h"

namespace hui
{
struct DragDockNodeInfo
{
	enum class DragSide
	{
		None,
		Right,
		Bottom
	};

	DockNode* node = nullptr;
	DragSide dragSide = DragSide::None;
};

HOsWindow createOsWindow(const std::string& title, OsWindowFlags flags, OsWindowState state, const Rect& rect);
void destroyOsWindow(HOsWindow wnd);
DockNode* createOsWindowRootDockNode(HOsWindow osWindow);
void deleteRootDockNode(HOsWindow window);
DockNode* getRootDockNode(HOsWindow window);
DragDockNodeInfo findDockNodeDragInfoAtMousePos(HOsWindow window, const Point& mousePos);
Window* createWindow(const std::string& id, DockNode* targetNode, DockType dockType, bool justPlaceIntoNode, const std::string& title, Rect* initialRect, HOsWindow osWnd, HImage icon);
void deleteWindow(Window* wnd);
void closeWindow(Window* wnd);
bool dockWindow(Window* wnd, DockNode* targetNode, DockType dockType, u32 tabIndex = 0, const Point* undockedWindowPos = nullptr);
void dockNodeTabs(DockNode* node);


}