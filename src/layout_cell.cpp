#include "layout_cell.h"
#include <string.h>
#include <algorithm>

namespace hui
{
bool ViewPane::serialize(HFile file, struct ViewHandler* viewHandler)
{
	size_t tabCount = viewTabs.size();

	HORUS_FILE->write(file, &tabCount, sizeof(tabCount));
	HORUS_FILE->write(file, &selectedTabIndex, sizeof(selectedTabIndex));

	for (auto tab : viewTabs)
	{
		HORUS_FILE->write(file, &tab->viewId, sizeof(tab->viewId));
		size_t len = strlen(tab->title);
		HORUS_FILE->write(file, &len, sizeof(len));
		HORUS_FILE->write(file, tab->title, len + 1);
		HORUS_FILE->write(file, &tab->userDataId, sizeof(tab->userDataId));
		
		if (viewHandler) viewHandler->onViewPaneTabSave(tab, tab->userDataId, file);
	}

	return true;
}

bool ViewPane::deserialize(HFile file, struct ViewHandler* viewHandler)
{
	size_t tabCount = 0;

	HORUS_FILE->read(file, &tabCount, sizeof(tabCount));
	HORUS_FILE->read(file, &selectedTabIndex, sizeof(selectedTabIndex));

	const size_t maxTitleSize = 512;
	char title[maxTitleSize] = { 0 };

	for (size_t i = 0; i < tabCount; i++)
	{
		ViewTab* tab = new ViewTab();

		tab->parentViewPane = this;
		HORUS_FILE->read(file, &tab->viewId, sizeof(tab->viewId));
		size_t len = 0;
		HORUS_FILE->read(file, &len, sizeof(len));
		HORUS_FILE->read(file, title, len + 1);
		tab->title = new char[len + 1];
		memset(tab->title, 0, len + 1);
		memcpy(tab->title, title, strlen(title));
		HORUS_FILE->read(file, &tab->userDataId, sizeof(tab->userDataId));
		viewTabs.push_back(tab);

		if (viewHandler) viewHandler->onViewPaneTabLoad(tab, tab->userDataId, file);
	}

	return true;
}

size_t ViewPane::getViewTabIndex(ViewTab* viewTab)
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

void ViewPane::removeViewTab(ViewTab* viewTab)
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

ViewTab* ViewPane::getSelectedViewTab()
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

void ViewPane::destroy()
{
	for (auto tab : viewTabs)
	{
		delete[] tab->title;
		delete tab;
	}

	viewTabs.clear();
}

bool ViewContainer::serialize(HFile file, struct ViewHandler* viewHandler)
{
	Rect rc = getWindowRect(window);
	i32 isMainWindow = window == getMainWindow();
	WindowState wstate = getWindowState(window);

	HORUS_FILE->write(file, &isMainWindow, sizeof(isMainWindow));
	HORUS_FILE->write(file, &wstate, sizeof(wstate));
	HORUS_FILE->write(file, &rc.x, sizeof(rc.x));
	HORUS_FILE->write(file, &rc.y, sizeof(rc.y));
	HORUS_FILE->write(file, &rc.width, sizeof(rc.width));
	HORUS_FILE->write(file, &rc.height, sizeof(rc.height));

	rootCell->serialize(file, viewHandler);

	return true;
}

bool ViewContainer::deserialize(HFile file, struct ViewHandler* viewHandler)
{
	Rect rc;
	i32 isMainWindow = 0;
	WindowState wstate = WindowState::Normal;

	HORUS_FILE->read(file, &isMainWindow, sizeof(isMainWindow));
	HORUS_FILE->read(file, &wstate, sizeof(wstate));
	HORUS_FILE->read(file, &rc.x, sizeof(rc.x));
	HORUS_FILE->read(file, &rc.y, sizeof(rc.y));
	HORUS_FILE->read(file, &rc.width, sizeof(rc.width));
	HORUS_FILE->read(file, &rc.height, sizeof(rc.height));

	if (!isMainWindow)
	{
		window = createWindow(
			"",
			rc.width, rc.height,
			WindowFlags::Resizable | WindowFlags::NoTaskbarButton | WindowFlags::CustomPosition,
			{ rc.x, rc.y });
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

bool LayoutCell::serialize(HFile file, struct ViewHandler* viewHandler)
{
	size_t childCount = children.size();
	i32 hasViewPane = viewPane != nullptr;

	HORUS_FILE->write(file, &childCount, sizeof(childCount));
	HORUS_FILE->write(file, &splitMode, sizeof(splitMode));
	HORUS_FILE->write(file, &normalizedSize.x, sizeof(normalizedSize.x));
	HORUS_FILE->write(file, &normalizedSize.y, sizeof(normalizedSize.y));
	HORUS_FILE->write(file, &hasViewPane, sizeof(hasViewPane));

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

bool LayoutCell::deserialize(HFile file, struct ViewHandler* viewHandler)
{
	size_t childCount = 0;
	i32 hasViewPane = 0;

	HORUS_FILE->read(file, &childCount, sizeof(childCount));
	HORUS_FILE->read(file, &splitMode, sizeof(splitMode));
	HORUS_FILE->read(file, &normalizedSize.x, sizeof(normalizedSize.x));
	HORUS_FILE->read(file, &normalizedSize.y, sizeof(normalizedSize.y));
	HORUS_FILE->read(file, &hasViewPane, sizeof(hasViewPane));

	if (hasViewPane)
	{
		viewPane = new ViewPane();
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

		if (parent->splitMode == CellSplitMode::Horizontal)
			sibling->normalizedSize.x += amountToAdd.x;

		if (parent->splitMode == CellSplitMode::Vertical)
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
	switch (splitMode)
	{
	case CellSplitMode::None:
		if (viewPane)
		{
			viewPane->rect = rect;
		}
		break;
	case CellSplitMode::Horizontal:
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
	case CellSplitMode::Vertical:
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
	if (splitMode != CellSplitMode::None)
	{
		if (!rect.contains(pt))
		{
			return nullptr;
		}

		switch (splitMode)
		{
		case LayoutCell::CellSplitMode::Horizontal:
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
		case LayoutCell::CellSplitMode::Vertical:
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

LayoutCell* LayoutCell::findDockCell(const Point& pt)
{
	if (splitMode == CellSplitMode::None
		|| (!parent && children.empty()))
	{
		if (rect.contains(pt))
			return this;
	}
	else
	{
		for (auto cell : children)
		{
			auto foundCell = cell->findDockCell(pt);

			if (foundCell)
			{
				return foundCell;
			}
		}
	}

	return nullptr;
}

void LayoutCell::gatherViewTabs(std::vector<ViewTab*>& tabs)
{
	if (viewPane)
	{
		tabs.insert(tabs.end(), viewPane->viewTabs.begin(), viewPane->viewTabs.end());
	}

	if (splitMode != CellSplitMode::None)
	{
		for (auto cell : children)
		{
			cell->gatherViewTabs(tabs);
		}
	}
}

void LayoutCell::gatherViewPanes(std::vector<ViewPane*>& viewPanes)
{
	if (viewPane)
	{
		viewPanes.push_back(viewPane);
		return;
	}

	if (splitMode != CellSplitMode::None)
	{
		for (auto cell : children)
		{
			cell->gatherViewPanes(viewPanes);
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

		switch (splitMode)
		{
		case CellSplitMode::Vertical:
			if (child->normalizedSize.y > maxWidth)
			{
				maxWidth = child->normalizedSize.y;
				cell = child;
			}
			break;
		case CellSplitMode::Horizontal:
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

LayoutCell* LayoutCell::dockViewPane(ViewPane* viewPaneToDock, DockType dock)
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
		case hui::DockType::RootLeft:
		case hui::DockType::RootRight:
			splitMode = CellSplitMode::Horizontal;
			break;
		case hui::DockType::Top:
		case hui::DockType::Bottom:
		case hui::DockType::RootTop:
		case hui::DockType::RootBottom:
			splitMode = CellSplitMode::Vertical;
			break;
		}

		LayoutCell* newCellChild = new LayoutCell();

		// move current stuff in the new cell child
		newCellChild->parent = this;
		newCellChild->viewPane = viewPane;

		switch (splitMode)
		{
		case CellSplitMode::Vertical:
			newCellChild->normalizedSize.x = 1.0f;
			newCellChild->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
			break;
		case CellSplitMode::Horizontal:
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
	case DockType::RootLeft:
	{
		auto newCell = new LayoutCell();
		CellSplitMode split = splitMode;

		if (parent)
		{
			split = parent->splitMode;
		}

		if (split == CellSplitMode::Horizontal)
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
		else if (split == CellSplitMode::Vertical)
		{
			LayoutCell* newCellChild = new LayoutCell();

			// move current stuff in the new cell child
			newCellChild->parent = this;
			newCellChild->normalizedSize.x = 1.0f - percentOfNewPaneSplit;
			newCellChild->viewPane = viewPane;
			newCellChild->children = children;
			newCellChild->splitMode = splitMode;

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
			splitMode = CellSplitMode::Horizontal;
		}

		break;
	}
	case DockType::Right:
	case DockType::RootRight:
	{
		auto newCell = new LayoutCell();
		CellSplitMode split = splitMode;

		if (parent)
		{
			split = parent->splitMode;
		}

		if (split == CellSplitMode::Horizontal)
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
		else if (split == CellSplitMode::Vertical)
		{
			LayoutCell* newCellChild = new LayoutCell();

			// move current stuff in the new cell child
			newCellChild->parent = this;
			newCellChild->normalizedSize.x = 1.0f - percentOfNewPaneSplit;
			newCellChild->viewPane = viewPane;
			newCellChild->children = children;
			newCellChild->splitMode = splitMode;

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
			splitMode = CellSplitMode::Horizontal;
		}

		break;
	}

	case DockType::Top:
	case DockType::RootTop:
	{
		auto newCell = new LayoutCell();
		CellSplitMode split = splitMode;

		if (parent)
		{
			split = parent->splitMode;
		}

		if (split == CellSplitMode::Vertical)
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
		else if (split == CellSplitMode::Horizontal)
		{
			LayoutCell* newCellChild = new LayoutCell();

			// move current stuff in the new cell child
			newCellChild->parent = this;
			newCellChild->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
			newCellChild->viewPane = viewPane;
			newCellChild->children = children;
			newCellChild->splitMode = splitMode;

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
			splitMode = CellSplitMode::Vertical;
		}

		break;
	}

	case DockType::Bottom:
	case DockType::RootBottom:
	{
		auto newCell = new LayoutCell();
		CellSplitMode split = splitMode;

		if (parent)
		{
			split = parent->splitMode;
		}

		if (split == CellSplitMode::Vertical)
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
		else if (split == CellSplitMode::Horizontal)
		{
			LayoutCell* newCellChild = new LayoutCell();

			// move current stuff in the new cell child
			newCellChild->parent = this;
			newCellChild->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
			newCellChild->viewPane = viewPane;
			newCellChild->children = children;
			newCellChild->splitMode = splitMode;

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
			splitMode = CellSplitMode::Vertical;
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
		switch (splitMode)
		{
		case CellSplitMode::Horizontal:
			amount += child->normalizedSize.x;
			break;
		case CellSplitMode::Vertical:
			amount += child->normalizedSize.y;
			break;
		}
	}

	if (!children.empty())
	{
		auto last = children.back();

		switch (splitMode)
		{
		case CellSplitMode::Horizontal:
			last->normalizedSize.x += 1.0f - amount;
			break;
		case CellSplitMode::Vertical:
			last->normalizedSize.y += 1.0f - amount;
			break;
		}
	}
}

bool LayoutCell::removeViewPaneCell(ViewPane* viewPaneToRemove)
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
				switch (splitMode)
				{
				case CellSplitMode::Vertical:
					widestCell->normalizedSize.y += child->normalizedSize.y;
					break;
				case CellSplitMode::Horizontal:
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
				splitMode = singleChild->splitMode;
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

LayoutCell* LayoutCell::findCellOfViewPane(ViewPane* viewPaneToFind)
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
