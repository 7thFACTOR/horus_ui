#include "horus.h"
#include "types.h"
#include "renderer.h"
#include "unicode_text_cache.h"
#include "font.h"
#include "theme.h"
#include "context.h"
#include "docking_system.h"
#include "docking.h"
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <thread>

#ifdef HORUS_TIMING_DEBUG
#include <ctime>
#include <chrono>
#include <ratio>
#endif
#define GLEW_STATIC
#include <GL/glew.h>

namespace hui
{

void handleDockingMouseDown(const InputEvent& event, DockNode* node)
{
	if (event.mouse.button != MouseButton::Left)
		return;

	auto& ds = ctx->dockingState;
	const Point& mousePos = ctx->mousePosition;

	ds.lastMousePosSinceMouseDown = mousePos;
	ds.lastMousePos = mousePos;
	ds.resizingNode = node->findResizeDockNode(mousePos);
	ds.dragWindow = nullptr;

	for (auto& wnd : ds.windows)
	{
		if (wnd.second->dockNode->osWindow != event.window)
			continue;

		// return if the widget is not visible, that is outside current clip rect
		if (wnd.second->tabRect.outside(wnd.second->dockNode->rect))
		{
			continue;
		}

		Rect clippedRect = wnd.second->tabRect.clipInside(wnd.second->dockNode->rect);

		if (clippedRect.contains(mousePos.x, mousePos.y))
		{
			ds.dragWindow = wnd.second;
			break;
		}
	}
}

void handleDockingMouseUp(const InputEvent& event, DockNode* node)
{
	auto& ds = ctx->dockingState;
	bool moved = fabs(ds.lastMousePosSinceMouseDown.x - ctx->mousePosition.x) > ctx->settings.dragStartDistance || fabs(ds.lastMousePosSinceMouseDown.y - ctx->mousePosition.y) > ctx->settings.dragStartDistance;

	if (ds.dockToNode && moved)
	{
		u32 tabIndex = 0;
		bool dontDock = (ds.dockType == DockType::AsTab && ds.dockToNode == ds.dragWindow->dockNode);

		if (ds.dockType == DockType::AsTab && ds.dockToNode != ds.dragWindow->dockNode)
		{
			tabIndex = ds.dockToNode->dockingTabSpaceIndex;

			if (tabIndex == ~0)
				tabIndex = ds.dockToNode->windows.size();
		}

		ds.dragWindow->dockingNow = false;
		ds.dockToNode->removeTabSpace();

		if (!dontDock)
		if (dockWindow(ds.dragWindow, ds.dockToNode, ds.dockType, tabIndex))
		{
			ds.dragWindow = nullptr;
			ds.dockToNode = nullptr;
		}

		hui::forceRepaint();
	}
	// we undock to a new native window
	else if (ds.dragWindow && moved)
	{
		ds.dragWindow->dockingNow = false;
		ds.dragWindow->dockNode->removeTabSpace();
		
		// undock the window if there is more than one in the dock node
		// and if the dock node is not a root node of the window
		if (ctx->settings.allowUndockingToNewOsWindow)
		{
			auto& rc = ds.dragWindow->dockNode->rect;

			// we use the current screen mouse pos to undock the window to
			Point pt = HORUS_INPUT->getMousePosition();
			// put the window in the middle of the mouse X coordinate
			pt.x -= rc.width / 2.0f;
			undockWindow(ds.dragWindow->id.c_str(), pt);
		}

		hui::forceRepaint();
	}

	if (ds.dragIndicatorOsWindow)
	{
		HORUS_INPUT->destroyWindow(ds.dragIndicatorOsWindow);
		ds.dragIndicatorOsWindow = nullptr;
	}

	ds.dockToNode = nullptr;
	ds.dragWindow = nullptr;
	ds.resizingNode = nullptr;
}

void handleDockNodeResize(DockNode* node)
{
	auto& ds = ctx->dockingState;
	auto& mousePos = ctx->mousePosition;

	if (ds.resizingNode && ds.resizingNode->parent)
	{
		switch (ds.resizingNode->parent->type)
		{
		case DockNode::Type::Vertical:
		{
			f32 pushAmount = 0;

			// moving up
			if (ds.mouseDragDelta.y < 0)
			{
				auto iterPrev = ds.resizingNode->parent->findPrevSiblingOf(ds.resizingNode);

				ds.resizingNode->computeMinSize();

				if (mousePos.y < ds.resizingNode->rect.bottom())
					ds.resizingNode->rect.height += ds.mouseDragDelta.y;

				if (ds.resizingNode->rect.height < ds.resizingNode->minSize.y)
				{
					pushAmount = ds.mouseDragDelta.y;

					// dont push y if first node
					if (ds.resizingNode->parent->children[0] != ds.resizingNode)
						ds.resizingNode->rect.y += pushAmount;

					ds.resizingNode->rect.height = ds.resizingNode->minSize.y;
				}

				while (iterPrev != ds.resizingNode->parent->children.rend())
				{
					(*iterPrev)->computeMinSize();
					auto iterNext = ds.resizingNode->parent->findNextSiblingOf(*iterPrev);

					(*iterPrev)->rect.height = (*iterNext)->rect.y - (*iterPrev)->rect.y - ctx->settings.dockNodeSpacing;

					if (*iterPrev != ds.resizingNode->parent->children[0]
						&& (*iterPrev)->rect.height <= (*iterPrev)->minSize.y)
					{
						(*iterPrev)->rect.y += pushAmount;
						(*iterPrev)->rect.height = (*iterPrev)->minSize.y;
					}
					else
					{
						// if first one, stop all from moving if its min size
						if (*iterPrev == ds.resizingNode->parent->children[0]
							&& (*iterPrev)->rect.height < (*iterPrev)->minSize.y)
						{
							auto iter = ds.resizingNode->parent->children.begin();
							auto crtY = (*iter)->rect.y;
							auto iterLast = ds.resizingNode->parent->getIteratorOf(ds.resizingNode);
							++iterLast; // need to advance to next

							while (true)
							{
								(*iter)->rect.y = crtY;
								(*iter)->rect.height = (*iter)->minSize.y;
								(*iter)->computeRect();
								++iter;
								if (iter == iterLast) break;
								crtY += (*iter)->minSize.y + ctx->settings.dockNodeSpacing;
							};
						}
						(*iterPrev)->computeRect();
						break;
					}

					(*iterPrev)->computeRect();
					++iterPrev;
				}

				// resize node height after the resizing one
				{
					auto iterNext = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);
					auto oldBottom = (*iterNext)->rect.bottom();

					(*iterNext)->rect.y = ds.resizingNode->rect.bottom() + ctx->settings.dockNodeSpacing;
					(*iterNext)->rect.height = oldBottom - (*iterNext)->rect.y;
					(*iterNext)->computeRect();
				}
			}

			// moving down
			if (ds.mouseDragDelta.y > 0)
			{
				auto iterNext = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);

				ds.resizingNode->computeMinSize();

				if (mousePos.y > ds.resizingNode->rect.bottom())
				{
					ds.resizingNode->rect.height += ds.mouseDragDelta.y;
					pushAmount = ds.mouseDragDelta.y;
				}

				auto prevNode = ds.resizingNode;

				while (iterNext != ds.resizingNode->parent->children.end())
				{
					(*iterNext)->computeMinSize();
					auto bottom = (*iterNext)->rect.bottom();
					(*iterNext)->rect.y = prevNode->rect.bottom() + ctx->settings.dockNodeSpacing;
					(*iterNext)->rect.height = bottom - (*iterNext)->rect.y;
					prevNode = (*iterNext);

					if (*iterNext != ds.resizingNode->parent->children.back()
						&& (*iterNext)->rect.height < (*iterNext)->minSize.y)
					{
						(*iterNext)->rect.height = (*iterNext)->minSize.y;
					}
					else
					{
						// if last one, stop all from moving if min size
						if (*iterNext == ds.resizingNode->parent->children.back()
							&& (*iterNext)->rect.height < (*iterNext)->minSize.y)
						{
							auto iter = ds.resizingNode->parent->children.rbegin();
							auto crtY = (*iterNext)->parent->rect.bottom() - (*iterNext)->minSize.y;
							auto iterLast = ds.resizingNode->parent->getReverseIteratorOf(ds.resizingNode);

							while (true)
							{
								(*iter)->computeMinSize();
								(*iter)->rect.y = crtY;
								(*iter)->rect.height = (*iter)->minSize.y;
								(*iter)->computeRect();
								++iter;
								if (iter == iterLast) break;
								crtY -= (*iter)->minSize.y + ctx->settings.dockNodeSpacing;
							};

							auto iterNext2 = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);

							ds.resizingNode->rect.height = (*iterNext2)->rect.y - ds.resizingNode->rect.y - ctx->settings.dockNodeSpacing;

						}

						(*iterNext)->computeRect();
						break;
					}

					(*iterNext)->computeRect();
					++iterNext;
				}
			}

			break;
		}
		case DockNode::Type::Horizontal:
		{
			f32 pushAmount = 0;

			// moving left
			if (ds.mouseDragDelta.x < 0)
			{
				auto iterPrev = ds.resizingNode->parent->findPrevSiblingOf(ds.resizingNode);

				ds.resizingNode->computeMinSize();

				if (mousePos.x < ds.resizingNode->rect.right())
					ds.resizingNode->rect.width += ds.mouseDragDelta.x;

				if (ds.resizingNode->rect.width < ds.resizingNode->minSize.x)
				{
					pushAmount = ds.mouseDragDelta.x;

					// dont push x if first node
					if (ds.resizingNode->parent->children[0] != ds.resizingNode)
						ds.resizingNode->rect.x += pushAmount;

					ds.resizingNode->rect.width = ds.resizingNode->minSize.x;
				}

				while (iterPrev != ds.resizingNode->parent->children.rend())
				{
					(*iterPrev)->computeMinSize();
					auto iterNext = ds.resizingNode->parent->findNextSiblingOf(*iterPrev);

					(*iterPrev)->rect.width = (*iterNext)->rect.x - (*iterPrev)->rect.x - ctx->settings.dockNodeSpacing;

					if (*iterPrev != ds.resizingNode->parent->children[0]
						&& (*iterPrev)->rect.width <= (*iterPrev)->minSize.x)
					{
						(*iterPrev)->rect.x += pushAmount;
						(*iterPrev)->rect.width = (*iterPrev)->minSize.x;
					}
					else
					{
						// if first one, stop all from moving if its min size
						if (*iterPrev == ds.resizingNode->parent->children[0]
							&& (*iterPrev)->rect.width < (*iterPrev)->minSize.x)
						{
							auto iter = ds.resizingNode->parent->children.begin();
							auto crtX = (*iter)->rect.x;
							auto iterLast = ds.resizingNode->parent->getIteratorOf(ds.resizingNode);
							++iterLast; // need to advance to next

							while (true)
							{
								(*iter)->rect.x = crtX;
								(*iter)->rect.width = (*iter)->minSize.x;
								(*iter)->computeRect();
								++iter;
								if (iter == iterLast) break;
								crtX += (*iter)->minSize.x + ctx->settings.dockNodeSpacing;
							};
						}
						(*iterPrev)->computeRect();
						break;
					}

					(*iterPrev)->computeRect();
					++iterPrev;
				}

				// resize node width after the resizing one
				{
					auto iterNext = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);
					auto oldRight = (*iterNext)->rect.right();

					(*iterNext)->rect.x = ds.resizingNode->rect.right() + ctx->settings.dockNodeSpacing;
					(*iterNext)->rect.width = oldRight - (*iterNext)->rect.x;
					(*iterNext)->computeRect();
				}
			}

