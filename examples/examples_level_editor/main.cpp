#pragma execution_character_set("utf-8")
#include "horus.h"
#include "../common/material_icon_atlas.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <initializer_list>
#include <fstream>

#include <json/json.h>
#include <json/reader.h>

// backends
#include "sdl3_input.h"
#include "opengl_graphics.h"
#include "dx11_graphics.h"
#include "dx12_graphics.h"
#include "vulkan_graphics.h"
#include "json_theme_loader.h"
#include "json_docking_state.h"
#include "stb_rectpack.h"
#include "freetype_fonts.h"
#include "native_file_dialogs.h"
#include "stdio_fileio.h"
#include "utfcpp.h"
#include "../common/dpi_awareness.h"

// all names loaded from the Material icon atlas, used by the icon browser window
static std::vector<std::string> gMaterialIconNames;

// separate from the asset drag type so tree drags never look like asset drops
static const u32 sceneTreeDragDropType = 101;
static const u32 levelEditorLayoutVersion = 3;

static const f32 treeDragStartDistance = 4.0f;
static const f32 uiScaleSteps[] = { 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };

// a dummy object in the edited scene graph, purely for the example
struct SceneObject
{
	char name[64] = { 0 };
	const char* type = "Entity";
	SceneObject* parent = nullptr;
	std::vector<SceneObject*> children;
	f32 position[3] = { 0, 0, 0 };
	f32 rotation[3] = { 0, 0, 0 };
	f32 scale[3] = { 1, 1, 1 };
	hui::Color color = hui::Color::white;
	bool visible = true;
	bool locked = false;
	bool isStatic = false;
	bool collision = true;
	bool trigger = false;
	bool expanded = true;
	bool transformExpanded = true;
	bool renderingExpanded = true;
	bool physicsExpanded = false;
	i32 meshIndex = 0;
};

// a dummy log line
struct LogEntry
{
	enum class Level
	{
		Info,
		Warning,
		Error
	};

	Level level = Level::Info;
	std::string text;
	bool selected = false;
};

// a dummy asset shown in the asset browser
struct AssetItem
{
	std::string name;
	const char* type = "Texture";
	u32 folder = 0;
	hui::HImage thumb = 0;
};

struct ViewportDragItem
{
	SceneObject* object = nullptr;
	f32 startPos[3] = { 0, 0, 0 };
};

enum class SceneTreeDropMode
{
	None,
	Before,
	After,
	Into
};

struct SceneTreeDropTarget
{
	SceneObject* object = nullptr;
	SceneObject* parent = nullptr;
	i32 index = -1;
	SceneTreeDropMode mode = SceneTreeDropMode::None;
};

// all the level editor state, kept in one place instead of file globals
struct LevelEditorState
{
	std::vector<std::unique_ptr<SceneObject>> objects;
	std::vector<SceneObject*> roots;
	SceneObject* selectedObject = nullptr;
	u32 newObjectCounter = 0;
	hui::Point treeScroll = 0;
	hui::Point inspectorScroll = 0;
	char objectSearch[128] = { 0 };

	std::vector<LogEntry> log;
	hui::Point logScroll = 0;
	bool logShowInfo = true;
	bool logShowWarning = true;
	bool logShowError = true;
	bool logAutoScroll = true;
	f32 logTimer = 0;
	u32 logMessageIndex = 0;
	i32 logSelectionAnchor = -1;

	static const u32 folderCount = 5;
	const char* folders[folderCount] = { "Textures", "Meshes", "Materials", "Audio", "Prefabs" };
	bool folderSelected[folderCount] = { true, false, false, false, false };
	std::vector<AssetItem> assets;
	std::vector<bool> assetSelected;
	u32 selectedFolder = 0;
	i32 assetSelectionAnchor = -1;
	char assetFilter[128] = { 0 };
	hui::Point assetScroll = 0;

	f32 uiScale = hui::scaleGet();
	f32 pendingUiScale = 0;
	i32 toolIndex = 0;
	bool gridVisible = true;
	bool snapEnabled = true;
	bool playing = false;
	f32 viewZoom = 1.0f;
	hui::Point viewPan = 0;
	bool viewportPanning = false;
	hui::Point viewportPanStartMouse = 0;
	hui::Point viewportPanStartValue = 0;
	bool viewportDragging = false;
	std::vector<ViewportDragItem> viewportDragItems;
	hui::Point viewportDragStartMouse = 0;

	bool viewportRectSelecting = false;
	hui::Point viewportRectStart = 0;
	hui::Point viewportRectEnd = 0;
	std::vector<SceneObject*> sceneSelection;

	SceneObject* treeDragObject = nullptr;
	hui::Point treeDragStartMouse = 0;
	bool treeDragging = false;
	std::vector<SceneObject*> treeDragObjects;
	SceneTreeDropTarget treeDropTarget;

	SceneObject* treeRenameObject = nullptr;
	char treeRenameId[64] = { 0 };
	char treeRenameText[64] = { 0 };
	bool treeRenameCancelled = false;
	bool treeRenameJustStarted = false;

	bool showSceneTree = true;
	bool showInspector = true;
	bool showAssetBrowser = true;
	bool showIconBrowser = true;
	bool iconBrowserDocked = false;
	bool showLog = true;
	bool showViewport = true;

	char iconFilter[128] = { 0 };
	hui::Point iconBrowserScroll = 0;

	bool saveLayoutRequested = false;
	bool loadLayoutRequested = false;
	bool resetLayoutRequested = false;

	hui::HImage iconMenu = 0;
	hui::HImage iconToolbar = 0;
	hui::HImage iconTree = 0;
	hui::HImage iconInspector = 0;
	hui::HImage iconViewport = 0;
	hui::HImage iconAssets = 0;
	hui::HImage iconLog = 0;
	hui::HImage iconNew = 0;
	hui::HImage iconOpen = 0;
	hui::HImage iconSave = 0;
	hui::HImage iconUndo = 0;
	hui::HImage iconRedo = 0;
	hui::HImage iconVisible = 0;
	hui::HImage iconLocked = 0;
	hui::HImage iconObject = 0;
	hui::HImage iconArrowClosed = 0;
	hui::HImage iconArrowOpen = 0;
	f32 iconButtonSize = 48;
	f32 toolbarGroupSpacing = 8;
	hui::HImage thumbLena = 0;
	hui::HImage thumbFolder = 0;
	hui::HImage thumbSettings = 0;
	hui::HImage thumbData = 0;
	hui::HImage thumbStacks = 0;
	void* materialRef = nullptr;
	bool materialModified = false;
};

static bool isObjectSelected(const LevelEditorState& state, const SceneObject* obj)
{
	if (state.selectedObject == obj)
		return true;

	for (SceneObject* selected : state.sceneSelection)
	{
		if (selected == obj)
			return true;
	}

	return false;
}

void appendLog(LevelEditorState& state, LogEntry::Level level, const std::string& text);

// true when 'ancestor' is 'obj' or lies above it in the hierarchy
static bool isAncestorOf(SceneObject* ancestor, SceneObject* obj)
{
	for (SceneObject* p = obj; p; p = p->parent)
	{
		if (p == ancestor)
			return true;
	}

	return false;
}

static std::vector<SceneObject*>& sceneChildren(LevelEditorState& state, SceneObject* parent)
{
	return parent ? parent->children : state.roots;
}

static i32 sceneObjectIndex(LevelEditorState& state, SceneObject* obj)
{
	if (!obj)
		return -1;

	std::vector<SceneObject*>& list = sceneChildren(state, obj->parent);
	auto it = std::find(list.begin(), list.end(), obj);

	return it == list.end() ? -1 : (i32)(it - list.begin());
}

static void detachSceneObject(LevelEditorState& state, SceneObject* obj)
{
	if (!obj)
		return;

	if (obj->parent)
	{
		auto& siblings = obj->parent->children;
		siblings.erase(std::remove(siblings.begin(), siblings.end(), obj), siblings.end());
	}
	else
	{
		state.roots.erase(std::remove(state.roots.begin(), state.roots.end(), obj), state.roots.end());
	}

	obj->parent = nullptr;
}

static bool performSceneTreeDrop(LevelEditorState& state)
{
	const SceneTreeDropTarget& target = state.treeDropTarget;

	if (state.treeDragObjects.empty() || target.mode == SceneTreeDropMode::None || !target.object)
		return false;

	// never drop an object into itself or into one of its descendants
	for (SceneObject* obj : state.treeDragObjects)
	{
		if (isAncestorOf(obj, target.object))
			return false;

		if (target.mode != SceneTreeDropMode::Into && isAncestorOf(obj, target.parent))
			return false;
	}

	for (SceneObject* obj : state.treeDragObjects)
	{
		SceneObject* parent = target.mode == SceneTreeDropMode::Into ? target.object : target.parent;
		i32 index = -1;

		if (target.mode == SceneTreeDropMode::Into)
		{
			index = (i32)parent->children.size();
		}
		else
		{
			index = sceneObjectIndex(state, target.object);

			if (index < 0)
				continue;

			if (target.mode == SceneTreeDropMode::After)
				++index;
		}

		detachSceneObject(state, obj);

		std::vector<SceneObject*>& list = sceneChildren(state, parent);

		if (index < 0 || index > (i32)list.size())
			index = (i32)list.size();

		list.insert(list.begin() + index, obj);
		obj->parent = parent;
	}

	if (target.mode == SceneTreeDropMode::Into)
		target.object->expanded = true;

	const char* where = target.mode == SceneTreeDropMode::Into ? "into" :
		(target.mode == SceneTreeDropMode::Before ? "before" : "after");

	appendLog(state, LogEntry::Level::Info,
		std::string("Moved '") + state.treeDragObjects.front()->name + "' " + where + " '" + target.object->name + "'");

	return true;
}

// keep the original name as the widget id so the text input can take over the
// capture the row label grabbed on mouse-down
static void beginSceneTreeRename(LevelEditorState& state, SceneObject* obj)
{
	state.treeRenameObject = obj;
	snprintf(state.treeRenameId, sizeof(state.treeRenameId), "%s", obj->name);
	snprintf(state.treeRenameText, sizeof(state.treeRenameText), "%s", obj->name);

	state.treeDragObject = nullptr;
	state.treeDragging = false;
	state.treeDragObjects.clear();
	state.treeDropTarget = SceneTreeDropTarget();
	state.treeRenameCancelled = false;
	state.treeRenameJustStarted = true;
	hui::forceRepaint();
}

SceneObject* createObject(LevelEditorState& state, const char* name, const char* type, SceneObject* parent)
{
	state.objects.push_back(std::make_unique<SceneObject>());
	SceneObject* obj = state.objects.back().get();

	snprintf(obj->name, sizeof(obj->name), "%s", name);
	obj->type = type;

	if (parent)
	{
		obj->parent = parent;
		parent->children.push_back(obj);
	}
	else
	{
		state.roots.push_back(obj);
	}

	return obj;
}

void deleteObject(LevelEditorState& state, SceneObject* obj)
{
	if (!obj)
		return;

	state.viewportDragItems.erase(std::remove_if(state.viewportDragItems.begin(), state.viewportDragItems.end(),
		[obj](const ViewportDragItem& item) { return item.object == obj; }), state.viewportDragItems.end());

	if (state.viewportDragItems.empty())
		state.viewportDragging = false;

	state.treeDragObjects.erase(std::remove(state.treeDragObjects.begin(), state.treeDragObjects.end(), obj),
		state.treeDragObjects.end());

	if (state.treeDragObject == obj)
	{
		state.treeDragObject = nullptr;
		state.treeDragging = false;
		state.treeDragObjects.clear();
		state.treeDropTarget = SceneTreeDropTarget();
		hui::dragDropEnd();
		hui::windowReleaseCapture();
	}

	if (state.treeRenameObject == obj)
		state.treeRenameObject = nullptr;

	// keep the children alive, re-parent them to the deleted object's parent
	for (SceneObject* child : obj->children)
	{
		child->parent = obj->parent;

		if (obj->parent)
			obj->parent->children.push_back(child);
		else
			state.roots.push_back(child);
	}

	if (obj->parent)
	{
		auto& siblings = obj->parent->children;
		siblings.erase(std::remove(siblings.begin(), siblings.end(), obj), siblings.end());
	}
	else
	{
		auto& r = state.roots;
		r.erase(std::remove(r.begin(), r.end(), obj), r.end());
	}

	state.objects.erase(std::remove_if(state.objects.begin(), state.objects.end(),
		[obj](const std::unique_ptr<SceneObject>& p) { return p.get() == obj; }), state.objects.end());

	state.sceneSelection.erase(std::remove(state.sceneSelection.begin(), state.sceneSelection.end(), obj), state.sceneSelection.end());

	if (state.selectedObject == obj)
		state.selectedObject = nullptr;
}

