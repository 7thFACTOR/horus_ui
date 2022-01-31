#pragma once
#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "theme.h"
#include <vector>

namespace hui
{
struct MemoryStream;
static constexpr f32 percentOfNewPaneSplit = 0.5f;

struct ViewTab
{
	std::string title;
	struct ViewPane* viewPane = nullptr;
	ViewId viewId = 0;
	Rect rect;
	HImage icon = nullptr;
	u64 userData = 0;
};

struct ViewPane
{
	enum class SplitMode
	{
		None,
		Vertical, // =
		Horizontal // ||
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
	ViewPane* findResizeViewPane(const Point& pt, i32 gripSize);
	ViewPane* findDockViewPane(const Point& pt);
	void gatherViewPanes(std::vector<ViewPane*>& tabs);
	void gatherViewTabs(std::vector<ViewTab*>& tabs);
	ViewPane* findWidestChild(ViewPane* skipPane = nullptr);
	ViewPane* deleteChild(ViewPane* child);
	bool removeChild(ViewPane* child);
	void debug(i32 level);
	void destroy();
	size_t getViewTabIndex(ViewTab* viewTab);
	void removeViewTab(ViewTab* viewTab);
	ViewTab* getSelectedViewTab();
	ViewPane* acquireViewTab(ViewTab* viewTab, DockType dock);

	ViewPane* parent = nullptr;
	HWindow window = 0;
	std::vector<ViewPane*> children;
	SplitMode splitMode = SplitMode::None;
	Point minSize = { 32, 32 };
	Rect rect;
	std::vector<ViewTab*> viewTabs;
	size_t selectedTabIndex = 0;
	f32 sideSpacing[4]; // spacing for all sides of the view pane, usually used for toolbars and main menu
};

}