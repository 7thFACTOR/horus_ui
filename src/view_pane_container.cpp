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
	auto viewContainer = new ViewContainer();

	viewContainer->rootCell = new LayoutCell();
	viewContainer->rootCell->normalizedSize.x = 1.0f;
	viewContainer->rootCell->normalizedSize.y = 1.0f;
	viewContainer->window = window;

	ctx->dockingData.viewContainers.push_back(viewContainer);

	return viewContainer;
}

void deleteViewContainer(HViewContainer viewContainer)
{
	auto viewContainerObj = (ViewContainer*)viewContainer;
	auto iter = std::find(ctx->dockingData.viewContainers.begin(), ctx->dockingData.viewContainers.begin(), viewContainerObj);

	if (iter != ctx->dockingData.viewContainers.end())
	{
		ctx->dockingData.viewContainers.erase(iter);
	}

	hui::destroyWindow(viewContainerObj->window);

	delete viewContainerObj;
}

u32 getViewContainers(HViewContainer* outViewContainers, u32 maxCount)
{
	std::vector<ViewPane*> viewPanes;
	auto count = (u32)fminl(maxCount, ctx->dockingData.viewContainers.size());

	for (size_t i = 0; i < count; i++)
	{
		outViewContainers[i] = ctx->dockingData.viewContainers[i];
	}

	return count;
}

HWindow getViewContainerWindow(HViewContainer viewContainer)
{
	auto iter = std::find(ctx->dockingData.viewContainers.begin(), ctx->dockingData.viewContainers.end(), (ViewContainer*)viewContainer);

	if (iter == ctx->dockingData.viewContainers.end())
		return 0;

	return ((ViewContainer*)viewContainer)->window;
}

HViewContainer getWindowViewContainer(HWindow window)
{
	for (auto& iter : ctx->dockingData.viewContainers)
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
	for (auto container : ctx->dockingData.viewContainers)
	{
		if (container->window == window)
		{
			auto iter = std::find(ctx->dockingData.viewContainers.begin(), ctx->dockingData.viewContainers.end(), container);

			if (iter != ctx->dockingData.viewContainers.end())
			{
				container->destroy();
				delete container;
				ctx->dockingData.viewContainers.erase(iter);
				return;
			}
		}
	}
}

u32 getViewContainerViewPanes(HViewContainer viewContainer, HViewPane* outViewPanes, u32 maxCount)
{
	std::vector<ViewPane*> viewPanes;
	auto viewContainerObj = (ViewContainer*)viewContainer;

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
	std::vector<ViewPane*> viewPanes;
	auto viewContainerObj = (ViewContainer*)viewContainer;

	viewContainerObj->rootCell->gatherViewPanes(viewPanes);

	return (u32)viewPanes.size();
}

HViewPane getViewContainerFirstViewPane(HViewContainer viewContainer)
{
	auto viewContainerObj = (ViewContainer*)viewContainer;
	std::vector<ViewPane*> viewPanes;

	viewContainerObj->rootCell->gatherViewPanes(viewPanes);

	if (viewPanes.size())
	{
		return viewPanes[0];
	}

	return nullptr;
}

bool saveViewContainersState(const char* filename)
{
	HFile file = HORUS_FILE->open(filename, "wb");

	if (!file)
		return false;

	i32 containerCount = ctx->dockingData.viewContainers.size();

	HORUS_FILE->write(file, &containerCount, sizeof(containerCount));

	for (auto container : ctx->dockingData.viewContainers)
	{
		container->serialize(file, ctx->currentViewHandler);
	}

	return true;
}

bool loadViewContainersState(const char* filename)
{
	HFile file = HORUS_FILE->open(filename, "rb");

	if (!file)
		return false;

	i32 containerCount = 0;

	if (!HORUS_FILE->read(file, &containerCount, sizeof(containerCount)))
	{
		HORUS_FILE->close(file);
		return false;
	}

	for (i32 i = 0; i < containerCount; i++)
	{
		auto container = new ViewContainer();

		container->deserialize(file, ctx->currentViewHandler);
		ctx->dockingData.viewContainers.push_back(container);
		updateViewContainerLayout(container);
	}

	HORUS_FILE->close(file);

	return true;
}

void setViewContainerSideSpacing(HViewContainer viewContainer, f32 left, f32 right, f32 bottom)
{
	ViewContainer* vc = (ViewContainer*)viewContainer;

	vc->sideSpacing[ViewContainer::SideSpacingLeft] = left;
	vc->sideSpacing[ViewContainer::SideSpacingRight] = right;
	vc->sideSpacing[ViewContainer::SideSpacingBottom] = bottom;
}

}