void deleteSelection(LevelEditorState& state)
{
	// copy: deleteObject() mutates both selectedObject and sceneSelection
	std::vector<SceneObject*> objects = state.sceneSelection;

	if (state.selectedObject && std::find(objects.begin(), objects.end(), state.selectedObject) == objects.end())
		objects.push_back(state.selectedObject);

	for (SceneObject* obj : objects)
		deleteObject(state, obj);
}

void buildDummyScene(LevelEditorState& state)
{
	SceneObject* world = createObject(state, "World", "Scene", nullptr);
	world->color = hui::Color::lightGray;

	SceneObject* terrain = createObject(state, "Terrain", "Entity", world);
	terrain->position[0] = -140;
	terrain->position[2] = 70;
	terrain->scale[0] = 3;
	terrain->scale[2] = 3;
	terrain->color = hui::Color::fromU8(90, 120, 80);
	terrain->isStatic = true;

	SceneObject* city = createObject(state, "CityBlock", "Entity", world);
	city->position[0] = 30;
	city->position[2] = -20;
	city->color = hui::Color::fromU8(120, 130, 160);

	SceneObject* bldgA = createObject(state, "Building_A", "Entity", city);
	bldgA->position[0] = 30;
	bldgA->position[2] = -20;
	bldgA->scale[0] = 1.6f;
	bldgA->scale[2] = 1.6f;
	bldgA->color = hui::Color::fromU8(160, 150, 130);

	SceneObject* bldgB = createObject(state, "Building_B", "Entity", city);
	bldgB->position[0] = 90;
	bldgB->position[2] = 5;
	bldgB->color = hui::Color::fromU8(150, 140, 120);
	bldgB->visible = false;

	SceneObject* lamp1 = createObject(state, "StreetLamp_01", "Light", city);
	lamp1->position[0] = 10;
	lamp1->position[2] = 30;
	lamp1->color = hui::Color::yellow;

	SceneObject* lamp2 = createObject(state, "StreetLamp_02", "Light", city);
	lamp2->position[0] = 60;
	lamp2->position[2] = 30;
	lamp2->color = hui::Color::yellow;

	SceneObject* spawn = createObject(state, "PlayerSpawn", "Marker", world);
	spawn->position[0] = -40;
	spawn->position[2] = -60;
	spawn->color = hui::Color::green;

	SceneObject* camera = createObject(state, "MainCamera", "Camera", world);
	camera->position[0] = 0;
	camera->position[2] = -100;
	camera->color = hui::Color::sky;

	SceneObject* sun = createObject(state, "SunLight", "Light", world);
	sun->position[0] = -100;
	sun->position[2] = -80;
	sun->color = hui::Color::orange;

	state.selectedObject = city;
}

void appendLog(LevelEditorState& state, LogEntry::Level level, const std::string& text)
{
	if (state.log.size() >= 200)
	{
		state.log.erase(state.log.begin());

		if (state.logSelectionAnchor > 0)
			state.logSelectionAnchor--;
		else
			state.logSelectionAnchor = -1;
	}

	LogEntry entry;
	entry.level = level;
	entry.text = text;
	entry.selected = false;
	state.log.push_back(entry);

	if (state.logAutoScroll)
		state.logScroll.y = 1e9f;
}

static void clearLogSelection(LevelEditorState& state)
{
	for (LogEntry& entry : state.log)
		entry.selected = false;

	state.logSelectionAnchor = -1;
}

static void copySelectedLogEntries(const LevelEditorState& state)
{
	std::string text;

	for (const LogEntry& entry : state.log)
	{
		if (!entry.selected)
			continue;

		const char* typeName = "Info";

		if (entry.level == LogEntry::Level::Warning)
			typeName = "Warning";
		else if (entry.level == LogEntry::Level::Error)
			typeName = "Error";

		if (!text.empty())
			text += "\n";

		text += "[";
		text += typeName;
		text += "] ";
		text += entry.text;
	}

	if (!text.empty())
		hui::clipboardSetText(text.c_str());
}

static void selectLogRow(LevelEditorState& state, const std::vector<u32>& visibleIndices, u32 visibleIndex, u32 logIndex, bool ctrl, bool shift)
{
	if (shift && state.logSelectionAnchor >= 0)
	{
		int anchorVisible = -1;

		for (u32 v = 0; v < visibleIndices.size(); v++)
		{
			if ((i32)visibleIndices[v] == state.logSelectionAnchor)
			{
				anchorVisible = (int)v;
				break;
			}
		}

		if (anchorVisible >= 0)
		{
			if (!ctrl)
				clearLogSelection(state);

			int from = (std::min)(anchorVisible, (int)visibleIndex);
			int to = (std::max)(anchorVisible, (int)visibleIndex);

			for (int v = from; v <= to; v++)
			{
				u32 idx = visibleIndices[(u32)v];

				if (idx < state.log.size())
					state.log[idx].selected = true;
			}

			return;
		}
	}

	if (ctrl)
	{
		state.log[logIndex].selected = !state.log[logIndex].selected;
		state.logSelectionAnchor = (i32)logIndex;
		return;
	}

	clearLogSelection(state);
	state.log[logIndex].selected = true;
	state.logSelectionAnchor = (i32)logIndex;
}

void appendNextDummyLog(LevelEditorState& state)
{
	static const char* messages[] = {
		"Streaming level chunk 12",
		"Rebuilding navigation mesh...",
		"Actor 'Building_A' bound to layer Static",
		"Imported texture T_CityWall_D (2048x2048)",
		"Light bake completed in 1.42s",
		"WARNING: Material 'M_Glass' missing UV channel 2",
		"ERROR: Failed to load mesh 'SM_Bridge_LOD2'",
		"Saved scene to /Content/Levels/Blockout.level",
	};
	static const LogEntry::Level levels[] = {
		LogEntry::Level::Info,
		LogEntry::Level::Info,
		LogEntry::Level::Info,
		LogEntry::Level::Info,
		LogEntry::Level::Info,
		LogEntry::Level::Warning,
		LogEntry::Level::Error,
		LogEntry::Level::Info,
	};

	constexpr u32 count = sizeof(messages) / sizeof(messages[0]);
	u32 index = state.logMessageIndex % count;

	appendLog(state, levels[index], messages[index]);
	state.logMessageIndex++;
}

