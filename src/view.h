#pragma once
#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "theme.h"
#include <vector>

namespace hui
{
struct MemoryStream;

struct View
{
	struct DockNode* dockNode = nullptr;
	std::string title;
	ViewType viewType = 0;
	HImage icon = nullptr;
	u64 userData = 0;
	Rect tabRect;
};

struct DockNode
{
	enum class Type
	{
		None,
		ViewTabs,
		Vertical, // =
		Horizontal, // ||
	};

	DockNode* parent = nullptr;
	std::vector<DockNode*> children;
	std::vector<View*> views;
	HWindow window = 0;
	Type type = Type::None;
	Point minSize = { 32, 32 };
	Rect rect;
	size_t selectedTabIndex = 0;

	void removeFromParent();
	void removeView(View* view);
	void computeRect();
	bool checkRedundancy();
	void gatherViewTabsNodes(std::vector<DockNode*>& outNodes);
	DockNode* findResizeNode(const Point& pt);
	DockNode* findTargetDockNode(const Point& pt);
	void debug(i32 level = 0);
};

}