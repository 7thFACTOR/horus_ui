#pragma once
#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "theme.h"
#include <vector>

namespace hui
{
struct MemoryStream;
static constexpr f32 percentOfNewViewSplit = 0.5f;

struct View
{
	std::string title;
	struct DockNode* dockNode = nullptr;
	ViewType viewType = 0;
	HImage icon = nullptr;
	u64 userData = 0;
	f32 size = 0;
};

struct DockNode
{
	enum class Type
	{
		None,
		View,
		Vertical, // -
		Horizontal, // |
		Tabs
	};


	DockNode* parent = nullptr;
	std::vector<DockNode*> children;
	HWindow window = 0;
	Type type = Type::None;
	View* view = nullptr;
	Point minSize = { 32, 32 };
	Rect rect;
	size_t selectedTabIndex = 0;
};

}