bool containsInsensitive(const std::string& haystack, const char* needle)
{
	if (!needle || !needle[0])
		return true;

	std::string hay = haystack;
	std::string ned = needle;

	std::transform(hay.begin(), hay.end(), hay.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	std::transform(ned.begin(), ned.end(), ned.begin(), [](unsigned char c) { return (char)std::tolower(c); });

	return hay.find(ned) != std::string::npos;
}

// Demo helper: get one of the atlas icons by its lowercase_underscore name.
hui::HImage materialIcon(const char* name)
{
	return hui::themeGetImage(hui::themeGet(), name);
}

void loadThemeImages(LevelEditorState& state)
{
	auto theme = hui::themeGet();

	state.iconMenu = materialIcon("menu");
	state.iconToolbar = materialIcon("build");
	state.iconTree = materialIcon("account_tree");
	state.iconInspector = materialIcon("settings");
	state.iconViewport = materialIcon("crop_free");
	state.iconAssets = materialIcon("folder_open");
	state.iconLog = materialIcon("article");
	state.iconNew = materialIcon("add");
	state.iconOpen = materialIcon("folder_open");
	state.iconSave = materialIcon("save");
	state.iconUndo = materialIcon("undo");
	state.iconRedo = materialIcon("redo");

	// hierarchy icons
	state.iconVisible = materialIcon("visibility_off");
	state.iconLocked = materialIcon("lock");
	state.iconObject = hui::loadThemeImage(theme, "../themes/default/cube_3d.png");
	state.iconArrowClosed = materialIcon("chevron_right");
	state.iconArrowOpen = materialIcon("expand_more");

	state.thumbLena = hui::loadThemeImage(theme, "../themes/default/lena.png");
	state.thumbFolder = materialIcon("folder_open");
	state.thumbSettings = materialIcon("settings");
	state.thumbData = materialIcon("fact_check");
	state.thumbStacks = materialIcon("layers");
}

void buildDummyAssets(LevelEditorState& state)
{
	auto add = [&state](const char* name, const char* type, u32 folder, hui::HImage thumb)
	{
		AssetItem item;
		item.name = name;
		item.type = type;
		item.folder = folder;
		item.thumb = thumb;
		state.assets.push_back(item);
	};

	add("T_CityWall_D.png", "Texture", 0, state.thumbLena);
	add("T_Road_Albedo.png", "Texture", 0, state.thumbLena);
	add("T_Grass_D.png", "Texture", 0, state.thumbLena);
	add("T_Sky_HDRI.exr", "Texture", 0, state.thumbData);
	add("T_Metal_Rough.png", "Texture", 0, state.thumbLena);
	add("SM_Building_A.fbx", "Mesh", 1, state.thumbStacks);
	add("SM_Building_B.fbx", "Mesh", 1, state.thumbStacks);
	add("SM_StreetLamp.fbx", "Mesh", 1, state.thumbStacks);
	add("SM_Bridge_LOD2.fbx", "Mesh", 1, state.thumbStacks);
	add("M_Default.mat", "Material", 2, state.thumbSettings);
	add("M_Glass.mat", "Material", 2, state.thumbSettings);
	add("M_Asphalt.mat", "Material", 2, state.thumbSettings);
	add("S_Ambient_01.wav", "Audio", 3, state.thumbData);
	add("S_Footstep.wav", "Audio", 3, state.thumbData);
	add("P_StreetLamp.prefab", "Prefab", 4, state.thumbFolder);
	add("P_Building.prefab", "Prefab", 4, state.thumbFolder);

	state.assetSelected.assign(state.assets.size(), false);
	state.assetSelectionAnchor = -1;
}

void drawMainMenu(LevelEditorState& state, bool& exitNow)
{
	// keep the view toggles in sync with the windows, they can also be closed from
	// the tab bar (middle click) or by a layout load/reset
	state.showViewport = hui::windowIsVisible("viewport");
	state.showSceneTree = hui::windowIsVisible("scene_tree");
	state.showInspector = hui::windowIsVisible("inspector");
	state.showAssetBrowser = hui::windowIsVisible("asset_browser");
	state.showIconBrowser = hui::windowIsVisible("icon_browser");
	state.showLog = hui::windowIsVisible("log");

	if (!hui::menuBarBegin())
		return;

	if (hui::menuBegin("File"))
	{
		if (hui::menuItem("New Scene", "Ctrl+N"))
			appendLog(state, LogEntry::Level::Info, "New scene created");

		if (hui::menuItem("Open Scene...", "Ctrl+O"))
			appendLog(state, LogEntry::Level::Info, "Open scene requested");

		if (hui::menuItem("Save", "Ctrl+S"))
			appendLog(state, LogEntry::Level::Info, "Scene saved");

		if (hui::menuItem("Save As...", "Ctrl+Shift+S"))
			appendLog(state, LogEntry::Level::Info, "Save scene as requested");

		hui::menuSeparator();

		if (hui::menuItem("Exit", "Alt+F4"))
			exitNow = true;

		hui::menuEnd();
	}

	if (hui::menuBegin("Edit"))
	{
		if (hui::menuItem("Undo", "Ctrl+Z"))
			appendLog(state, LogEntry::Level::Info, "Undo");

		if (hui::menuItem("Redo", "Ctrl+Y"))
			appendLog(state, LogEntry::Level::Info, "Redo");

		hui::menuSeparator();

		hui::menuItem("Cut", "Ctrl+X");
		hui::menuItem("Copy", "Ctrl+C");
		hui::menuItem("Paste", "Ctrl+V");

		hui::menuSeparator();

		if (hui::menuItem("Delete", "Del"))
			deleteSelection(state);

		hui::menuEnd();
	}

	if (hui::menuBegin("View"))
	{
		auto toggle = [](bool& value, const char* windowId)
		{
			value = !value;
			hui::windowSetVisible(windowId, value);
		};

		if (hui::menuItem("Viewport", "", 0, state.showViewport ? hui::SelectableFlags::Checked : hui::SelectableFlags::Normal))
			toggle(state.showViewport, "viewport");

		if (hui::menuItem("Scene Tree", "", 0, state.showSceneTree ? hui::SelectableFlags::Checked : hui::SelectableFlags::Normal))
			toggle(state.showSceneTree, "scene_tree");

		if (hui::menuItem("Object Inspector", "", 0, state.showInspector ? hui::SelectableFlags::Checked : hui::SelectableFlags::Normal))
			toggle(state.showInspector, "inspector");

		if (hui::menuItem("Asset Browser", "", 0, state.showAssetBrowser ? hui::SelectableFlags::Checked : hui::SelectableFlags::Normal))
			toggle(state.showAssetBrowser, "asset_browser");

		if (hui::menuItem("Icon Browser", "", 0, state.showIconBrowser ? hui::SelectableFlags::Checked : hui::SelectableFlags::Normal))
			toggle(state.showIconBrowser, "icon_browser");

		if (hui::menuItem("Log", "", 0, state.showLog ? hui::SelectableFlags::Checked : hui::SelectableFlags::Normal))
			toggle(state.showLog, "log");

		hui::menuSeparator();

		if (hui::menuBegin("UI Scale"))
		{
			char label[16];

			for (f32 step : uiScaleSteps)
			{
				snprintf(label, sizeof(label), "%.0f%%", step * 100.0f);
				auto flags = std::fabs(step - state.uiScale) < 0.001f
					? hui::SelectableFlags::Checked
					: hui::SelectableFlags::Normal;

				if (hui::menuItem(label, "", 0, flags))
					state.pendingUiScale = step;
			}

			hui::menuEnd();
		}

		hui::menuSeparator();

		if (hui::menuItem("Reset Layout"))
			state.resetLayoutRequested = true;

		hui::menuSeparator();

		if (hui::menuItem("Save Layout"))
			state.saveLayoutRequested = true;

		if (hui::menuItem("Load Layout"))
			state.loadLayoutRequested = true;

		hui::menuEnd();
	}

	if (hui::menuBegin("GameObject"))
	{
		auto create = [&state](const char* name, const char* type)
		{
			SceneObject* parent = state.selectedObject ? state.selectedObject : (state.roots.empty() ? nullptr : state.roots[0]);
			SceneObject* obj = createObject(state, name, type, parent);

			obj->position[0] = (f32)(state.newObjectCounter % 8) * 12.0f - 40.0f;
			obj->position[2] = (f32)(state.newObjectCounter % 5) * 12.0f - 20.0f;
			state.newObjectCounter++;
			state.selectedObject = obj;

			appendLog(state, LogEntry::Level::Info, std::string("Created '") + name + "'");
		};

		if (hui::menuItem("Create Empty"))
			create("GameObject", "Entity");

		if (hui::menuItem("Create Cube"))
			create("Cube", "Entity");

		if (hui::menuItem("Create Sphere"))
			create("Sphere", "Entity");

		if (hui::menuItem("Create Light"))
			create("PointLight", "Light");

		if (hui::menuItem("Create Camera"))
			create("Camera", "Camera");

		hui::menuEnd();
	}

	if (hui::menuBegin("Help"))
	{
		hui::menuItem("Documentation", "F1");
		hui::menuItem("About Level Editor");
		hui::menuEnd();
	}

	hui::menuBarEnd();
}

static const char* editorWindowTitle(const char* windowId)
{
	if (!strcmp(windowId, "main_menu")) return "Main Menu";
	if (!strcmp(windowId, "scene_tree")) return "Scene Tree";
	if (!strcmp(windowId, "inspector")) return "Object Inspector";
	if (!strcmp(windowId, "viewport")) return "Viewport";
	if (!strcmp(windowId, "asset_browser")) return "Asset Browser";
	if (!strcmp(windowId, "log")) return "Log";

	return windowId;
}

// true if the layout state knows about the window with the given id, docked or closed
static bool layoutStateHasWindow(const hui::WindowsDockingState& layoutState, const char* windowId)
{
	for (const auto& node : layoutState.dockNodes)
	{
		for (const auto& wnd : node.windows)
		{
			if (wnd.id == windowId)
				return true;
		}
	}

	for (const auto& wnd : layoutState.closedWindows)
	{
		if (wnd.id == windowId)
			return true;
	}

	return false;
}

static bool layoutWindowsShareNode(const hui::WindowsDockingState& layoutState, const char* windowIdA, const char* windowIdB)
{
	for (const auto& node : layoutState.dockNodes)
	{
		bool hasA = false;
		bool hasB = false;

		for (const auto& wnd : node.windows)
		{
			hasA = hasA || wnd.id == windowIdA;
			hasB = hasB || wnd.id == windowIdB;
		}

		if (hasA && hasB)
			return true;
	}

	return false;
}

hui::WindowsDockingState buildDefaultLayoutState()
{
	hui::WindowsDockingState state;
	hui::WindowsDockingState::NativeWindowInfo nativeWindow;

	nativeWindow.rect = hui::Rect(0, 0, 1000, 1000);
	nativeWindow.floating = false;
	state.nativeWindows.push_back(nativeWindow);

	auto addNode = [&state](i32 parentIndex, hui::DockNodeType type, const hui::Rect& rect, std::initializer_list<const char*> windowIds) -> i32
	{
		hui::WindowsDockingState::DockNodeInfo node;

		node.parentIndex = parentIndex;
		node.nativeWindowIndex = 0;
		node.type = type;
		node.rect = rect;

		for (auto windowId : windowIds)
			node.windows.push_back({ windowId, editorWindowTitle(windowId) });

		state.dockNodes.push_back(node);

		return (i32)state.dockNodes.size() - 1;
	};

	// root, the menu chrome on top and the content dock area below it
	addNode(-1, hui::DockNodeType::Vertical, hui::Rect(0, 0, 1000, 1000), {});
	addNode(0, hui::DockNodeType::Tabs, hui::Rect(0, 0, 1000, 68), { "main_menu" });
	addNode(0, hui::DockNodeType::Horizontal, hui::Rect(0, 71, 1000, 929), {});
	addNode(2, hui::DockNodeType::Tabs, hui::Rect(0, 71, 200, 929), { "scene_tree" });
	addNode(2, hui::DockNodeType::Horizontal, hui::Rect(201, 71, 799, 929), {});
	addNode(4, hui::DockNodeType::Vertical, hui::Rect(201, 71, 600, 929), {});
	addNode(5, hui::DockNodeType::Tabs, hui::Rect(201, 71, 600, 650), { "viewport" });
	addNode(5, hui::DockNodeType::Tabs, hui::Rect(201, 721, 600, 279), { "asset_browser", "log" });
	addNode(4, hui::DockNodeType::Tabs, hui::Rect(803, 71, 197, 929), { "inspector" });

	// the chrome window is pinned, mark it with the same flags the app sets every frame
	state.dockNodes[1].windows[0].flags = (u32)(hui::WindowFlags::NoTab | hui::WindowFlags::NoResize | hui::WindowFlags::NoPadding);

	state.dockAreaNodeIndex = 2;

	return state;
}

void buildDefaultLayout(hui::DockNodeId root)
{
	hui::DockNodeId n1, n2;

	// top: the menu strip with the main toolbar right below it, in one chrome window
	hui::dockNodeSplit(root, hui::DockNodeSplitType::Top, 0.12f, &n1, &n2);
	hui::dockNodeSetWindow(n1, "main_menu");

	// windows are docked only inside the content area below the chrome window,
	// so the menu and toolbar always stay full width at the top
	hui::dockNodeSetDockArea(n2);

	// left: scene tree
	hui::dockNodeSplit(n2, hui::DockNodeSplitType::Left, 0.20f, &n1, &n2);
	hui::dockNodeSetWindow(n1, "scene_tree");

	// right: object inspector
	hui::dockNodeSplit(n2, hui::DockNodeSplitType::Right, 0.75f, &n1, &n2);
	hui::dockNodeSetWindow(n2, "inspector");

	// center: viewport on top, asset browser and log as tabs at the bottom
	hui::dockNodeSplit(n1, hui::DockNodeSplitType::Top, 0.70f, &n1, &n2);
	hui::dockNodeSetWindow(n1, "viewport");
	hui::dockNodeSetWindow(n2, "asset_browser");
	hui::dockNodeSetWindow(n2, "log");

	hui::dockNodeLayoutRecalculate();
}

void drawMainToolbar(LevelEditorState& state)
{
	static const char* toolItems[] = { "Select", "Move", "Rotate", "Scale" };

	if (hui::imageButton(state.iconNew, state.iconButtonSize, state.iconButtonSize))
		appendLog(state, LogEntry::Level::Info, "Toolbar: New");
	hui::tooltip("New Scene");

	hui::sameLine(0, 0.001f);

	if (hui::imageButton(state.iconOpen, state.iconButtonSize, state.iconButtonSize))
		appendLog(state, LogEntry::Level::Info, "Toolbar: Open");
	hui::tooltip("Open Scene");

	hui::sameLine(0, 0.001f);

	if (hui::imageButton(state.iconSave, state.iconButtonSize, state.iconButtonSize))
		appendLog(state, LogEntry::Level::Info, "Toolbar: Save");
	hui::tooltip("Save");

	hui::sameLine(0, 0.001f);

	if (hui::imageButton(state.iconUndo, state.iconButtonSize, state.iconButtonSize))
		appendLog(state, LogEntry::Level::Info, "Toolbar: Undo");
	hui::tooltip("Undo");

	hui::sameLine(0, 0.001f);

	if (hui::imageButton(state.iconRedo, state.iconButtonSize, state.iconButtonSize))
		appendLog(state, LogEntry::Level::Info, "Toolbar: Redo");
	hui::tooltip("Redo");

	hui::sameLine();
	hui::space(state.toolbarGroupSpacing);

	hui::dropdown("##toolSelect", state.toolIndex, toolItems, 4);

	hui::sameLine();

	if (hui::check("Grid", &state.gridVisible))
		appendLog(state, LogEntry::Level::Info, state.gridVisible ? "Grid shown" : "Grid hidden");

	hui::sameLine();

	hui::check("Snap", &state.snapEnabled);

	hui::sameLine();

	hui::check("Play", &state.playing);

	hui::sameLine();

	if (hui::button("Reset"))
	{
		state.viewZoom = 1.0f;
		appendLog(state, LogEntry::Level::Info, "View reset");
	}
}

void drawSceneTreeNode(LevelEditorState& state, SceneObject* obj, u32 depth)
{
	hui::idPush((void*)obj);
	hui::tableRowNext();

	hui::Rect cellRect = hui::tableCellGetRect();
	hui::Point rowBasePos = hui::widgetGetPosition();

	bool hasChildren = !obj->children.empty();

	// match the icons to the real height of the row (the selectable/text box).
	// tableCellGetRect() is in screen pixels, image() expects unscaled height.
	f32 scale = hui::scaleGet();
	f32 rowHeight = scale > 0.0f ? cellRect.height / scale : 16.0f;
	f32 iconStep = rowHeight;

	// the whole row is hoverable, so the view/lock icons can appear anywhere on it
	hui::Rect rowRect(cellRect.x, cellRect.y, cellRect.width, cellRect.height);
	bool layerActive = hui::layerIsActive();
	bool rowHovered = layerActive && hui::windowIsMouseOver() && rowRect.contains(hui::mouseGetPosition());

	// Unity-like full-row highlight. Colors come from the theme; tableRowSetColor
	// paints behind every widget in the row, including the hidden/lock icons.
	// Selection takes priority over hover.
	bool rowSelected = isObjectSelected(state, obj);

	if (rowSelected || rowHovered)
	{
		hui::Color rowColor = hui::themeGetUserWidgetElementParameterColor(
			hui::themeGet(), "sceneTreeRow", "default",
			rowSelected ? "selectedColor" : "hoverColor",
			hui::Color::transparent);

		hui::tableRowSetColor(rowColor);
	}

	// icons live in a fixed gutter on the left; they are visible when the flag is
	// set, otherwise only while the row is hovered. A flag that is only previewed
	// on hover uses a dim variant; a set flag is drawn at full alpha.
	bool hideSet = !obj->visible;
	bool showView = hideSet || rowHovered;
	bool showLock = obj->locked || rowHovered;

	// A flag preview is dimmed while merely hovering the row, but becomes fully
	// white when the pointer is over that particular icon (Unity-style).
	hui::Rect viewIconRect(rowBasePos.x, rowBasePos.y, rowHeight, rowHeight);
	hui::Rect lockIconRect(rowBasePos.x + iconStep, rowBasePos.y, rowHeight, rowHeight);
	const hui::Point mousePos = hui::mouseGetPosition();
	bool viewIconHovered = layerActive && hui::windowIsMouseOver() && viewIconRect.contains(mousePos);
	bool lockIconHovered = layerActive && hui::windowIsMouseOver() && lockIconRect.contains(mousePos);

	hui::HImage viewIcon = materialIcon(hideSet || viewIconHovered ? "visibility_off" : "visibility_off_dim");
	hui::HImage lockIcon = materialIcon(obj->locked || lockIconHovered ? "lock" : "lock_dim");

	if (showView && viewIcon)
	{
		hui::widgetPushPosition();
		hui::widgetSetPosition(rowBasePos);
		if (hui::image(viewIcon, rowHeight, hui::HAlignType::Left, hui::VAlignType::Center))
			obj->visible = !obj->visible;
		hui::widgetPopPosition();
	}

	if (showLock && lockIcon)
	{
		hui::widgetPushPosition();
		hui::widgetSetPosition({ rowBasePos.x + iconStep, rowBasePos.y });
		if (hui::image(lockIcon, rowHeight, hui::HAlignType::Left, hui::VAlignType::Center))
			obj->locked = !obj->locked;
		hui::widgetPopPosition();
	}

	// indented content: expand arrow, object icon, then the label
	f32 indent = depth * 18.0f * hui::scaleGet();
	f32 contentX = rowBasePos.x + 2.0f * iconStep + indent + (hasChildren ? 0.0f : iconStep);
	hui::widgetSetPosition({ contentX, rowBasePos.y });

	if (hasChildren)
	{
		hui::HImage arrow = obj->expanded ? state.iconArrowOpen : state.iconArrowClosed;
		if (arrow && hui::image(arrow, rowHeight, hui::HAlignType::Left, hui::VAlignType::Center))
			obj->expanded = !obj->expanded;
		hui::sameLine();
	}

	if (state.iconObject && hui::image(state.iconObject, rowHeight, hui::HAlignType::Left, hui::VAlignType::Center))
	{
		state.selectedObject = obj;
		state.sceneSelection.clear();
	}

	hui::sameLine();

	// plain label: a selectable would paint its own body over the themed row
	// highlight, so selection is handled on the whole row instead. Clicking the
	// content of an already-selected row swaps it for an in-place editor using
	// the same widget id, so the capture taken on mouse-down goes to the text input.
	hui::Rect labelRect;

	if (state.treeRenameObject == obj)
	{
		const hui::InputEvent& renameEv = hui::inputEventGet();

		// esc aborts the rename and keeps the original name
		if (renameEv.type == hui::InputEvent::Type::Key
			&& renameEv.key.down
			&& renameEv.key.code == hui::KeyCode::Esc)
		{
			state.treeRenameCancelled = true;
		}

		if (state.treeRenameJustStarted)
		{
			hui::textInputStartEdit(state.treeRenameId);
			state.treeRenameJustStarted = false;
		}

		hui::widgetSetNextHeight(rowHeight);
		hui::textInput(state.treeRenameId, state.treeRenameText, sizeof(state.treeRenameText), hui::TextInputFlags::AutoSelectAll);

		if (!hui::widgetIsFocused())
		{
			if (!state.treeRenameCancelled && state.treeRenameText[0])
			{
				snprintf(obj->name, sizeof(obj->name), "%s", state.treeRenameText);
				appendLog(state, LogEntry::Level::Info, std::string("Renamed object to '") + obj->name + "'");
			}

			state.treeRenameObject = nullptr;
			state.treeRenameCancelled = false;
			hui::forceRepaint();
		}
	}
	else
	{
		hui::label(obj->name);
		labelRect = hui::widgetGetRect();
	}

	const hui::InputEvent& ev = hui::inputEventGet();

	if (!state.treeRenameObject && rowHovered
		&& ev.type == hui::InputEvent::Type::MouseDown
		&& ev.mouse.button == hui::MouseButton::Left)
	{
		// clicking anywhere in the content of an already-selected row starts a rename
		if (rowSelected && mousePos.x >= labelRect.x)
		{
			beginSceneTreeRename(state, obj);
		}
		else
		{
			state.selectedObject = obj;
			state.sceneSelection.clear();

			// start a hierarchy drag from the content part of the row; the icon
			// gutter on the left keeps its own click behavior
			if (mousePos.x >= contentX)
			{
				state.treeDragObject = obj;
				state.treeDragStartMouse = mousePos;
				state.treeDragging = false;
				state.treeDragObjects.clear();
			}
		}
	}

	if (state.treeDragging && rowHovered && !obj->locked && state.treeDragObject != obj && !isAncestorOf(state.treeDragObject, obj))
	{
		f32 relY = (mousePos.y - rowRect.y) / rowRect.height;
		SceneTreeDropMode mode = SceneTreeDropMode::Into;

		if (relY < 0.25f)
			mode = SceneTreeDropMode::Before;
		else if (relY > 0.75f)
			mode = SceneTreeDropMode::After;

		state.treeDropTarget.object = obj;
		state.treeDropTarget.parent = obj->parent;
		state.treeDropTarget.index = sceneObjectIndex(state, obj);
		state.treeDropTarget.mode = mode;

		hui::renderSetLineStyle(hui::LineStyle(hui::Color::fromU8(90, 180, 255), 2.0f));

		const f32 lineX0 = contentX + (mode == SceneTreeDropMode::Into ? iconStep : 0.0f);
		const f32 lineX1 = rowRect.right() - 4.0f;

		if (mode == SceneTreeDropMode::Before)
		{
			const f32 y = rowRect.y + 1.0f;
			hui::renderDrawLine({ lineX0, y }, { lineX1, y });
		}
		else if (mode == SceneTreeDropMode::After)
		{
			const f32 y = rowRect.bottom() - 1.0f;
			hui::renderDrawLine({ lineX0, y }, { lineX1, y });
		}
		else
		{
			// dropping into an object: draw the line at the child indent level
			const f32 y = rowRect.bottom() - 2.0f;
			hui::renderDrawLine({ lineX0, y }, { lineX1, y });
		}

		hui::renderSetLineStyle(hui::LineStyle(hui::Color::white, 1.0f));
	}

	if (hasChildren && obj->expanded)
	{
		for (SceneObject* child : obj->children)
			drawSceneTreeNode(state, child, depth + 1);
	}

	hui::idPop();
}

void drawSceneTree(LevelEditorState& state)
{
	const hui::InputEvent& ev = hui::inputEventGet();

	if (hui::layerIsActive()
		&& hui::windowIsMouseOver()
		&& ev.type == hui::InputEvent::Type::Key
		&& ev.key.down
		&& ev.key.code == hui::KeyCode::Delete
		&& state.selectedObject)
	{
		deleteSelection(state);
	}

	const hui::Point treeMouse = hui::mouseGetPosition();
	bool treeLeftDown = HUI_SERVICES.isMouseButtonDownNow && HUI_SERVICES.isMouseButtonDownNow(hui::MouseButton::Left);

	if (state.treeDragObject)
	{
		f32 treeDragDistance = state.treeDragStartMouse.getDistance(treeMouse);
		bool treeMouseUp = ev.type == hui::InputEvent::Type::MouseUp && ev.mouse.button == hui::MouseButton::Left;

		if (!treeLeftDown
			|| treeMouseUp
			|| ev.type == hui::InputEvent::Type::WindowLostFocus)
		{
			if (state.treeDragging)
			{
				performSceneTreeDrop(state);
				hui::dragDropEnd();
			}

			// buttonBehavior() on the row label/icon grabs the widget capture on
			// mouse-down. If the row is gone by mouse-up, that capture is never
			// released and every other window stops responding. Release it here.
			hui::windowReleaseCapture();

			state.treeDragObject = nullptr;
			state.treeDragging = false;
			state.treeDragObjects.clear();
			state.treeDropTarget = SceneTreeDropTarget();
			hui::forceRepaint();
		}
		else if (!state.treeDragging
			&& !state.treeDragObject->locked
			&& treeDragDistance >= treeDragStartDistance)
		{
			state.treeDragging = true;
			state.treeDragObjects.clear();
			state.treeDragObjects.push_back(state.treeDragObject);

			// dragDropBegin() clears ctx->widget.captureId, so the capture taken
			// by the row's label/image widgets cannot leak after the drop.
			hui::dragDropBegin(sceneTreeDragDropType, state.treeDragObject);
			hui::windowReleaseCapture();
		}
	}

	// recomputed every frame while dragging; rows set it again below
	state.treeDropTarget = SceneTreeDropTarget();

	hui::widgetSetNextWidth(0.55f);
	hui::textInput("##objectSearch", state.objectSearch, 128, hui::TextInputFlags::None, "Search objects...");

	hui::sameLine();

	hui::widgetSetNextWidth(0.21f);

	if (hui::button("+ New"))
	{
		SceneObject* parent = state.selectedObject ? state.selectedObject : (state.roots.empty() ? nullptr : state.roots[0]);
		char name[64];
		snprintf(name, sizeof(name), "GameObject_%u", state.newObjectCounter++);

		SceneObject* obj = createObject(state, name, "Entity", parent);
		obj->position[0] = (f32)(state.newObjectCounter % 6) * 14.0f - 35.0f;
		state.selectedObject = obj;

		appendLog(state, LogEntry::Level::Info, std::string("Created '") + name + "'");
	}

	hui::sameLine();

	hui::widgetSetNextWidth(0.21f);

	if (hui::button("Delete"))
		deleteSelection(state);

	hui::line();

	// a single borderless column is used only for stable row positioning; the
	// hierarchy itself is drawn manually as indented labels with icons
	hui::scrollViewBegin("##treeScroll", hui::layoutGetRemainingHeight(), state.treeScroll, hui::Point(0, 0), hui::ScrollViewFlags::NoHorizontalScroll);

	if (hui::tableBegin("##sceneTreeTable", 1, 0, hui::TableFlags::Stretch))
	{
		hui::tableColumnSetup(0, 0.0f, hui::TableColumnFlags::Stretch);

		for (SceneObject* root : state.roots)
			drawSceneTreeNode(state, root, 0);

		hui::tableEnd();
	}

	state.treeScroll = hui::scrollViewEnd();
}

template <typename EditorFn>
static void propGridRow(const char* name, EditorFn&& editorFn)
{
	hui::tableRowNext();
	hui::label(name, hui::HAlignType::Right);
	hui::tableCellNext();
	editorFn();
}

void drawInspector(LevelEditorState& state)
{
	SceneObject* obj = state.selectedObject;

	if (!obj)
	{
		hui::space();
		hui::label("No object selected", hui::HAlignType::Center);
		hui::labelCustomColor("Select an object in the Scene Tree to edit it.", hui::Color::gray, hui::HAlignType::Center);
		return;
	}

	static const char* meshItems[] = { "None", "Cube", "Sphere", "Cylinder", "Plane", "Custom" };

	hui::scrollViewBegin("##inspectorScroll", hui::layoutGetRemainingHeight(), state.inspectorScroll, hui::Point(0, 0), hui::ScrollViewFlags::NoHorizontalScroll | hui::ScrollViewFlags::NoBorder | hui::ScrollViewFlags::NoPadding);

	// borderless property grid, label and value share one row
	if (hui::tableBegin("##inspectorProps", 2, 0, hui::TableFlags::Stretch | hui::TableFlags::Resizable | hui::TableFlags::FixedSize))
	{
		hui::tableColumnSetup(0, 110, hui::TableColumnFlags::FixedResize);
		hui::tableColumnSetup(1, 0, hui::TableColumnFlags::Stretch);
		hui::tableCellPaddingPush(8.0f, 4.0f);

		propGridRow("Name", [&]() { hui::textInput("##objectName", obj->name, sizeof(obj->name)); });
		propGridRow("Type", [&]() { hui::labelCustomColor(obj->type, hui::Color::yellow); });

		hui::tableRowNext();
		hui::tableCellNext(2);
		bool transformOpen = hui::expandable("Transform", &obj->transformExpanded);

		if (transformOpen)
		{
			propGridRow("Position", [&]() { hui::vec3Editor("##position", obj->position[0], obj->position[1], obj->position[2], 0.05f); });
			propGridRow("Rotation", [&]() { hui::vec3Editor("##rotation", obj->rotation[0], obj->rotation[1], obj->rotation[2], 0.5f); });
			propGridRow("Scale", [&]() { hui::vec3Editor("##scale", obj->scale[0], obj->scale[1], obj->scale[2], 0.02f); });
		}

		hui::tableRowNext();
		hui::tableCellNext(2);
		bool renderingOpen = hui::expandable("Rendering", &obj->renderingExpanded);

		if (renderingOpen)
		{
			propGridRow("Mesh", [&]() { hui::dropdown("##meshSelect", obj->meshIndex, meshItems, 6); });
			propGridRow("Material", [&]() { hui::objectRefEditor("##materialRef", state.iconInspector, state.iconAssets, state.iconInspector, "Material", "M_Default", 1, &state.materialRef, &state.materialModified); });
			propGridRow("Tint Color", [&]() { hui::colorPickerPopup("##tintColor", &obj->color, hui::ColorPickerFlags::ShowPalette); });

			propGridRow("Visible", [&]() {
				hui::idPush("visible");
				hui::check("", &obj->visible);
				hui::idPop();
			});
			propGridRow("Static", [&]() {
				hui::idPush("static");
				hui::check("", &obj->isStatic);
				hui::idPop();
			});
		}

		hui::tableRowNext();
		hui::tableCellNext(2);
		bool physicsOpen = hui::expandable("Physics", &obj->physicsExpanded);

		if (physicsOpen)
		{
			propGridRow("Generate Collision", [&]() {
				hui::idPush("collision");
				hui::check("", &obj->collision);
				hui::idPop();
			});
			propGridRow("Is Trigger", [&]() {
				hui::idPush("trigger");
				hui::check("", &obj->trigger);
				hui::idPop();
			});

			if (obj->collision && obj->trigger)
			{
				hui::tableRowNext();
				hui::tableCellNext(2);
				hui::labelCustomColor("Triggers are not solid, overlap events only.", hui::Color::gray);
			}
		}

		hui::tableCellPaddingPop();
		hui::tableEnd();
	}

	state.inspectorScroll = hui::scrollViewEnd();
}

static const u32 assetDragDropType = 100;

static hui::Point sceneWorldToView(const hui::Rect& vp, f32 zoom, const hui::Point& pan, f32 x, f32 z)
{
	hui::Point origin = vp.center() + pan;

	return { origin.x + x * zoom, origin.y + z * zoom };
}

static void sceneViewToWorld(const hui::Rect& vp, f32 zoom, const hui::Point& pan, const hui::Point& p, f32& outX, f32& outZ)
{
	hui::Point origin = vp.center() + pan;

	outX = (p.x - origin.x) / zoom;
	outZ = (p.y - origin.y) / zoom;
}

static hui::Rect rectFromPoints(const hui::Point& a, const hui::Point& b)
{
	return { a.x < b.x ? a.x : b.x,
		a.y < b.y ? a.y : b.y,
		a.x > b.x ? a.x - b.x : b.x - a.x,
		a.y > b.y ? a.y - b.y : b.y - a.y };
}

static void drawDashedRect(const hui::Rect& rc, const hui::Color& color, f32 width, f32 dash, f32 gap)
{
	hui::LineStyle style(color, width, true);
	style.stipplePattern[0] = dash;
	style.stipplePattern[1] = gap;
	hui::renderSetLineStyle(style);
	hui::renderDrawRectangle(rc);
}

static bool sceneObjectHitTest(SceneObject* obj, const hui::Rect& vp, f32 zoom, const hui::Point& pan, const hui::Point& mouse)
{
	if (!obj || !obj->visible)
		return false;

	hui::Point p = sceneWorldToView(vp, zoom, pan, obj->position[0], obj->position[2]);

	if (strcmp(obj->type, "Light") == 0 || strcmp(obj->type, "Camera") == 0 || strcmp(obj->type, "Marker") == 0)
	{
		f32 dx = mouse.x - p.x;
		f32 dy = mouse.y - p.y;

		return (dx * dx + dy * dy) <= 18.0f * 18.0f;
	}

	f32 half = 16.0f * obj->scale[0] * zoom;
	if (half < 8.0f)
		half = 8.0f;

	hui::Rect rc = { p.x - half, p.y - half, half * 2.0f, half * 2.0f };

	return rc.contains(mouse);
}

static SceneObject* pickSceneObject(LevelEditorState& state, const hui::Rect& vp, f32 zoom, const hui::Point& mouse)
{
	SceneObject* picked = nullptr;

	for (auto& objPtr : state.objects)
	{
		SceneObject* obj = objPtr.get();

		if (sceneObjectHitTest(obj, vp, zoom, state.viewPan, mouse))
			picked = obj;
	}

	return picked;
}

static i32 meshIndexFromAssetName(const std::string& name)
{
	std::string lower = name;

	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });

	if (lower.find("cube") != std::string::npos)
		return 1;

	if (lower.find("sphere") != std::string::npos)
		return 2;

	if (lower.find("cylinder") != std::string::npos)
		return 3;

	if (lower.find("plane") != std::string::npos)
		return 4;

	return 5;
}