			// moving right
			if (ds.mouseDragDelta.x > 0)
			{
				auto iterNext = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);

				ds.resizingNode->computeMinSize();

				if (mousePos.x > ds.resizingNode->rect.right())
				{
					ds.resizingNode->rect.width += ds.mouseDragDelta.x;
					pushAmount = ds.mouseDragDelta.x;
				}

				auto prevNode = ds.resizingNode;

				while (iterNext != ds.resizingNode->parent->children.end())
				{
					(*iterNext)->computeMinSize();
					auto right = (*iterNext)->rect.right();
					(*iterNext)->rect.x = prevNode->rect.right() + ctx->settings.dockNodeSpacing;
					(*iterNext)->rect.width = right - (*iterNext)->rect.x;

					prevNode = (*iterNext);

					if (*iterNext != ds.resizingNode->parent->children.back()
						&& (*iterNext)->rect.width < (*iterNext)->minSize.x)
					{
						(*iterNext)->rect.width = (*iterNext)->minSize.x;
					}
					else
					{
						// if last one, stop all from moving if min size
						if (*iterNext == ds.resizingNode->parent->children.back()
							&& (*iterNext)->rect.width < (*iterNext)->minSize.x)
						{
							auto iter = ds.resizingNode->parent->children.rbegin();
							auto crtX = (*iterNext)->parent->rect.right() - (*iterNext)->minSize.x;
							auto iterLast = ds.resizingNode->parent->getReverseIteratorOf(ds.resizingNode);

							while (true)
							{
								(*iter)->computeMinSize();
								(*iter)->rect.x = crtX;
								(*iter)->rect.width = (*iter)->minSize.x;
								(*iter)->computeRect();
								++iter;
								if (iter == iterLast) break;
								crtX -= (*iter)->minSize.x + ctx->settings.dockNodeSpacing;
							};

							auto iterNext2 = ds.resizingNode->parent->findNextSiblingOf(ds.resizingNode);

							ds.resizingNode->rect.width = (*iterNext2)->rect.x - ds.resizingNode->rect.x - ctx->settings.dockNodeSpacing;

						}
						(*iterNext)->computeRect();
						break;
					}

					(*iterNext)->computeRect();
					++iterNext;
				}
			}

			break;
		}
		}

		ds.resizingNode->computeRect();
	}
}

