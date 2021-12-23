#include "horus.h"
#include "layout_cell.h"
#include "docking_system.h"
#include <string.h>
#include <algorithm>
#include <math.h>
#include "context.h"

namespace hui
{
HViewContainer createViewContainer(HWindow window)
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

void deleteViewContainer(HViewContainer viewContainer)
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

u32 getViewContainers(HViewContainer* outViewContainers, u32 maxCount)
{
	std::vector<UiViewPane*> viewPanes;
	auto count = (u32)fminl(maxCount, dockingData.viewContainers.size());

	for (size_t i = 0; i < count; i++)
	{
		outViewContainers[i] = dockingData.viewContainers[i];
	}

	return count;
}

HWindow getViewContainerWindow(HViewContainer viewContainer)
{
	auto iter = std::find(dockingData.viewContainers.begin(), dockingData.viewContainers.end(), (UiViewContainer*)viewContainer);

	if (iter == dockingData.viewContainers.end())
		return 0;

	return ((UiViewContainer*)viewContainer)->window;
}

HViewContainer getWindowViewContainer(HWindow window)
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

void deleteViewContainerFromWindow(HWindow window)
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

u32 getViewContainerViewPanes(HViewContainer viewContainer, HViewPane* outViewPanes, u32 maxCount)
{
	std::vector<UiViewPane*> viewPanes;
	auto viewContainerObj = (UiViewContainer*)viewContainer;

	viewContainerObj->rootCell->gatherViewPanes(viewPanes);
	auto count = (u32)std::min(maxCount, (u32)viewPanes.size());

	for (size_t i = 0; i < count; i++)
	{
		outViewPanes[i] = viewPanes[i];
	}

	return count;
}

u32 getViewContainerViewPaneCount(HViewContainer viewContainer)
{
	std::vector<UiViewPane*> viewPanes;
	auto viewContainerObj = (UiViewContainer*)viewContainer;

	viewContainerObj->rootCell->gatherViewPanes(viewPanes);

	return (u32)viewPanes.size();
}

HViewPane getViewContainerFirstViewPane(HViewContainer viewContainer)
{
	auto viewContainerObj = (UiViewContainer*)viewContainer;
	std::vector<UiViewPane*> viewPanes;

	viewContainerObj->rootCell->gatherViewPanes(viewPanes);

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
		container->serialize(file, ctx->currentViewHandler);
	}

	return true;
}

bool loadViewContainersState(const char* filename)
{
	//TODO: use HORUS_FILE provider
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

		container->deserialize(file, ctx->currentViewHandler);
		dockingData.viewContainers.push_back(container);
		updateViewContainerLayout(container);
	}

	fclose(file);

	return true;
}

void setViewContainerSideSpacing(HViewContainer viewContainer, f32 left, f32 right, f32 bottom)
{
	UiViewContainer* vc = (UiViewContainer*)viewContainer;

	vc->sideSpacing[UiViewContainer::SideSpacingLeft] = left;
	vc->sideSpacing[UiViewContainer::SideSpacingRight] = right;
	vc->sideSpacing[UiViewContainer::SideSpacingBottom] = bottom;
}

}