static SceneObject* createObjectFromAsset(LevelEditorState& state, u32 assetIndex, const hui::Rect& vp, f32 zoom, const hui::Point& dropPos)
{
	if (assetIndex >= state.assets.size())
		return nullptr;

	const AssetItem& asset = state.assets[assetIndex];

	if (strcmp(asset.type, "Mesh") != 0 && strcmp(asset.type, "Prefab") != 0)
		return nullptr;

	f32 worldX = 0.0f;
	f32 worldZ = 0.0f;
	sceneViewToWorld(vp, zoom, state.viewPan, dropPos, worldX, worldZ);

	if (state.snapEnabled)
	{
		worldX = roundf(worldX / 40.0f) * 40.0f;
		worldZ = roundf(worldZ / 40.0f) * 40.0f;
	}

	SceneObject* parent = state.selectedObject ? state.selectedObject : (state.roots.empty() ? nullptr : state.roots[0]);
	SceneObject* obj = nullptr;

	if (strcmp(asset.type, "Prefab") == 0)
	{
		obj = createObject(state, asset.name.c_str(), "Prefab", parent);
		obj->color = hui::Color::fromU8(180, 200, 120);
	}
	else
	{
		obj = createObject(state, asset.name.c_str(), "Entity", parent);
		obj->meshIndex = meshIndexFromAssetName(asset.name);
		obj->color = hui::Color::fromU8(140, 150, 170);
	}

	obj->position[0] = worldX;
	obj->position[2] = worldZ;
	state.selectedObject = obj;

	appendLog(state, LogEntry::Level::Info, std::string("Dropped '") + asset.name + "' into scene");

	return obj;
}

