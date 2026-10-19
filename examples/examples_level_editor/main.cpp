#pragma execution_character_set("utf-8")
#include "horus.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

// backends
#include "sdl3_input.h"
#include "opengl_graphics.h"
#include "dx11_graphics.h"
#include "dx12_graphics.h"
#include "vulkan_graphics.h"
#include "json_theme_loader.h"
#include "stb_rectpack.h"
#include "freetype_fonts.h"
#include "native_file_dialogs.h"
#include "stdio_fileio.h"
#include "utfcpp.h"

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
};

// a dummy asset shown in the asset browser
struct AssetItem
{
	std::string name;
	const char* type = "Texture";
	u32 folder = 0;
	hui::HImage thumb = 0;
};

// all the level editor state, kept in one place instead of file globals
struct LevelEditorState
{
	std::vector<std::unique_ptr<SceneObject>> objects;
	std::vector<SceneObject*> roots;
	SceneObject* selectedObject = nullptr;
	u32 newObjectCounter = 0;
	hui::Point treeScroll = 0;
	char objectSearch[128] = { 0 };

	std::vector<LogEntry> log;
	hui::Point logScroll = 0;
	bool logShowInfo = true;
	bool logShowWarning = true;
	bool logShowError = true;
	bool logAutoScroll = true;
	f32 logTimer = 0;
	u32 logMessageIndex = 0;

	static const u32 folderCount = 5;
	const char* folders[folderCount] = { "Textures", "Meshes", "Materials", "Audio", "Prefabs" };
	bool folderSelected[folderCount] = { true, false, false, false, false };
	std::vector<AssetItem> assets;
	u32 selectedFolder = 0;
	i32 selectedAsset = -1;
	char assetFilter[128] = { 0 };
	hui::Point assetScroll = 0;

	i32 toolIndex = 0;
	bool gridVisible = true;
	bool snapEnabled = true;
	bool playing = false;
	f32 viewZoom = 1.0f;

	bool showSceneTree = true;
	bool showInspector = true;
	bool showAssetBrowser = true;
	bool showLog = true;

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
	f32 iconButtonSize = 48;
	hui::HImage thumbLena = 0;
	hui::HImage thumbFolder = 0;
	hui::HImage thumbSettings = 0;
	hui::HImage thumbData = 0;
	hui::HImage thumbStacks = 0;
	void* materialRef = nullptr;
	bool materialModified = false;
};

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

	if (state.selectedObject == obj)
		state.selectedObject = nullptr;
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
		state.log.erase(state.log.begin());

	state.log.push_back({ level, text });
	if (state.logAutoScroll)
		state.logScroll.y = 1e9f;
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

void loadThemeImages(LevelEditorState& state)
{
	auto theme = hui::themeGet();

	state.iconMenu = hui::loadThemeImage(theme, "../themes/icons/stacks_FILL0_wght400_GRAD0_opsz20.png");
	state.iconToolbar = hui::loadThemeImage(theme, "../themes/icons/settings_FILL0_wght400_GRAD0_opsz20.png");
	state.iconTree = hui::loadThemeImage(theme, "../themes/icons/icons8-opened-folder-20.png");
	state.iconInspector = hui::loadThemeImage(theme, "../themes/icons/icons8-settings-20.png");
	state.iconViewport = hui::loadThemeImage(theme, "../themes/icons/ic_border_all_white_24dp.png");
	state.iconAssets = hui::loadThemeImage(theme, "../themes/icons/icons8-opened-folder-20.png");
	state.iconLog = hui::loadThemeImage(theme, "../themes/icons/icons8-equivalent-20.png");
	state.iconNew = hui::loadThemeImage(theme, "../themes/icons/stacks_FILL0_wght400_GRAD0_opsz20.png");
	state.iconOpen = hui::loadThemeImage(theme, "../themes/icons/icons8-opened-folder-20.png");
	state.iconSave = hui::loadThemeImage(theme, "../themes/icons/data_check_FILL0_wght400_GRAD0_opsz20.png");
	state.iconUndo = hui::loadThemeImage(theme, "../themes/icons/ic_border_left_white_24dp.png");
	state.iconRedo = hui::loadThemeImage(theme, "../themes/icons/icons8-equivalent-20.png");

	state.thumbLena = hui::loadThemeImage(theme, "../themes/default/lena.png");
	state.thumbFolder = hui::loadThemeImage(theme, "../themes/icons/icons8-opened-folder-20.png");
	state.thumbSettings = hui::loadThemeImage(theme, "../themes/icons/icons8-settings-20.png");
	state.thumbData = hui::loadThemeImage(theme, "../themes/icons/data_check_FILL0_wght400_GRAD0_opsz20.png");
	state.thumbStacks = hui::loadThemeImage(theme, "../themes/icons/stacks_FILL0_wght400_GRAD0_opsz20.png");
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
}

