#include "horus.h"
#include "layout_cell.h"
#include "context.h"
#include "docking_system.h"
#include <string.h>
#include <algorithm>

namespace hui
{
Rect getViewPaneRect(HViewPane viewPane)
{
	ViewPane* viewPaneObj = (ViewPane*)viewPane;

	return viewPaneObj->rect;
}

f32 getRemainingViewPaneHeight(HViewPane viewPane)
{
	ViewPane* viewPaneObj = (ViewPane*)viewPane;

	return round((f32)viewPaneObj->rect.height - (ctx->penPosition.y - viewPaneObj->rect.y));
}

HViewPane createViewPane(HViewContainer viewContainer, DockType dock, f32 size)
{
	ViewContainer* viewContainerObj = (ViewContainer*)viewContainer;
	ViewPane* viewPane = new ViewPane();

	auto dockCell = viewContainerObj->rootCell->dockViewPane(viewPane, dock);

	if (size != 0.0f)
	{
		dockCell->setNewSize(size);
	}

	updateViewContainerLayout(viewContainerObj);

	return viewPane;
}

HViewPane createChildViewPane(HViewPane viewPane, DockType dock, f32 size)
{
	ViewPane* newViewPane = new ViewPane();

	for (auto& viewCtr : ctx->dockingData.viewContainers)
	{
		auto cell = viewCtr->rootCell->findCellOfViewPane((ViewPane*)viewPane);

		if (cell)
		{
			auto dockCell = cell->dockViewPane(newViewPane, dock);

			if (size != 0.0f)
			{
				dockCell->setNewSize(size);
			}

			updateViewContainerLayout(viewCtr);
			break;
		}
	}

	return newViewPane;
}

ViewId viewPaneTabs(HViewPane viewPane)
{
	auto viewPaneObj = (ViewPane*)viewPane;

	if (ctx->layoutStack.back().width <= (viewPaneObj->viewTabs.size() * (ctx->paneGroupState.tabWidth + ctx->paneGroupState.sideSpacing)) * ctx->globalScale)
	{
		ctx->paneGroupState.forceTabWidth = ctx->layoutStack.back().width / (f32)viewPaneObj->viewTabs.size();
		ctx->paneGroupState.forceSqueezeTabs = true;
	}
	else
	{
		ctx->paneGroupState.forceSqueezeTabs = false;
	}

	u32 closeTabIndex = ~0;

	ctx->drawingViewPaneTabs = true;
	beginTabGroup(viewPaneObj->selectedTabIndex);

	for (size_t i = 0; i < viewPaneObj->viewTabs.size(); i++)
	{
		hui::tab(viewPaneObj->viewTabs[i]->title, viewPaneObj->viewTabs[i]->icon);
		viewPaneObj->viewTabs[i]->rect = ctx->widget.rect;

		if (ctx->widget.hovered
			&& ctx->event.type == InputEvent::Type::MouseDown
			&& ctx->event.mouse.button == MouseButton::Middle)
		{
			// close tab
			closeTabIndex = i;
			ctx->event.type = InputEvent::Type::None;
		}
	}

	u32 selectedIndex = hui::endTabGroup();

	ctx->drawingViewPaneTabs = false;

	if (closeTabIndex != ~0)
	{
		viewPaneObj->removeViewTab(viewPaneObj->viewTabs[closeTabIndex]);

		if (viewPaneObj->viewTabs.empty())
		{
			ctx->dockingData.currentViewContainer->rootCell->removeViewPaneCell(viewPaneObj);
			hui::updateViewContainerLayout(ctx->dockingData.currentViewContainer);

			// close window if no tabs
			if (ctx->dockingData.currentViewContainer->rootCell->children.empty())
			{
				//dockingData.closeWindow = true;
			}
		}

		if (selectedIndex >= viewPaneObj->viewTabs.size())
			selectedIndex = viewPaneObj->viewTabs.size() - 1;
	}

	if (!viewPaneObj->viewTabs.empty())
	{
		if (viewPaneObj->selectedTabIndex != selectedIndex)
		{
			viewPaneObj->selectedTabIndex = selectedIndex;
			hui::forceRepaint();
		}

		return viewPaneObj->viewTabs[viewPaneObj->selectedTabIndex]->viewId;
	}

	return ~0;
}

ViewId beginViewPane(HViewPane viewPane)
{
	ViewPane* viewPaneObj = (ViewPane*)viewPane;

	beginContainer(viewPaneObj->rect);

	return viewPaneTabs(viewPane);
}

void endViewPane()
{
	endContainer();
}

void setViewPaneTabUserDataId(HViewPaneTab viewPaneTab, u64 userDataId)
{
	((ViewTab*)viewPaneTab)->userDataId = userDataId;
}

u64 getViewPaneTabUserDataId(HViewPaneTab viewPaneTab)
{
	return ((ViewTab*)viewPaneTab)->userDataId;
}

void setViewPaneTabTitle(HViewPaneTab viewPaneTab, const char* title)
{
	auto viewPaneTabObj = (ViewTab*)viewPaneTab;

	delete[] viewPaneTabObj->title;
	viewPaneTabObj->title = new char[strlen(title) + 1];
	memset(viewPaneTabObj->title, 0, strlen(title) + 1);
	memcpy(viewPaneTabObj->title, title, strlen(title));
}

const char* getViewPaneTabTitle(HViewPaneTab viewPaneTab)
{
	auto viewPaneTabObj = (ViewTab*)viewPaneTab;

	return viewPaneTabObj->title;
}

ViewId getViewPaneTabViewId(HViewPaneTab viewPaneTab)
{
	auto viewPaneTabObj = (ViewTab*)viewPaneTab;

	return viewPaneTabObj->viewId;
}

void setViewIcon(ViewId id, HImage icon)
{
	for (auto& dc : ctx->dockingData.viewContainers)
	{
		std::vector<ViewTab*> tabs;

		dc->rootCell->gatherViewTabs(tabs);

		for (auto& tab : tabs)
		{
			if (tab->viewId == id)
			{
				tab->icon = icon;
			}
		}
	}
}

HViewPaneTab addViewPaneTab(HViewPane viewPane, const char* title, ViewId id, u64 userDataId)
{
	auto viewPaneObj = (ViewPane*)viewPane;
	auto view = new ViewTab();

	view->viewId = id;
	view->parentViewPane = viewPaneObj;
	view->title = new char[strlen(title) + 1];
	view->userDataId = userDataId;
	memset(view->title, 0, strlen(title) + 1);
	memcpy(view->title, title, strlen(title));

	viewPaneObj->viewTabs.push_back(view);

	return view;
}

void removeViewPaneTab(HViewPaneTab viewPaneTab)
{
	auto viewTabObj = (ViewTab*)viewPaneTab;
	auto viewPaneObj = viewTabObj->parentViewPane;

	auto iter = std::find(viewPaneObj->viewTabs.begin(), viewPaneObj->viewTabs.end(), viewTabObj);

	if (iter != viewPaneObj->viewTabs.end())
	{
		viewPaneObj->viewTabs.erase(iter);
	}

	if (viewPaneObj->selectedTabIndex >= viewPaneObj->viewTabs.size())
	{
		viewPaneObj->selectedTabIndex--;
	}

	if (viewPaneObj->selectedTabIndex < 0)
	{
		viewPaneObj->selectedTabIndex = 0;
	}

	if (viewPaneObj->viewTabs.empty())
	{
		viewPaneObj->selectedTabIndex = ~0;
	}
}

void dockViewPane(HViewPane viewPane, HViewContainer viewContainer, DockType dock)
{
	auto viewPaneObj = (ViewPane*)viewPane;
	auto viewContainerObj = (ViewContainer*)viewContainer;

	viewContainerObj->rootCell->dockViewPane(viewPaneObj, dock);
}

void activateViewPane(HViewPane viewPane)
{
	//TODO
}

void closeViewPane(HViewPane viewPane)
{
	//TODO
}

void restoreViewPane(HViewPane viewPane)
{
	//TODO
}

void maximizeViewPane(HViewPane viewPane)
{
	//TODO
}

u32 getViewPaneTabs(HViewPane viewPane, HViewPaneTab* outViewPaneTabs, u32 maxCount)
{
	auto viewPaneObj = (ViewPane*)viewPane;
	auto count = (u32)std::min(maxCount, (u32)viewPaneObj->viewTabs.size());

	for (size_t i = 0; i < count; i++)
	{
		outViewPaneTabs[i] = viewPaneObj->viewTabs[i];
	}

	return count;
}

u32 getViewPaneTabCount(HViewPane viewPane)
{
	auto viewPaneObj = (ViewPane*)viewPane;
	return (u32)viewPaneObj->viewTabs.size();
}

}