void drawViewport(LevelEditorState& state)
{
	hui::Rect vp = hui::viewportBegin("##levelViewport", 0);
	bool hovered = hui::widgetIsHovered();

	hui::renderSetFillStyle(hui::FillStyle(hui::Color::fromU8(28, 30, 36)));
	hui::renderDrawSolidRectangle(vp);

	const hui::InputEvent& ev = hui::inputEventGet();
	hui::Point mouse = hui::mouseGetPosition();
	bool mouseDownNow = HUI_SERVICES.isMouseButtonDownNow && HUI_SERVICES.isMouseButtonDownNow(hui::MouseButton::Left);
	bool middleDownNow = HUI_SERVICES.isMouseButtonDownNow && HUI_SERVICES.isMouseButtonDownNow(hui::MouseButton::Middle);

	// middle-mouse drag pans the view
	if (hovered && ev.type == hui::InputEvent::Type::MouseDown && ev.mouse.button == hui::MouseButton::Middle)
	{
		state.viewportPanning = true;
		state.viewportPanStartMouse = mouse;
		state.viewportPanStartValue = state.viewPan;
		hui::inputEventCancel();
	}

	if (state.viewportPanning)
	{
		if (!middleDownNow)
		{
			state.viewportPanning = false;
		}
		else
		{
			state.viewPan = state.viewportPanStartValue + (mouse - state.viewportPanStartMouse);
			hui::inputEventCancel();
			hui::forceRepaint();
		}
	}

	// mouse wheel zooms around the point under the cursor
	if (hovered && !state.viewportPanning && ev.type == hui::InputEvent::Type::MouseWheel)
	{
		f32 wheel = ev.mouse.wheel.y;

		if (fabsf(wheel) > 0.0f)
		{
			f32 oldZoom = state.viewZoom;
			f32 newZoom = oldZoom * (1.0f + 0.1f * wheel);

			if (newZoom < 0.1f)
				newZoom = 0.1f;
			if (newZoom > 10.0f)
				newZoom = 10.0f;

			if (newZoom != oldZoom)
			{
				hui::Point origin = vp.center() + state.viewPan;
				f32 worldX = (mouse.x - origin.x) / oldZoom;
				f32 worldZ = (mouse.y - origin.y) / oldZoom;

				state.viewPan = mouse - vp.center() - hui::Point(worldX * newZoom, worldZ * newZoom);
				state.viewZoom = newZoom;
				hui::inputEventCancel();
				hui::forceRepaint();
			}
		}
	}

	hui::Point center = vp.center() + state.viewPan;
	f32 zoom = state.viewZoom;
	f32 gridStep = 40.0f * zoom;

	if (gridStep < 4.0f)
		gridStep = 4.0f;

	// ground grid
	if (state.gridVisible)
	{
		hui::renderSetLineStyle(hui::LineStyle(hui::Color::fromU8(52, 56, 66), 1.0f));

		f32 startX = center.x;

		while (startX - gridStep >= vp.left())
			startX -= gridStep;

		for (f32 x = startX; x <= vp.right(); x += gridStep)
			hui::renderDrawLine({ x, vp.top() }, { x, vp.bottom() });

		f32 startY = center.y;

		while (startY - gridStep >= vp.top())
			startY -= gridStep;

		for (f32 y = startY; y <= vp.bottom(); y += gridStep)
			hui::renderDrawLine({ vp.left(), y }, { vp.right(), y });
	}

	// world axes
	hui::renderSetLineStyle(hui::LineStyle(hui::Color::fromU8(150, 60, 60), 2.0f));
	hui::renderDrawLine({ vp.left(), center.y }, { vp.right(), center.y });

	hui::renderSetLineStyle(hui::LineStyle(hui::Color::fromU8(60, 150, 70), 2.0f));
	hui::renderDrawLine({ center.x, vp.top() }, { center.x, vp.bottom() });

	// drag and drop assets into the scene
	if (hovered && hui::dragDropGetObjectType() == assetDragDropType)
	{
		u32 assetIndex = (u32)(uintptr_t)hui::dragDropGetObject();

		if (assetIndex < state.assets.size())
		{
			const AssetItem& asset = state.assets[assetIndex];

			if (strcmp(asset.type, "Mesh") == 0 || strcmp(asset.type, "Prefab") == 0)
			{
				hui::dragDropAllow();

				if (hui::dragDropDroppedOnWidget())
				{
					createObjectFromAsset(state, assetIndex, vp, zoom, mouse);
					hui::dragDropEnd();
					hui::forceRepaint();
				}
			}
		}
	}

	// selection and moving
	if (!hui::dragDropGetObject() && hovered && !state.viewportPanning && ev.type == hui::InputEvent::Type::MouseDown && ev.mouse.button == hui::MouseButton::Left)
	{
		SceneObject* picked = pickSceneObject(state, vp, zoom, mouse);
		bool pickedAlreadySelected = picked && isObjectSelected(state, picked);

		if (!pickedAlreadySelected)
		{
			state.selectedObject = picked;
			state.sceneSelection.clear();
		}
		else
		{
			// keep the multi-selection and make the clicked object the primary one
			state.selectedObject = picked;
		}

		// Select and Move tools both allow dragging objects with the mouse
		if (picked && !picked->locked && (state.toolIndex == 0 || state.toolIndex == 1))
		{
			std::vector<SceneObject*> objectsToDrag;

			if (pickedAlreadySelected && !state.sceneSelection.empty())
				objectsToDrag = state.sceneSelection;
			else
				objectsToDrag.push_back(picked);

			state.viewportDragItems.clear();
			state.viewportDragStartMouse = mouse;

			for (SceneObject* obj : objectsToDrag)
			{
				if (!obj || obj->locked)
					continue;

				ViewportDragItem item;
				item.object = obj;
				item.startPos[0] = obj->position[0];
				item.startPos[1] = obj->position[1];
				item.startPos[2] = obj->position[2];
				state.viewportDragItems.push_back(item);
			}

			state.viewportDragging = !state.viewportDragItems.empty();
		}
		else if (!picked && state.toolIndex == 0)
		{
			// drag on empty space with the Select tool -> marquee selection
			state.viewportRectSelecting = true;
			state.viewportRectStart = mouse;
			state.viewportRectEnd = mouse;
		}
	}

	if (state.viewportDragging)
	{
		if (!mouseDownNow)
		{
			state.viewportDragging = false;
			state.viewportDragItems.clear();
		}
		else
		{
			f32 dx = (mouse.x - state.viewportDragStartMouse.x) / zoom;
			f32 dz = (mouse.y - state.viewportDragStartMouse.y) / zoom;

			// free movement while dragging - no grid snapping
			for (const ViewportDragItem& item : state.viewportDragItems)
			{
				item.object->position[0] = item.startPos[0] + dx;
				item.object->position[2] = item.startPos[2] + dz;
			}
		}
	}

	if (state.viewportRectSelecting)
	{
		if (!mouseDownNow)
		{
			const hui::Rect rect = rectFromPoints(state.viewportRectStart, state.viewportRectEnd);

			state.sceneSelection.clear();
			state.selectedObject = nullptr;

			for (auto& objPtr : state.objects)
			{
				SceneObject* obj = objPtr.get();

				if (!obj->visible)
					continue;

				const hui::Point p = sceneWorldToView(vp, zoom, state.viewPan, obj->position[0], obj->position[2]);

				if (rect.contains(p))
				{
					state.sceneSelection.push_back(obj);

					if (!state.selectedObject)
						state.selectedObject = obj;
				}
			}

			state.viewportRectSelecting = false;
			hui::forceRepaint();
		}
		else
		{
			state.viewportRectEnd = mouse;
		}
	}

	// entities
	for (auto& objPtr : state.objects)
	{
		SceneObject* obj = objPtr.get();

		if (!obj->visible)
			continue;

		hui::Point p = sceneWorldToView(vp, zoom, state.viewPan, obj->position[0], obj->position[2]);
		bool selected = isObjectSelected(state, obj);

		// bounds used for the selection outline; a little larger than the object itself
		f32 outlinePad = 6.0f * hui::scaleGet();
		hui::Rect outline = { p.x - 12.0f - outlinePad, p.y - 12.0f - outlinePad,
			24.0f + 2.0f * outlinePad, 24.0f + 2.0f * outlinePad };

		if (strcmp(obj->type, "Light") == 0)
		{
			hui::renderSetFillStyle(hui::FillStyle(obj->color));
			hui::renderDrawSolidCircle(p, 8.0f, 16);

			hui::renderSetLineStyle(hui::LineStyle(hui::Color::white, 1.0f));
			hui::renderDrawLine({ p.x - 12, p.y }, { p.x + 12, p.y });
			hui::renderDrawLine({ p.x, p.y - 12 }, { p.x, p.y + 12 });
		}
		else if (strcmp(obj->type, "Camera") == 0)
		{
			hui::Point a = { p.x, p.y - 14 };
			hui::Point b = { p.x - 14, p.y + 12 };
			hui::Point c = { p.x + 14, p.y + 12 };

			hui::renderSetFillStyle(hui::FillStyle(obj->color));
			hui::renderDrawSolidTriangle(a, b, c);
		}
		else if (strcmp(obj->type, "Marker") == 0)
		{
			hui::renderSetLineStyle(hui::LineStyle(obj->color, 2.0f));
			hui::renderDrawLine({ p.x, p.y - 12 }, { p.x + 12, p.y });
			hui::renderDrawLine({ p.x + 12, p.y }, { p.x, p.y + 12 });
			hui::renderDrawLine({ p.x, p.y + 12 }, { p.x - 12, p.y });
			hui::renderDrawLine({ p.x - 12, p.y }, { p.x, p.y - 12 });
		}
		else
		{
			f32 half = 16.0f * obj->scale[0] * zoom;
			hui::Rect rc = { p.x - half, p.y - half, half * 2.0f, half * 2.0f };
			outline = { rc.x - outlinePad, rc.y - outlinePad,
				rc.width + 2.0f * outlinePad, rc.height + 2.0f * outlinePad };

			hui::renderSetFillStyle(hui::FillStyle(obj->color));
			hui::renderDrawSolidRectangle(rc);
			hui::renderSetLineStyle(hui::LineStyle(hui::Color::fromU8(20, 20, 24), 1.0f));
			hui::renderDrawRectangle(rc);
		}

		if (selected)
		{
			const hui::Color outlineColor = obj->locked ? hui::Color::red : hui::Color::orange;
			drawDashedRect(outline, outlineColor, 1.5f, 4.0f, 4.0f);
		}

		hui::renderSetColor(selected ? hui::Color::orange : hui::Color::lightGray);
		hui::renderDrawTextInBox(obj->name, { p.x - 60, p.y + 18, 120, 16 }, hui::HAlignType::Center, hui::VAlignType::Top);
	}

	if (state.viewportRectSelecting)
	{
		const hui::Rect marquee = rectFromPoints(state.viewportRectStart, state.viewportRectEnd);

		if (marquee.width > 0.5f || marquee.height > 0.5f)
		{
			hui::renderSetFillStyle(hui::FillStyle(hui::Color::fromU8(70, 140, 220, 40)));
			hui::renderDrawSolidRectangle(marquee);
			drawDashedRect(marquee, hui::Color::fromU8(120, 190, 255), 1.5f, 4.0f, 4.0f);
		}
	}

	// status overlay
	const char* toolName = "Select";

	if (state.toolIndex == 1)
		toolName = "Move";
	else if (state.toolIndex == 2)
		toolName = "Rotate";
	else if (state.toolIndex == 3)
		toolName = "Scale";

	char selectionText[128];

	if (state.sceneSelection.size() > 1)
		snprintf(selectionText, sizeof(selectionText), "%u selected", (u32)state.sceneSelection.size());
	else
		snprintf(selectionText, sizeof(selectionText), "%s", state.selectedObject ? state.selectedObject->name : "no selection");

	char status[192];
	snprintf(status, sizeof(status), "Objects: %u    Zoom: %.2fx    Tool: %s    %s    %s    Wheel: zoom    MMB: pan",
		(u32)state.objects.size(), state.viewZoom,
		toolName,
		state.snapEnabled ? "SNAP ON" : "SNAP OFF",
		selectionText);

	hui::renderSetColor(hui::Color::fromU8(190, 195, 205));
	hui::renderDrawTextInBox(status, { vp.x + 10, vp.y + 8, 760, 20 }, hui::HAlignType::Left, hui::VAlignType::Top);

	hui::viewportEnd();
}

