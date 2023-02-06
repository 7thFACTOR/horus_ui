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
	struct ViewContainer* viewContainer = nullptr;
	ViewType viewType = 0;
	Rect rect;
	f32 size = 0;
	HImage icon = nullptr;
	u64 userData = 0;
};

struct ViewContainer
{
	enum class SplitMode
	{
		None,
		Vertical, // =
		Horizontal, // ||
		Tabs
	};

	enum class SideSpacing
	{
		SideSpacingTop = 0,
		SideSpacingBottom,
		SideSpacingLeft,
		SideSpacingRight
	};

	bool serialize(MemoryStream& stream, struct ViewHandler* viewHandler);
	bool deserialize(MemoryStream& stream, struct ViewHandler* viewHandler);
	View* findViewToResize(const Point& pt, i32 gripSize);
	View* findDockView(const Point& pt);
	void gatherViews(std::vector<View*>& views);
	View* deleteView(View* view);
	bool removeView(View* view);
	void debug(i32 level);
	void destroy();
	size_t getViewTabIndex(ViewTab* viewTab);
	void removeViewTab(ViewTab* viewTab);
	ViewTab* getSelectedViewTab();
	ViewPane* acquireViewTab(ViewTab* viewTab, DockType dock);
	void reparent(ViewPane* newParent);

	ViewContainer* parent = nullptr;
	HWindow window = 0;
	std::vector<View*> views;
	SplitMode splitMode = SplitMode::None;
	Point minSize = { 32, 32 };
	Rect rect;
	size_t selectedTabIndex = 0;
	f32 sideSpacing[4] = { 0 }; // spacing for all sides of the view pane, usually used for toolbars and main menu
}

}