#include "horus.h"
#include "view_pane.h"
#include "docking_system.h"
#include <cstring>
#include <algorithm>
#include <math.h>
#include "context.h"

namespace hui
{
HViewPane createRootViewPane(HWindow window)
{
	auto rect = getWindowRect(window);
	auto viewPane = new ViewPane();

	viewPane->rect.set(0, 0, rect.width, rect.height);
	viewPane->window = window;
	ctx->dockingData.rootViewPanes.push_back(viewPane);

	return viewPane;
}

HViewPane createViewPane(HViewPane parentViewPane, f32 size, DockType dock)
{
	ViewPane* parentViewPanePtr = (ViewPane*)parentViewPane;
	auto newViewPane = new ViewPane();

	newViewPane->window = parentViewPanePtr->window;
	newViewPane->parent = parentViewPanePtr;
	newViewPane->rect.width = newViewPane->rect.height = size;
	dockViewPane(newViewPane, parentViewPane, dock);

	return newViewPane;
}

void resizePanesAfterDocking(DockType dockType, ViewPane* viewPaneToDock, ViewPane* siblingPane, f32 splitFactor)
{
	switch (dockType)
	{
		case hui::DockType::Left:
		{
			viewPaneToDock->rect.x = siblingPane->rect.x;
			viewPaneToDock->rect.y = siblingPane->rect.y;
			viewPaneToDock->rect.width = siblingPane->rect.width * splitFactor;
			viewPaneToDock->rect.height = siblingPane->rect.height;
			siblingPane->rect.width -= viewPaneToDock->rect.width;
			siblingPane->rect.x = viewPaneToDock->rect.right();
			break;
		}
		case hui::DockType::Top:
		{
			viewPaneToDock->rect.x = siblingPane->rect.x;
			viewPaneToDock->rect.y = siblingPane->rect.y;
			viewPaneToDock->rect.height = siblingPane->rect.height * splitFactor;
			viewPaneToDock->rect.width = siblingPane->rect.width;
			siblingPane->rect.height -= viewPaneToDock->rect.height;
			siblingPane->rect.y = viewPaneToDock->rect.bottom();
			break;
		}
		case hui::DockType::Right:
		{
			viewPaneToDock->rect.y = siblingPane->rect.y;
			viewPaneToDock->rect.width = siblingPane->rect.width * splitFactor;
			viewPaneToDock->rect.height = siblingPane->rect.height;
			siblingPane->rect.width -= viewPaneToDock->rect.width;
			viewPaneToDock->rect.x = siblingPane->rect.right();
			break;
		}
		case hui::DockType::Bottom:
		{
			viewPaneToDock->rect.x = siblingPane->rect.x;
			viewPaneToDock->rect.height = siblingPane->rect.height * splitFactor;
			viewPaneToDock->rect.width = siblingPane->rect.width;
			siblingPane->rect.height -= viewPaneToDock->rect.height;
			viewPaneToDock->rect.y = siblingPane->rect.y;
			break;
		}
		default:
		break;
	}
}

void insertViewPaneOnDocking(ViewPane* pane, ViewPane* sibling, ViewPane* parent, DockType dockType)
{
	switch (dockType)
	{
	case hui::DockType::Left:
	case hui::DockType::Top:
	{
		if (sibling)
		{
			auto iter = std::find(parent->children.begin(), parent->children.end(), sibling);
			
			if (iter != parent->children.end())
			{
				parent->children.insert(iter, pane);
			}
		}
		else
		{
			parent->children.insert(parent->children.begin(), pane);
		}
		break;
	}
	case hui::DockType::Right:
	case hui::DockType::Bottom:
	{
		if (sibling)
		{
			auto iter = std::find(parent->children.begin(), parent->children.end(), sibling);

			if (iter != parent->children.end())
			{
				parent->children.insert(++iter, pane);
			}
		}
		else
		{
			parent->children.push_back(pane);
		}
		break;
	}
	}
}

bool dockViewPane(HViewPane viewPane, HViewPane toViewPane, DockType dockType)
{
	auto viewPanePtr = (ViewPane*)viewPane;
	auto toViewPanePtr = (ViewPane*)toViewPane;
	auto toParentPane = toViewPanePtr->parent;
	auto isRootPane = !toParentPane;
	auto dockInside = isRootPane ? toViewPanePtr : toParentPane;

	if (!dockInside)
		return false;

	ViewPane::SplitMode neededSplitMode = ViewPane::SplitMode::None;

	switch (dockType)
	{
	case hui::DockType::Left:
	case hui::DockType::Right:
		neededSplitMode = ViewPane::SplitMode::Horizontal;
		break;
	case hui::DockType::Top:
	case hui::DockType::Bottom:
		neededSplitMode = ViewPane::SplitMode::Vertical;
		break;
	case hui::DockType::TopAsViewTab:
		neededSplitMode = ViewPane::SplitMode::None;
		break;
	}

	// if this is the first view pane to be created and docked, set the split mode
	if (dockInside->children.empty())
	{
		dockInside->splitMode = neededSplitMode;
		dockInside->children.push_back(viewPanePtr);
		viewPanePtr->parent = dockInside;
		viewPanePtr->rect = dockInside->rect;
		return true;
	}

	// dock inside the parent view pane, next to a sibling view pane
	// search the sibling where we want to dock next to
	std::vector<ViewPane*>::iterator iter = dockInside->children.end();

	if (!isRootPane)
	{
		iter = std::find(dockInside->children.begin(), dockInside->children.end(), toViewPanePtr);
	}

	const f32 splitFactor = 0.333333f;
	// is same split mode if its the same with requested split mode of the docking type, or if the host has only 1 child, in which case split mode doesnt matter what it is, it will be set
	bool isSameSplit = dockInside->splitMode == neededSplitMode || (dockInside->children.size() == 1);
	bool foundSibling = iter != dockInside->children.end();

	// if we dock to root pane
	if (isRootPane)
	{
		// if same split then just insert before or after sibling (if any)
		if (isSameSplit)
		{
			insertViewPaneOnDocking(viewPanePtr, foundSibling ? *iter : nullptr, dockInside, dockType);
			resizePanesAfterDocking(dockType, viewPanePtr, foundSibling ? *iter : nullptr, splitFactor);
		}
		else // create a new pane to contain the current panes
		{
			// new container pane
			auto newPane = new ViewPane();
			newPane->parent = dockInside;
			newPane->rect = dockInside->rect;
			newPane->sideSpacing = dockInside->sideSpacing;
			newPane->splitMode = dockInside->splitMode;
			newPane->window = dockInside->window;
			
			for (auto& child : dockInside->children)
			{
				child->reparent(newPane);
			}

			insertViewPaneOnDocking(viewPanePtr, newPane, dockInside, dockType);
			resizePanesAfterDocking(dockType, viewPanePtr, newPane, splitFactor);
		}

		dockInside->splitMode = neededSplitMode;
	}
/*
	if (isSameSplit && !isRootPane && foundSibling)
	{
		ViewPane* siblingPane = nullptr;
		
		if (foundSibling)
			siblingPane = *iter;

		switch (dockType)
		{
		case hui::DockType::Left:
		case hui::DockType::Top:
		{
			if (foundSibling)
			{
				// insert before
				dockInside->children.insert(iter, viewPanePtr);
			}
			else if (isRootPane)
			{
				siblingPane = dockInside->children.front();
				// insert before
				dockInside->children.insert(dockInside->children.begin(), viewPanePtr);
			}

			if (dockType == hui::DockType::Left)
			{
				viewPanePtr->rect.x = siblingPane->rect.x;
				viewPanePtr->rect.y = siblingPane->rect.y;
				viewPanePtr->rect.width = siblingPane->rect.width * splitFactor;
				viewPanePtr->rect.height = siblingPane->rect.height;
				siblingPane->rect.width -= viewPanePtr->rect.width;
				siblingPane->rect.x = viewPanePtr->rect.right();
			}
			else if (dockType == hui::DockType::Top)
			{
				viewPanePtr->rect.x = siblingPane->rect.x;
				viewPanePtr->rect.y = siblingPane->rect.y;
				viewPanePtr->rect.height = siblingPane->rect.height * splitFactor;
				viewPanePtr->rect.width = siblingPane->rect.width;
				siblingPane->rect.height -= viewPanePtr->rect.height;
				siblingPane->rect.y = viewPanePtr->rect.bottom();
			}

			break;
		}
		case hui::DockType::Right:
		case hui::DockType::Bottom:
		{
			if (foundSibling)
			{
				// insert after
				dockInside->children.insert(++iter, viewPanePtr);
			}
			else if(isRootPane)
			{
				siblingPane = dockInside->children.back();
				dockInside->children.push_back(viewPanePtr);
			}

			if (dockType == hui::DockType::Right)
			{
				viewPanePtr->rect.y = siblingPane->rect.y;
				viewPanePtr->rect.width = siblingPane->rect.width * splitFactor;
				viewPanePtr->rect.height = siblingPane->rect.height;
				siblingPane->rect.width -= viewPanePtr->rect.width;
				viewPanePtr->rect.x = siblingPane->rect.right();
			}
			else if (dockType == hui::DockType::Bottom)
			{
				viewPanePtr->rect.x = siblingPane->rect.x;
				viewPanePtr->rect.height = siblingPane->rect.height * splitFactor;
				viewPanePtr->rect.width = siblingPane->rect.width;
				siblingPane->rect.height -= viewPanePtr->rect.height;
				viewPanePtr->rect.y = siblingPane->rect.y;
			}

			break;
		}
		default:
			break;
		}
	}

	// not same split mode between pane parent's and wanted dock type, so we will create a new holder view pane to host the two panes
	if (!isSameSplit)
	{
		switch (dockType)
		{
		case hui::DockType::Left:
		case hui::DockType::Top:
		{
			// new container pane
			dockInside = new ViewPane();
			// copy fields from dock to pane
			*dockInside = *toViewPanePtr;
			// clear up everything
			dockInside->children.clear();
			dockInside->viewTabs.clear();

			if (isRootPane)
			{
				dockInside->parent = nullptr;
				dockInside->reparent(toViewPanePtr);
			}
			else
			{
				// reparent the dock to pane, to new parent pane
				toViewPanePtr->reparent(dockInside);
			}

			if (dockType == DockType::Left)
				dockInside->splitMode = ViewPane::SplitMode::Vertical;
			else
				dockInside->splitMode = ViewPane::SplitMode::Horizontal;

			// add the view panes to new parent
			//viewPaneToDock = dockInside->acquireViewTab(viewTabPtr, dockType);

			dockInside->children.push_back(viewPanePtr);

			auto siblingPane = toViewPanePtr;

			if (dockType == hui::DockType::Left)
			{
				viewPanePtr->rect.x = siblingPane->rect.x;
				viewPanePtr->rect.y = siblingPane->rect.y;
				viewPanePtr->rect.width = siblingPane->rect.width * splitFactor;
				viewPanePtr->rect.height = siblingPane->rect.height;
				siblingPane->rect.width -= viewPanePtr->rect.width;
				siblingPane->rect.x = viewPanePtr->rect.right();
			}
			else if (dockType == hui::DockType::Top)
			{
				viewPanePtr->rect.x = siblingPane->rect.x;
				viewPanePtr->rect.y = siblingPane->rect.y;
				viewPanePtr->rect.height = siblingPane->rect.height * splitFactor;
				viewPanePtr->rect.width = siblingPane->rect.width;
				siblingPane->rect.height -= viewPanePtr->rect.height;
				siblingPane->rect.y = viewPanePtr->rect.bottom();
			}

			break;
		}
		case hui::DockType::Right:
		case hui::DockType::Bottom:
		{
			dockInside = new ViewPane();
			*dockInside = *toViewPanePtr;
			dockInside->children.clear();
			dockInside->viewTabs.clear();
			toViewPanePtr->parent = dockInside;

			if (dockType == DockType::Right)
				dockInside->splitMode = ViewPane::SplitMode::Vertical;
			else
				dockInside->splitMode = ViewPane::SplitMode::Horizontal;

			// add the view panes to new parent
			dockInside->children.push_back(toViewPanePtr);
			dockInside->children.push_back(viewPanePtr);

			auto siblingPane = toViewPanePtr;

			if (dockType == hui::DockType::Right)
			{
				viewPanePtr->rect.y = siblingPane->rect.y;
				viewPanePtr->rect.width = siblingPane->rect.width * splitFactor;
				viewPanePtr->rect.height = siblingPane->rect.height;
				siblingPane->rect.width -= viewPanePtr->rect.width;
				viewPanePtr->rect.x = siblingPane->rect.right();
			}
			else if (dockType == hui::DockType::Bottom)
			{
				viewPanePtr->rect.x = siblingPane->rect.x;
				viewPanePtr->rect.height = siblingPane->rect.height * splitFactor;
				viewPanePtr->rect.width = siblingPane->rect.width;
				siblingPane->rect.height -= viewPanePtr->rect.height;
				for (auto& child : siblingPane->children)
				{
					child->rect.height = siblingPane->rect.height;
				}
				viewPanePtr->rect.y = siblingPane->rect.bottom();
			}

			break;
		}
		default:
			break;
		}
	}
}

bool dockViewTab(HViewPaneTab viewTab, HViewPane toViewPane, DockType dockType)
{
	ViewTab* viewTabPtr = (ViewTab*)viewTab;

	if (viewTabPtr->viewPane->children.size() == 1)

	return true;
}

void deleteViewPane(HViewPane viewPane)
{
	auto viewPaneObj = (ViewPane*)viewPane;
	auto iter = std::find(ctx->dockingData.rootViewPanes.begin(), ctx->dockingData.rootViewPanes.begin(), viewPaneObj);

	if (iter != ctx->dockingData.rootViewPanes.end())
	{
		ctx->dockingData.rootViewPanes.erase(iter);
	}

	if (!viewPaneObj->parent)
	{
		hui::destroyWindow(viewPaneObj->window);
	}

	delete viewPaneObj;
}

void deleteWindowRootViewPane(HWindow window)
{
	for (auto pane : ctx->dockingData.rootViewPanes)
	{
		if (pane->window == window)
		{
			deleteViewPane(pane);
			break;
		}
	}
}

u32 getRootViewPanes(HViewPane* outViewPanes, u32 maxCount)
{
	std::vector<ViewPane*> viewPanes;
	auto count = (u32)fminl(maxCount, ctx->dockingData.rootViewPanes.size());

	for (size_t i = 0; i < count; i++)
	{
		outViewPanes[i] = ctx->dockingData.rootViewPanes[i];
	}

	return count;
}

HWindow getViewPaneWindow(HViewPane viewPane)
{
	return ((ViewPane*)viewPane)->window;
}

HViewPane getWindowRootViewPane(HWindow window)
{
	for (auto pane : ctx->dockingData.rootViewPanes)
	{
		if (pane->window == window)
		{
			return pane;
		}
	}

	return nullptr;
}

bool saveViewPaneState(const char* filename)
{
	HFile file = HORUS_FILE->open(filename, "wb");

	if (!file)
		return false;

	size_t dataSize = 0;
	u8* data = saveViewPaneStateToMemory(dataSize);

	if (data && dataSize)
	{
		HORUS_FILE->write(file, data, dataSize);
		HORUS_FILE->close(file);
		delete[] data;
		return true;
	}

	delete[] data;
	HORUS_FILE->close(file);

	return false;
}

u8* saveViewPaneStateToMemory(size_t& outStateInfoSize)
{
	MemoryStream stream;

	u32 paneCount = ctx->dockingData.rootViewPanes.size();

	stream.beginWrite();
	stream.write(&paneCount, sizeof(paneCount));

	for (auto pane : ctx->dockingData.rootViewPanes)
	{
		pane->serialize(stream, ctx->currentViewHandler);
	}

	outStateInfoSize = stream.data.size();
	auto data = new u8[outStateInfoSize];
	std::memcpy(data, stream.data.data(), outStateInfoSize);

	return data;
}

bool loadViewPaneState(const char* filename)
{
	HFile file = HORUS_FILE->open(filename, "rb");

	if (!file)
		return false;
	
	size_t fileSize = HORUS_FILE->seek(file, FileSeekMode::End, 0);
	HORUS_FILE->seek(file, FileSeekMode::Set, 0);
	u8* fileData = new u8[fileSize];

	if (!HORUS_FILE->read(file, fileData, fileSize))
	{
		HORUS_FILE->close(file);
		delete[] fileData;
		return false;
	}

	auto ret = loadViewPaneStateFromMemory(fileData, fileSize);

	HORUS_FILE->close(file);

	return ret;
}

bool loadViewPaneStateFromMemory(const u8* stateInfo, size_t stateInfoSize)
{
	MemoryStream stream;

	stream.beginRead(stateInfo, stateInfoSize);

	u32 paneCount = 0;

	if (!stream.read(&paneCount, sizeof(paneCount)))
	{
		return false;
	}

	for (i32 i = 0; i < paneCount; i++)
	{
		auto pane = new ViewPane();

		pane->deserialize(stream, ctx->currentViewHandler);
		ctx->dockingData.rootViewPanes.push_back(pane);
	}

	return true;
}

void setViewPaneSideSpacing(HViewPane viewPane, f32 left, f32 right, f32 bottom)
{
	ViewPane* pane = (ViewPane*)viewPane;

	pane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingLeft] = left;
	pane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingRight] = right;
	pane->sideSpacing[(int)ViewPane::SideSpacing::SideSpacingBottom] = bottom;
}

Rect getViewPaneClientRect(HViewPane viewPane)
{
	ViewPane* viewPaneObj = (ViewPane*)viewPane;

	return viewPaneObj->rect;
}

f32 getRemainingViewPaneClientHeight(HViewPane viewPane)
{
	ViewPane* viewPaneObj = (ViewPane*)viewPane;

	return round((f32)viewPaneObj->rect.height - (ctx->penPosition.y - viewPaneObj->rect.y));
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
		hui::tab(viewPaneObj->viewTabs[i]->title.c_str(), viewPaneObj->viewTabs[i]->icon);
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
			ctx->dockingData.currentViewPane->removeChild(viewPaneObj);

			// close window if no tabs
			if (ctx->dockingData.currentViewPane->children.empty())
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

void setViewPaneTabUserData(HViewPaneTab viewPaneTab, u64 userData)
{
	((ViewTab*)viewPaneTab)->userData = userData;
}

u64 getViewPaneTabUserData(HViewPaneTab viewPaneTab)
{
	return ((ViewTab*)viewPaneTab)->userData;
}

void setViewPaneTabTitle(HViewPaneTab viewPaneTab, const char* title)
{
	auto viewPaneTabObj = (ViewTab*)viewPaneTab;

	viewPaneTabObj->title = title;
}

const char* getViewPaneTabTitle(HViewPaneTab viewPaneTab)
{
	auto viewPaneTabObj = (ViewTab*)viewPaneTab;

	return viewPaneTabObj->title.c_str();
}

ViewId getViewPaneTabViewId(HViewPaneTab viewPaneTab)
{
	auto viewPaneTabObj = (ViewTab*)viewPaneTab;

	return viewPaneTabObj->viewId;
}

void setViewIcon(ViewId id, HImage icon)
{
	std::vector<ViewTab*> tabs;

	for (auto& dc : ctx->dockingData.rootViewPanes)
	{
		tabs.clear();
		dc->gatherViewTabs(tabs);

		for (auto& tab : tabs)
		{
			if (tab->viewId == id)
			{
				tab->icon = icon;
			}
		}
	}
}

HViewPaneTab createViewPaneTab(HViewPane viewPane, const char* title, ViewId id, u64 userData)
{
	auto viewPaneObj = (ViewPane*)viewPane;
	auto viewTab = new ViewTab();

	viewTab->viewId = id;
	viewTab->viewPane = viewPaneObj;
	viewTab->title = title;
	viewTab->userData = userData;
	viewPaneObj->viewTabs.push_back(viewTab);

	return viewTab;
}

void removeViewPaneTab(HViewPaneTab viewPaneTab)
{
	auto viewTabObj = (ViewTab*)viewPaneTab;
	auto viewPaneObj = viewTabObj->viewPane;

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