void drawAssetBrowser(LevelEditorState& state)
{
	u32 selectedCount = 0;
	i32 lastSelected = -1;

	for (u32 i = 0; i < state.assetSelected.size(); i++)
	{
		if (state.assetSelected[i])
		{
			selectedCount++;
			lastSelected = (i32)i;
		}
	}

	std::string path = "/ Content / Assets";

	if (selectedCount == 1 && lastSelected >= 0 && (u32)lastSelected < state.assets.size())
		path += " / " + state.assets[lastSelected].name;
	else if (selectedCount > 1)
		path += " / (" + std::to_string(selectedCount) + " selected)";

	hui::labelCustomColor(path.c_str(), hui::Color::gray);
	hui::sameLine();
	hui::widgetSetNextWidth(0.45f);
	hui::textInput("##assetFilter", state.assetFilter, 128, hui::TextInputFlags::None, "Filter assets...");

	hui::line();

	hui::Rect client = hui::windowGetClientRect();
	hui::Point cursor = hui::widgetGetPosition();

	f32 paneHeight = client.bottom() - cursor.y - 10.0f;

	if (paneHeight < 60.0f)
		paneHeight = 60.0f;

	f32 leftWidth = 130.0f;
	f32 rightX = cursor.x + leftWidth + 8.0f;
	f32 rightWidth = client.right() - rightX - 10.0f;

	if (rightWidth < 40.0f)
		rightWidth = 40.0f;

	hui::paddingPush(hui::PaddingType::Layout, 0);

	// folders pane
	hui::layoutBegin({ cursor.x, cursor.y, leftWidth, paneHeight });

	if (hui::list("##assetFolders", state.folderSelected, hui::ListSelectionMode::Single, state.folders, LevelEditorState::folderCount, paneHeight))
	{
		for (u32 i = 0; i < LevelEditorState::folderCount; i++)
		{
			if (state.folderSelected[i])
			{
				state.selectedFolder = i;
				break;
			}
		}

		std::fill(state.assetSelected.begin(), state.assetSelected.end(), false);
		state.assetSelectionAnchor = -1;
	}

	hui::layoutEnd();

	// assets pane
	hui::layoutBegin({ rightX, cursor.y, rightWidth, paneHeight });

	std::vector<u32> visibleAssets;

	for (u32 i = 0; i < state.assets.size(); i++)
	{
		const AssetItem& asset = state.assets[i];

		if (asset.folder != state.selectedFolder)
			continue;

		if (!containsInsensitive(asset.name, state.assetFilter))
			continue;

		visibleAssets.push_back(i);
	}

	hui::scrollViewBegin("##assetScroll", paneHeight, state.assetScroll, hui::Point(0, 0), hui::ScrollViewFlags::NoHorizontalScroll);

		u32 column = 0;

		for (u32 visibleIndex = 0; visibleIndex < visibleAssets.size(); visibleIndex++)
		{
			u32 i = visibleAssets[visibleIndex];
			const AssetItem& asset = state.assets[i];

			if (column > 0)
				hui::sameLine();

			hui::idPush((u32)i);

			bool selected = (i < state.assetSelected.size()) ? state.assetSelected[i] : false;

			if (hui::imageButton(asset.thumb, 72, 72, 0, selected))
			{
				const hui::InputEvent& ev = hui::inputEventGet();
				bool ctrl = hui::has(ev.mouse.modifiers, hui::KeyModifiers::Control);
				bool shift = hui::has(ev.mouse.modifiers, hui::KeyModifiers::Shift);

				if (shift && state.assetSelectionAnchor >= 0)
				{
					int anchorVisible = -1;
					int clickedVisible = (int)visibleIndex;

					for (u32 v = 0; v < visibleAssets.size(); v++)
					{
						if ((i32)visibleAssets[v] == state.assetSelectionAnchor)
						{
							anchorVisible = (int)v;
							break;
						}
					}

					if (anchorVisible >= 0)
					{
						int from = (std::min)(anchorVisible, clickedVisible);
						int to = (std::max)(anchorVisible, clickedVisible);

						if (!ctrl)
							std::fill(state.assetSelected.begin(), state.assetSelected.end(), false);

						for (int v = from; v <= to; v++)
						{
							u32 idx = visibleAssets[(u32)v];

							if (idx < state.assetSelected.size())
								state.assetSelected[idx] = true;
						}
					}
					else
					{
						if (!ctrl)
							std::fill(state.assetSelected.begin(), state.assetSelected.end(), false);

						if (i < state.assetSelected.size())
							state.assetSelected[i] = true;

						state.assetSelectionAnchor = (i32)i;
					}
				}
				else if (ctrl)
				{
					if (i < state.assetSelected.size())
						state.assetSelected[i] = !state.assetSelected[i];

					state.assetSelectionAnchor = (i32)i;
				}
				else
				{
					std::fill(state.assetSelected.begin(), state.assetSelected.end(), false);

					if (i < state.assetSelected.size())
						state.assetSelected[i] = true;

					state.assetSelectionAnchor = (i32)i;
				}
			}

			if ((strcmp(asset.type, "Mesh") == 0 || strcmp(asset.type, "Prefab") == 0) && hui::dragDropWantsTo())
				hui::dragDropBegin(assetDragDropType, (void*)(uintptr_t)i);

			hui::tooltip(asset.name.c_str());

			hui::Rect btnRect = hui::widgetGetRect();
			hui::idPop();

			hui::renderSetColor(hui::Color::lightGray);
			hui::renderDrawTextInBox(asset.name.c_str(), { btnRect.x, btnRect.bottom() + 2.0f, btnRect.width, 16.0f }, hui::HAlignType::Center, hui::VAlignType::Top);

			column = (column + 1) % 4;
		}

	state.assetScroll = hui::scrollViewEnd();

	hui::layoutEnd();

	hui::paddingPop(hui::PaddingType::Layout);
}

