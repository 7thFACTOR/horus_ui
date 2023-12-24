#include "types.h"
#include "horus.h"
#include "horus_interfaces.h"
#include "docking.h"
#include "context.h"
#include "renderer.h"
#include "dock_node.h"

namespace hui
{

void createMainWindow(HOsWindow osWnd)
{
	auto title = HORUS_INPUT->getWindowTitle(osWnd);
	auto rc = HORUS_INPUT->getWindowRect(osWnd);
	auto wnd = createWindow(HORUS_MAIN_WINDOW_ID, nullptr, DockType::None, title, &rc, osWnd);
	ctx->dockingState.mainWindow = wnd;
	ctx->currentWindow = wnd;
	ctx->hoveringThisWindow = true;
}

void closeMainWindow()
{
	deleteWindow(ctx->dockingState.mainWindow);
	ctx->dockingState.mainWindow = 0;
}

bool beginWindow(const char* id, const char* title, const char* dockTo, DockType dockType, Rect* initialRect)
{
	Window* wnd = nullptr;

	if (ctx->dockingState.windows.find(id) == ctx->dockingState.windows.end())
	{
		auto iter = ctx->dockingState.windows.find(dockTo ? dockTo : "");
		DockNode* dockToNode = nullptr;

		if (dockTo && iter != ctx->dockingState.windows.end())
		{
			dockToNode = ctx->dockingState.windows[dockTo]->dockNode;
		}

		wnd = createWindow(id, dockToNode, dockType, title, initialRect, 0);
		wnd->id = id;
	}
	else
	{
		wnd = ctx->dockingState.windows[id];
	}

	ctx->currentWindow = wnd;
	ctx->hoveringThisWindow = isMouseOverWindow();
	ctx->renderer->setOsWindow(wnd->dockNode->osWindow);
	ctx->renderer->beginFrame();
	beginContainer(wnd->clientRect);

	return true;
}

void endWindow()
{
	endContainer();
	auto r = ctx->currentWindow->clientRect;
	r = r.contract(1);
	LineStyle ls;
	ls.color = Color::red;
	ctx->renderer->cmdSetLineStyle(ls);
	ctx->renderer->cmdDrawRectangle(r);
	ctx->renderer->cmdDrawRectangle(ctx->currentWindow->tabRect);
	ctx->renderer->endFrame();
	ctx->currentWindowIndex++;
	//TODO: make scroll struct stack
}

bool isMouseOverWindow()
{
	if (ctx->currentWindow)
	{
		return ctx->currentWindow->dockNode->osWindow == HORUS_INPUT->getHoveredWindow();
	}

	return false;
}

void setCapture()
{
	HORUS_INPUT->setCapture(ctx->currentWindow);
}

void releaseCapture()
{
	HORUS_INPUT->releaseCapture();
}

Rect getWindowClientRect()
{
	return ctx->currentWindow->clientRect;
}	
	
}