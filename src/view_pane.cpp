#include "view_pane.h"
#include <string.h>
#include <algorithm>

namespace hui
{
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

bool ViewPane::serialize(MemoryStream& stream, struct ViewHandler* viewHandler)
{
	stream.write(&splitMode, sizeof(splitMode));
	stream.write(&normalizedSize.x, sizeof(normalizedSize.x));
	stream.write(&normalizedSize.y, sizeof(normalizedSize.y));

	Rect rc = getWindowRect(window);
	i32 isMainWindow = window == getMainWindow();
	WindowState wstate = getWindowState(window);

	stream.write(&isMainWindow, sizeof(isMainWindow));
	stream.write(&wstate, sizeof(wstate));
	stream.write(&rc.x, sizeof(rc.x));
	stream.write(&rc.y, sizeof(rc.y));
	stream.write(&rc.width, sizeof(rc.width));
	stream.write(&rc.height, sizeof(rc.height));

	size_t tabCount = viewTabs.size();

	stream.write(&tabCount, sizeof(tabCount));
	stream.write(&selectedTabIndex, sizeof(selectedTabIndex));

	for (auto tab : viewTabs)
	{
		stream.write(&tab->viewId, sizeof(tab->viewId));
		size_t len = tab->title.size();
		stream.write(&len, sizeof(size_t));
		stream.write(tab->title.data(), len);
		stream.write(&tab->userData, sizeof(tab->userData));

		if (viewHandler) viewHandler->onViewPaneTabSave(tab, tab->userData);
	}

	size_t childCount = children.size();

	stream.write(&childCount, sizeof(childCount));

	for (auto child : children)
	{
		child->serialize(stream, viewHandler);
	}

	return true;
}

bool ViewPane::deserialize(MemoryStream& stream, struct ViewHandler* viewHandler)
{
	Rect rc;
	i32 isMainWindow = 0;
	WindowState wstate = WindowState::Normal;

	stream.read(&isMainWindow, sizeof(isMainWindow));
	stream.read(&wstate, sizeof(wstate));
	stream.read(&rc.x, sizeof(rc.x));
	stream.read(&rc.y, sizeof(rc.y));
	stream.read(&rc.width, sizeof(rc.width));
	stream.read(&rc.height, sizeof(rc.height));

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

	size_t tabCount = 0;

	stream.read(&tabCount, sizeof(tabCount));
	stream.read(&selectedTabIndex, sizeof(selectedTabIndex));

	for (size_t i = 0; i < tabCount; i++)
	{
		ViewTab* tab = new ViewTab();

		tab->viewPane = this;
		stream.read(&tab->viewId, sizeof(tab->viewId));
		size_t len = 0;
		stream.read(&len, sizeof(len));
		tab->title.resize(len);
		stream.read(&tab->title[0], len);
		stream.read(&tab->userData, sizeof(tab->userData));
		viewTabs.push_back(tab);

		if (viewHandler) viewHandler->onViewPaneTabLoad(tab, tab->userData);
	}

	size_t childCount = 0;

	stream.read(&childCount, sizeof(childCount));
	stream.read(&splitMode, sizeof(splitMode));
	stream.read(&normalizedSize.x, sizeof(normalizedSize.x));
	stream.read(&normalizedSize.y, sizeof(normalizedSize.y));

	for (size_t i = 0; i < childCount; i++)
	{
		ViewPane* child = new ViewPane();

		children.push_back(child);
		child->parent = this;
		child->deserialize(stream, viewHandler);
	}

	return true;
}

void ViewPane::setNewSize(f32 size)
{
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

		if (parent->splitMode == SplitMode::Horizontal)
			sibling->normalizedSize.x += amountToAdd.x;

		if (parent->splitMode == SplitMode::Vertical)
			sibling->normalizedSize.y += amountToAdd.y;
	}

	parent->computeSize();
}

void ViewPane::computeRect(const Point& startPos)
{
	if (parent)
	{
		rect.x = startPos.x;
		rect.y = startPos.y;
		rect.width = round(normalizedSize.x * parent->rect.width);
		rect.height = round(normalizedSize.y * parent->rect.height);
	}
}