void drawIconBrowser(LevelEditorState& state)
{
	hui::labelCustomColor("Double-click an icon to copy its name", hui::Color::gray);
	hui::sameLine();
	hui::widgetSetNextWidth(0.45f);
	hui::textInput("##iconFilter", state.iconFilter, (u32)sizeof(state.iconFilter), hui::TextInputFlags::None, "Filter icons...");
	hui::line();

	std::vector<const std::string*> visibleIcons;

	for (const std::string& name : gMaterialIconNames)
	{
		if (!containsInsensitive(name, state.iconFilter))
			continue;

		visibleIcons.push_back(&name);
	}

	char countText[64];
	snprintf(countText, sizeof(countText), "%u icons", (u32)visibleIcons.size());
	hui::label(countText);
	hui::line();

	hui::Rect client = hui::windowGetClientRect();
	hui::Point cursor = hui::widgetGetPosition();
	f32 paneHeight = client.bottom() - cursor.y - 10.0f;

	if (paneHeight < 60.0f)
		paneHeight = 60.0f;

	hui::paddingPush(hui::PaddingType::Layout, 0);

	hui::scrollViewBegin("##iconBrowserScroll", paneHeight, state.iconBrowserScroll, hui::Point(0, 0), hui::ScrollViewFlags::NoHorizontalScroll);

	f32 scale = hui::scaleGet();
	f32 spacing = hui::sameLineSpacingGet() * scale;
	f32 cellWidth = 40.0f * scale + spacing;
	u32 columns = (u32)(std::max)(1.0f, std::floor((hui::layoutGetSize().x + spacing) / cellWidth));

	u32 column = 0;

	for (const std::string* name : visibleIcons)
	{
		if (column > 0)
			hui::sameLine();

		hui::idPush(name->c_str());

		hui::HImage icon = materialIcon(name->c_str());

		if (icon && hui::imageButton(icon, 40.0f, 40.0f))
		{
			const hui::InputEvent& ev = hui::inputEventGet();

			if (ev.mouse.clickCount >= 2 && hui::clipboardSetText(name->c_str()))
				appendLog(state, LogEntry::Level::Info, ("Copied icon name: " + *name).c_str());
		}

		// Give the icon preview tooltip a dark gray background. The popup draws
		// its body with the TooltipBody element color directly, so override it
		// only while this tooltip is being built and restore it afterwards.
		hui::WidgetElementInfo tooltipBodyInfo;
		hui::themeGetWidgetElementInfo(hui::WidgetElementId::TooltipBody, hui::WidgetStateType::Normal, tooltipBodyInfo);
		const hui::Color originalTooltipColor = tooltipBodyInfo.color;
		tooltipBodyInfo.color = hui::Color::fromU8(45, 45, 48);
		hui::themeSetWidgetElement(hui::themeGet(), hui::WidgetElementId::TooltipBody, hui::WidgetStateType::Normal, tooltipBodyInfo);

		if (hui::customTooltipBegin(140.0f))
		{
			if (icon)
			{
				f32 s = hui::scaleGet();
				f32 iconSize = 48.0f * s;
				hui::Point iconPos = hui::widgetGetPosition();
				f32 layoutWidth = hui::layoutGetSize().x;
				iconPos.x += (layoutWidth - iconSize) * 0.5f;
				hui::renderSetColor(hui::Color::white);
				hui::renderDrawStretchedImage(icon, { iconPos.x, iconPos.y, iconSize, iconSize });
				hui::space(48.0f);
			}
			hui::label(name->c_str());
			hui::customTooltipEnd();
		}

		tooltipBodyInfo.color = originalTooltipColor;
		hui::themeSetWidgetElement(hui::themeGet(), hui::WidgetElementId::TooltipBody, hui::WidgetStateType::Normal, tooltipBodyInfo);

		hui::idPop();

		column = (column + 1) % columns;
	}

	state.iconBrowserScroll = hui::scrollViewEnd();

	hui::paddingPop(hui::PaddingType::Layout);
}

void drawLogWindow(LevelEditorState& state)
{
	u32 selectedLogCount = 0;

	for (const LogEntry& entry : state.log)
	{
		if (entry.selected)
			selectedLogCount++;
	}

	hui::sameLineGroupBegin(5);
	hui::check("Info##logFilterInfo", &state.logShowInfo);
	hui::sameLineGroupNext();
	hui::check("Warning##logFilterWarning", &state.logShowWarning);
	hui::sameLineGroupNext();
	hui::check("Error##logFilterError", &state.logShowError);
	hui::sameLineGroupNext();

	if (hui::button("Clear"))
	{
		state.log.clear();
		state.logSelectionAnchor = -1;
	}

	hui::sameLineGroupNext();

	char selectedText[32];
	snprintf(selectedText, sizeof(selectedText), "%u selected", selectedLogCount);
	hui::label(selectedText);

	hui::sameLineGroupEnd();

	hui::line();

	f32 viewHeight = hui::layoutGetRemainingHeight();
	if (viewHeight <= 0) viewHeight = 200.0f;
	hui::scrollViewBegin("##logScroll", viewHeight, state.logScroll, hui::Point(0, 0), hui::ScrollViewFlags::NoBorder | hui::ScrollViewFlags::NoPadding | hui::ScrollViewFlags::NoHorizontalScroll);

	hui::Point contentStart = hui::widgetGetPosition();

	std::vector<u32> visibleLogIndices;

	for (u32 i = 0; i < state.log.size(); i++)
	{
		const LogEntry& entry = state.log[i];

		if (entry.level == LogEntry::Level::Info && !state.logShowInfo)
			continue;

		if (entry.level == LogEntry::Level::Warning && !state.logShowWarning)
			continue;

		if (entry.level == LogEntry::Level::Error && !state.logShowError)
			continue;

		visibleLogIndices.push_back(i);
	}

	auto drawLogContextMenu = [&]()
	{
		if (hui::contextMenuBegin())
		{
			bool anySelected = false;

			for (const LogEntry& entry : state.log)
			{
				if (entry.selected)
				{
					anySelected = true;
					break;
				}
			}

			hui::widgetPushDisabled(!anySelected);

			if (hui::menuItem("Copy", "Ctrl+C"))
				copySelectedLogEntries(state);

			hui::widgetPopDisabled();
			hui::contextMenuEnd();
		}
	};

	// rows table, same column setup as the header so the dividers line up
	if (hui::tableBegin("##logTableBody", 2, 0, hui::TableFlags::Borders | hui::TableFlags::Stretch | hui::TableFlags::AltRowBg))
	{
		hui::tableColumnSetup(0, 80.0f, hui::TableColumnFlags::Fixed);
		hui::tableColumnSetup(1, 0.0f, hui::TableColumnFlags::Stretch);

		for (u32 visibleIndex = 0; visibleIndex < visibleLogIndices.size(); visibleIndex++)
		{
			u32 i = visibleLogIndices[visibleIndex];
			LogEntry& entry = state.log[i];

			hui::tableRowNext();
			hui::idPush((u32)i);

			hui::Rect typeCellRect = hui::tableCellGetRect();

			hui::Color highlight = hui::Color::fromU8(0, 140, 195, 255); // dark azure

			if (entry.selected)
			{
				hui::tableRowSetColor(highlight);
				hui::tableCellSetColor(highlight);
			}

			const char* typeName = "Info";
			hui::Color color = hui::Color::lightGray;

			if (entry.level == LogEntry::Level::Warning)
			{
				typeName = "Warning";
				color = hui::Color::yellow;
			}
			else if (entry.level == LogEntry::Level::Error)
			{
				typeName = "Error";
				color = hui::Color::red;
			}

			// force the label to span the whole cell so clicks anywhere on the row register,
			// not just on the text glyphs (labels otherwise shrink to text width)
			hui::widgetSetNextWidth((typeCellRect.width - 2.0f * hui::scaleGet()) / hui::scaleGet());
			hui::labelCustomColor(typeName, color);
			bool typeClicked = hui::widgetIsClicked();
			bool typeHovered = hui::widgetIsHovered();
			drawLogContextMenu();

			hui::tableCellNext();

			if (entry.selected)
				hui::tableCellSetColor(highlight);

			hui::Rect msgCellRect = hui::tableCellGetRect();
			hui::widgetSetNextWidth((msgCellRect.width - 2.0f * hui::scaleGet()) / hui::scaleGet());
			hui::labelCustomColor(entry.text.c_str(), hui::Color::lightGray);
			bool msgClicked = hui::widgetIsClicked();
			bool msgHovered = hui::widgetIsHovered();
			drawLogContextMenu();

			const hui::InputEvent& ev = hui::inputEventGet();
			bool ctrl = hui::has(ev.mouse.modifiers, hui::KeyModifiers::Control);
			bool shift = hui::has(ev.mouse.modifiers, hui::KeyModifiers::Shift);

			if (typeClicked || msgClicked)
			{
				selectLogRow(state, visibleLogIndices, visibleIndex, i, ctrl, shift);
			}
			else if ((typeHovered || msgHovered)
				&& ev.type == hui::InputEvent::Type::MouseDown
				&& ev.mouse.button == hui::MouseButton::Right
				&& !entry.selected)
			{
				// right-clicking an already selected row keeps the multi-selection intact
				selectLogRow(state, visibleLogIndices, visibleIndex, i, false, false);
			}

			hui::idPop();
		}

		hui::tableEnd();
	}

	hui::Point contentEnd = hui::widgetGetPosition();
	state.logScroll = hui::scrollViewEnd();

	f32 contentHeight = contentEnd.y - contentStart.y;
	f32 maxScroll = (std::max)(0.0f, contentHeight - viewHeight);
	state.logAutoScroll = (state.logScroll.y >= maxScroll - 5.0f);
}

