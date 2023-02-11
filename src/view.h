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
	struct DockNode* parent = nullptr;
	std::string title;
	ViewType viewType = 0;
	HImage icon = nullptr;
	u64 userData = 0;
};

struct DockNode
{
	enum class Type
	{
		None,
		ViewTabs,
		Vertical, // -
		Horizontal, // |
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
};

}