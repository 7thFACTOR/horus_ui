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

	if (crumbLink("/"))
	{
		if (state.currentPath != "/")
			customFileDialogNavigate(state, "/");
	}

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

		sameLine();
		hui::label(">");

		sameLine();

		if (crumbLink(segment.c_str()))
		{
			if (target != state.currentPath)
				customFileDialogNavigate(state, target);
		}

		if (slash == std::string::npos)
			break;

		accumulated = target;
		start = slash + 1;
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
	void* previewUserData)
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
			popupWidth += ctx->settings.customFileDialogPreviewWidth + ctx->sameLine.spacing;
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

			sameLine();

			customFileDialogBreadcrumbs(state);

			space();

			// keep Go inside the popup: size the path input to the row minus the button
			auto& goBtnElem = ctx->theme->getElement(WidgetElementId::ButtonBody).normalState();
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
				widgetSetNextWidth(ctx->settings.customFileDialogPreviewWidth);
				Rect previewRect = customWidgetBegin("##cfdpPreview", listHeight);
				
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