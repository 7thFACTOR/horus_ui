#pragma once
#include "types.h"

namespace hui
{
struct MemoryStream;

struct DockNode
{
	enum class Type
	{
		None,
		Tabs, //[A][B]
		Vertical, // = vertical splits
		Horizontal, // || horizontal splits
	};

	u64 id = 0;
	bool createdByDockingSystem = false; /// only dock nodes created by the docking system can be deleted automatically, user ones cannot
	DockNode* parent = nullptr;
	std::vector<DockNode*> children;
	std::vector<Window*> windows;
	HNativeWindow nativeWindow = 0;
	Type type = Type::None;
	Point minSize = { 32, 32 };
	Rect rect;
	size_t selectedTabIndex = 0;
	size_t dockingTabSpaceIndex = 0;
	f32 dockingTabSpaceWidth = 0;

	DockNode();
	void copyFrom(DockNode* other);
	void adoptChildren();
	void adoptWindows();
	bool hasSingleWindow() const;
	void removeWindowsAndDeleteChildrenRecursive();
	DockNode* removeFromParent();
	void removeWindow(Window* window);
	void computeRect();
	void computeMinSize();
	bool checkRedundancy();
	void gatherWindowTabsNodes(std::vector<DockNode*>& outNodes);
	DockNode* findResizeDockNode(const Point& pt);
	DockNode* findTargetDockNode(const Point& pt);
	std::vector<DockNode*>::iterator findNextSiblingOf(DockNode* node);
	std::vector<DockNode*>::reverse_iterator findPrevSiblingOf(DockNode* node);
	std::vector<DockNode*>::iterator getIteratorOf(DockNode* node);
	std::vector<DockNode*>::reverse_iterator getReverseIteratorOf(DockNode* node);
	size_t getWindowIndex(Window* window);
	void insertTabSpaceAt(const Point& mousePos, f32 spaceWidth);
	void removeTabSpace();
	void moveWindowTabAt(const Point& mousePos, Window* window);
	void debug(i32 level = 0);
};

HNativeWindow nativeWindowCreate(const std::string& title, NativeWindowFlags flags, NativeWindowState state, const Rect& rect);
void nativeWindowDestroy(HNativeWindow wnd);
DockNode* dockNodeRootCreateInternal(HNativeWindow nativeWindow);
void dockNodeRootDelete(HNativeWindow window);
DockNode* dockNodeRootGet(HNativeWindow window);
Window* windowCreateInternal(const std::string& id, DockNode* targetNode, DockType dockType, const std::string& title, Rect* initialRect, HNativeWindow nativeWnd, HImage img);
void windowDeleteInternal(Window* wnd);
void windowCloseInternal(Window* wnd);
bool windowDockInternal(Window* wnd, DockNode* targetNode, DockType dockType, u32 tabIndex = 0, const Point* undockedWindowPos = nullptr);
void dockNodeTabs(DockNode* node);
void dockingSystemUpdate();
void dockNodeEventsHandle(DockNode* node);
void dockingMouseUpHandle();


}