void drawMainMenu(LevelEditorState& state, bool& exitNow)
{
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
			deleteObject(state, state.selectedObject);

		hui::menuEnd();
	}

	if (hui::menuBegin("View"))
	{
		auto toggle = [](bool& value, const char* windowId)
		{
			value = !value;
			hui::windowSetVisible(windowId, value);
		};

		if (hui::menuItem("Scene Tree", "", 0, state.showSceneTree ? hui::SelectableFlags::Checked : hui::SelectableFlags::Normal))
			toggle(state.showSceneTree, "scene_tree");

		if (hui::menuItem("Object Inspector", "", 0, state.showInspector ? hui::SelectableFlags::Checked : hui::SelectableFlags::Normal))
			toggle(state.showInspector, "inspector");

		if (hui::menuItem("Asset Browser", "", 0, state.showAssetBrowser ? hui::SelectableFlags::Checked : hui::SelectableFlags::Normal))
			toggle(state.showAssetBrowser, "asset_browser");

		if (hui::menuItem("Log", "", 0, state.showLog ? hui::SelectableFlags::Checked : hui::SelectableFlags::Normal))
			toggle(state.showLog, "log");

		hui::menuSeparator();

		if (hui::menuItem("Reset Layout"))
			appendLog(state, LogEntry::Level::Info, "Layout reset requested");

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

void drawObjectNode(LevelEditorState& state, SceneObject* obj)
{
	auto flags = (state.selectedObject == obj) ? hui::SelectableFlags::Selected : hui::SelectableFlags::Normal;

	hui::idPush((void*)obj);

	bool clicked = false;

	if (obj->children.empty())
	{
		// leaf nodes are simple selectables, no expand arrow
		clicked = hui::selectable(obj->name, flags);
	}
	else
	{
		bool expanded = hui::treeNodeBegin(obj->name, &obj->expanded, flags, hui::TreeNodeFlags::Normal);
		clicked = hui::widgetIsClicked();

		if (expanded)
		{
			for (SceneObject* child : obj->children)
				drawObjectNode(state, child);

			hui::treeNodeEnd();
		}
	}

	if (clicked)
		state.selectedObject = obj;

	hui::idPop();
}

void drawSceneTree(LevelEditorState& state)
{
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
		deleteObject(state, state.selectedObject);

	hui::line();

	hui::scrollViewBegin("##treeScroll", hui::layoutGetRemainingHeight(), state.treeScroll, hui::Point(0, 0), hui::ScrollViewFlags::None);

	for (SceneObject* root : state.roots)
		drawObjectNode(state, root);

	state.treeScroll = hui::scrollViewEnd();
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

	hui::line();

	hui::labelCustomColor("Name", hui::Color::gray);
	hui::widgetSetNextWidth(1);
	hui::textInput("##objectName", obj->name, sizeof(obj->name));

	hui::labelCustomColor("Type", hui::Color::gray);
	hui::sameLine();
	hui::labelCustomColor(obj->type, hui::Color::yellow);

	hui::space();

	if (hui::expandableBegin("Transform", &obj->transformExpanded))
	{
		hui::label("Position");
		hui::vec3Editor("##position", obj->position[0], obj->position[1], obj->position[2], 0.05f);

		hui::label("Rotation");
		hui::vec3Editor("##rotation", obj->rotation[0], obj->rotation[1], obj->rotation[2], 0.5f);

		hui::label("Scale");
		hui::vec3Editor("##scale", obj->scale[0], obj->scale[1], obj->scale[2], 0.02f);

		hui::expandableEnd();
	}

	if (hui::expandableBegin("Rendering", &obj->renderingExpanded))
	{
hui::label("Mesh");
	hui::sameLine();
	hui::dropdown("##meshSelect", obj->meshIndex, meshItems, 6);

		hui::label("Material");
		hui::objectRefEditor("##materialRef", state.iconInspector, state.iconAssets, state.iconInspector, "Material", "M_Default", 1, &state.materialRef, &state.materialModified);

		hui::label("Tint Color");
		hui::colorPickerPopup("##tintColor", &obj->color, hui::ColorPickerFlags::ShowPalette);

		hui::check("Visible", &obj->visible);
		hui::check("Static", &obj->isStatic);

		hui::expandableEnd();
	}

	if (hui::expandableBegin("Physics", &obj->physicsExpanded))
	{
		hui::check("Generate Collision", &obj->collision);
		hui::check("Is Trigger", &obj->trigger);

		if (obj->collision && obj->trigger)
			hui::labelCustomColor("Triggers are not solid, overlap events only.", hui::Color::gray);

		hui::expandableEnd();
	}
}

void drawViewport(LevelEditorState& state)
{
	// the viewport is a passive drawing area, disable interaction so it does not eat input
	hui::widgetSetNextDisabled();

	hui::Rect vp = hui::viewportBegin("##levelViewport", 0);

	hui::renderSetFillStyle(hui::FillStyle(hui::Color::fromU8(28, 30, 36)));
	hui::renderDrawSolidRectangle(vp);

	hui::Point center = vp.center();
	f32 zoom = state.viewZoom;
	f32 gridStep = 40.0f * zoom;

	// ground grid
	if (state.gridVisible)
	{
		hui::renderSetLineStyle(hui::LineStyle(hui::Color::fromU8(52, 56, 66), 1.0f));

		for (f32 x = center.x; x < vp.right(); x += gridStep)
		{
			hui::renderDrawLine({ x, vp.top() }, { x, vp.bottom() });

			if (x - gridStep >= vp.left())
				hui::renderDrawLine({ x - gridStep, vp.top() }, { x - gridStep, vp.bottom() });
		}

		for (f32 y = center.y; y < vp.bottom(); y += gridStep)
		{
			hui::renderDrawLine({ vp.left(), y }, { vp.right(), y });

			if (y - gridStep >= vp.top())
				hui::renderDrawLine({ vp.left(), y - gridStep }, { vp.right(), y - gridStep });
		}
	}

	// world axes
	hui::renderSetLineStyle(hui::LineStyle(hui::Color::fromU8(150, 60, 60), 2.0f));
	hui::renderDrawLine({ vp.left(), center.y }, { vp.right(), center.y });

	hui::renderSetLineStyle(hui::LineStyle(hui::Color::fromU8(60, 150, 70), 2.0f));
	hui::renderDrawLine({ center.x, vp.top() }, { center.x, vp.bottom() });

	// entities
	for (auto& objPtr : state.objects)
	{
		SceneObject* obj = objPtr.get();

		if (!obj->visible)
			continue;

		hui::Point p = { center.x + obj->position[0] * zoom, center.y + obj->position[2] * zoom };
		bool selected = (state.selectedObject == obj);

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

			hui::renderSetFillStyle(hui::FillStyle(obj->color));
			hui::renderDrawSolidRectangle(rc);
			hui::renderSetLineStyle(hui::LineStyle(hui::Color::fromU8(20, 20, 24), 1.0f));
			hui::renderDrawRectangle(rc);
		}

		if (selected)
		{
			hui::renderSetLineStyle(hui::LineStyle(hui::Color::orange, 2.0f));
			hui::renderDrawCircle(p, 24.0f * zoom, 28);
		}

		hui::renderSetColor(selected ? hui::Color::orange : hui::Color::lightGray);
		hui::renderDrawTextInBox(obj->name, { p.x - 60, p.y + 18, 120, 16 }, hui::HAlignType::Center, hui::VAlignType::Top);
	}

	// status overlay
	char status[192];
	snprintf(status, sizeof(status), "Objects: %u    Zoom: %.2fx    Tool: %s    %s",
		(u32)state.objects.size(), state.viewZoom,
		state.snapEnabled ? "SNAP ON" : "SNAP OFF",
		state.selectedObject ? state.selectedObject->name : "no selection");

	hui::renderSetColor(hui::Color::fromU8(190, 195, 205));
	hui::renderDrawTextInBox(status, { vp.x + 10, vp.y + 8, 700, 20 }, hui::HAlignType::Left, hui::VAlignType::Top);

	hui::viewportEnd();
}

