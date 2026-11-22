#pragma once

// Loads the white Material icon sheet and adds every named sub-image to the
// current theme atlas. The JSON maps lowercase_underscore names to rectangles
// inside the sheet PNG: [x, y, width, height].
//
// Pass outNames to collect the loaded icon names (used by the icon browser).

#include <horus.h>
#include "../../backends/json_theme_loader.h"

#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include <json/json.h>
#include <json/reader.h>

inline bool loadMaterialIconAtlas(
	const char* atlasPath,
	const char* jsonPath,
	std::vector<std::string>* outNames = nullptr)
{
	auto theme = hui::themeGet();
	if (!theme)
		return false;

	hui::ImageData sheet = {};
	if (!hui::loadPngImage(atlasPath, sheet) || !sheet.pixels)
		return false;

	std::ifstream jsonFile(jsonPath);
	if (!jsonFile)
	{
		hui::deleteImageData(sheet);
		return false;
	}

	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(jsonFile, root) || !root.isObject() || !root["icons"].isArray())
	{
		hui::deleteImageData(sheet);
		return false;
	}

	const Json::Value& icons = root["icons"];

	for (Json::ArrayIndex i = 0; i < icons.size(); ++i)
	{
		const Json::Value& entry = icons[i];

		if (!entry.isObject() || !entry["name"].isString() || !entry["rect"].isArray())
			continue;

		std::string name = entry["name"].asString();
		const Json::Value& r = entry["rect"];
		u32 x = r[0].asUInt();
		u32 y = r[1].asUInt();
		u32 w = r[2].asUInt();
		u32 h = r[3].asUInt();

		if (!w || !h || x + w > sheet.width || y + h > sheet.height)
			continue;

		if (outNames)
			outNames->push_back(name);

		hui::ImageData sub = {};
		sub.width = w;
		sub.height = h;
		sub.pixels = new hui::Rgba32[(size_t)w * h];

		for (u32 row = 0; row < h; ++row)
		{
			memcpy(sub.pixels + (size_t)row * w,
				sheet.pixels + (size_t)(y + row) * sheet.width + x,
				sizeof(hui::Rgba32) * w);
		}

		hui::themeAddImage(theme, name.c_str(), sub);

		// Dim variants for the hierarchy flag icons: shown on hover while the flag is off.
		if (name == "visibility_off" || name == "lock")
		{
			hui::ImageData dim = {};
			dim.width = w;
			dim.height = h;
			dim.pixels = new hui::Rgba32[(size_t)w * h];

			for (size_t pixel = 0; pixel < (size_t)w * h; ++pixel)
			{
				hui::Rgba32 c = sub.pixels[pixel];
				u32 alpha = (u32)((c >> 24) * 0.5f);
				dim.pixels[pixel] = (c & 0x00FFFFFF) | (alpha << 24);
			}

			std::string dimName = name + "_dim";
			hui::themeAddImage(theme, dimName.c_str(), dim);
			hui::deleteImageData(dim);
		}

		hui::deleteImageData(sub);
	}

	hui::deleteImageData(sheet);
	return true;
}