void handleDockingMouseMove(const InputEvent& event, DockNode* node)
{
	auto& ds = ctx->dockingState;
	DockNode* hoveredResizingNode = nullptr;

	if (!ds.dragWindow)
	{
		hoveredResizingNode = node->findResizeDockNode(ctx->mousePosition);
	}

	if (ds.resizingNode || (hoveredResizingNode && hoveredResizingNode->parent))
	{
		DockNode::Type nodeType = DockNode::Type::None;

		if (ds.resizingNode)
			nodeType = ds.resizingNode->parent->type;
		else
			if (hoveredResizingNode) nodeType = hoveredResizingNode->parent->type;

		switch (nodeType)
		{
		case DockNode::Type::Horizontal:
		{
			ctx->mouseCursor = MouseCursorType::SizeWE;
			break;
		}
		case DockNode::Type::Vertical:
		{
			ctx->mouseCursor = MouseCursorType::SizeNS;
			break;
		}
		default:
			break;
		}
	}

	Point mousePos = ctx->mousePosition;

	if (ds.dragWindow)
	{
		auto& tabGroupElem = ctx->theme->getElement(WidgetElementId::TabGroupBody);

		if (ds.dragWindow && ds.draggingStarted)
		{
			ds.dockToNode = nullptr;
			ds.hoveredNode = nullptr;
			ds.dockType = DockType::None;

			if (ctx->lastHoveredOsWindow == node->osWindow)
				ds.hoveredNode = node->findTargetDockNode(mousePos);

			bool isSameNode = ds.hoveredNode == ds.dragWindow->dockNode;
			bool isSingleWindow = ds.dragWindow->dockNode->windows.size() == 1;

			// we've started to drag the window, so prepare objects and state
			if (!ds.dragIndicatorOsWindow)
			{
				ds.dragIndicatorOsWindow = HORUS_INPUT->createWindow(ds.dragWindow->title.c_str(),  OsWindowFlags::NoInput | OsWindowFlags::NoTaskBar|OsWindowFlags::NoDecoration, OsWindowState::Normal, Rect(0, 0, 200, 150));

				ds.dragWindow->dockingNow = true;
			}

			auto rootNode = node;
			auto& rootRect = rootNode->rect;
			auto parentRect = ds.hoveredNode ? ds.hoveredNode->rect : rootRect;
			Rect hitBoxLeft;
			Rect hitBoxRight;
			Rect hitBoxTop;
			Rect hitBoxBottom;
			Rect hitBoxTabs;
			Rect hitBoxRootLeft;
			Rect hitBoxRootRight;
			Rect hitBoxRootTop;
			Rect hitBoxRootBottom;

			// do check hit tests only if this is the hovered window
			if (ds.hoveredNode && ctx->lastHoveredOsWindow == node->osWindow)
			{
				hitBoxLeft = parentRect;
				hitBoxRight = parentRect;
				hitBoxTop = parentRect;
				hitBoxBottom = parentRect;
				hitBoxTabs = parentRect;

				hitBoxLeft.width *= ctx->settings.dockNodeDockingSizeRatio * ctx->settings.dockNodeDockingHitSizeRatio;

				hitBoxRight.x += parentRect.width * (1.0f - ctx->settings.dockNodeDockingSizeRatio * ctx->settings.dockNodeDockingHitSizeRatio);
				hitBoxRight.width *= ctx->settings.dockNodeDockingSizeRatio * ctx->settings.dockNodeDockingHitSizeRatio;

				hitBoxTop.height *= ctx->settings.dockNodeDockingSizeRatio * ctx->settings.dockNodeDockingHitSizeRatio;
					
				hitBoxBottom.y += parentRect.height * (1.0f - ctx->settings.dockNodeDockingSizeRatio * ctx->settings.dockNodeDockingHitSizeRatio);
				hitBoxBottom.height *= ctx->settings.dockNodeDockingSizeRatio * ctx->settings.dockNodeDockingHitSizeRatio;

				hitBoxTabs.height = tabGroupElem.normalState().height * 2.0f;

				hitBoxRootLeft = rootNode->rect;
				hitBoxRootRight = rootNode->rect;
				hitBoxRootTop = rootNode->rect;
				hitBoxRootBottom = rootNode->rect;

				hitBoxRootLeft.width = ctx->settings.dockNodeRootDockingHitSize;

				hitBoxRootRight.x = rootNode->rect.right() - ctx->settings.dockNodeRootDockingHitSize;
				hitBoxRootRight.width = ctx->settings.dockNodeRootDockingHitSize;

				hitBoxRootTop.height = ctx->settings.dockNodeRootDockingHitSize;
				hitBoxRootTop.y -= 40;

				hitBoxRootBottom.y = rootNode->rect.bottom() - ctx->settings.dockNodeRootDockingHitSize;
				hitBoxRootBottom.height = ctx->settings.dockNodeRootDockingHitSize + 40;

				auto isHitBoxLeftHovered = hitBoxLeft.contains(mousePos);
				auto isHitBoxRightHovered = hitBoxRight.contains(mousePos);
				auto isHitBoxTopHovered = hitBoxTop.contains(mousePos);
				auto isHitBoxBottomHovered = hitBoxBottom.contains(mousePos);
				auto isHitBoxTabsHovered = hitBoxTabs.contains(mousePos);

				auto isHitBoxRootLeftHovered = hitBoxRootLeft.contains(mousePos);
				auto isHitBoxRootRightHovered = hitBoxRootRight.contains(mousePos);
				auto isHitBoxRootTopHovered = hitBoxRootTop.contains(mousePos);
				auto isHitBoxRootBottomHovered = hitBoxRootBottom.contains(mousePos);

				if (isHitBoxLeftHovered)
				{
					ds.dockToNode = ds.hoveredNode;
					ds.dockType = DockType::Left;
					ds.draggedRect = parentRect;
					ds.draggedRect.width *= ctx->settings.dockNodeDockingSizeRatio;
				}

				if (isHitBoxRightHovered)
				{
					ds.dockToNode = ds.hoveredNode;
					ds.dockType = DockType::Right;
					ds.draggedRect = parentRect;
					ds.draggedRect.x += ds.draggedRect.width * (1.0f - ctx->settings.dockNodeDockingSizeRatio);
					ds.draggedRect.width *= ctx->settings.dockNodeDockingSizeRatio;
				}

				if (isHitBoxTopHovered)
				{
					ds.dockToNode = ds.hoveredNode;
					ds.dockType = DockType::Top;
					ds.draggedRect = parentRect;
					ds.draggedRect.height *= ctx->settings.dockNodeDockingSizeRatio;
				}

				if (isHitBoxBottomHovered)
				{
					ds.dockToNode = ds.hoveredNode;
					ds.dockType = DockType::Bottom;
					ds.draggedRect = parentRect;
					ds.draggedRect.y += floorf(ds.draggedRect.height * (1.0f - ctx->settings.dockNodeDockingSizeRatio));
					ds.draggedRect.height *= ctx->settings.dockNodeDockingSizeRatio;
				}

				if (isHitBoxRootLeftHovered)
				{
					ds.dockToNode = rootNode;
					ds.dockType = DockType::Left;
					ds.draggedRect = ds.dockToNode->rect;
					ds.draggedRect.width *= ctx->settings.dockNodeDockingSizeRatio;
				}

				if (isHitBoxRootRightHovered)
				{
					ds.dockToNode = rootNode;
					ds.dockType = DockType::Right;
					ds.draggedRect = ds.dockToNode->rect;
					ds.draggedRect.x += ds.draggedRect.width * (1.0f - ctx->settings.dockNodeDockingSizeRatio);
					ds.draggedRect.width *= ctx->settings.dockNodeDockingSizeRatio;
				}

				if (isHitBoxRootTopHovered)
				{
					ds.dockToNode = rootNode;
					ds.dockType = DockType::Top;
					ds.draggedRect = ds.dockToNode->rect;
					ds.draggedRect.height *= ctx->settings.dockNodeDockingSizeRatio;
				}

				if (isHitBoxRootBottomHovered)
				{
					ds.dockToNode = rootNode;
					ds.dockType = DockType::Bottom;
					ds.draggedRect = ds.dockToNode->rect;
					ds.draggedRect.y += floorf(ds.draggedRect.height * (1.0f - ctx->settings.dockNodeDockingSizeRatio));
					ds.draggedRect.height *= ctx->settings.dockNodeDockingSizeRatio;
				}

				if (isHitBoxTabsHovered)
				{
					ds.dockToNode = ds.hoveredNode;
					ds.dockType = DockType::AsTab;
					ds.draggedRect = parentRect;
					ds.draggedRect.x = mousePos.x;
					ds.draggedRect.width = ds.dragWindow->tabRect.width;
					ds.draggedRect.height = tabGroupElem.normalState().height;

					if (isSameNode)
					{
						ds.hoveredNode->moveWindowTabAt(mousePos, ds.dragWindow);
					}
					else
					{
						ds.hoveredNode->insertTabSpaceAt(mousePos, ds.dragWindow->tabRect.width);
					}
				}
			}
		}
	}

	if (ds.resizingNode)
	{
		handleDockNodeResize(node);
	}
}

void handleDockNodeEvents(DockNode* node)
{
	auto& rect = node->rect;
	auto& event = hui::getInputEvent();

	//TODO: find current window index better
	// find if the current window of the view pane had a layer index > 0
	// if so, then we must be having popups or menus
	if (ctx->maxLayerIndex)
	{
		// node is disabled for input
		return;
	}

	printf("hwnd %p\n", ctx->lastHoveredOsWindow);

	// is the event for this window ?
	if (ctx->lastHoveredOsWindow != node->osWindow)
	{
		return;
	}

	switch (event.type)
	{
	case InputEvent::Type::MouseDown: handleDockingMouseDown(event, node); break;
	case InputEvent::Type::MouseUp: handleDockingMouseUp(event, node); break;
	default:
		break;
	};

	handleDockingMouseMove(event, node);
}

}