void drawAssetBrowser(LevelEditorState& state)
{
	std::string path = "/ Content / Assets";

	if (state.selectedAsset >= 0 && (u32)state.selectedAsset < state.assets.size())
		path += " / " + state.assets[state.selectedAsset].name;

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
				state.selectedAsset = -1;
				break;
			}
		}
	}

	hui::layoutEnd();

	// assets pane
	hui::layoutBegin({ rightX, cursor.y, rightWidth, paneHeight });

	hui::scrollViewBegin("##assetScroll", paneHeight, state.assetScroll, hui::Point(0, 0), hui::ScrollViewFlags::None);

	u32 column = 0;

	for (u32 i = 0; i < state.assets.size(); i++)
	{
		const AssetItem& asset = state.assets[i];

		if (asset.folder != state.selectedFolder)
			continue;

		if (!containsInsensitive(asset.name, state.assetFilter))
			continue;

		if (column > 0)
hui::sameLine();

		hui::idPush((u32)i);

		bool selected = (state.selectedAsset == (i32)i);

		if (hui::imageButton(asset.thumb, 72, 72, 0, selected))
			state.selectedAsset = (i32)i;

		hui::tooltip(asset.name.c_str());

		hui::idPop();

		column = (column + 1) % 4;
	}

	state.assetScroll = hui::scrollViewEnd();

	hui::layoutEnd();

	hui::paddingPop(hui::PaddingType::Layout);
}

