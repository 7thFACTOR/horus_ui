#include "layout_cell.h"
#include <string.h>
#include <algorithm>

namespace hui
{
bool UiViewPane::serialize(FILE* file, struct ViewHandler* viewHandler)
{
	size_t tabCount = viewTabs.size();

	fwrite(&tabCount, sizeof(tabCount), 1, file);
	fwrite(&selectedTabIndex, sizeof(selectedTabIndex), 1, file);

	for (auto tab : viewTabs)
	{
		fwrite(&tab->viewId, sizeof(tab->viewId), 1, file);
		size_t len = strlen(tab->title);
		fwrite(&len, sizeof(len), 1, file);
		fwrite(tab->title, len + 1, 1, file);
		fwrite(&tab->userDataId, sizeof(tab->userDataId), 1, file);
		if (viewHandler) viewHandler->onViewPaneTabSave(tab, tab->userDataId, file);
	}

	return true;
}

bool UiViewPane::deserialize(FILE* file, struct ViewHandler* viewHandler)
{
	size_t tabCount = 0;

	fread(&tabCount, sizeof(tabCount), 1, file);
	fread(&selectedTabIndex, sizeof(selectedTabIndex), 1, file);

	const size_t maxTitleSize = 512;
	char title[maxTitleSize] = { 0 };

	for (size_t i = 0; i < tabCount; i++)
	{
		UiViewTab* tab = new UiViewTab();

		tab->parentViewPane = this;
		fread(&tab->viewId, sizeof(tab->viewId), 1, file);
		size_t len = 0;
		fread(&len, sizeof(len), 1, file);
		fread(title, len + 1, 1, file);
		tab->title = new char[len + 1];
		memset(tab->title, 0, len + 1);
		memcpy(tab->title, title, strlen(title));
		fread(&tab->userDataId, sizeof(tab->userDataId), 1, file);
		viewTabs.push_back(tab);

		if (viewHandler) viewHandler->onViewPaneTabLoad(tab, tab->userDataId, file);
	}

	return true;
}

size_t UiViewPane::getViewTabIndex(UiViewTab* viewTab)
{
	for (size_t i = 0; i < viewTabs.size(); i++)
	{
		if (viewTab == viewTabs[i])
		{
			return i;
		}
	}

	return ~0;
}

void UiViewPane::removeViewTab(UiViewTab* viewTab)
{
	auto iter = std::find(viewTabs.begin(), viewTabs.end(), viewTab);

	if (iter != viewTabs.end())
	{
		viewTabs.erase(iter);
	}

	if (selectedTabIndex >= viewTabs.size())
	{
		selectedTabIndex = viewTabs.size() - 1;

		if (selectedTabIndex < 0)
		{
			selectedTabIndex = ~0;
		}
	}
}

UiViewTab* UiViewPane::getSelectedViewTab()
{
	if (selectedTabIndex == ~0)
	{
		return nullptr;
	}

	if (viewTabs.empty())
	{
		return nullptr;
	}

	return viewTabs[selectedTabIndex];
}

void UiViewPane::destroy()
{
	for (auto tab : viewTabs)
	{
		delete[] tab->title;
		delete tab;
	}

	viewTabs.clear();
}

bool UiViewContainer::serialize(FILE* file, struct ViewHandler* viewHandler)
{
	Rect rc = getWindowRect(window);
	i32 isMainWindow = window == getMainWindow();
	WindowState wstate = getWindowState(window);

	fwrite(&isMainWindow, sizeof(isMainWindow), 1, file);
	fwrite(&wstate, sizeof(wstate), 1, file);
	fwrite(&rc.x, sizeof(rc.x), 1, file);
	fwrite(&rc.y, sizeof(rc.y), 1, file);
	fwrite(&rc.width, sizeof(rc.width), 1, file);
	fwrite(&rc.height, sizeof(rc.height), 1, file);

	rootCell->serialize(file, viewHandler);

	return true;
}

bool UiViewContainer::deserialize(FILE* file, struct ViewHandler* viewHandler)
{
	Rect rc;
	i32 isMainWindow = 0;
	WindowState wstate = WindowState::Normal;

	fread(&isMainWindow, sizeof(isMainWindow), 1, file);
	fread(&wstate, sizeof(wstate), 1, file);
	fread(&rc.x, sizeof(rc.x), 1, file);
	fread(&rc.y, sizeof(rc.y), 1, file);
	fread(&rc.width, sizeof(rc.width), 1, file);
	fread(&rc.height, sizeof(rc.height), 1, file);

	if (!isMainWindow)
	{
		window = createWindow(
			"",
			rc.width, rc.height, WindowFlags::Resizable, WindowPositionType::Custom,
			{ rc.x, rc.y }, false);
	}
	else
	{
		window = getMainWindow();
		setWindowRect(window, rc);
	}

	if (wstate == WindowState::Hidden)
	{
		//TODO: maybe not hide ?
		hideWindow(window);
	}
	else if (wstate == WindowState::Maximized)
	{
		maximizeWindow(window);
	}
	else if (wstate == WindowState::Minimized)
	{
		minimizeWindow(window);
	}

	rootCell = new LayoutCell();
	rootCell->deserialize(file, viewHandler);

	return true;
}

bool LayoutCell::serialize(FILE* file, struct ViewHandler* viewHandler)
{
	size_t childCount = children.size();
	i32 hasViewPane = viewPane != nullptr;

	fwrite(&childCount, sizeof(childCount), 1, file);
	fwrite(&tileType, sizeof(tileType), 1, file);
	fwrite(&normalizedSize.x, sizeof(normalizedSize.x), 1, file);
	fwrite(&normalizedSize.y, sizeof(normalizedSize.y), 1, file);
	fwrite(&hasViewPane, sizeof(hasViewPane), 1, file);

	if (viewPane)
	{
		viewPane->serialize(file, viewHandler);
	}

	for (auto child : children)
	{
		child->serialize(file, viewHandler);
	}

	return true;
}

bool LayoutCell::deserialize(FILE* file, struct ViewHandler* viewHandler)
{
	size_t childCount = 0;
	i32 hasViewPane = 0;

	fread(&childCount, sizeof(childCount), 1, file);
	fread(&tileType, sizeof(tileType), 1, file);
	fread(&normalizedSize.x, sizeof(normalizedSize.x), 1, file);
	fread(&normalizedSize.y, sizeof(normalizedSize.y), 1, file);
	fread(&hasViewPane, sizeof(hasViewPane), 1, file);

	if (hasViewPane)
	{
		viewPane = new UiViewPane();
		viewPane->deserialize(file, viewHandler);
	}

	for (size_t i = 0; i < childCount; i++)
	{
		LayoutCell* child = new LayoutCell();

		children.push_back(child);
		child->parent = this;
		child->deserialize(file, viewHandler);
	}

	return true;
}

void LayoutCell::setNewSize(f32 size)
{
	// it must have a parent cell
	if (!parent)
		return;

	auto deltaSize = normalizedSize - size;
	auto& siblings = parent->children;
	auto amountToAdd = deltaSize / (f32)(siblings.size() - 1);

	for (auto& sibling : siblings)
	{
		if (sibling == this)
		{
			continue;
		}

		if (parent->tileType == CellTileType::Horizontal)
			sibling->normalizedSize.x += amountToAdd.x;

		if (parent->tileType == CellTileType::Vertical)
			sibling->normalizedSize.y += amountToAdd.y;
	}

	parent->computeSize();
}

void LayoutCell::computeRect(const Point& startPos)
{
	if (parent)
	{
		rect.x = startPos.x;
		rect.y = startPos.y;
		rect.width = round(normalizedSize.x * parent->rect.width);
		rect.height = round(normalizedSize.y * parent->rect.height);
	}
}

void LayoutCell::computeSize()
{
	switch (tileType)
	{
	case CellTileType::None:
		if (viewPane)
		{
			viewPane->rect = rect;
		}
		break;
	case CellTileType::Horizontal:
		if (!children.empty())
			for (size_t i = 0; i < children.size() - 1; i++)
			{
				if (i == 0)
				{
					children[i]->computeRect({ rect.x, rect.y });
				}

				children[i]->computeSize();

				auto iNext = i + 1;

				if (iNext < children.size())
				{
					children[iNext]->rect.x = children[i]->rect.x + children[i]->rect.width;
					children[iNext]->rect.y = rect.y;
					children[iNext]->rect.width = round(rect.width * children[iNext]->normalizedSize.x);
					children[iNext]->rect.height = rect.height;

					if (iNext == children.size() - 1)
					{
						children[iNext]->computeSize();
					}
				}
			}

		break;
	case CellTileType::Vertical:
		if (!children.empty())
			for (size_t i = 0; i < children.size() - 1; i++)
			{
				if (i == 0)
				{
					children[i]->computeRect({ rect.x, rect.y });
				}

				children[i]->computeSize();

				auto iNext = i + 1;

				if (iNext < children.size())
				{
					children[iNext]->rect.x = rect.x;
					children[iNext]->rect.y = children[i]->rect.y + children[i]->rect.height;
					children[iNext]->rect.width = rect.width;
					children[iNext]->rect.height = rect.height * children[iNext]->normalizedSize.y;

					if (iNext == children.size() - 1)
					{
						children[iNext]->computeSize();
					}
				}
			}
		break;
	}
}

LayoutCell* LayoutCell::findResizeCell(const Point& pt, i32 gripSize)
{
	if (tileType != CellTileType::None)
	{
		if (!rect.contains(pt))
		{
			return nullptr;
		}

		switch (tileType)
		{
		case LayoutCell::CellTileType::Horizontal:
			for (auto cell : children)
			{
				if (cell != children.back())
				{
					if (pt.x >= cell->rect.right() - gripSize / 2
						&& pt.x <= cell->rect.right() + gripSize / 2)
					{
						return cell;
					}
				}
			}
			break;
		case LayoutCell::CellTileType::Vertical:
			for (auto cell : children)
			{
				if (cell != children.back())
				{
					if (pt.y >= cell->rect.bottom() - gripSize / 2
						&& pt.y <= cell->rect.bottom() + gripSize / 2)
					{
						return cell;
					}
				}
			}
			break;
		default:
			break;
		}

		for (auto cell : children)
		{
			auto foundCell = cell->findResizeCell(pt, gripSize);

			if (foundCell)
			{
				return foundCell;
			}
		}
	}

	return nullptr;
}

LayoutCell* LayoutCell::findDockToCell(const Point& pt, DockType& outDockType, Rect& outDockRect, f32 dockBorderSize, f32 tabGroupHeight)
{
	if (tileType == CellTileType::None
		|| (!parent && children.empty()))
	{
		Rect rcLeft = rect;
		Rect rcRight = rect;
		Rect rcTop = rect;
		Rect rcTopTabs = rect;
		Rect rcBottom = rect;

		rcLeft.width = rect.width * dockBorderSize;

		rcRight.width = rect.width * dockBorderSize;
		rcRight.x = rect.x + rect.width - rcRight.width;

		rcTopTabs.height = tabGroupHeight;

		rcTop.y += tabGroupHeight;
		rcTop.height = rect.height * dockBorderSize;

		rcBottom.height = rect.height * dockBorderSize;
		rcBottom.y = rect.y + rect.height - rcBottom.height;

		if (rcLeft.contains(pt))
		{
			outDockType = DockType::Left;
			outDockRect = rcLeft;
			return this;
		}

		if (rcRight.contains(pt))
		{
			outDockType = DockType::Right;
			outDockRect = rcRight;
			return this;
		}

		if (rcTop.contains(pt))
		{
			outDockType = DockType::Top;
			outDockRect = rcTop;
			return this;
		}

		if (rcTopTabs.contains(pt))
		{
			outDockType = DockType::TopAsViewTab;
			outDockRect = rcTopTabs;
			return this;
		}

		if (rcBottom.contains(pt))
		{
			outDockType = DockType::Bottom;
			outDockRect = rcBottom;
			return this;
		}
	}
	else
	{
		for (auto cell : children)
		{
			auto foundCell = cell->findDockToCell(pt, outDockType, outDockRect, dockBorderSize, tabGroupHeight);

			if (foundCell)
			{
				return foundCell;
			}
		}
	}

	return nullptr;
}

void LayoutCell::fillViewTabs(std::vector<UiViewTab*>& tabs)
{
	if (viewPane)
	{
		tabs.insert(tabs.end(), viewPane->viewTabs.begin(), viewPane->viewTabs.end());
	}

	if (tileType != CellTileType::None)
	{
		for (auto cell : children)
		{
			cell->fillViewTabs(tabs);
		}
	}
}

void LayoutCell::fillViewPanes(std::vector<UiViewPane*>& viewPanes)
{
	if (viewPane)
	{
		viewPanes.push_back(viewPane);
		return;
	}

	if (tileType != CellTileType::None)
	{
		for (auto cell : children)
		{
			cell->fillViewPanes(viewPanes);
		}
	}
}

LayoutCell* LayoutCell::findWidestChild(LayoutCell* skipCell)
{
	f32 maxWidth = 0;
	LayoutCell* cell = nullptr;

	for (auto child : children)
	{
		if (skipCell == child)
		{
			continue;
		}

		switch (tileType)
		{
		case CellTileType::Vertical:
			if (child->normalizedSize.y > maxWidth)
			{
				maxWidth = child->normalizedSize.y;
				cell = child;
			}
			break;
		case CellTileType::Horizontal:
			if (child->normalizedSize.x > maxWidth)
			{
				maxWidth = child->normalizedSize.x;
				cell = child;
			}
			break;
		default:
			break;
		}
	}

	return cell;
}

LayoutCell* LayoutCell::dockViewPane(UiViewPane* viewPaneToDock, DockType dock)
{
	LayoutCell* dockedToCell = nullptr;

	// if cell is totally empty, use it
	if (children.empty() && !viewPane)
	{
		viewPane = viewPaneToDock;
		return this;
	}

	bool parentHasManyChildren = false;

	if (parent)
	{
		// if we have more than one child 
		if (parent->children.size() > 1)
		{
			parentHasManyChildren = true;
		}
	}

	// if we have a normal cell
	if (children.empty() && viewPane && !parentHasManyChildren)
	{
		switch (dock)
		{
		case hui::DockType::Left:
		case hui::DockType::Right:
			tileType = CellTileType::Horizontal;
			break;
		case hui::DockType::Top:
		case hui::DockType::Bottom:
			tileType = CellTileType::Vertical;
			break;
		}

		LayoutCell* newCellChild = new LayoutCell();

		// move current stuff in the new cell child
		newCellChild->parent = this;
		newCellChild->viewPane = viewPane;

		switch (tileType)
		{
		case CellTileType::Vertical:
			newCellChild->normalizedSize.x = 1.0f;
			newCellChild->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
			break;
		case CellTileType::Horizontal:
			newCellChild->normalizedSize.x = 1.0f - percentOfNewPaneSplit;
			newCellChild->normalizedSize.y = 1.0f;
			break;
		}

		children.push_back(newCellChild);
		// now its a child bearer
		viewPane = nullptr;
	}

	switch (dock)
	{
	case DockType::Left:
	{
		auto newCell = new LayoutCell();
		CellTileType tile = tileType;

		if (parent)
		{
			tile = parent->tileType;
		}

		if (tile == CellTileType::Horizontal)
		{
			newCell->normalizedSize.x = percentOfNewPaneSplit;
			newCell->normalizedSize.y = 1.0f;
			newCell->viewPane = viewPaneToDock;
			dockedToCell = newCell;

			if (parentHasManyChildren)
			{
				newCell->parent = parent;
				auto iter = std::find(parent->children.begin(), parent->children.end(), this);
				parent->children.insert(iter, newCell);
				auto widestCell = parent->findWidestChild();
				widestCell->normalizedSize.x -= percentOfNewPaneSplit;
			}
			else
			{
				newCell->parent = this;
				auto widestCell = findWidestChild();
				widestCell->normalizedSize.x -= percentOfNewPaneSplit;
				children.insert(children.begin(), newCell);
			}
		}
		else if (tile == CellTileType::Vertical)
		{
			LayoutCell* newCellChild = new LayoutCell();

			// move current stuff in the new cell child
			newCellChild->parent = this;
			newCellChild->normalizedSize.x = 1.0f - percentOfNewPaneSplit;
			newCellChild->viewPane = viewPane;
			newCellChild->children = children;
			newCellChild->tileType = tileType;

			for (auto child : newCellChild->children)
				child->parent = newCellChild;

			children.clear();
			children.push_back(newCellChild);

			newCell->parent = this;
			newCell->normalizedSize.x = percentOfNewPaneSplit;
			newCell->viewPane = viewPaneToDock;
			dockedToCell = newCell;

			children.insert(children.begin(), newCell);

			viewPane = nullptr;
			tileType = CellTileType::Horizontal;
		}

		break;
	}
	case DockType::Right:
	{
		auto newCell = new LayoutCell();
		CellTileType tile = tileType;

		if (parent)
		{
			tile = parent->tileType;
		}

		if (tile == CellTileType::Horizontal)
		{
			newCell->normalizedSize.x = percentOfNewPaneSplit;
			newCell->normalizedSize.y = 1.0f;
			newCell->viewPane = viewPaneToDock;
			dockedToCell = newCell;

			if (parentHasManyChildren)
			{
				newCell->parent = parent;
				auto iter = std::find(parent->children.begin(), parent->children.end(), this);
				parent->children.insert(++iter, newCell);
				auto widestCell = parent->findWidestChild();
				widestCell->normalizedSize.x -= percentOfNewPaneSplit;
			}
			else
			{
				newCell->parent = this;
				auto widestCell = findWidestChild();
				widestCell->normalizedSize.x -= percentOfNewPaneSplit;
				children.push_back(newCell);
			}
		}
		else if (tile == CellTileType::Vertical)
		{
			LayoutCell* newCellChild = new LayoutCell();

			// move current stuff in the new cell child
			newCellChild->parent = this;
			newCellChild->normalizedSize.x = 1.0f - percentOfNewPaneSplit;
			newCellChild->viewPane = viewPane;
			newCellChild->children = children;
			newCellChild->tileType = tileType;

			for (auto child : newCellChild->children)
				child->parent = newCellChild;

			children.clear();
			children.push_back(newCellChild);

			newCell->parent = this;
			newCell->normalizedSize.x = percentOfNewPaneSplit;
			newCell->viewPane = viewPaneToDock;
			dockedToCell = newCell;
			children.push_back(newCell);

			viewPane = nullptr;
			tileType = CellTileType::Horizontal;
		}

		break;
	}

	case DockType::Top:
	{
		auto newCell = new LayoutCell();
		CellTileType tile = tileType;

		if (parent)
		{
			tile = parent->tileType;
		}

		if (tile == CellTileType::Vertical)
		{
			newCell->normalizedSize.x = 1.0f;
			newCell->normalizedSize.y = percentOfNewPaneSplit;
			newCell->viewPane = viewPaneToDock;
			dockedToCell = newCell;

			if (parentHasManyChildren)
			{
				newCell->parent = parent;
				auto iter = std::find(parent->children.begin(), parent->children.end(), this);
				parent->children.insert(iter, newCell);
				auto widestCell = parent->findWidestChild();

				if (widestCell)
					widestCell->normalizedSize.y -= percentOfNewPaneSplit;
			}
			else
			{
				newCell->parent = this;
				auto widestCell = findWidestChild();

				if (widestCell)
					widestCell->normalizedSize.y -= percentOfNewPaneSplit;

				children.push_back(newCell);
			}
		}
		else if (tile == CellTileType::Horizontal)
		{
			LayoutCell* newCellChild = new LayoutCell();

			// move current stuff in the new cell child
			newCellChild->parent = this;
			newCellChild->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
			newCellChild->viewPane = viewPane;
			newCellChild->children = children;
			newCellChild->tileType = tileType;

			for (auto child : newCellChild->children)
				child->parent = newCellChild;

			children.clear();
			children.push_back(newCellChild);

			newCell->parent = this;
			newCell->normalizedSize.y = percentOfNewPaneSplit;
			newCell->viewPane = viewPaneToDock;
			dockedToCell = newCell;
			children.insert(children.begin(), newCell);

			viewPane = nullptr;
			tileType = CellTileType::Vertical;
		}

		break;
	}

	case DockType::Bottom:
	{
		auto newCell = new LayoutCell();
		CellTileType tile = tileType;

		if (parent)
		{
			tile = parent->tileType;
		}

		if (tile == CellTileType::Vertical)
		{
			newCell->normalizedSize.x = 1.0f;
			newCell->normalizedSize.y = percentOfNewPaneSplit;
			newCell->viewPane = viewPaneToDock;
			dockedToCell = newCell;

			if (parentHasManyChildren)
			{
				newCell->parent = parent;
				auto iter = std::find(parent->children.begin(), parent->children.end(), this);
				parent->children.insert(++iter, newCell);
				auto widestCell = parent->findWidestChild();
				if (widestCell)
					widestCell->normalizedSize.y -= percentOfNewPaneSplit;
			}
			else
			{
				newCell->parent = this;
				auto widestCell = findWidestChild();

				if (widestCell)
					widestCell->normalizedSize.y -= percentOfNewPaneSplit;

				children.push_back(newCell);
			}
		}
		else if (tile == CellTileType::Horizontal)
		{
			LayoutCell* newCellChild = new LayoutCell();

			// move current stuff in the new cell child
			newCellChild->parent = this;
			newCellChild->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
			newCellChild->viewPane = viewPane;
			newCellChild->children = children;
			newCellChild->tileType = tileType;

			for (auto child : newCellChild->children)
				child->parent = newCellChild;

			children.clear();
			children.push_back(newCellChild);

			newCell->parent = this;
			newCell->normalizedSize.y = percentOfNewPaneSplit;
			newCell->viewPane = viewPaneToDock;
			dockedToCell = newCell;
			children.push_back(newCell);

			viewPane = nullptr;
			tileType = CellTileType::Vertical;
		}

		break;
	}
	default:
		break;
	}

	fixNormalizedSizes();

	return dockedToCell;
}

void LayoutCell::fixNormalizedSizes()
{
	f64 amount = 0.0f;

	for (auto child : children)
	{
		switch (tileType)
		{
		case CellTileType::Horizontal:
			amount += child->normalizedSize.x;
			break;
		case CellTileType::Vertical:
			amount += child->normalizedSize.y;
			break;
		}
	}

	if (!children.empty())
	{
		auto last = children.back();

		switch (tileType)
		{
		case CellTileType::Horizontal:
			last->normalizedSize.x += 1.0f - amount;
			break;
		case CellTileType::Vertical:
			last->normalizedSize.y += 1.0f - amount;
			break;
		}
	}
}

bool LayoutCell::removeViewPaneCell(UiViewPane* viewPaneToRemove)
{
	if (viewPane == viewPaneToRemove && children.empty())
	{
		if (!parent)
		{
			viewPane = nullptr;
		}
	}

	for (auto child : children)
	{
		if (child->viewPane == viewPaneToRemove)
		{
			auto iter = std::find(children.begin(), children.end(), child);

			// get the widest, beside this
			auto widestCell = findWidestChild(child);

			if (widestCell)
			{
				switch (tileType)
				{
				case CellTileType::Vertical:
					widestCell->normalizedSize.y += child->normalizedSize.y;
					break;
				case CellTileType::Horizontal:
					widestCell->normalizedSize.x += child->normalizedSize.x;
					break;
				default:
					break;
				}
			}

			delete child;

			if (iter != children.end())
			{
				children.erase(iter);
			}

			// if one single cell remains, collapse it to parent
			if (children.size() == 1)
			{
				auto singleChild = children[0];

				viewPane = singleChild->viewPane;
				tileType = singleChild->tileType;
				children = singleChild->children;

				// re-parent
				for (auto child2 : children)
				{
					child2->parent = this;
				}

				delete singleChild;
			}

			return true;
		}
		else
		{
			auto deleted = child->removeViewPaneCell(viewPaneToRemove);

			if (deleted)
			{
				return true;
			}
		}
	}

	return false;
}

LayoutCell* LayoutCell::deleteChildCell(LayoutCell* cell)
{
	for (auto child : children)
	{
		if (child == cell)
		{
			auto iter = std::find(children.begin(), children.end(), child);
			delete cell;
			children.erase(iter);
			return this;
		}
	}

	return nullptr;
}

LayoutCell* LayoutCell::findCellOfViewPane(UiViewPane* viewPaneToFind)
{
	if (viewPane == viewPaneToFind)
	{
		return this;
	}

	for (auto child : children)
	{
		auto foundCell = child->findCellOfViewPane(viewPaneToFind);

		if (foundCell)
		{
			return foundCell;
		}
	}

	return nullptr;
}

void LayoutCell::debug(i32 level)
{
	std::string spaces;

	spaces.resize(level, ' ');
	printf("%sCell: %p\n", spaces.c_str(), this);

	if (viewPane)
	{
		printf("%s  holds pane: %s (%f %f %f %f)\n", spaces.c_str(), viewPane->viewTabs[0]->title, viewPane->rect.x, viewPane->rect.y, viewPane->rect.width, viewPane->rect.height);
	}

	if (!viewPane)
		printf("%s  has %d children\n", spaces.c_str(), children.size());

	for (auto child : children)
	{
		child->debug(level + 5);
	}
}

void LayoutCell::destroy()
{
	for (auto child : children)
	{
		child->destroy();
		delete child;
	}

	children.clear();

	if (viewPane)
	{
		viewPane->destroy();
		delete viewPane;
	}

	viewPane = nullptr;
}

}
