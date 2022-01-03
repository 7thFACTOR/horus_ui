#include "horus.h"
#include "view_pane.h"
#include "docking_system.h"
#include <string.h>
#include <algorithm>
#include <math.h>
#include "context.h"

namespace hui
{
HViewPane createRootViewPane(HWindow window)
{
	auto rect = getWindowRect(window);
	auto viewPane = new ViewPane();

	viewPane->normalizedSize.x = 1.0f;
	viewPane->normalizedSize.y = 1.0f;
	viewPane->window = window;
	ctx->dockingData.rootViewPanes.push_back(viewPane);

	return viewPane;
}
//
//HViewPane createViewPane(HViewPane parentViewPane, DockType dock, f32 size)
//{
//	ViewPane* viewPaneObj = (ViewPane*)parentViewPane;
//	auto newViewPane = new ViewPane();
//
//	dockViewTab
//
//	viewPaneObj->doc
//
//	newViewPane->normalizedSize.x = 1.0f;
//	newViewPane->normalizedSize.y = 1.0f;
//	newViewPane->window = viewPaneObj->window;
//	newViewPane->parent = viewPaneObj;
//	viewPaneObj->children.push_back(newViewPane);
//
//	return viewPane;
//}

bool dockViewTab(HViewPane viewPane, HViewPaneTab viewTab, DockType dockType)
{
	auto viewPanePtr = (ViewPane*)viewPane;
	auto viewTabPtr = (ViewTab*)viewTab;
	auto parent = viewPanePtr->parent;
	auto dockTo = parent ? parent : viewPanePtr;
	ViewPane* newViewPane = nullptr;

	if (dockTo->children.empty())
	{
		switch (dockType)
		{
		case hui::DockType::Left:
		case hui::DockType::Right:
			dockTo->splitMode = ViewPane::SplitMode::Horizontal;
			break;
		case hui::DockType::Top:
		case hui::DockType::Bottom:
			dockTo->splitMode = ViewPane::SplitMode::Vertical;
			break;
		case hui::DockType::TopAsViewTab:
			dockTo->splitMode = ViewPane::SplitMode::None;
			break;
		}
	}

	// if we have a parent, dock inside it, next to our view pane
	if (parent)
	{
		auto iter = parent->children.begin();
		while (iter != parent->children.end())
		{
			if (*iter == viewPanePtr)
			{
				switch (dockType)
				{
				case hui::DockType::Left:
				case hui::DockType::Top:
				{
					newViewPane = parent->acquireViewTab(viewTabPtr, dockType);
					parent->children.insert(iter, newViewPane);
					newViewPane->setNewSize();
					break;
				}
				case hui::DockType::Right:
				case hui::DockType::Bottom:
				{
					newViewPane = parent->acquireViewTab(viewTabPtr, dockType);
					parent->children.insert(++iter, newViewPane);
					break;
				}
				case hui::DockType::TopAsViewTab:
				{
					newViewPane = parent->acquireViewTab(viewTabPtr, dockType);
					break;
				}
				}
				break;
			}
		}
	}
	else
	{
		// no parent, that means this is the root view pane
		switch (dockType)
		{
		case hui::DockType::Left:
		case hui::DockType::Top:
		{
			newViewPane = viewPanePtr->acquireViewTab(viewTabPtr, dockType);
			viewPanePtr->children.insert(viewPanePtr->children.begin(), newViewPane);
			break;
		}
		case hui::DockType::Right:
		case hui::DockType::Bottom:
		{
			newViewPane = viewPanePtr->acquireViewTab(viewTabPtr, dockType);
			viewPanePtr->children.insert(viewPanePtr->children.end(), newViewPane);
			break;
		}
		case hui::DockType::TopAsViewTab:
		{
			newViewPane = viewPanePtr->acquireViewTab(viewTabPtr, dockType);
			break;
		}
		}
	}


	switch (dockTo->splitMode)
	{
	case ViewPane::SplitMode::Vertical:
		dockTo->normalizedSize.x = 1.0f;
		newViewPane->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
		break;
	case ViewPane::SplitMode::Horizontal:
		newViewPane->normalizedSize.x = 1.0f - percentOfNewPaneSplit;
		newViewPane->normalizedSize.y = 1.0f;
		break;
	}


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
	u8* data = 	saveViewPaneStateToMemory(dataSize);

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
		updateViewPaneLayout(pane);
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
			hui::updateViewPaneLayout(ctx->dockingData.currentViewPane);

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

HViewPaneTab addViewPaneTab(HViewPane viewPane, const char* title, ViewId id, u64 userData)
{
	auto viewPaneObj = (ViewPane*)viewPane;
	auto view = new ViewTab();

	view->viewId = id;
	view->viewPane = viewPaneObj;
	view->title = title;
	view->userData = userData;
	viewPaneObj->viewTabs.push_back(view);

	return view;
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