void drawLogWindow(LevelEditorState& state)
{
	hui::sameLineGroupBegin(4);
	hui::check("Info", &state.logShowInfo);
	hui::sameLineGroupNext();
	hui::check("Warning", &state.logShowWarning);
	hui::sameLineGroupNext();
	hui::check("Error", &state.logShowError);
	hui::sameLineGroupNext();

	if (hui::button("Clear"))
		state.log.clear();

	hui::sameLineGroupEnd();

	hui::line();

	f32 viewHeight = hui::layoutGetRemainingHeight();
	if (viewHeight <= 0) viewHeight = 200.0f;
	hui::scrollViewBegin("##logScroll", viewHeight, state.logScroll, hui::Point(0, 0), hui::ScrollViewFlags::None);

	hui::Point contentStart = hui::widgetGetPosition();

	for (const LogEntry& entry : state.log)
	{
		if (entry.level == LogEntry::Level::Info && !state.logShowInfo)
			continue;

		if (entry.level == LogEntry::Level::Warning && !state.logShowWarning)
			continue;

		if (entry.level == LogEntry::Level::Error && !state.logShowError)
			continue;

		const char* prefix = "INFO:     ";
		hui::Color color = hui::Color::lightGray;

		if (entry.level == LogEntry::Level::Warning)
		{
			prefix = "WARNING:  ";
			color = hui::Color::yellow;
		}
		else if (entry.level == LogEntry::Level::Error)
		{
			prefix = "ERROR:    ";
			color = hui::Color::red;
		}

		hui::labelCustomColor((std::string(prefix) + entry.text).c_str(), color);
	}

	hui::Point contentEnd = hui::widgetGetPosition();
	state.logScroll = hui::scrollViewEnd();

	f32 contentHeight = contentEnd.y - contentStart.y;
	f32 maxScroll = std::max<float>(0.0f, contentHeight - viewHeight);
	state.logAutoScroll = (state.logScroll.y >= maxScroll - 5.0f);
}

int main(int argc, char** args)
{
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
	{
		hui::DockNodeId n1, n2;

		// top: menu strip, then the main toolbar strip below it
		hui::dockNodeSplit(mainDockNode, hui::DockNodeSplitType::Top, 0.07f, &n1, &n2);
		hui::dockNodeSetWindow(n1, "main_menu");

		hui::dockNodeSplit(n2, hui::DockNodeSplitType::Top, 0.09f, &n1, &n2);
		hui::dockNodeSetWindow(n1, "main_toolbar");

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

	bool exitNow = false;

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
			hui::frameBegin();
			hui::skipRenderingThisFrame(!lastEventInQueue);

			// the menu strip and toolbar are dockable but have no tab bar and a fixed size
			hui::windowSetNextFlags(hui::WindowFlags::NoTab | hui::WindowFlags::NoResize | hui::WindowFlags::NoPadding);
			hui::windowSetNextSize(0, menuBarInfo.height);

			if (hui::windowBegin("main_menu", "Main Menu", nullptr, state.iconMenu))
			{
				drawMainMenu(state, exitNow);
				hui::windowEnd();
			}

			hui::windowSetNextFlags(hui::WindowFlags::NoTab | hui::WindowFlags::NoResize | hui::WindowFlags::NoPadding);
			hui::windowSetNextSize(0, state.iconButtonSize);

			if (hui::windowBegin("main_toolbar", "Main Toolbar", nullptr, state.iconToolbar))
			{
				drawMainToolbar(state);
				hui::windowEnd();
			}

			hui::windowSetNextSize(0, 200);

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

			hui::frameEnd();

			if (lastEventInQueue)
				hui::present();
		};

		// if we have events, then go through all of them and call the frame render and input
		if (eventCount)
		{
			for (int i = 0; i < eventCount; i++)
			{
				hui::inputEventSet(hui::inputEventGetAtIndex(i));

				if (hui::inputEventGet().type == hui::InputEvent::Type::WindowClose)
				{
					if (hui::inputEventGet().window == mainWnd)
					{
						exitNow = true;
					}
				}

				doFrame(i == eventCount - 1);
			}
		}
		else
		{
			doFrame(true);
		}
	}

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
