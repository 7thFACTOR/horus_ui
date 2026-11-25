#include "json_docking_state.h"
#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>
#include <cstring>

namespace hui
{
static std::string readDockingStateFile(const char* path)
{
	auto file = contextGetSettings().services.open(path, "rb");

	if (!file)
		return std::string();

	contextGetSettings().services.seek(file, FileSeekMode::End, 0);
	auto size = contextGetSettings().services.tell(file);
	std::string text;

	if (size > 0)
	{
		contextGetSettings().services.seek(file, FileSeekMode::Set, 0);
		std::vector<char> buffer(size + 1, 0);

		if (contextGetSettings().services.read(file, buffer.data(), size) == size)
			text.assign(buffer.data(), size);
	}

	contextGetSettings().services.close(file);

	return text;
}

static bool writeDockingStateFile(const char* path, const std::string& text)
{
	auto file = contextGetSettings().services.open(path, "wb");

	if (!file)
		return false;

	auto written = contextGetSettings().services.write(file, (void*)text.data(), text.size());

	contextGetSettings().services.close(file);

	return written == text.size();
}

static Json::Value rectToJson(const Rect& rect)
{
	Json::Value value(Json::arrayValue);

	value.append(rect.x);
	value.append(rect.y);
	value.append(rect.width);
	value.append(rect.height);

	return value;
}

static Rect rectFromJson(const Json::Value& value)
{
	Rect rect;

	if (value.isArray() && value.size() == 4)
		rect.set(value[0].asFloat(), value[1].asFloat(), value[2].asFloat(), value[3].asFloat());

	return rect;
}

static const char* dockNodeTypeToText(DockNodeType type)
{
	switch (type)
	{
	case DockNodeType::Tabs: return "tabs";
	case DockNodeType::Vertical: return "vertical";
	case DockNodeType::Horizontal: return "horizontal";
	default: return "none";
	}
}

static DockNodeType dockNodeTypeFromText(const std::string& text)
{
	if (text == "tabs") return DockNodeType::Tabs;
	if (text == "vertical") return DockNodeType::Vertical;
	if (text == "horizontal") return DockNodeType::Horizontal;

	return DockNodeType::None;
}

std::string dockingStateToJson(const WindowsDockingState& dockingState)
{
	Json::Value root;

	root["version"] = dockingState.version;

	Json::Value nativeWindows(Json::arrayValue);

	for (const auto& info : dockingState.nativeWindows)
	{
		Json::Value nativeWindow;

		nativeWindow["rect"] = rectToJson(info.rect);
		nativeWindow["floating"] = info.floating;
		nativeWindows.append(nativeWindow);
	}

	root["nativeWindows"] = nativeWindows;

	Json::Value dockNodes(Json::arrayValue);

	for (const auto& info : dockingState.dockNodes)
	{
		Json::Value node;

		node["parent"] = info.parentIndex;
		node["nativeWindow"] = info.nativeWindowIndex;
		node["type"] = dockNodeTypeToText(info.type);
		node["rect"] = rectToJson(info.rect);
		node["selectedTab"] = (Json::UInt)info.selectedTabIndex;

		Json::Value windows(Json::arrayValue);

		for (const auto& wndInfo : info.windows)
		{
			Json::Value wnd;

			wnd["id"] = wndInfo.id;
			wnd["title"] = wndInfo.title;
			wnd["flags"] = wndInfo.flags;
			windows.append(wnd);
		}

		node["windows"] = windows;
		dockNodes.append(node);
	}

	root["dockNodes"] = dockNodes;

	Json::Value closedWindows(Json::arrayValue);

	for (const auto& info : dockingState.closedWindows)
	{
		Json::Value closedWindow;

		closedWindow["id"] = info.id;
		closedWindow["rect"] = rectToJson(info.rect);
		closedWindow["parent"] = info.parentIndex;
		closedWindow["slotParent"] = info.slotParentIndex;
		closedWindow["slotIndex"] = info.slotIndex;
		closedWindow["slotVertical"] = info.slotVertical;
		closedWindow["slotFraction"] = info.slotFraction;
		closedWindows.append(closedWindow);
	}

	root["closedWindows"] = closedWindows;

	if (dockingState.dockAreaNodeIndex >= 0)
		root["dockArea"] = dockingState.dockAreaNodeIndex;

	Json::StreamWriterBuilder builder;

	builder["indentation"] = "\t";

	return Json::writeString(builder, root);
}

bool dockingStateFromJson(WindowsDockingState& dockingState, const char* json)
{
	if (!json)
		return false;

	Json::CharReaderBuilder builder;
	Json::CharReader* reader = builder.newCharReader();
	Json::Value root;
	std::string errors;
	bool parsed = reader->parse(json, json + strlen(json), &root, &errors);

	delete reader;

	if (!parsed || !root.isObject())
		return false;

	auto nativeWindows = root["nativeWindows"];
	auto dockNodes = root["dockNodes"];

	if (!nativeWindows.isArray() || !dockNodes.isArray())
		return false;

	dockingState = WindowsDockingState();
	dockingState.version = root["version"].asUInt();

	for (const auto& nativeWindowValue : nativeWindows)
	{
		WindowsDockingState::NativeWindowInfo info;

		info.rect = rectFromJson(nativeWindowValue["rect"]);
		info.floating = nativeWindowValue["floating"].asBool();
		dockingState.nativeWindows.push_back(info);
	}

	for (const auto& nodeValue : dockNodes)
	{
		WindowsDockingState::DockNodeInfo info;

		info.parentIndex = nodeValue["parent"].asInt();
		info.nativeWindowIndex = nodeValue["nativeWindow"].asInt();
		info.type = dockNodeTypeFromText(nodeValue["type"].asString());
		info.rect = rectFromJson(nodeValue["rect"]);
		info.selectedTabIndex = nodeValue["selectedTab"].asUInt();

		auto windows = nodeValue["windows"];

		if (windows.isArray())
		{
			for (const auto& wndValue : windows)
			{
				WindowsDockingState::WindowInfo wndInfo;

				wndInfo.id = wndValue["id"].asString();
				wndInfo.title = wndValue["title"].asString();
				wndInfo.flags = wndValue["flags"].asUInt();
				info.windows.push_back(wndInfo);
			}
		}

		dockingState.dockNodes.push_back(info);
	}

	auto closedWindows = root["closedWindows"];

	if (closedWindows.isArray())
	{
		for (const auto& closedValue : closedWindows)
		{
			WindowsDockingState::ClosedWindowInfo info;

			info.id = closedValue["id"].asString();
			info.rect = rectFromJson(closedValue["rect"]);
			info.parentIndex = closedValue["parent"].asInt();
			info.slotParentIndex = closedValue["slotParent"].asInt();
			info.slotIndex = closedValue["slotIndex"].asInt();
			info.slotVertical = closedValue["slotVertical"].asBool();
			info.slotFraction = closedValue["slotFraction"].asFloat();
			dockingState.closedWindows.push_back(info);
		}
	}

	if (root.isMember("dockArea"))
		dockingState.dockAreaNodeIndex = root["dockArea"].asInt();

	return !dockingState.nativeWindows.empty() && !dockingState.dockNodes.empty();
}

bool dockingStateSaveToFile(const char* filename, u32 version)
{
	if (!filename || !*filename)
		return false;

	WindowsDockingState dockingState;

	dockingStateSaveInMemory(dockingState);
	dockingState.version = version;

	return writeDockingStateFile(filename, dockingStateToJson(dockingState));
}

bool dockingStateLoadFromFile(const char* filename, u32 minVersion)
{
	if (!filename || !*filename)
		return false;

	auto text = readDockingStateFile(filename);

	if (text.empty())
		return false;

	WindowsDockingState dockingState;

	if (!dockingStateFromJson(dockingState, text.c_str()))
		return false;

	if (dockingState.version < minVersion)
		return false;

	return dockingStateLoadFromMemory(dockingState);
}

}