void ViewPane::computeSize()
{
	switch (splitMode)
	{
	case SplitMode::None:
		break;
	case SplitMode::Horizontal:
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
	case SplitMode::Vertical:
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

ViewPane* ViewPane::findResizeViewPane(const Point& pt, i32 gripSize)
{
	if (splitMode != SplitMode::None)
	{
		if (!rect.contains(pt))
		{
			return nullptr;
		}

		switch (splitMode)
		{
		case ViewPane::SplitMode::Horizontal:
			for (auto child : children)
			{
				if (child != children.back())
				{
					if (pt.x >= child->rect.right() - gripSize / 2
						&& pt.x <= child->rect.right() + gripSize / 2)
					{
						return child;
					}
				}
			}
			break;
		case ViewPane::SplitMode::Vertical:
			for (auto child : children)
			{
				if (child != children.back())
				{
					if (pt.y >= child->rect.bottom() - gripSize / 2
						&& pt.y <= child->rect.bottom() + gripSize / 2)
					{
						return child;
					}
				}
			}
			break;
		default:
			break;
		}

		for (auto child : children)
		{
			auto foundPane = child->findResizeViewPane(pt, gripSize);

			if (foundPane)
			{
				return foundPane;
			}
		}
	}

	return nullptr;
}

ViewPane* ViewPane::findDockViewPane(const Point& pt)
{
	if (splitMode == SplitMode::None
		|| (!parent && children.empty()))
	{
		if (rect.contains(pt))
			return this;
	}
	else
	{
		for (auto child : children)
		{
			auto foundPane = child->findDockViewPane(pt);

			if (foundPane)
			{
				return foundPane;
			}
		}
	}

	return nullptr;
}

ViewPane* ViewPane::acquireViewTab(ViewTab* viewTab, DockType dock)
{
	auto tabCount = viewTab->viewPane->viewTabs.size();

	// if there is just one tab in the pane, reparent the view pane to this
	if (tabCount == 1)
	{
		if (viewTab->viewPane->parent)
		{
			viewTab->viewPane->parent->removeChild(viewTab->viewPane);
			// grab to self
			viewTab->viewPane->parent = this;
		}

		return viewTab->viewPane;
	}

	// remove tab from the old view pane
	viewTab->viewPane->removeViewTab(viewTab);

	if (dock == DockType::TopAsViewTab)
	{
		viewTabs.push_back(viewTab); // TODO tab ordering
		return this;
	}
	else
	{
		// create a new view pane and add the tab to it
		auto viewPane = new ViewPane();
		viewPane->window = this->window;
		viewPane->viewTabs.push_back(viewTab);
		viewPane->parent = this;

		return viewPane;
	}
}

void ViewPane::gatherViewPanes(std::vector<ViewPane*>& panes)
{
	panes.insert(panes.end(), children.begin(), children.end());

	if (splitMode != SplitMode::None)
	{
		for (auto child : children)
		{
			child->gatherViewPanes(panes);
		}
	}
}

void ViewPane::gatherViewTabs(std::vector<ViewTab*>& tabs)
{
	tabs.insert(tabs.end(), viewTabs.begin(), viewTabs.end());

	if (splitMode != SplitMode::None)
	{
		for (auto child : children)
		{
			child->gatherViewTabs(tabs);
		}
	}
}

ViewPane* ViewPane::findWidestChild(ViewPane* skipChild)
{
	f32 maxWidth = 0;
	ViewPane* widestChild = nullptr;

	for (auto child : children)
	{
		if (skipChild == child)
		{
			continue;
		}

		switch (splitMode)
		{
		case SplitMode::Vertical:
			if (child->normalizedSize.y > maxWidth)
			{
				maxWidth = child->normalizedSize.y;
				widestChild = child;
			}
			break;
		case SplitMode::Horizontal:
			if (child->normalizedSize.x > maxWidth)
			{
				maxWidth = child->normalizedSize.x;
				widestChild = child;
			}
			break;
		}
	}

	return widestChild;
}

//ViewPane* ViewPane::dockViewPane1(ViewPane* viewPaneToDock, DockType dock)
//{
//	ViewPane* dockedToCell = nullptr;
//
//	// if cell is totally empty, use it
//	if (children.empty() && !viewPane)
//	{
//		viewPane = viewPaneToDock;
//		return this;
//	}
//
//	bool parentHasManyChildren = false;
//
//	if (parent)
//	{
//		// if we have more than one child 
//		if (parent->children.size() > 1)
//		{
//			parentHasManyChildren = true;
//		}
//	}
//
//	// if we have a normal cell
//	if (children.empty() && viewPane && !parentHasManyChildren)
//	{
//		switch (dock)
//		{
//		case hui::DockType::Left:
//		case hui::DockType::Right:
//		case hui::DockType::RootLeft:
//		case hui::DockType::RootRight:
//			splitMode = SplitMode::Horizontal;
//			break;
//		case hui::DockType::Top:
//		case hui::DockType::Bottom:
//		case hui::DockType::RootTop:
//		case hui::DockType::RootBottom:
//			splitMode = SplitMode::Vertical;
//			break;
//		}
//
//		ViewPane* newCellChild = new ViewPane();
//
//		// move current stuff in the new cell child
//		newCellChild->parent = this;
//		newCellChild->viewPane = viewPane;
//
//		switch (splitMode)
//		{
//		case SplitMode::Vertical:
//			newCellChild->normalizedSize.x = 1.0f;
//			newCellChild->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
//			break;
//		case SplitMode::Horizontal:
//			newCellChild->normalizedSize.x = 1.0f - percentOfNewPaneSplit;
//			newCellChild->normalizedSize.y = 1.0f;
//			break;
//		}
//
//		children.push_back(newCellChild);
//		// now its a child bearer
//		viewPane = nullptr;
//	}
//
//	switch (dock)
//	{
//	case DockType::Left:
//	case DockType::RootLeft:
//	{
//		auto newCell = new ViewPane();
//		SplitMode split = splitMode;
//
//		if (parent)
//		{
//			split = parent->splitMode;
//		}
//
//		if (split == SplitMode::Horizontal)
//		{
//			newCell->normalizedSize.x = percentOfNewPaneSplit;
//			newCell->normalizedSize.y = 1.0f;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//
//			if (parentHasManyChildren)
//			{
//				newCell->parent = parent;
//				auto iter = std::find(parent->children.begin(), parent->children.end(), this);
//				parent->children.insert(iter, newCell);
//				auto widestCell = parent->findWidestChild();
//				widestCell->normalizedSize.x -= percentOfNewPaneSplit;
//			}
//			else
//			{
//				newCell->parent = this;
//				auto widestCell = findWidestChild();
//				widestCell->normalizedSize.x -= percentOfNewPaneSplit;
//				children.insert(children.begin(), newCell);
//			}
//		}
//		else if (split == SplitMode::Vertical)
//		{
//			ViewPane* newCellChild = new ViewPane();
//
//			// move current stuff in the new cell child
//			newCellChild->parent = this;
//			newCellChild->normalizedSize.x = 1.0f - percentOfNewPaneSplit;
//			newCellChild->viewPane = viewPane;
//			newCellChild->children = children;
//			newCellChild->splitMode = splitMode;
//
//			for (auto child : newCellChild->children)
//				child->parent = newCellChild;
//
//			children.clear();
//			children.push_back(newCellChild);
//
//			newCell->parent = this;
//			newCell->normalizedSize.x = percentOfNewPaneSplit;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//
//			children.insert(children.begin(), newCell);
//
//			viewPane = nullptr;
//			splitMode = SplitMode::Horizontal;
//		}
//
//		break;
//	}
//	case DockType::Right:
//	case DockType::RootRight:
//	{
//		auto newCell = new ViewPane();
//		SplitMode split = splitMode;
//
//		if (parent)
//		{
//			split = parent->splitMode;
//		}
//
//		if (split == SplitMode::Horizontal)
//		{
//			newCell->normalizedSize.x = percentOfNewPaneSplit;
//			newCell->normalizedSize.y = 1.0f;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//
//			if (parentHasManyChildren)
//			{
//				newCell->parent = parent;
//				auto iter = std::find(parent->children.begin(), parent->children.end(), this);
//				parent->children.insert(++iter, newCell);
//				auto widestCell = parent->findWidestChild();
//				widestCell->normalizedSize.x -= percentOfNewPaneSplit;
//			}
//			else
//			{
//				newCell->parent = this;
//				auto widestCell = findWidestChild();
//				widestCell->normalizedSize.x -= percentOfNewPaneSplit;
//				children.push_back(newCell);
//			}
//		}
//		else if (split == SplitMode::Vertical)
//		{
//			ViewPane* newCellChild = new ViewPane();
//
//			// move current stuff in the new cell child
//			newCellChild->parent = this;
//			newCellChild->normalizedSize.x = 1.0f - percentOfNewPaneSplit;
//			newCellChild->viewPane = viewPane;
//			newCellChild->children = children;
//			newCellChild->splitMode = splitMode;
//
//			for (auto child : newCellChild->children)
//				child->parent = newCellChild;
//
//			children.clear();
//			children.push_back(newCellChild);
//
//			newCell->parent = this;
//			newCell->normalizedSize.x = percentOfNewPaneSplit;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//			children.push_back(newCell);
//
//			viewPane = nullptr;
//			splitMode = SplitMode::Horizontal;
//		}
//
//		break;
//	}
//
//	case DockType::Top:
//	case DockType::RootTop:
//	{
//		auto newCell = new ViewPane();
//		SplitMode split = splitMode;
//
//		if (parent)
//		{
//			split = parent->splitMode;
//		}
//
//		if (split == SplitMode::Vertical)
//		{
//			newCell->normalizedSize.x = 1.0f;
//			newCell->normalizedSize.y = percentOfNewPaneSplit;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//
//			if (parentHasManyChildren)
//			{
//				newCell->parent = parent;
//				auto iter = std::find(parent->children.begin(), parent->children.end(), this);
//				parent->children.insert(iter, newCell);
//				auto widestCell = parent->findWidestChild();
//
//				if (widestCell)
//					widestCell->normalizedSize.y -= percentOfNewPaneSplit;
//			}
//			else
//			{
//				newCell->parent = this;
//				auto widestCell = findWidestChild();
//
//				if (widestCell)
//					widestCell->normalizedSize.y -= percentOfNewPaneSplit;
//
//				children.push_back(newCell);
//			}
//		}
//		else if (split == SplitMode::Horizontal)
//		{
//			ViewPane* newCellChild = new ViewPane();
//
//			// move current stuff in the new cell child
//			newCellChild->parent = this;
//			newCellChild->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
//			newCellChild->viewPane = viewPane;
//			newCellChild->children = children;
//			newCellChild->splitMode = splitMode;
//
//			for (auto child : newCellChild->children)
//				child->parent = newCellChild;
//
//			children.clear();
//			children.push_back(newCellChild);
//
//			newCell->parent = this;
//			newCell->normalizedSize.y = percentOfNewPaneSplit;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//			children.insert(children.begin(), newCell);
//
//			viewPane = nullptr;
//			splitMode = SplitMode::Vertical;
//		}
//
//		break;
//	}
//
//	case DockType::Bottom:
//	case DockType::RootBottom:
//	{
//		auto newCell = new ViewPane();
//		SplitMode split = splitMode;
//
//		if (parent)
//		{
//			split = parent->splitMode;
//		}
//
//		if (split == SplitMode::Vertical)
//		{
//			newCell->normalizedSize.x = 1.0f;
//			newCell->normalizedSize.y = percentOfNewPaneSplit;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//
//			if (parentHasManyChildren)
//			{
//				newCell->parent = parent;
//				auto iter = std::find(parent->children.begin(), parent->children.end(), this);
//				parent->children.insert(++iter, newCell);
//				auto widestCell = parent->findWidestChild();
//				if (widestCell)
//					widestCell->normalizedSize.y -= percentOfNewPaneSplit;
//			}
//			else
//			{
//				newCell->parent = this;
//				auto widestCell = findWidestChild();
//
//				if (widestCell)
//					widestCell->normalizedSize.y -= percentOfNewPaneSplit;
//
//				children.push_back(newCell);
//			}
//		}
//		else if (split == SplitMode::Horizontal)
//		{
//			ViewPane* newCellChild = new ViewPane();
//
//			// move current stuff in the new cell child
//			newCellChild->parent = this;
//			newCellChild->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
//			newCellChild->viewPane = viewPane;
//			newCellChild->children = children;
//			newCellChild->splitMode = splitMode;
//
//			for (auto child : newCellChild->children)
//				child->parent = newCellChild;
//
//			children.clear();
//			children.push_back(newCellChild);
//
//			newCell->parent = this;
//			newCell->normalizedSize.y = percentOfNewPaneSplit;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//			children.push_back(newCell);
//
//			viewPane = nullptr;
//			splitMode = SplitMode::Vertical;
//		}
//
//		break;
//	}
//	default:
//		break;
//	}
//
//	fixNormalizedSizes();
//
//	return dockedToCell;
//}
//
//ViewPane* ViewPane::dockViewPane2(ViewPane* viewPaneToDock, DockType dock)
//{
//	ViewPane* dockedToCell = nullptr;
//
//	// if cell is totally empty, use it
//	if (children.empty() && !viewPane)
//	{
//		viewPane = viewPaneToDock;
//		return this;
//	}
//
//	bool parentHasManyChildren = false;
//
//	if (parent)
//	{
//		// if we have more than one child 
//		if (parent->children.size() > 1)
//		{
//			parentHasManyChildren = true;
//		}
//	}
//
//	// if we have a normal cell
//	if (children.empty() && viewPane && !parentHasManyChildren)
//	{
//		switch (dock)
//		{
//		case hui::DockType::Left:
//		case hui::DockType::Right:
//		case hui::DockType::RootLeft:
//		case hui::DockType::RootRight:
//			splitMode = SplitMode::Horizontal;
//			break;
//		case hui::DockType::Top:
//		case hui::DockType::Bottom:
//		case hui::DockType::RootTop:
//		case hui::DockType::RootBottom:
//			splitMode = SplitMode::Vertical;
//			break;
//		}
//
//		ViewPane* newCellChild = new ViewPane();
//
//		// move current stuff in the new cell child
//		newCellChild->parent = this;
//		newCellChild->viewPane = viewPane;
//
//		switch (splitMode)
//		{
//		case SplitMode::Vertical:
//			newCellChild->normalizedSize.x = 1.0f;
//			newCellChild->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
//			break;
//		case SplitMode::Horizontal:
//			newCellChild->normalizedSize.x = 1.0f - percentOfNewPaneSplit;
//			newCellChild->normalizedSize.y = 1.0f;
//			break;
//		}
//
//		children.push_back(newCellChild);
//		// now its a child bearer
//		viewPane = nullptr;
//	}
//
//	switch (dock)
//	{
//	case DockType::Left:
//	case DockType::RootLeft:
//	{
//		auto newCell = new ViewPane();
//		SplitMode split = splitMode;
//
//		if (parent)
//		{
//			split = parent->splitMode;
//		}
//
//		if (split == SplitMode::Horizontal)
//		{
//			newCell->normalizedSize.x = percentOfNewPaneSplit;
//			newCell->normalizedSize.y = 1.0f;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//
//			if (parentHasManyChildren)
//			{
//				newCell->parent = parent;
//				auto iter = std::find(parent->children.begin(), parent->children.end(), this);
//				parent->children.insert(iter, newCell);
//				auto widestCell = parent->findWidestChild();
//				widestCell->normalizedSize.x -= percentOfNewPaneSplit;
//			}
//			else
//			{
//				newCell->parent = this;
//				auto widestCell = findWidestChild();
//				widestCell->normalizedSize.x -= percentOfNewPaneSplit;
//				children.insert(children.begin(), newCell);
//			}
//		}
//		else if (split == SplitMode::Vertical)
//		{
//			ViewPane* newCellChild = new ViewPane();
//
//			// move current stuff in the new cell child
//			newCellChild->parent = this;
//			newCellChild->normalizedSize.x = 1.0f - percentOfNewPaneSplit;
//			newCellChild->viewPane = viewPane;
//			newCellChild->children = children;
//			newCellChild->splitMode = splitMode;
//
//			for (auto child : newCellChild->children)
//				child->parent = newCellChild;
//
//			children.clear();
//			children.push_back(newCellChild);
//
//			newCell->parent = this;
//			newCell->normalizedSize.x = percentOfNewPaneSplit;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//
//			children.insert(children.begin(), newCell);
//
//			viewPane = nullptr;
//			splitMode = SplitMode::Horizontal;
//		}
//
//		break;
//	}
//	case DockType::Right:
//	case DockType::RootRight:
//	{
//		auto newCell = new ViewPane();
//		SplitMode split = splitMode;
//
//		if (parent)
//		{
//			split = parent->splitMode;
//		}
//
//		if (split == SplitMode::Horizontal)
//		{
//			newCell->normalizedSize.x = percentOfNewPaneSplit;
//			newCell->normalizedSize.y = 1.0f;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//
//			if (parentHasManyChildren)
//			{
//				newCell->parent = parent;
//				auto iter = std::find(parent->children.begin(), parent->children.end(), this);
//				parent->children.insert(++iter, newCell);
//				auto widestCell = parent->findWidestChild();
//				widestCell->normalizedSize.x -= percentOfNewPaneSplit;
//			}
//			else
//			{
//				newCell->parent = this;
//				auto widestCell = findWidestChild();
//				widestCell->normalizedSize.x -= percentOfNewPaneSplit;
//				children.push_back(newCell);
//			}
//		}
//		else if (split == SplitMode::Vertical)
//		{
//			ViewPane* newCellChild = new ViewPane();
//
//			// move current stuff in the new cell child
//			newCellChild->parent = this;
//			newCellChild->normalizedSize.x = 1.0f - percentOfNewPaneSplit;
//			newCellChild->viewPane = viewPane;
//			newCellChild->children = children;
//			newCellChild->splitMode = splitMode;
//
//			for (auto child : newCellChild->children)
//				child->parent = newCellChild;
//
//			children.clear();
//			children.push_back(newCellChild);
//
//			newCell->parent = this;
//			newCell->normalizedSize.x = percentOfNewPaneSplit;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//			children.push_back(newCell);
//
//			viewPane = nullptr;
//			splitMode = SplitMode::Horizontal;
//		}
//
//		break;
//	}
//
//	case DockType::Top:
//	case DockType::RootTop:
//	{
//		auto newCell = new ViewPane();
//		SplitMode split = splitMode;
//
//		if (parent)
//		{
//			split = parent->splitMode;
//		}
//
//		if (split == SplitMode::Vertical)
//		{
//			newCell->normalizedSize.x = 1.0f;
//			newCell->normalizedSize.y = percentOfNewPaneSplit;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//
//			if (parentHasManyChildren)
//			{
//				newCell->parent = parent;
//				auto iter = std::find(parent->children.begin(), parent->children.end(), this);
//				parent->children.insert(iter, newCell);
//				auto widestCell = parent->findWidestChild();
//
//				if (widestCell)
//					widestCell->normalizedSize.y -= percentOfNewPaneSplit;
//			}
//			else
//			{
//				newCell->parent = this;
//				auto widestCell = findWidestChild();
//
//				if (widestCell)
//					widestCell->normalizedSize.y -= percentOfNewPaneSplit;
//
//				children.push_back(newCell);
//			}
//		}
//		else if (split == SplitMode::Horizontal)
//		{
//			ViewPane* newCellChild = new ViewPane();
//
//			// move current stuff in the new cell child
//			newCellChild->parent = this;
//			newCellChild->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
//			newCellChild->viewPane = viewPane;
//			newCellChild->children = children;
//			newCellChild->splitMode = splitMode;
//
//			for (auto child : newCellChild->children)
//				child->parent = newCellChild;
//
//			children.clear();
//			children.push_back(newCellChild);
//
//			newCell->parent = this;
//			newCell->normalizedSize.y = percentOfNewPaneSplit;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//			children.insert(children.begin(), newCell);
//
//			viewPane = nullptr;
//			splitMode = SplitMode::Vertical;
//		}
//
//		break;
//	}
//
//	case DockType::Bottom:
//	case DockType::RootBottom:
//	{
//		auto newCell = new ViewPane();
//		SplitMode split = splitMode;
//
//		if (parent)
//		{
//			split = parent->splitMode;
//		}
//
//		if (split == SplitMode::Vertical)
//		{
//			newCell->normalizedSize.x = 1.0f;
//			newCell->normalizedSize.y = percentOfNewPaneSplit;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//
//			if (parentHasManyChildren)
//			{
//				newCell->parent = parent;
//				auto iter = std::find(parent->children.begin(), parent->children.end(), this);
//				parent->children.insert(++iter, newCell);
//				auto widestCell = parent->findWidestChild();
//				if (widestCell)
//					widestCell->normalizedSize.y -= percentOfNewPaneSplit;
//			}
//			else
//			{
//				newCell->parent = this;
//				auto widestCell = findWidestChild();
//
//				if (widestCell)
//					widestCell->normalizedSize.y -= percentOfNewPaneSplit;
//
//				children.push_back(newCell);
//			}
//		}
//		else if (split == SplitMode::Horizontal)
//		{
//			ViewPane* newCellChild = new ViewPane();
//
//			// move current stuff in the new cell child
//			newCellChild->parent = this;
//			newCellChild->normalizedSize.y = 1.0f - percentOfNewPaneSplit;
//			newCellChild->viewPane = viewPane;
//			newCellChild->children = children;
//			newCellChild->splitMode = splitMode;
//
//			for (auto child : newCellChild->children)
//				child->parent = newCellChild;
//
//			children.clear();
//			children.push_back(newCellChild);
//
//			newCell->parent = this;
//			newCell->normalizedSize.y = percentOfNewPaneSplit;
//			newCell->viewPane = viewPaneToDock;
//			dockedToCell = newCell;
//			children.push_back(newCell);
//
//			viewPane = nullptr;
//			splitMode = SplitMode::Vertical;
//		}
//
//		break;
//	}
//	default:
//		break;
//	}
//
//	fixNormalizedSizes();
//
//	return dockedToCell;
//}

void ViewPane::fixNormalizedSizes()
{
	f64 amount = 0.0f;

	for (auto child : children)
	{
		switch (splitMode)
		{
		case SplitMode::Horizontal:
			amount += child->normalizedSize.x;
			break;
		case SplitMode::Vertical:
			amount += child->normalizedSize.y;
			break;
		}
	}

	if (!children.empty())
	{
		auto last = children.back();

		switch (splitMode)
		{
		case SplitMode::Horizontal:
			last->normalizedSize.x += 1.0f - amount;
			break;
		case SplitMode::Vertical:
			last->normalizedSize.y += 1.0f - amount;
			break;
		}
	}
}

bool ViewPane::removeChild(ViewPane* viewPaneToRemove)
{
	for (auto child : children)
	{
		if (child == viewPaneToRemove)
		{
			auto iter = std::find(children.begin(), children.end(), child);

			// get the widest, beside this
			auto widestChild = findWidestChild(child);

			if (widestChild)
			{
				switch (splitMode)
				{
				case SplitMode::Vertical:
					widestChild->normalizedSize.y += child->normalizedSize.y;
					break;
				case SplitMode::Horizontal:
					widestChild->normalizedSize.x += child->normalizedSize.x;
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
			auto deleted = child->removeChild(viewPaneToRemove);

			if (deleted)
			{
				return true;
			}
		}
	}

	return false;
}

ViewPane* ViewPane::deleteChild(ViewPane* child)
{
	auto iter = std::find(children.begin(), children.end(), child);

	if (iter != children.end())
	{
		delete child;
		children.erase(iter);
		return this;
	}

	return nullptr;
}

void ViewPane::debug(i32 level)
{
	std::string spaces;

	spaces.resize(level, ' ');
	printf("%sCell: %p\n", spaces.c_str(), this);

	printf("%s  holds pane: %s (%f %f %f %f)\n", spaces.c_str(), viewTabs[0]->title, rect.x,  rect.y,  rect.width,  rect.height);

	printf("%s  has %d children\n", spaces.c_str(), children.size());

	for (auto child : children)
	{
		child->debug(level + 5);
	}
}

void ViewPane::destroy()
{
	for (auto child : children)
	{
		child->destroy();
		delete child;
	}

	children.clear();

	for (auto tab : viewTabs)
	{
		delete tab;
	}

	viewTabs.clear();
}

}