int main(int argc, char** args)
{
	horusExamples::makeProcessDpiAware();

	// initialize SDL input provider
	hui::Sdl3InitParams sdlParams;

	sdlParams.vSync = true;
	//sdlParams.gfxApi = hui::Sdl3GfxApi::OpenGL;
	//sdlParams.gfxApi = hui::Sdl3GfxApi::DX11;
	//sdlParams.gfxApi = hui::Sdl3GfxApi::DX12;
	sdlParams.gfxApi = hui::Sdl3GfxApi::Vulkan;

	// setup a Horus UI context, with given service providers
	hui::Settings settings;

	settings.dockNodeSpacing = 3;
	settings.dockNodeResizeSplitterHitSize = 8;
	settings.fpsThrottleEnable = true;

	hui::initStdioFileIO(settings.services);
	hui::initFreetype(settings.services);
	hui::initSdl3(settings.services, sdlParams);
	hui::initStbRectPack(settings.services);
	hui::initUtf(settings.services);

	std::string gfxApiName;

	switch (sdlParams.gfxApi)
	{
	case hui::Sdl3GfxApi::OpenGL:
		gfxApiName = "OpenGL";
		hui::initOpenGL(settings.services);
		break;
	case hui::Sdl3GfxApi::DX11:
		gfxApiName = "DX11";
		hui::initDx11(settings.services);
		break;
	case hui::Sdl3GfxApi::DX12:
		gfxApiName = "DX12";
		hui::initDx12(settings.services);
		break;
	case hui::Sdl3GfxApi::Vulkan:
		gfxApiName = "Vulkan";
		hui::initVulkan(settings.services);
		break;
	}

	auto huiContext = hui::contextCreate(settings);
	hui::contextSet(huiContext); // set as current context

	// create the main window (this will also create a graphics (GL/VK/D3D/etc.) context)
	auto mainWnd = hui::contextGetSettings().services.createWindow((std::string("Horus Example - Level Editor - ") + gfxApiName).c_str(), hui::NativeWindowFlags::Resizable, hui::NativeWindowState::Maximized, hui::Rect(0, 0, 1500, 850));

	// create a main dock node for the main window, so we can dock windows in there
	hui::DockNodeId mainDockNode = hui::dockNodeCreateRoot(mainWnd);

	// build the editor layout by splitting dock nodes around
	buildDefaultLayout(mainDockNode);

	// load a theme
	const u32 errSize = 2048;
	char err[errSize] = { 0 };

	auto theme = hui::loadThemeFromJson("../themes/default.theme.json", err, errSize);

	if (!theme)
	{
		printf("Theme JSON error: %s\n", err);
		theme = hui::themeCreate(hui::contextGetSettings().defaultAtlasSize);

		// initialize all elements with white image
		hui::WidgetElementInfo defInfo;
		defInfo.image = hui::themeGetImage(theme, "__WHITEIMAGE__");
		defInfo.color = hui::Color::white;
		defInfo.textColor = hui::Color::white;

		for (u32 i = 0; i < (u32)hui::WidgetElementId::Count; i++)
		{
			for (u32 j = 0; j < (u32)hui::WidgetStateType::Count; j++)
			{
				hui::themeSetWidgetElement(theme, (hui::WidgetElementId)i, (hui::WidgetStateType)j, defInfo);
			}
		}
	}

	hui::themeSet(theme);

	LevelEditorState state;

	if (!loadMaterialIconAtlas("../themes/icons/material_icons.png", "../themes/icons/material_icons.json", &gMaterialIconNames))
		printf("Warning: could not load the Material icon atlas\n");

	loadThemeImages(state);

	// build the theme into the atlas after all images were added
	hui::themeBuild(theme);

	// the main menu window must be exactly as tall as the menu bar drawn inside it
	hui::WidgetElementInfo menuBarInfo;
	hui::themeGetWidgetElementInfo(hui::WidgetElementId::MenuBarBody, hui::WidgetStateType::Normal, menuBarInfo);

	hui::OpenGLTexture texAtlasGL;
	hui::Dx11Texture texAtlasDX11;
	hui::Dx12Texture texAtlasDX12;
	hui::VulkanTexture texAtlasVK;

	auto uploadThemeAtlas = [&]()
	{
		switch (sdlParams.gfxApi)
		{
		case hui::Sdl3GfxApi::OpenGL:
			texAtlasGL.resize(hui::themeGetAtlasImageData().width, hui::themeGetAtlasImageData().height);
			texAtlasGL.updateData(hui::themeGetAtlasImageData().pixels);
			hui::themeSetAtlasTexture(texAtlasGL.getHandle());
			break;
		case hui::Sdl3GfxApi::DX11:
			texAtlasDX11.resize(hui::themeGetAtlasImageData().width, hui::themeGetAtlasImageData().height);
			texAtlasDX11.updateData(hui::themeGetAtlasImageData().pixels);
			hui::themeSetAtlasTexture(texAtlasDX11.getHandle());
			break;
		case hui::Sdl3GfxApi::DX12:
			texAtlasDX12.resize(hui::themeGetAtlasImageData().width, hui::themeGetAtlasImageData().height);
			texAtlasDX12.updateData(hui::themeGetAtlasImageData().pixels);
			hui::themeSetAtlasTexture(texAtlasDX12.getHandle());
			break;
		case hui::Sdl3GfxApi::Vulkan:
			texAtlasVK.resize(hui::themeGetAtlasImageData().width, hui::themeGetAtlasImageData().height);
			texAtlasVK.updateData(hui::themeGetAtlasImageData().pixels);
			hui::themeSetAtlasTexture(texAtlasVK.getHandle());
			break;
		}
	};

	uploadThemeAtlas();

	buildDummyScene(state);
	buildDummyAssets(state);

	appendLog(state, LogEntry::Level::Info, "Level editor started");
	appendLog(state, LogEntry::Level::Info, "Loaded scene /Content/Levels/Blockout.level");
	appendLog(state, LogEntry::Level::Warning, "WARNING: 3 objects use deprecated LOD settings");

	// restore the docking layout saved in the previous session, if there is one
	const char* layoutFilename = "docking_state.json";

	if (hui::dockingStateLoadFromFile(layoutFilename, levelEditorLayoutVersion))
	{
		appendLog(state, LogEntry::Level::Info, "Docking layout restored");

		// layouts saved before the dock area existed do not carry it
		if (!hui::dockNodeGetDockArea())
			hui::dockNodeSetDockAreaByWindow("viewport");

		// older layouts had a separate toolbar window, it is merged into the menu now
		hui::windowSetVisible("main_toolbar", false);

		// the one-time icon browser dock below must not override a layout that already
		// places it properly; if it ended up in the menu chrome node, redock it
		hui::WindowsDockingState loadedLayout;
		hui::dockingStateSaveInMemory(loadedLayout);
		state.iconBrowserDocked = layoutStateHasWindow(loadedLayout, "icon_browser")
			&& !layoutWindowsShareNode(loadedLayout, "icon_browser", "main_menu");
	}

	// sync the View menu toggles with the restored layout
	state.showViewport = hui::windowIsVisible("viewport");
	state.showSceneTree = hui::windowIsVisible("scene_tree");
	state.showInspector = hui::windowIsVisible("inspector");
	state.showAssetBrowser = hui::windowIsVisible("asset_browser");
	state.showIconBrowser = hui::windowIsVisible("icon_browser");
	state.showLog = hui::windowIsVisible("log");

	bool exitNow = false;
	bool layoutSavedOnClose = false;

	while (!exitNow)
	{
		HUI_SERVICES.setCurrentWindow(mainWnd);
		HUI_SERVICES.clearBackbuffer({ 0.10f, 0.10f, 0.12f, 1 });

		hui::contextGetSettings().deltaTime = hui::getSdl3DeltaTime();

		f32 dt = hui::contextGetSettings().deltaTime;

		state.logTimer += dt;

		if (state.logTimer >= 1.75f)
		{
			state.logTimer = 0;
			appendNextDummyLog(state);
			hui::forceRepaint();
		}

		// get the events from SDL or whatever input provider is set, it will fill a queue of events
		hui::contextUpdate();

		auto eventCount = hui::inputEventGetCount();

		// the main frame rendering and input handling
		auto doFrame = [&](bool lastEventInQueue)
		{
			if (state.pendingUiScale > 0)
			{
				state.uiScale = state.pendingUiScale;
				state.pendingUiScale = 0;

				hui::scaleSet(state.uiScale);
				hui::themeBuild(theme);
				uploadThemeAtlas();

				hui::themeGetWidgetElementInfo(
					hui::WidgetElementId::MenuBarBody,
					hui::WidgetStateType::Normal,
					menuBarInfo);

				char scaleLog[64];
				snprintf(scaleLog, sizeof(scaleLog), "UI scale set to %.0f%%", state.uiScale * 100.0f);
				appendLog(state, LogEntry::Level::Info, scaleLog);
			}

			hui::frameBegin();
			hui::skipRenderingThisFrame(!lastEventInQueue);

			// the menu strip and toolbar share one dockable chrome window, without a tab bar
			hui::windowSetNextFlags(hui::WindowFlags::NoTab | hui::WindowFlags::NoResize | hui::WindowFlags::NoPadding);
			hui::windowSetNextSize(0, menuBarInfo.height + state.iconButtonSize);

			if (hui::windowBegin("main_menu", "Main Menu", nullptr, state.iconMenu))
			{
				drawMainMenu(state, exitNow);
				drawMainToolbar(state);
				hui::windowEnd();
			}

			if (hui::windowBegin("log", "Log", nullptr, state.iconLog))
			{
				drawLogWindow(state);
				hui::windowEnd();
			}

			if (hui::windowBegin("scene_tree", "Scene Tree", nullptr, state.iconTree))
			{
				drawSceneTree(state);
				hui::windowEnd();
			}

			if (hui::windowBegin("viewport", "Viewport", nullptr, state.iconViewport))
			{
				drawViewport(state);
				hui::windowEnd();
			}

			if (hui::windowBegin("inspector", "Object Inspector", nullptr, state.iconInspector))
			{
				drawInspector(state);
				hui::windowEnd();
			}

			if (hui::windowBegin("asset_browser", "Asset Browser", nullptr, state.iconAssets))
			{
				drawAssetBrowser(state);
				hui::windowEnd();
			}

			if (hui::windowBegin("icon_browser", "Icon Browser", nullptr, materialIcon("search")))
			{
				drawIconBrowser(state);
				hui::windowEnd();
			}

			hui::frameEnd();

			// a layout saved before the icon browser existed cannot place it, so dock it once
			if (!state.iconBrowserDocked && hui::windowIsVisible("icon_browser"))
			{
				hui::windowDock("icon_browser", "asset_browser", hui::DockType::Bottom);
				state.iconBrowserDocked = true;
			}

			// the layout save and load must happen outside the window pass
			if (state.saveLayoutRequested)
			{
				state.saveLayoutRequested = false;

				if (hui::dockingStateSaveToFile(layoutFilename, levelEditorLayoutVersion))
					appendLog(state, LogEntry::Level::Info, "Docking layout saved");
				else
					appendLog(state, LogEntry::Level::Warning, "Failed to save the docking layout");
			}

			if (state.loadLayoutRequested)
			{
				state.loadLayoutRequested = false;

				if (hui::dockingStateLoadFromFile(layoutFilename, levelEditorLayoutVersion))
				{
					appendLog(state, LogEntry::Level::Info, "Docking layout loaded");

					// layouts saved before the dock area existed do not carry it
					if (!hui::dockNodeGetDockArea())
						hui::dockNodeSetDockAreaByWindow("viewport");

					// older layouts had a separate toolbar window, it is merged into the menu now
					hui::windowSetVisible("main_toolbar", false);

					// sync the View menu toggles with the loaded layout
					state.showViewport = hui::windowIsVisible("viewport");
					state.showSceneTree = hui::windowIsVisible("scene_tree");
					state.showInspector = hui::windowIsVisible("inspector");
					state.showAssetBrowser = hui::windowIsVisible("asset_browser");
					state.showIconBrowser = hui::windowIsVisible("icon_browser");
					state.showLog = hui::windowIsVisible("log");
				}
				else
				{
					appendLog(state, LogEntry::Level::Warning, "Docking layout is missing or older than this version, resetting to default");
					state.resetLayoutRequested = true;
				}
			}

			if (state.resetLayoutRequested)
			{
				state.resetLayoutRequested = false;

				static const char* layoutWindows[] = { "main_menu", "scene_tree", "inspector", "viewport", "asset_browser", "icon_browser", "log" };

				// show all the windows again, then apply the default layout state
				for (auto windowId : layoutWindows)
					hui::windowSetVisible(windowId, true);

				if (hui::dockingStateLoadFromMemory(buildDefaultLayoutState()))
				{
					state.showSceneTree = true;
					state.showInspector = true;
					state.showAssetBrowser = true;
					state.showIconBrowser = true;
					state.showLog = true;
					state.showViewport = true;

					appendLog(state, LogEntry::Level::Info, "Docking layout reset");
				}
			}

			if (lastEventInQueue)
				hui::present();
		};

		// if we have events, then go through all of them and call the frame render and input
		if (eventCount)
		{
			for (int i = 0; i < eventCount; i++)
			{
				hui::InputEvent ev = hui::inputEventGetAtIndex(i);
				hui::inputEventSet(ev);

				if (ev.type == hui::InputEvent::Type::WindowClose)
				{
					if (ev.window == mainWnd)
					{
						// frameBegin tears down the closed native window's docking
						// layout, so save it before the frame is drawn
						hui::dockingStateSaveToFile(layoutFilename, levelEditorLayoutVersion);
						layoutSavedOnClose = true;
						exitNow = true;
					}
				}
				else if (ev.type == hui::InputEvent::Type::Key && ev.key.down)
				{
					// copy selected log lines to clipboard
					if (ev.key.code == hui::KeyCode::C && hui::has(ev.key.modifiers, hui::KeyModifiers::Control))
						copySelectedLogEntries(state);
				}

				doFrame(i == eventCount - 1);
			}
		}
		else
		{
			doFrame(true);
		}
	}

	// remember the docking layout for the next session, unless the app was closed
	// with the OS close button, in which case it was saved before the teardown
	if (!layoutSavedOnClose)
		hui::dockingStateSaveToFile(layoutFilename, levelEditorLayoutVersion);

	hui::contextDestroy(huiContext);

	hui::shutdownStdioFileIO(settings.services);
	hui::shutdownFreetype(settings.services);
	hui::shutdownSdl3(settings.services);
	hui::shutdownStbRectPack(settings.services);
	hui::shutdownUtf(settings.services);

	switch (sdlParams.gfxApi)
	{
	case hui::Sdl3GfxApi::OpenGL:
		hui::shutdownOpenGL(settings.services);
		break;
	case hui::Sdl3GfxApi::DX11:
		hui::shutdownDx11(settings.services);
		break;
	case hui::Sdl3GfxApi::DX12:
		hui::shutdownDx12(settings.services);
		break;
	case hui::Sdl3GfxApi::Vulkan:
		hui::shutdownVulkan(settings.services);
		break;
	}

	hui::shutdown();

	return 0;
}
