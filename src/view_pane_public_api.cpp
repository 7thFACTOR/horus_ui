#include "horus.h"
#include "view.h"
#include "docking_system.h"
#include <cstring>
#include <algorithm>
#include <math.h>
#include "context.h"

namespace hui
{
HViewContainer createRootViewContainer(HWindow window)
{
	auto rect = getWindowRect(window);
	auto viewContainer = new ViewContainer();

	viewContainer->rect.set(0, 0, rect.width, rect.height);
	viewContainer->window = window;
	ctx->dockingData.rootViewContainers.push_back(viewContainer);

	return View;
}

HView createView(HViewContainer viewContainer, f32 size, DockType dock, const char* title, ViewType viewType, u64 userData)
{
	auto viewContainerPtr = (ViewContainer*)viewContainer;
	auto newView = new View();

	newView->window = viewContainerPtr->window;
	newView->parent = viewContainerPtr;
	newView->rect.width = newView->rect.height = size;
	newView->size = size;
	dockView(newView, viewContainer, dock);

	return newView;
}

void resizeViewsAfterDocking(DockType dockType, View* viewToDock, View* siblingView, f32 splitFactor)
{
	switch (dockType)
	{
		case hui::DockType::Left:
		{
			viewToDock->rect.x = siblingView->rect.x;
			viewToDock->rect.y = siblingView->rect.y;
			viewToDock->rect.width = siblingView->rect.width * splitFactor;
			viewToDock->rect.height = siblingView->rect.height;
			siblingView->rect.width -= viewToDock->rect.width;
			siblingView->rect.x = viewToDock->rect.right();
			break;
		}
		case hui::DockType::Top:
		{
			viewToDock->rect.x = siblingView->rect.x;
			viewToDock->rect.y = siblingView->rect.y;
			viewToDock->rect.height = siblingView->rect.height * splitFactor;
			viewToDock->rect.width = siblingView->rect.width;
			siblingView->rect.height -= viewToDock->rect.height;
			siblingView->rect.y = viewToDock->rect.bottom();
			break;
		}
		case hui::DockType::Right:
		{
			viewToDock->rect.y = siblingView->rect.y;
			viewToDock->rect.width = siblingView->rect.width * splitFactor;
			viewToDock->rect.height = siblingView->rect.height;
			siblingView->rect.width -= viewToDock->rect.width;
			viewToDock->rect.x = siblingView->rect.right();
			break;
		}
		case hui::DockType::Bottom:
		{
			viewToDock->rect.x = siblingView->rect.x;
			viewToDock->rect.height = siblingView->rect.height * splitFactor;
			viewToDock->rect.width = siblingView->rect.width;
			siblingView->rect.height -= viewToDock->rect.height;
			viewToDock->rect.y = siblingView->rect.y;
			break;
		}
		default:
		break;
	}
}

void insertViewOnDocking(View* pane, View* sibling, View* parent, DockType dockType)
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

bool dockView(HView view, HView toView, DockType dockType)
{
	auto viewPtr = (View*)view;
	auto toViewPtr = (View*)toView;
	auto toViewContainer = toViewPtr->viewContainer;
	auto isRootViewContainer = !toViewContainer->parent;

	View::SplitMode neededSplitMode = View::SplitMode::None;

	switch (dockType)
	{
	case hui::DockType::Left:
	case hui::DockType::Right:
		neededSplitMode = View::SplitMode::Horizontal;
		break;
	case hui::DockType::Top:
	case hui::DockType::Bottom:
		neededSplitMode = View::SplitMode::Vertical;
		break;
	case hui::DockType::TopAsTab:
		neededSplitMode = View::SplitMode::Tabs;
		break;
	}

	// if this is the first view pane to be created and docked, set the split mode
	if (toViewContainer->views.empty())
	{
		toViewContainer->splitMode = neededSplitMode;
		toViewContainer->views.push_back(ViewPtr);
		viewPtr->parent = toViewContainer;
		viewPtr->rect = toViewContainer->rect;
		return true;
	}

	// dock inside the parent view pane, next to a sibling view pane
	// search the sibling where we want to dock next to
	std::vector<View*>::iterator iter = dockInside->children.end();

	iter = std::find(toViewContainer->views.begin(), toViewContainer->views.end(), toViewPtr);

	const f32 splitFactor = 0.333333f;
	// is same split mode if its the same with requested split mode of the docking type, or if the host has only 1 child, in which case split mode doesnt matter what it is, it will be set
	bool isSameSplit = toViewContainer->splitMode == neededSplitMode || (toViewContainer->views.size() == 1);
	bool foundSibling = iter != toViewContainer->views.end();

	// if same split then just insert before or after sibling (if any)
	if (isSameSplit)
	{
		insertViewOnDocking(viewPtr, foundSibling ? *iter : nullptr, toViewContainer, dockType);
		resizeViewsAfterDocking(dockType, viewPtr, foundSibling ? *iter : nullptr, splitFactor);
	}
	else // create a new view container to contain the current views
	{
		auto newViewContainer = new ViewContainer();
		newViewContainer->parent = toViewContainer;
		newViewContainer->rect = toViewContainer->rect;
		newViewContainer->splitMode = toViewContainer->splitMode;
		newViewContainer->window = dockInside->window;
		
		for (auto& child : dockInside->children)
		{
			child->reparent(newPane);
		}

		insertViewOnDocking(ViewPtr, newPane, dockInside, dockType);
		resizePanesAfterDocking(dockType, ViewPtr, newPane, splitFactor);
	}

	dockInside->splitMode = neededSplitMode;
/*
	if (isSameSplit && !isRootPane && foundSibling)
	{
		View* siblingView = nullptr;
		
		if (foundSibling)
			siblingView = *iter;

		switch (dockType)
		{
		case hui::DockType::Left:
		case hui::DockType::Top:
		{
			if (foundSibling)
			{
				// insert before
				dockInside->children.insert(iter, ViewPtr);
			}
			else if (isRootPane)
			{
				siblingView = dockInside->children.front();
				// insert before
				dockInside->children.insert(dockInside->children.begin(), ViewPtr);
			}

			if (dockType == hui::DockType::Left)
			{
				ViewPtr->rect.x = siblingView->rect.x;
				ViewPtr->rect.y = siblingView->rect.y;
				ViewPtr->rect.width = siblingView->rect.width * splitFactor;
				ViewPtr->rect.height = siblingView->rect.height;
				siblingView->rect.width -= ViewPtr->rect.width;
				siblingView->rect.x = ViewPtr->rect.right();
			}
			else if (dockType == hui::DockType::Top)
			{
				ViewPtr->rect.x = siblingView->rect.x;
				ViewPtr->rect.y = siblingView->rect.y;
				ViewPtr->rect.height = siblingView->rect.height * splitFactor;
				ViewPtr->rect.width = siblingView->rect.width;
				siblingView->rect.height -= ViewPtr->rect.height;
				siblingView->rect.y = ViewPtr->rect.bottom();
			}

			break;
		}
		case hui::DockType::Right:
		case hui::DockType::Bottom:
		{
			if (foundSibling)
			{
				// insert after
				dockInside->children.insert(++iter, ViewPtr);
			}
			else if(isRootPane)
			{
				siblingView = dockInside->children.back();
				dockInside->children.push_back(ViewPtr);
			}

			if (dockType == hui::DockType::Right)
			{
				ViewPtr->rect.y = siblingView->rect.y;
				ViewPtr->rect.width = siblingView->rect.width * splitFactor;
				ViewPtr->rect.height = siblingView->rect.height;
				siblingView->rect.width -= ViewPtr->rect.width;
				ViewPtr->rect.x = siblingView->rect.right();
			}
			else if (dockType == hui::DockType::Bottom)
			{
				ViewPtr->rect.x = siblingView->rect.x;
				ViewPtr->rect.height = siblingView->rect.height * splitFactor;
				ViewPtr->rect.width = siblingView->rect.width;
				siblingView->rect.height -= ViewPtr->rect.height;
				ViewPtr->rect.y = siblingView->rect.y;
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
			dockInside = new View();
			// copy fields from dock to pane
			*dockInside = *toViewPtr;
			// clear up everything
			dockInside->children.clear();
			dockInside->viewTabs.clear();

			if (isRootPane)
			{
				dockInside->parent = nullptr;
				dockInside->reparent(toViewPtr);
			}
			else
			{
				// reparent the dock to pane, to new parent pane
				toViewPtr->reparent(dockInside);
			}

			if (dockType == DockType::Left)
				dockInside->splitMode = View::SplitMode::Vertical;
			else
				dockInside->splitMode = View::SplitMode::Horizontal;

			// add the view panes to new parent
			//viewToDock = dockInside->acquireViewTab(viewTabPtr, dockType);

			dockInside->children.push_back(ViewPtr);

			auto siblingView = toViewPtr;

			if (dockType == hui::DockType::Left)
			{
				ViewPtr->rect.x = siblingView->rect.x;
				ViewPtr->rect.y = siblingView->rect.y;
				ViewPtr->rect.width = siblingView->rect.width * splitFactor;
				ViewPtr->rect.height = siblingView->rect.height;
				siblingView->rect.width -= ViewPtr->rect.width;
				siblingView->rect.x = ViewPtr->rect.right();
			}
			else if (dockType == hui::DockType::Top)
			{
				ViewPtr->rect.x = siblingView->rect.x;
				ViewPtr->rect.y = siblingView->rect.y;
				ViewPtr->rect.height = siblingView->rect.height * splitFactor;
				ViewPtr->rect.width = siblingView->rect.width;
				siblingView->rect.height -= ViewPtr->rect.height;
				siblingView->rect.y = ViewPtr->rect.bottom();
			}

			break;
		}
		case hui::DockType::Right:
		case hui::DockType::Bottom:
		{
			dockInside = new View();
			*dockInside = *toViewPtr;
			dockInside->children.clear();
			dockInside->viewTabs.clear();
			toViewPtr->parent = dockInside;

			if (dockType == DockType::Right)
				dockInside->splitMode = View::SplitMode::Vertical;
			else
				dockInside->splitMode = View::SplitMode::Horizontal;

			// add the view panes to new parent
			dockInside->children.push_back(toViewPtr);
			dockInside->children.push_back(ViewPtr);

			auto siblingView = toViewPtr;

			if (dockType == hui::DockType::Right)
			{
				ViewPtr->rect.y = siblingView->rect.y;
				ViewPtr->rect.width = siblingView->rect.width * splitFactor;
				ViewPtr->rect.height = siblingView->rect.height;
				siblingView->rect.width -= ViewPtr->rect.width;
				ViewPtr->rect.x = siblingView->rect.right();
			}
			else if (dockType == hui::DockType::Bottom)
			{
				ViewPtr->rect.x = siblingView->rect.x;
				ViewPtr->rect.height = siblingView->rect.height * splitFactor;
				ViewPtr->rect.width = siblingView->rect.width;
				siblingView->rect.height -= ViewPtr->rect.height;
				for (auto& child : siblingView->children)
				{
					child->rect.height = siblingView->rect.height;
				}
				ViewPtr->rect.y = siblingView->rect.bottom();
			}

			break;
		}
		default:
			break;
		}
	}
}

bool dockViewTab(HViewTab viewTab, HView toView, DockType dockType)
{
	ViewTab* viewTabPtr = (ViewTab*)viewTab;

	if (viewTabPtr->View->children.size() == 1)

	return true;
}

void deleteView(HView View)
{
	auto ViewObj = (View*)View;
	auto iter = std::find(ctx->dockingData.rootViews.begin(), ctx->dockingData.rootViews.begin(), ViewObj);

	if (iter != ctx->dockingData.rootViews.end())
	{
		ctx->dockingData.rootViews.erase(iter);
	}

	if (!ViewObj->parent)
	{
		hui::destroyWindow(ViewObj->window);
	}

	delete ViewObj;
}

void deleteWindowRootView(HWindow window)
{
	for (auto pane : ctx->dockingData.rootViews)
	{
		if (pane->window == window)
		{
			deleteView(pane);
			break;
		}
	}
}

u32 getRootViews(HView* outViews, u32 maxCount)
{
	std::vector<View*> Views;
	auto count = (u32)fminl(maxCount, ctx->dockingData.rootViews.size());

	for (size_t i = 0; i < count; i++)
	{
		outViews[i] = ctx->dockingData.rootViews[i];
	}

	return count;
}

HWindow getViewWindow(HView View)
{
	return ((View*)View)->window;
}

HView getWindowRootView(HWindow window)
{
	for (auto pane : ctx->dockingData.rootViews)
	{
		if (pane->window == window)
		{
			return pane;
		}
	}

	return nullptr;
}

bool saveViewState(const char* filename)
{
	HFile file = HORUS_FILE->open(filename, "wb");

	if (!file)
		return false;

	size_t dataSize = 0;
	u8* data = saveViewStateToMemory(dataSize);

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

u8* saveViewStateToMemory(size_t& outStateInfoSize)
{
	MemoryStream stream;

	u32 paneCount = ctx->dockingData.rootViews.size();

	stream.beginWrite();
	stream.write(&paneCount, sizeof(paneCount));

	for (auto pane : ctx->dockingData.rootViews)
	{
		pane->serialize(stream, ctx->currentViewHandler);
	}

	outStateInfoSize = stream.data.size();
	auto data = new u8[outStateInfoSize];
	std::memcpy(data, stream.data.data(), outStateInfoSize);

	return data;
}

bool loadViewState(const char* filename)
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

	auto ret = loadViewStateFromMemory(fileData, fileSize);

	HORUS_FILE->close(file);

	return ret;
}

bool loadViewStateFromMemory(const u8* stateInfo, size_t stateInfoSize)
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
		auto pane = new View();

		pane->deserialize(stream, ctx->currentViewHandler);
		ctx->dockingData.rootViews.push_back(pane);
	}

	return true;
}

void setViewSideSpacing(HView View, f32 left, f32 right, f32 bottom)
{
	View* pane = (View*)View;

	pane->sideSpacing[(int)View::SideSpacing::SideSpacingLeft] = left;
	pane->sideSpacing[(int)View::SideSpacing::SideSpacingRight] = right;
	pane->sideSpacing[(int)View::SideSpacing::SideSpacingBottom] = bottom;
}

Rect getViewClientRect(HView View)
{
	View* ViewObj = (View*)View;

	return ViewObj->rect;
}

f32 getRemainingViewClientHeight(HView View)
{
	View* ViewObj = (View*)View;

	return round((f32)ViewObj->rect.height - (ctx->penPosition.y - ViewObj->rect.y));
}

ViewId ViewTabs(HView View)
{
	auto ViewObj = (View*)View;

	if (ctx->layoutStack.back().width <= (ViewObj->viewTabs.size() * (ctx->paneGroupState.tabWidth + ctx->paneGroupState.sideSpacing)) * ctx->globalScale)
	{
		ctx->paneGroupState.forceTabWidth = ctx->layoutStack.back().width / (f32)ViewObj->viewTabs.size();
		ctx->paneGroupState.forceSqueezeTabs = true;
	}
	else
	{
		ctx->paneGroupState.forceSqueezeTabs = false;
	}

	u32 closeTabIndex = ~0;

	ctx->drawingViewTabs = true;
	beginTabGroup(ViewObj->selectedTabIndex);

	for (size_t i = 0; i < ViewObj->viewTabs.size(); i++)
	{
		hui::tab(ViewObj->viewTabs[i]->title.c_str(), ViewObj->viewTabs[i]->icon);
		ViewObj->viewTabs[i]->rect = ctx->widget.rect;

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

	ctx->drawingViewTabs = false;

	if (closeTabIndex != ~0)
	{
		ViewObj->removeViewTab(ViewObj->viewTabs[closeTabIndex]);

		if (ViewObj->viewTabs.empty())
		{
			ctx->dockingData.currentView->removeChild(ViewObj);

			// close window if no tabs
			if (ctx->dockingData.currentView->children.empty())
			{
				//dockingData.closeWindow = true;
			}
		}

		if (selectedIndex >= ViewObj->viewTabs.size())
			selectedIndex = ViewObj->viewTabs.size() - 1;
	}

	if (!ViewObj->viewTabs.empty())
	{
		if (ViewObj->selectedTabIndex != selectedIndex)
		{
			ViewObj->selectedTabIndex = selectedIndex;
			hui::forceRepaint();
		}

		return ViewObj->viewTabs[ViewObj->selectedTabIndex]->viewId;
	}

	return ~0;
}

ViewId beginView(HView View)
{
	View* ViewObj = (View*)View;

	beginContainer(ViewObj->rect);

	return ViewTabs(View);
}

void endView()
{
	endContainer();
}

void setViewTabUserData(HViewTab ViewTab, u64 userData)
{
	((ViewTab*)ViewTab)->userData = userData;
}

u64 getViewTabUserData(HViewTab ViewTab)
{
	return ((ViewTab*)ViewTab)->userData;
}

void setViewTabTitle(HViewTab ViewTab, const char* title)
{
	auto ViewTabObj = (ViewTab*)ViewTab;

	ViewTabObj->title = title;
}

const char* getViewTabTitle(HViewTab ViewTab)
{
	auto ViewTabObj = (ViewTab*)ViewTab;

	return ViewTabObj->title.c_str();
}

ViewId getViewTabViewId(HViewTab ViewTab)
{
	auto ViewTabObj = (ViewTab*)ViewTab;

	return ViewTabObj->viewId;
}

void setViewIcon(ViewId id, HImage icon)
{
	std::vector<ViewTab*> tabs;

	for (auto& dc : ctx->dockingData.rootViews)
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

HViewTab createViewTab(HView View, const char* title, ViewId id, u64 userData)
{
	auto ViewObj = (View*)View;
	auto viewTab = new ViewTab();

	viewTab->viewId = id;
	viewTab->View = ViewObj;
	viewTab->title = title;
	viewTab->userData = userData;
	ViewObj->viewTabs.push_back(viewTab);

	return viewTab;
}

void removeViewTab(HViewTab ViewTab)
{
	auto viewTabObj = (ViewTab*)ViewTab;
	auto ViewObj = viewTabObj->View;

	auto iter = std::find(ViewObj->viewTabs.begin(), ViewObj->viewTabs.end(), viewTabObj);

	if (iter != ViewObj->viewTabs.end())
	{
		ViewObj->viewTabs.erase(iter);
	}

	if (ViewObj->selectedTabIndex >= ViewObj->viewTabs.size())
	{
		ViewObj->selectedTabIndex--;
	}

	if (ViewObj->selectedTabIndex < 0)
	{
		ViewObj->selectedTabIndex = 0;
	}

	if (ViewObj->viewTabs.empty())
	{
		ViewObj->selectedTabIndex = ~0;
	}
}

void activateView(HView View)
{
	//TODO
}

void closeView(HView View)
{
	//TODO
}

void restoreView(HView View)
{
	//TODO
}

void maximizeView(HView View)
{
	//TODO
}

u32 getViewTabs(HView View, HViewTab* outViewTabs, u32 maxCount)
{
	auto ViewObj = (View*)View;
	auto count = (u32)std::min(maxCount, (u32)ViewObj->viewTabs.size());

	for (size_t i = 0; i < count; i++)
	{
		outViewTabs[i] = ViewObj->viewTabs[i];
	}

	return count;
}

u32 getViewTabCount(HView View)
{
	auto ViewObj = (View*)View;
	return (u32)ViewObj->viewTabs.size();
}

}
