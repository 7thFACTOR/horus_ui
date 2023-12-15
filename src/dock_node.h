#pragma once
#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "theme.h"
#include <vector>

namespace hui
{
struct MemoryStream;

struct DockNode
{
	enum class Type
	{
		None,
		Tabs, //[A][B]
		Vertical, // =
		Horizontal, // ||
	};

	DockNode* parent = nullptr;
	std::vector<DockNode*> children;
	std::vector<Window*> windows;
	HOsWindow osWindow = 0;
	Type type = Type::None;
	Point minSize = { 32, 32 };
	Rect rect;
	size_t selectedTabIndex = 0;

	void removeFromParent();
	void removeWindow(Window* window);
	void computeRect();
	bool checkRedundancy();
	void gatherWindowTabsNodes(std::vector<DockNode*>& outNodes);
	DockNode* findResizeDockNode(const Point& pt);
	DockNode* findTargetDockNode(const Point& pt);
	DockNode* findDockNode(const Point& pt);
	size_t getWindowIndex(Window* window);
	void debug(i32 level = 0);
};

}