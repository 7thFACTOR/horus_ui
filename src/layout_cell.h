#pragma once
#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "theme.h"
#include <vector>

namespace hui
{
static constexpr f32 percentOfNewPaneSplit = 0.5f;

struct ViewTab
{
	char* title = nullptr;
	struct ViewPane* parentViewPane = nullptr;
	ViewId viewId = 0;
	Rect rect;
	HImage icon = nullptr;
	HWindow nativeWindow = 0;
	u64 userDataId = 0;
};

struct ViewPane
{
	bool serialize(HFile file, struct ViewHandler* viewHandler);
	bool deserialize(HFile file, struct ViewHandler* viewHandler);
	size_t getViewTabIndex(ViewTab* viewTab);
	void removeViewTab(ViewTab* viewTab);
	ViewTab* getSelectedViewTab();
	void destroy();

	Rect rect;
	std::vector<ViewTab*> viewTabs;
	size_t selectedTabIndex = 0;
};

struct LayoutCell
{
	enum class CellSplitMode
	{
		None,
		Vertical,
		Horizontal
	};

	bool serialize(HFile file, struct ViewHandler* viewHandler);
	bool deserialize(HFile, struct ViewHandler* viewHandler);
	void setNewSize(f32 size);
	void computeRect(const Point& startPos);
	void computeSize();
	LayoutCell* findResizeCell(const Point& pt, i32 gripSize);
	LayoutCell* findDockCell(const Point& pt);
	void gatherViewTabs(std::vector<ViewTab*>& tabs);
	void gatherViewPanes(std::vector<ViewPane*>& viewPanes);
	LayoutCell* findWidestChild(LayoutCell* skipCell = nullptr);
	LayoutCell* dockViewPane(ViewPane* viewPaneToDock, DockType dock);
	void fixNormalizedSizes();
	bool removeViewPaneCell(ViewPane* viewPaneToRemove);
	LayoutCell* deleteChildCell(LayoutCell* cell);
	LayoutCell* findCellOfViewPane(ViewPane* viewPaneToFind);
	void debug(i32 level);
	void destroy();

	LayoutCell* parent = nullptr;
	ViewPane* viewPane = nullptr;
	std::vector<LayoutCell*> children;
	CellSplitMode splitMode = CellSplitMode::None;
	Point normalizedSize = { 1, 1 };
	Rect rect;
};

struct ViewContainer
{
	void destroy()
	{
		delete rootCell;
		rootCell = nullptr;
	}

	bool serialize(HFile file, struct ViewHandler* viewHandler);
	bool deserialize(HFile file, struct ViewHandler* viewHandler);

	LayoutCell* rootCell = nullptr;
	HWindow window = 0;

	enum SideSpacing
	{
		SideSpacingTop = 0,
		SideSpacingBottom,
		SideSpacingLeft,
		SideSpacingRight
	};

	f32 sideSpacing[4]; // spacing for all sides of the view container, usually used for toolbars and main menu
};

}