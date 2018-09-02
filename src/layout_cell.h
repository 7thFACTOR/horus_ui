#pragma once
#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "ui_theme.h"
#include <vector>

namespace hui
{
static constexpr f32 percentOfNewPaneSplit = 0.2f;
	
struct UiViewTab
{
	char* title = nullptr;
	struct UiViewPane* parentViewPane = nullptr;
	ViewId id = 0;
	Rect rect;
	Image icon = nullptr;
	Window nativeWindow = 0;
	u64 userDataId = 0;
};

struct UiViewPane
{
	bool serialize(FILE* file);
	bool deserialize(FILE* file);
	size_t getViewTabIndex(UiViewTab* viewTab);
	void removeViewTab(UiViewTab* viewTab);
	UiViewTab* getSelectedViewTab();
	void destroy();

	Rect rect;
	std::vector<UiViewTab*> viewTabs;
	size_t selectedTabIndex = 0;
};

struct LayoutCell
{
	enum class CellTileType
	{
		None,
		Vertical,
		Horizontal
	};

	bool serialize(FILE* file);
	bool deserialize(FILE* file);
	void setNewSize(f32 size);
	void computeRect(const Point& startPos);
	void computeSize();
	LayoutCell* findResizeCell(const Point& pt, i32 gripSize);
	LayoutCell* findDockToCell(const Point& pt, DockType& outDockType, Rect& outDockRect, f32 dockBorderSize, f32 tabGroupHeight);
	void fillViewTabs(std::vector<UiViewTab*>& tabs);
	void fillViewPanes(std::vector<UiViewPane*>& viewPanes);
	LayoutCell* findWidestChild(LayoutCell* skipCell = nullptr);
	LayoutCell* dockViewPane(UiViewPane* viewPaneToDock, DockType dock);
	void fixNormalizedSizes();
	bool removeViewPaneCell(UiViewPane* viewPaneToRemove);
	LayoutCell* deleteChildCell(LayoutCell* cell);
	LayoutCell* findCellOfViewPane(UiViewPane* viewPaneToFind);
	void debug(i32 level);
	void destroy();

	LayoutCell* parent = nullptr;
	UiViewPane* viewPane = nullptr;
	std::vector<LayoutCell*> children;
	CellTileType tileType = CellTileType::None;
	Point normalizedSize = { 1, 1 };
	Rect rect;
};

struct UiViewContainer
{
	void destroy()
	{
		delete rootCell;
		rootCell = nullptr;
	}

	bool serialize(FILE* file);
	bool deserialize(FILE* file);

	LayoutCell* rootCell = nullptr;
	Window window = 0;
	f32 mainMenuHeight = 0;
};


}