#include <algorithm>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "context.h"
#include "theme.h"
#include "font.h"
#include "util.h"

namespace hui
{
static std::string customFileDialogJoinPath(const std::string& path, const std::string& name)
{
	if (path.empty())
		return name;

	std::string result = path;

	if (result.back() != '/' && result.back() != '\\')
		result += '/';

	result += name;

	return result;
}

static std::string customFileDialogParentPath(const std::string& path)
{
	if (path.empty())
		return path;

	size_t end = path.size();

	while (end > 1 && (path[end - 1] == '/' || path[end - 1] == '\\'))
		end--;

	size_t lastSeparator = std::string::npos;

	for (size_t i = end; i > 0; i--)
	{
		if (path[i - 1] == '/' || path[i - 1] == '\\')
		{
			lastSeparator = i - 1;
			break;
		}
	}

	if (lastSeparator == std::string::npos)
		return "/";

	if (lastSeparator == 0)
		return path.substr(0, 1);

	return path.substr(0, lastSeparator);
}

static bool customFileDialogNameLess(const std::string& a, const std::string& b)
{
	size_t count = std::min(a.size(), b.size());

	for (size_t i = 0; i < count; i++)
	{
		char ca = (char)tolower((unsigned char)a[i]);
		char cb = (char)tolower((unsigned char)b[i]);

		if (ca != cb)
			return ca < cb;
	}

	return a.size() < b.size();
}

static std::string customFileDialogNormalizePath(const std::string& input, const std::string& currentPath)
{
	size_t start = input.find_first_not_of(" \t\r\n");

	if (start == std::string::npos)
		return "";

	size_t end = input.find_last_not_of(" \t\r\n");
	std::string text = input.substr(start, end - start + 1);

	while (text.size() > 1 && (text.back() == '/' || text.back() == '\\'))
		text.pop_back();

	if (text.front() != '/' && text.front() != '\\')
	{
		if (currentPath == "/")
			text = "/" + text;
		else
			text = customFileDialogJoinPath(currentPath, text);
	}

	return text;
}

static void customFileDialogRelist(CustomFileDialogState& state, CustomFileDialogListCallback listCallback, void* userData, bool foldersOnly)
{
	state.entries.clear();
	state.selectedIndex = -1;

	if (listCallback)
	{
		listCallback(state.currentPath.c_str(), state.entries, userData);
	}

	if (foldersOnly)
	{
		state.entries.erase(
			std::remove_if(state.entries.begin(), state.entries.end(), [](const CustomFileDialogEntry& entry) { return !entry.isDirectory; }),
			state.entries.end());
	}

	// folders first, then case-insensitive names
	std::stable_sort(
		state.entries.begin(),
		state.entries.end(),
		[](const CustomFileDialogEntry& a, const CustomFileDialogEntry& b)
		{
			if (a.isDirectory != b.isDirectory)
				return a.isDirectory;

			return customFileDialogNameLess(a.name, b.name);
		});

	state.needsListing = false;
}

static void customFileDialogNavigate(CustomFileDialogState& state, const std::string& path)
{
	state.currentPath = path.empty() ? "/" : path;
	state.selectedIndex = -1;
	state.scrollPos.y = 0;
	state.needsListing = true;
	snprintf(state.pathInput, sizeof(state.pathInput), "%s", state.currentPath.c_str());
}

static void customFileDialogBreadcrumbs(CustomFileDialogState& state)
{
	Color linkColor = ctx->theme->getElement(WidgetElementId::LabelBody).normalState().textColor;

	auto crumbLink = [&](const char* label)
	{
		tintPush(linkColor, TintColorType::Text);
		bool clicked = hui::link(label);
		tintPop();

		return clicked;
	};

	auto& linkElem = ctx->theme->getElement(WidgetElementId::LinkBody).normalState();
	auto& labelElem = ctx->theme->getElement(WidgetElementId::LabelBody).normalState();
	Font* linkFont = linkElem.font;
	Font* labelFont = labelElem.font;

	auto measure = [&](Font* font, const char* text) -> f32
	{
		return font ? font->computeTextSize(text).width : 0.0f;
	};

	// build the crumb list: the root plus one crumb per path segment
	struct Crumb
	{
		std::string name;
		std::string target;
		f32 width;
	};

	std::vector<Crumb> crumbs;
	crumbs.push_back({ "/", "/", measure(linkFont, "/") });

	std::string remainder = state.currentPath;

	if (remainder.size() > 1 && (remainder.front() == '/' || remainder.front() == '\\'))
		remainder.erase(0, 1);

	std::string accumulated = "/";
	size_t start = 0;

	while (start < remainder.size())
	{
		size_t slash = remainder.find('/', start);
		std::string segment = (slash == std::string::npos)
			? remainder.substr(start)
			: remainder.substr(start, slash - start);
		std::string target = (slash == std::string::npos)
			? state.currentPath
			: customFileDialogJoinPath(accumulated, segment);

		Crumb crumb;
		crumb.name = segment;
		crumb.target = target;
		crumb.width = measure(linkFont, segment.c_str());
		crumbs.push_back(crumb);

		if (slash == std::string::npos)
			break;

		accumulated = target;
		start = slash + 1;
	}

	f32 separatorWidth = measure(labelFont, ">");
	f32 ellipsisWidth = measure(labelFont, "...");
	f32 spacing = ctx->sameLine.spacing * ctx->scale;

	// width already consumed on this row by the Back/New folder buttons: the same-line
	// cursor advanced past them, but a trailing normal-mode widget only leaves lastLineWidth
	f32 consumedRow = ctx->position.x - ctx->layout.savedPosition.x;

	if (!ctx->sameLine.wasEnabled)
		consumedRow = ctx->sameLine.lastLineWidth + ctx->sameLine.nextSpacing * ctx->scale;

	f32 available = ctx->layout.width - consumedRow;
	size_t lastIndex = crumbs.size() - 1;

	// an item drawn after another on the same line consumes leading and trailing gaps
	auto itemCost = [&](f32 width) -> f32
	{
		return separatorWidth + width + 2.0f * spacing;
	};

	// indices of crumbs that fit on the line, and whether the tail is elided
	std::vector<size_t> visible;
	visible.push_back(0);
	bool showEllipsis = false;
	bool showLast = false;
	f32 used = crumbs[0].width;
	f32 totalWidth = used;

	for (size_t i = 1; i < crumbs.size(); i++)
		totalWidth += itemCost(crumbs[i].width);

	if (totalWidth <= available)
	{
		for (size_t i = 1; i < crumbs.size(); i++)
			visible.push_back(i);
	}
	else if (lastIndex >= 1)
	{
		// keep as many leading crumbs as fit while reserving room for "..." + the last crumb
		for (size_t i = 1; i < lastIndex; i++)
		{
			f32 need = itemCost(crumbs[i].width);
			f32 reserve = itemCost(ellipsisWidth) + itemCost(crumbs[lastIndex].width);

			if (used + need + reserve <= available)
			{
				used += need;
				visible.push_back(i);
			}
			else
			{
				break;
			}
		}

		f32 ellipsisCost = itemCost(ellipsisWidth);
		f32 lastCost = itemCost(crumbs[lastIndex].width);

		if (visible.back() != lastIndex && used + ellipsisCost + lastCost <= available)
		{
			showEllipsis = true;
			used += ellipsisCost + lastCost;
		}
		else if (visible.back() != lastIndex && used + lastCost <= available)
		{
			showLast = true;
		}
	}

	if (crumbLink("/"))
	{
		if (state.currentPath != "/")
			customFileDialogNavigate(state, "/");
	}

	for (size_t i = 1; i < visible.size(); i++)
	{
		size_t idx = visible[i];

		sameLine();
		hui::label(">");

		sameLine();

		if (crumbLink(crumbs[idx].name.c_str()))
		{
			if (crumbs[idx].target != state.currentPath)
				customFileDialogNavigate(state, crumbs[idx].target);
		}
	}

	if (showEllipsis)
	{
		sameLine();
		hui::label(">");

		sameLine();
		hui::label("...");

		sameLine();
		hui::label(">");

		sameLine();

		if (crumbLink(crumbs[lastIndex].name.c_str()))
		{
			if (crumbs[lastIndex].target != state.currentPath)
				customFileDialogNavigate(state, crumbs[lastIndex].target);
		}
	}
	else if (showLast)
	{
		sameLine();
		hui::label(">");

		sameLine();

		if (crumbLink(crumbs[lastIndex].name.c_str()))
		{
			if (crumbs[lastIndex].target != state.currentPath)
				customFileDialogNavigate(state, crumbs[lastIndex].target);
		}
	}
}

bool customFileDialog(
	const char* id,
	CustomFileDialogListCallback listCallback,
	void* userData,
	char* outResult,
	u32 resultBufferSize,
	CustomFileDialogFlags flags,
	CustomFileDialogPreviewCallback previewCallback,
	void* previewUserData,
	CustomFileDialogCreateFolderCallback createFolderCallback,
	void* createFolderUserData)
{
	ctx->setLabelAndId(id);
	WidgetId dialogId = ctx->id;

	auto& state = ctx->customFileDialogs[dialogId];
	state.lastUsedFrame = ctx->frameCount;

	std::string openLabel = state.result.empty()
		? std::string(has(flags, CustomFileDialogFlags::PickFolder) ? "Choose folder..."
			: has(flags, CustomFileDialogFlags::SaveFile) ? "Choose save path..."
			: "Choose file...")
		: state.result;

	if (hui::button(openLabel.c_str()))
	{
		if (!state.open)
		{
			state.open = true;

			if (state.currentPath.empty())
				customFileDialogNavigate(state, "/");
		}
	}

	bool resultChosen = false;

	if (state.open)
	{
		std::string popupId = "##cfd" + std::to_string(dialogId);
		f32 popupWidth = ctx->settings.customFileDialogWidth;

		if (previewCallback)
		{
			// the popup content area is inset by the popup body border on both sides,
			// so add it back, otherwise the list + preview stick out on the right
			f32 popupBorder = (f32)ctx->theme->getElement(WidgetElementId::PopupBody).normalState().border;
			popupWidth += ctx->settings.customFileDialogPreviewWidth + ctx->sameLine.spacing + popupBorder * 2.0f;
		}

		popupBegin(popupId.c_str(), popupWidth, PopupFlags::FadeBackground | PopupFlags::Centered);

		idPush((void*)dialogId);

		if (popupMustClose())
		{
			popupClose();
			state.open = false;
		}
		else
		{
			auto commit = [&](const std::string& chosenPath)
			{
				state.result = chosenPath;

				if (outResult && resultBufferSize > 0)
					snprintf(outResult, resultBufferSize, "%s", chosenPath.c_str());

				resultChosen = true;
				popupClose();
				state.open = false;
			};

			if (state.needsListing)
				customFileDialogRelist(state, listCallback, userData, has(flags, CustomFileDialogFlags::PickFolder));

			// navigation header
			if (hui::button("< Back##backBtn"))
			{
				std::string parent = customFileDialogParentPath(state.currentPath);

				if (parent != state.currentPath)
					customFileDialogNavigate(state, parent);
			}

			bool focusNewFolderInput = false;

			if (createFolderCallback)
			{
				sameLine();
				if (hui::button("New folder"))
				{
					state.creatingFolder = !state.creatingFolder;

					if (state.creatingFolder)
						focusNewFolderInput = true;
					else
						memset(state.newFolderName, 0, sizeof(state.newFolderName));
				}
			}

			sameLine();

			customFileDialogBreadcrumbs(state);

			space();

			auto& goBtnElem = ctx->theme->getElement(WidgetElementId::ButtonBody).normalState();

			// "New folder" editor: a name input + Create/Cancel that calls the create callback
			if (state.creatingFolder && createFolderCallback)
			{
				f32 createButtonWidthPx = (goBtnElem.border * 2.0f + goBtnElem.font->computeTextSize("Create").width) * ctx->scale;
				f32 cancelButtonWidthPx = (goBtnElem.border * 2.0f + goBtnElem.font->computeTextSize("Cancel").width) * ctx->scale;
				f32 folderInputWidthPx = ctx->layout.width - createButtonWidthPx - cancelButtonWidthPx - ctx->sameLine.spacing * 2.0f * ctx->scale;
				widgetSetNextWidth(std::max(folderInputWidthPx, 1.0f) / ctx->scale);
				WidgetId newFolderInputId = genId("##cfdpNewFolder");

				bool enterOnNewFolder = ctx->event.type == InputEvent::Type::Key
					&& ctx->event.key.down
					&& ctx->event.key.code == KeyCode::Enter
					&& ctx->textInput.id == newFolderInputId;
				hui::textInput("##cfdpNewFolder", state.newFolderName, sizeof(state.newFolderName));

				// make the field the active editor as soon as the editor appears
				if (focusNewFolderInput)
				{
					focusNewFolderInput = false;
					ctx->widget.focusedId = newFolderInputId;
					ctx->textInput.id = newFolderInputId;
					ctx->textInput.editNow = true;
					ctx->textInput.selectAllOnFocus = true;
					ctx->textInput.selectionActive = false;
					ctx->textInput.selectingWithMouse = false;
					ctx->textInput.mouseDown = false;
					// the editor was forced after the input was drawn, so initialize its
					// buffer and caret state the way textInput would have on focus
					ctx->textInput.scrollOffset = 0;
					ctx->textInput.caretPosition = 0;
					ctx->textInput.selectionBegin = 0;
					ctx->textInput.selectionEnd = 0;
					ctx->settings.services.utf8To32(state.newFolderName, ctx->textInput.text);
					forceRepaint();
				}

				sameLine();

				bool createRequested = hui::button("Create");

				sameLine();

				bool cancelNewFolder = hui::button("Cancel##cfdpNewFolder");

				if (cancelNewFolder)
				{
					state.creatingFolder = false;
					memset(state.newFolderName, 0, sizeof(state.newFolderName));
				}
				else if (createRequested || enterOnNewFolder)
				{
					std::string input = state.newFolderName;
					size_t trimStart = input.find_first_not_of(" \t\r\n");
					size_t trimEnd = input.find_last_not_of(" \t\r\n");
					std::string folderName = (trimStart == std::string::npos)
						? std::string()
						: input.substr(trimStart, trimEnd - trimStart + 1);

					if (!folderName.empty()
						&& createFolderCallback(state.currentPath.c_str(), folderName.c_str(), createFolderUserData))
					{
						customFileDialogNavigate(state, customFileDialogJoinPath(state.currentPath, folderName));
						state.creatingFolder = false;
						memset(state.newFolderName, 0, sizeof(state.newFolderName));
					}
				}

				// end the same-line row the way addWidget would: the next row starts below
				ctx->sameLine.enabled = false;
				ctx->sameLine.wasEnabled = false;
				ctx->position.y += ctx->sameLine.maxHeight + ctx->spacing * ctx->scale;
				ctx->sameLine.maxHeight = 0;
				ctx->position.x = ctx->layout.savedPosition.x;

				space();
			}

			// keep Go inside the popup: size the path input to the row minus the button
			f32 goButtonWidthPx = (goBtnElem.border * 2.0f + goBtnElem.font->computeTextSize("Go").width) * ctx->scale;
			f32 pathInputWidthPx = ctx->layout.width - goButtonWidthPx - ctx->sameLine.spacing * ctx->scale;
			widgetSetNextWidth(std::max(pathInputWidthPx, 1.0f) / ctx->scale);
			WidgetId pathInputId = genId("##cfdpPath");
			bool enterPressedOnPath = ctx->event.type == InputEvent::Type::Key
				&& ctx->event.key.down
				&& ctx->event.key.code == KeyCode::Enter
				&& ctx->textInput.id == pathInputId;
			hui::textInput("##cfdpPath", state.pathInput, sizeof(state.pathInput));

			sameLine();

			bool goRequested = hui::button("Go");

			if (goRequested || enterPressedOnPath)
			{
				std::string target = customFileDialogNormalizePath(state.pathInput, state.currentPath);

				if (!target.empty() && target != state.currentPath)
					customFileDialogNavigate(state, target);
			}

			// end the same-line row the way addWidget would: the scroll view reads
			// ctx->position directly, so step below the row and back to the line start
			ctx->sameLine.enabled = false;
			ctx->sameLine.wasEnabled = false;
			ctx->position.y += ctx->sameLine.maxHeight + ctx->spacing * ctx->scale;
			ctx->sameLine.maxHeight = 0;
			ctx->position.x = ctx->layout.savedPosition.x;

			space();

			// entries list
			auto& selectableBodyElem = ctx->theme->getElement(WidgetElementId::SelectableBody).normalState();
			Font* entriesFont = selectableBodyElem.font;

			if (!entriesFont)
				entriesFont = (Font*)hui::themeFontGetFromTheme(ctx->theme, "normal");

			Font* folderFont = (Font*)hui::themeFontGetFromTheme(ctx->theme, "normal-bold");

			if (!folderFont)
				folderFont = entriesFont;

			f32 itemHeight = std::max(
				selectableBodyElem.height,
				entriesFont ? entriesFont->getMetrics().height / ctx->scale : 16.0f) * ctx->scale;
			f32 totalListHeight = itemHeight * (f32)state.entries.size();
			f32 listHeight = ctx->settings.customFileDialogEntriesHeight * ctx->scale;

			if (ctx->settings.scaleScrollViewHeight)
				listHeight /= ctx->scale;

			if (previewCallback)
			{
				f32 listWidth = ctx->settings.customFileDialogWidth;
				widgetSetNextWidth(listWidth);
			}

			// top-left of the entries list; scrollViewEnd() advances ctx->position past
			// the list bottom, so capture it here for the preview placement
			Point entriesListPosition = ctx->position;

			spacingPush(0.0f);
			paddingPush(PaddingType::ScrollView, Point());
			scrollViewBegin(
				"##cfdpEntries",
				listHeight,
				state.scrollPos.y,
				totalListHeight,
				ScrollViewFlags::NoHorizontalScroll | ScrollViewFlags::NoBorder);

			u32 entryCount = (u32)state.entries.size();

			for (u32 i = 0; i < entryCount; i++)
			{
				auto& entry = state.entries[i];
				std::string entryLabel = entry.name;

				if (entry.isDirectory)
					entryLabel += "/";

				SelectableFlags selFlags = (i == (u32)state.selectedIndex)
					? SelectableFlags::Selected
					: SelectableFlags::Normal;
				bool clicked = entry.isDirectory
					? selectableCustomFont(entryLabel.c_str(), folderFont, selFlags)
					: selectable(entryLabel.c_str(), selFlags);

				if (clicked)
				{
					state.selectedIndex = (i32)i;
					bool doubleClicked = ctx->event.mouse.clickCount >= 2;

					if (entry.isDirectory)
					{
						if (doubleClicked)
						{
							customFileDialogNavigate(state, customFileDialogJoinPath(state.currentPath, entry.name));
						}
					}
					else
					{
						if (has(flags, CustomFileDialogFlags::SaveFile))
						{
							snprintf(state.fileName, sizeof(state.fileName), "%s", entry.name.c_str());
						}

						if (doubleClicked)
						{
							commit(customFileDialogJoinPath(state.currentPath, entry.name));
						}
					}
				}
			}

			state.scrollPos = scrollViewEnd();
			paddingPop(PaddingType::ScrollView);
			spacingPop();

			if (previewCallback)
			{
				sameLine();
				// scrollViewEnd()'s layoutPop() restores the sameLine state captured at
				// scrollViewBegin() (the pre-entry-row one), so re-anchor the current
				// position to the entries list top; otherwise the preview lands on the
				// path input row above the list
				ctx->sameLine.currentPosition = entriesListPosition;
				// advance past the full entries list width; the scroll view reports a
				// narrower width when it reserves a vertical scroll bar
				ctx->sameLine.lastLineWidth = ctx->settings.customFileDialogWidth * ctx->scale;
				widgetSetNextWidth(ctx->settings.customFileDialogPreviewWidth);
				Rect previewRect = customWidgetBegin("##cfdpPreview", ctx->settings.customFileDialogEntriesHeight);
				
				std::string previewPath = state.currentPath;
				if (state.selectedIndex >= 0 && state.selectedIndex < (i32)state.entries.size())
				{
					auto& selectedEntry = state.entries[state.selectedIndex];
					previewPath = customFileDialogJoinPath(state.currentPath, selectedEntry.name);
				}

				previewCallback(previewPath.c_str(), previewRect, previewUserData);
				customWidgetEnd();
			}

			if (has(flags, CustomFileDialogFlags::SaveFile))
			{
				space();
				hui::label("File name:");
				WidgetId fileNameInputId = genId("##cfdpFileName");
				bool enterPressedOnFileName = ctx->event.type == InputEvent::Type::Key
					&& ctx->event.key.down
					&& ctx->event.key.code == KeyCode::Enter
					&& ctx->textInput.id == fileNameInputId;
				hui::textInput("##cfdpFileName", state.fileName, sizeof(state.fileName));

				if (enterPressedOnFileName && state.fileName[0])
					commit(customFileDialogJoinPath(state.currentPath, state.fileName));

				space();
			}

			space();
			hui::line();
			space();

			const char* confirmLabel = "Open";

			if (has(flags, CustomFileDialogFlags::PickFolder))
				confirmLabel = "Select";
			else if (has(flags, CustomFileDialogFlags::SaveFile))
				confirmLabel = "Save";

			bool confirmRequested = hui::button(confirmLabel);
			sameLine();
			bool cancelRequested = hui::button("Cancel");

			if (cancelRequested)
			{
				popupClose();
				state.open = false;
			}
			else if (confirmRequested)
			{
				std::string chosenPath;
				bool doCommit = false;

				if (has(flags, CustomFileDialogFlags::PickFolder))
				{
					if (state.selectedIndex >= 0 && state.entries[(size_t)state.selectedIndex].isDirectory)
						chosenPath = customFileDialogJoinPath(state.currentPath, state.entries[(size_t)state.selectedIndex].name);
					else
						chosenPath = state.currentPath;

					doCommit = !chosenPath.empty();
				}
				else if (has(flags, CustomFileDialogFlags::SaveFile))
				{
					// a typed file name wins over the selection
					if (state.fileName[0])
					{
						chosenPath = customFileDialogJoinPath(state.currentPath, state.fileName);
						doCommit = chosenPath != state.currentPath;
					}
					else if (state.selectedIndex >= 0)
					{
						auto& selectedEntry = state.entries[(size_t)state.selectedIndex];

						if (selectedEntry.isDirectory)
							customFileDialogNavigate(state, customFileDialogJoinPath(state.currentPath, selectedEntry.name));
						else
						{
							chosenPath = customFileDialogJoinPath(state.currentPath, selectedEntry.name);
							doCommit = true;
						}
					}
				}
				else if (state.selectedIndex >= 0)
				{
					auto& selectedEntry = state.entries[(size_t)state.selectedIndex];

					if (selectedEntry.isDirectory)
						customFileDialogNavigate(state, customFileDialogJoinPath(state.currentPath, selectedEntry.name));
					else
					{
						chosenPath = customFileDialogJoinPath(state.currentPath, selectedEntry.name);
						doCommit = true;
					}
				}

				if (doCommit)
				{
					commit(chosenPath);
				}
			}
		}

		idPop();
		popupEnd();
	}

	return resultChosen;
}

}