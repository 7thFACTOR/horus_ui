#include "horus.h"
#include "layout_cell.h"
#include "docking_system.h"
#include <string.h>
#include <algorithm>
#include <math.h>

namespace hui
{
ViewContainer createViewContainer(Window window)
{
	auto rect = getWindowRect(window);
	auto viewContainer = new UiViewContainer();

	viewContainer->rootCell = new LayoutCell();
	viewContainer->rootCell->normalizedSize.x = 1.0f;
	viewContainer->rootCell->normalizedSize.y = 1.0f;
	viewContainer->window = window;

	dockingData.viewContainers.push_back(viewContainer);

	return viewContainer;
}

void deleteViewContainer(ViewContainer viewContainer)
{
	auto viewContainerObj = (UiViewContainer*)viewContainer;
	auto iter = std::find(dockingData.viewContainers.begin(), dockingData.viewContainers.begin(), viewContainerObj);

	if (iter != dockingData.viewContainers.end())
	{
		dockingData.viewContainers.erase(iter);
	}

	hui::destroyWindow(viewContainerObj->window);

	delete viewContainerObj;
}

u32 getViewContainers(ViewContainer* outViewContainers, u32 maxCount)
{
	std::vector<UiViewPane*> viewPanes;
	auto count = (u32)fminl(maxCount, dockingData.viewContainers.size());

	for (size_t i = 0; i < count; i++)
	{
		outViewContainers[i] = dockingData.viewContainers[i];
	}

	return count;
}

Window getViewContainerWindow(ViewContainer viewContainer)
{
	auto iter = std::find(dockingData.viewContainers.begin(), dockingData.viewContainers.end(), (UiViewContainer*)viewContainer);

	if (iter == dockingData.viewContainers.end())
		return 0;

	return ((UiViewContainer*)viewContainer)->window;
}

ViewContainer getWindowViewContainer(Window window)
{
	for (auto& iter : dockingData.viewContainers)
	{
		if (iter->window == window)
		{
			return iter;
		}
	}

	return nullptr;
}

void deleteViewContainerFromWindow(Window window)
{
	for (auto container : dockingData.viewContainers)
	{
		if (container->window == window)
		{
			auto iter = std::find(dockingData.viewContainers.begin(), dockingData.viewContainers.end(), container);

			if (iter != dockingData.viewContainers.end())
			{
				container->destroy();
				delete container;
				dockingData.viewContainers.erase(iter);
				return;
			}
		}
	}
}

u32 getViewContainerViewPanes(ViewContainer viewContainer, ViewPane* outViewPanes, u32 maxCount)
{
	std::vector<UiViewPane*> viewPanes;
	auto viewContainerObj = (UiViewContainer*)viewContainer;

	viewContainerObj->rootCell->fillViewPanes(viewPanes);
	auto count = (u32)std::min(maxCount, (u32)viewPanes.size());

	for (size_t i = 0; i < count; i++)
	{
		outViewPanes[i] = viewPanes[i];
	}

	return count;
}

ViewPane getViewContainerFirstViewPane(ViewContainer viewContainer)
{
	auto viewContainerObj = (UiViewContainer*)viewContainer;
	std::vector<UiViewPane*> viewPanes;

	viewContainerObj->rootCell->fillViewPanes(viewPanes);

	if (viewPanes.size())
	{
		return viewPanes[0];
	}

	return nullptr;
}

bool saveViewContainersState(const char* filename)
{
	FILE* file = fopen(filename, "wb");

	if (!file)
		return false;

	i32 containerCount = dockingData.viewContainers.size();

	fwrite(&containerCount, sizeof(containerCount), 1, file);

	for (auto container : dockingData.viewContainers)
	{
		container->serialize(file);
	}

	return true;
}

bool loadViewContainersState(const char* filename)
{
	FILE* file = fopen(filename, "rb");

	if (!file)
		return false;

	i32 containerCount = 0;

	if (!fread(&containerCount, sizeof(containerCount), 1, file))
	{
		fclose(file);
		return false;
	}

	for (i32 i = 0; i < containerCount; i++)
	{
		auto container = new UiViewContainer();

		container->deserialize(file);
		dockingData.viewContainers.push_back(container);
		updateViewContainerLayout(container);
	}

	fclose(file);

	return true;
}

void setViewContainerSideSpacing(ViewContainer viewContainer, f32 left, f32 right, f32 bottom)
{
	UiViewContainer* vc = (UiViewContainer*)viewContainer;

	vc->sideSpacing[UiViewContainer::SideSpacingLeft] = left;
	vc->sideSpacing[UiViewContainer::SideSpacingRight] = right;
	vc->sideSpacing[UiViewContainer::SideSpacingBottom] = bottom;
}

}
