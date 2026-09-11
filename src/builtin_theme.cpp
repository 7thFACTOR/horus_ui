#include "horus.h"
#include "theme.h"
#include "builtin_font_data.h"
#include "util.h"
#include <math.h>

namespace hui {

#define HUI_DEFAULT_STYLE(elId) theme->getElement(WidgetElementId::elId).styles["default"]

static void drawPixel(Image* img, int x, int y, Rgba32 col)
{
	if (x >= 0 && x < (int)img->width && y >= 0 && y < (int)img->height)
	{
		img->pixels[y * img->width + x] = col;
	}
}

// Alpha blend pixel
static void blendPixel(Image* img, int x, int y, Color col)
{
	if (x >= 0 && x < (int)img->width && y >= 0 && y < (int)img->height)
	{
		Rgba32 dst = img->pixels[y * img->width + x];
		Color dstCol = Color::fromU8(dst & 0xFF, (dst >> 8) & 0xFF, (dst >> 16) & 0xFF, (dst >> 24) & 0xFF);
		
		f32 outA = col.a + dstCol.a * (1.0f - col.a);
		if (outA > 0.0001f)
		{
			f32 outR = (col.r * col.a + dstCol.r * dstCol.a * (1.0f - col.a)) / outA;
			f32 outG = (col.g * col.a + dstCol.g * dstCol.a * (1.0f - col.a)) / outA;
			f32 outB = (col.b * col.a + dstCol.b * dstCol.a * (1.0f - col.a)) / outA;
			Color out(outR, outG, outB, outA);
			img->pixels[y * img->width + x] = out.getRgba();
		}
	}
}

static void drawAALine(Image* img, f32 x0, f32 y0, f32 x1, f32 y1, f32 thickness, Color color)
{
	// Simple distance-based anti-aliased line rendering
	int xmin = std::max(0, (int)std::floor(std::min(x0, x1) - thickness - 1));
	int xmax = std::min((int)img->width - 1, (int)std::ceil(std::max(x0, x1) + thickness + 1));
	int ymin = std::max(0, (int)std::floor(std::min(y0, y1) - thickness - 1));
	int ymax = std::min((int)img->height - 1, (int)std::ceil(std::max(y0, y1) + thickness + 1));

	f32 dx = x1 - x0;
	f32 dy = y1 - y0;
	f32 lenSq = dx * dx + dy * dy;
	
	for (int y = ymin; y <= ymax; y++)
	{
		for (int x = xmin; x <= xmax; x++)
		{
			f32 px = x + 0.5f;
			f32 py = y + 0.5f;
			
			f32 t = 0.0f;
			if (lenSq > 0.00001f)
				t = std::max(0.0f, std::min(1.0f, ((px - x0) * dx + (py - y0) * dy) / lenSq));
				
			f32 projX = x0 + t * dx;
			f32 projY = y0 + t * dy;
			
			f32 dist = std::sqrt((px - projX) * (px - projX) + (py - projY) * (py - projY));
			f32 distToEdge = dist - thickness / 2.0f;
			
			f32 alpha = 1.0f - std::max(0.0f, std::min(1.0f, distToEdge + 0.5f));
			if (alpha > 0)
			{
				Color c = color;
				c.a *= alpha;
				blendPixel(img, x, y, c);
			}
		}
	}
}

static void drawAACircle(Image* img, f32 cx, f32 cy, f32 r, f32 thickness, bool fill, Color color)
{
	int xmin = std::max(0, (int)std::floor(cx - r - thickness - 1));
	int xmax = std::min((int)img->width - 1, (int)std::ceil(cx + r + thickness + 1));
	int ymin = std::max(0, (int)std::floor(cy - r - thickness - 1));
	int ymax = std::min((int)img->height - 1, (int)std::ceil(cy + r + thickness + 1));
	
	for (int y = ymin; y <= ymax; y++)
	{
		for (int x = xmin; x <= xmax; x++)
		{
			f32 px = x + 0.5f;
			f32 py = y + 0.5f;
			f32 dist = std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
			f32 alpha = 0.0f;
			
			if (fill)
			{
				alpha = 1.0f - std::max(0.0f, std::min(1.0f, dist - r + 0.5f));
			}
			else
			{
				f32 distToEdge = std::abs(dist - r) - thickness / 2.0f;
				alpha = 1.0f - std::max(0.0f, std::min(1.0f, distToEdge + 0.5f));
			}
			
			if (alpha > 0)
			{
				Color c = color;
				c.a *= alpha;
				blendPixel(img, x, y, c);
			}
		}
	}
}

static Image* createProceduralImage(Theme* theme, const char* name, u32 width, u32 height)
{
	Image* img = new Image();
	img->id = hashString(name);
	img->width = width;
	img->height = height;
	img->pixels.resize(width * height, 0); // Transparent black
	theme->images[img->id] = img;
	return img;
}

HTheme createBuiltinTheme(u32 atlasTextureSize)
{
	Theme* theme = new Theme(atlasTextureSize);

	Font* font = theme->createFontFromMemory("normal", roboto_regular_data, roboto_regular_data_size, 12);
	Font* fontBold = theme->createFontFromMemory("normal-bold", roboto_regular_data, roboto_regular_data_size, 12); // fallback to same font
	Font* fontLarge = theme->createFontFromMemory("large", roboto_regular_data, roboto_regular_data_size, 25);
	Font* fontTab = theme->createFontFromMemory("window-tab", roboto_regular_data, roboto_regular_data_size, 17);

	// Generate Procedural Images
	Image* imgRadioBody = createProceduralImage(theme, "flat/radio_body", 22, 22);
	drawAACircle(imgRadioBody, 11, 11, 7, 2, false, Color::white);

	Image* imgRadioMark = createProceduralImage(theme, "flat/radio_mark", 22, 22);
	drawAACircle(imgRadioMark, 11, 11, 4.5f, 0, true, Color::white);
	
	Image* imgCheckMark = createProceduralImage(theme, "flat/check_mark", 22, 22);
	drawAALine(imgCheckMark, 5, 12, 10, 16, 2.5f, Color::white);
	drawAALine(imgCheckMark, 10, 16, 17, 6, 2.5f, Color::white);

	Image* imgCheckMarkIndeterminate = createProceduralImage(theme, "flat/check_mark_indeterminate", 22, 22);
	drawAALine(imgCheckMarkIndeterminate, 6, 11, 16, 11, 3.0f, Color::white);
	
	Image* imgDropdownArrow = createProceduralImage(theme, "flat/dropdown_arrow", 20, 22);
	drawAALine(imgDropdownArrow, 5, 8, 10, 14, 2.0f, Color::white);
	drawAALine(imgDropdownArrow, 10, 14, 15, 8, 2.0f, Color::white);
	
	Image* imgExpandableCollapsed = createProceduralImage(theme, "flat/expandable_collapsed_arrow", 22, 22);
	drawAALine(imgExpandableCollapsed, 8, 6, 14, 11, 2.0f, Color::white);
	drawAALine(imgExpandableCollapsed, 14, 11, 8, 16, 2.0f, Color::white);

	Image* imgExpandableExpanded = createProceduralImage(theme, "flat/expandable_expanded_arrow", 22, 22);
	drawAALine(imgExpandableExpanded, 6, 8, 11, 14, 2.0f, Color::white);
	drawAALine(imgExpandableExpanded, 11, 14, 16, 8, 2.0f, Color::white);
	
	Image* imgComboSliderLeftArrow = createProceduralImage(theme, "flat/comboslider_left_arrow", 22, 20);
	drawAALine(imgComboSliderLeftArrow, 14, 6, 8, 10, 2.0f, Color::white);
	drawAALine(imgComboSliderLeftArrow, 8, 10, 14, 14, 2.0f, Color::white);

	Image* imgComboSliderRightArrow = createProceduralImage(theme, "flat/comboslider_right_arrow", 22, 20);
	drawAALine(imgComboSliderRightArrow, 8, 6, 14, 10, 2.0f, Color::white);
	drawAALine(imgComboSliderRightArrow, 14, 10, 8, 14, 2.0f, Color::white);

	Image* imgCircularSliderBody = createProceduralImage(theme, "flat/circularslider_body", 90, 90);
	drawAACircle(imgCircularSliderBody, 45, 45, 30, 4.0f, false, Color::white);

	Image* imgCircularSliderValueDot = createProceduralImage(theme, "flat/circularslider_value_dot", 20, 20);
	drawAACircle(imgCircularSliderValueDot, 10, 10, 5, 0, true, Color::white);

	Image* imgCircularSliderMark = createProceduralImage(theme, "flat/circularslider_mark", 20, 20);
	drawAALine(imgCircularSliderMark, 10, 2, 10, 8, 2.0f, Color::white);

	Image* imgSliderKnob = createProceduralImage(theme, "flat/slider_knob", 16, 16);
	drawAACircle(imgSliderKnob, 8, 8, 5, 0, true, Color::white);

	// The checkers pattern for color picker
	Image* imgCheckers = createProceduralImage(theme, "flat/checkers", 16, 16);
	for (int y = 0; y < 16; y++)
		for (int x = 0; x < 16; x++)
			drawPixel(imgCheckers, x, y, (((x / 8) + (y / 8)) % 2 == 0) ? Color::fromU8(200, 200, 200).getRgba() : Color::white.getRgba());
	
	// Default color scheme map from flat.theme.json
	Color cBodyNormal = Color::fromU8(45, 45, 55);
	Color cBodyHovered = Color::fromU8(60, 60, 75);
	Color cBodyPressed = Color::fromU8(75, 75, 95);
	Color cBodyFocused = Color::fromU8(60, 60, 75);
	Color cBodyDisabled = Color::fromU8(45, 45, 55, 128);

	Color cButtonNormal = Color::fromU8(55, 95, 135);
	Color cButtonHovered = Color::fromU8(70, 120, 165);
	Color cButtonPressed = Color::fromU8(40, 75, 110);
	Color cButtonFocused = Color::fromU8(60, 100, 145);
	
	Color cAccent = Color::fromU8(55, 185, 255);
	Color cAccentHovered = Color::fromU8(70, 200, 255);
	
	Color cGreen = Color::fromU8(55, 185, 100);
	Color cGreenHovered = Color::fromU8(70, 220, 120);

	Color cText = Color::white;
	Color cTextDisabled = Color::fromU8(128, 128, 128);

	for (u32 i = 0; i < (u32)WidgetElementId::Count; i++)
	{
		WidgetElementId id = (WidgetElementId)i;
		ThemeElement& elem = theme->getElement(id);

		for (u32 j = 0; j < (u32)WidgetStateType::Count; j++)
		{
			WidgetStateType state = (WidgetStateType)j;
			ThemeElement::State& s = elem.styles["default"].states[j];

			s.image = theme->whiteImage;
			s.font = font;
			s.textColor = cText;
			s.color = cBodyNormal;

			if (state == WidgetStateType::Hovered) s.color = cBodyHovered;
			if (state == WidgetStateType::Pressed) s.color = cBodyPressed;
			if (state == WidgetStateType::Focused) s.color = cBodyFocused;
			if (state == WidgetStateType::Disabled) { s.color = cBodyDisabled; s.textColor = cTextDisabled; }
		}
	}

	auto setSize = [&](WidgetElementId id, f32 width, f32 height, u32 border = 0)
	{
		ThemeElement& elem = theme->getElement(id);
		auto& style = elem.styles["default"];
		for (u32 j = 0; j < (u32)WidgetStateType::Count; j++)
		{
			style.states[j].width = width;
			style.states[j].height = height;
			style.states[j].border = border;
		}
	};

	auto setElementColors = [&](WidgetElementId id, Color n, Color h, Color p, Color f, Color d)
	{
		auto& style = theme->getElement(id).styles["default"];
		style.states[(u32)WidgetStateType::Normal].color = n;
		style.states[(u32)WidgetStateType::Hovered].color = h;
		style.states[(u32)WidgetStateType::Pressed].color = p;
		style.states[(u32)WidgetStateType::Focused].color = f;
		style.states[(u32)WidgetStateType::Disabled].color = d;
	};

	auto setElementImage = [&](WidgetElementId id, Image* img)
	{
		auto& style = theme->getElement(id).styles["default"];
		for (u32 j = 0; j < (u32)WidgetStateType::Count; j++)
			style.states[j].image = img;
	};

	// --- Buttons ---
	setSize(WidgetElementId::ButtonBody, 0, 22, 3);
	setElementColors(WidgetElementId::ButtonBody, cButtonNormal, cButtonHovered, cButtonPressed, cButtonFocused, Color::fromU8(55, 95, 135, 128));

	setSize(WidgetElementId::ButtonGroupLeftBody, 0, 22, 0);
	setElementColors(WidgetElementId::ButtonGroupLeftBody, cButtonNormal, cButtonHovered, Color::black, cButtonFocused, Color::fromU8(55, 95, 135, 128));
	
	setSize(WidgetElementId::ButtonGroupMiddleBody, 0, 22, 0);
	setElementColors(WidgetElementId::ButtonGroupMiddleBody, cButtonNormal, cButtonHovered, Color::black, cButtonFocused, Color::fromU8(55, 95, 135, 128));
	
	setSize(WidgetElementId::ButtonGroupRightBody, 0, 22, 0);
	setElementColors(WidgetElementId::ButtonGroupRightBody, cButtonNormal, cButtonHovered, Color::black, cButtonFocused, Color::fromU8(55, 95, 135, 128));
	
	setSize(WidgetElementId::ImageButtonBody, 0, 22, 0);
	setElementColors(WidgetElementId::ImageButtonBody, cButtonNormal, cButtonHovered, cButtonPressed, cButtonFocused, Color::fromU8(55, 95, 135, 128));

	// --- Inputs ---
	setSize(WidgetElementId::TextInputBody, 0, 22, 0);
	setElementColors(WidgetElementId::TextInputBody, Color::fromU8(30, 35, 40), Color::fromU8(40, 45, 55), Color::fromU8(40, 45, 55), Color::fromU8(35, 45, 55), Color::fromU8(30, 35, 40, 128));
	
	setSize(WidgetElementId::MultilineTextInputBody, 0, 0, 0);
	setElementColors(WidgetElementId::MultilineTextInputBody, Color::fromU8(25, 30, 35), Color::fromU8(25, 30, 35), Color::fromU8(25, 30, 35), Color::fromU8(30, 35, 45), Color::fromU8(25, 30, 35, 128));

	setSize(WidgetElementId::TextInputCaret, 2, 3, 0);
	setElementColors(WidgetElementId::TextInputCaret, cAccent, cAccent, cAccent, cAccent, cAccent);

	setSize(WidgetElementId::TextInputSelection, 0, 24, 0);
	setElementColors(WidgetElementId::TextInputSelection, Color::fromU8(55, 185, 255, 120), Color::fromU8(55, 185, 255, 120), Color::fromU8(55, 185, 255, 120), Color::fromU8(55, 185, 255, 120), Color::fromU8(55, 185, 255, 120));

	// --- Sliders ---
	setSize(WidgetElementId::SliderBody, 0, 16, 0);
	setElementColors(WidgetElementId::SliderBody, Color::fromU8(60, 60, 70), Color::fromU8(60, 60, 70), Color::fromU8(60, 60, 70), Color::fromU8(60, 60, 70), Color::fromU8(60, 60, 70, 128));
	
	setSize(WidgetElementId::SliderBodyFilled, 0, 16, 0);
	setElementColors(WidgetElementId::SliderBodyFilled, Color::fromU8(55, 125, 185), Color::fromU8(55, 125, 185), Color::fromU8(55, 125, 185), Color::fromU8(55, 125, 185), Color::fromU8(55, 125, 185, 128));

	setSize(WidgetElementId::SliderKnob, 16, 16, 0);
	setElementImage(WidgetElementId::SliderKnob, imgSliderKnob);
	setElementColors(WidgetElementId::SliderKnob, cAccent, cAccentHovered, cAccent, cAccentHovered, Color::fromU8(55, 185, 255, 128));

	// --- Check & Radio ---
	setSize(WidgetElementId::CheckBody, 22, 22, 0);
	setSize(WidgetElementId::CheckMark, 22, 22, 0);
	setElementImage(WidgetElementId::CheckMark, imgCheckMark);
	setElementColors(WidgetElementId::CheckMark, cGreen, cGreenHovered, cGreen, cGreen, Color::fromU8(55, 185, 100, 128));

	setSize(WidgetElementId::CheckMarkIndeterminate, 22, 22, 0);
	setElementImage(WidgetElementId::CheckMarkIndeterminate, imgCheckMarkIndeterminate);
	setElementColors(WidgetElementId::CheckMarkIndeterminate, cGreen, cGreenHovered, cGreen, cGreen, Color::fromU8(55, 185, 100, 128));

	setSize(WidgetElementId::RadioBody, 22, 22, 0);
	setElementImage(WidgetElementId::RadioBody, imgRadioBody);
	
	setSize(WidgetElementId::RadioMark, 22, 22, 0);
	setElementImage(WidgetElementId::RadioMark, imgRadioMark);
	setElementColors(WidgetElementId::RadioMark, cGreen, cGreenHovered, cGreen, cGreen, Color::fromU8(55, 185, 100, 128));

	// --- Dropdown ---
	setSize(WidgetElementId::DropdownBody, 0, 22, 0);
	setSize(WidgetElementId::DropdownArrowBox, 20, 22, 0);
	setElementColors(WidgetElementId::DropdownArrowBox, cButtonNormal, cButtonHovered, cButtonPressed, cButtonFocused, Color::fromU8(55, 95, 135, 128));
	
	setSize(WidgetElementId::DropdownArrow, 20, 22, 0);
	setElementImage(WidgetElementId::DropdownArrow, imgDropdownArrow);
	setElementColors(WidgetElementId::DropdownArrow, Color::white, Color::white, Color::white, Color::white, Color::fromU8(100, 100, 110));

	// --- Expandable & Tree ---
	setSize(WidgetElementId::ExpandableBody, 0, 22, 0);
	HUI_DEFAULT_STYLE(ExpandableBody).states[(u32)WidgetStateType::Normal].font = fontBold;
	
	setSize(WidgetElementId::ExpandableCollapsedArrow, 22, 22, 0);
	setElementImage(WidgetElementId::ExpandableCollapsedArrow, imgExpandableCollapsed);
	setElementColors(WidgetElementId::ExpandableCollapsedArrow, Color::fromU8(180, 180, 190), cAccent, cAccent, cAccent, Color::fromU8(120, 120, 130, 128));
	
	setSize(WidgetElementId::ExpandableExpandedArrow, 22, 22, 0);
	setElementImage(WidgetElementId::ExpandableExpandedArrow, imgExpandableExpanded);
	setElementColors(WidgetElementId::ExpandableExpandedArrow, Color::fromU8(180, 180, 190), cAccent, cAccent, cAccent, Color::fromU8(120, 120, 130, 128));

	setSize(WidgetElementId::TreeNodeCollapsedArrow, 22, 22, 0);
	setElementImage(WidgetElementId::TreeNodeCollapsedArrow, imgExpandableCollapsed); // reuse expandable arrow for tree node
	setElementColors(WidgetElementId::TreeNodeCollapsedArrow, Color::fromU8(180, 180, 190), cAccent, cAccent, cAccent, Color::fromU8(120, 120, 130, 128));
	
	setSize(WidgetElementId::TreeNodeExpandedArrow, 22, 22, 0);
	setElementImage(WidgetElementId::TreeNodeExpandedArrow, imgExpandableExpanded);
	setElementColors(WidgetElementId::TreeNodeExpandedArrow, Color::fromU8(180, 180, 190), cAccent, cAccent, cAccent, Color::fromU8(120, 120, 130, 128));

	// --- ComboSlider ---
	setSize(WidgetElementId::ComboSliderLeftButton, 22, 20, 0);
	setSize(WidgetElementId::ComboSliderMiddleButton, 0, 20, 0);
	setSize(WidgetElementId::ComboSliderRightButton, 22, 20, 0);
	
	setSize(WidgetElementId::ComboSliderLeftArrow, 22, 20, 0);
	setElementImage(WidgetElementId::ComboSliderLeftArrow, imgComboSliderLeftArrow);
	setElementColors(WidgetElementId::ComboSliderLeftArrow, Color::fromU8(180, 180, 190), cAccent, cAccent, cAccent, Color::fromU8(120, 120, 130, 128));

	setSize(WidgetElementId::ComboSliderRightArrow, 22, 20, 0);
	setElementImage(WidgetElementId::ComboSliderRightArrow, imgComboSliderRightArrow);
	setElementColors(WidgetElementId::ComboSliderRightArrow, Color::fromU8(180, 180, 190), cAccent, cAccent, cAccent, Color::fromU8(120, 120, 130, 128));
	
	setSize(WidgetElementId::ComboSliderRangeBar, 0, 2, 0);
	setElementColors(WidgetElementId::ComboSliderRangeBar, cAccent, cAccent, cAccent, cAccent, Color::fromU8(55, 185, 255, 55));

	// --- CircularSlider ---
	setSize(WidgetElementId::CircularSliderBody, 90, 90, 0);
	setElementImage(WidgetElementId::CircularSliderBody, imgCircularSliderBody);
	
	setSize(WidgetElementId::CircularSliderMark, 20, 10, 0);
	setElementImage(WidgetElementId::CircularSliderMark, imgCircularSliderMark);
	setElementColors(WidgetElementId::CircularSliderMark, Color::fromU8(120, 120, 130), Color::fromU8(120, 120, 130), Color::fromU8(120, 120, 130), Color::fromU8(120, 120, 130), Color::fromU8(120, 120, 130, 128));
	
	setSize(WidgetElementId::CircularSliderValueDot, 20, 20, 0);
	setElementImage(WidgetElementId::CircularSliderValueDot, imgCircularSliderValueDot);
	setElementColors(WidgetElementId::CircularSliderValueDot, Color::fromU8(120, 120, 130), Color::fromU8(120, 120, 130), Color::fromU8(255, 180, 60), Color::fromU8(120, 120, 130), Color::fromU8(120, 120, 130, 128));
	
	// Set specific properties requested by CircularSlider
	HUI_DEFAULT_STYLE(CircularSliderBody).cachedFloatParameters["labelSpacing"] = 8;
	HUI_DEFAULT_STYLE(CircularSliderMark).cachedFloatParameters["placementRadius"] = 20;
	HUI_DEFAULT_STYLE(CircularSliderValueDot).cachedFloatParameters["count"] = 30;
	HUI_DEFAULT_STYLE(CircularSliderValueDot).cachedFloatParameters["placementRadius"] = 36;
	HUI_DEFAULT_STYLE(CircularSliderValueDot).cachedColorParameters["negativeColor"] = Color::fromU8(220, 60, 60);
	HUI_DEFAULT_STYLE(CircularSliderValueDot).cachedColorParameters["positiveColor"] = Color::fromU8(55, 185, 255);

	// --- Other layout and containers ---
	setSize(WidgetElementId::MenuBarBody, 0, 20, 0);
	setElementColors(WidgetElementId::MenuBarBody, Color::fromU8(30, 35, 40), Color::fromU8(30, 35, 40), Color::fromU8(30, 35, 40), Color::fromU8(30, 35, 40), Color::fromU8(30, 35, 40));

	setSize(WidgetElementId::MenuBarItem, 0, 20, 0);
	setElementColors(WidgetElementId::MenuBarItem, Color::transparent, cButtonNormal, cButtonPressed, cButtonNormal, Color::transparent);

	setSize(WidgetElementId::MenuBody, 0, 0, 0);
	setElementColors(WidgetElementId::MenuBody, Color::fromU8(35, 40, 50, 240), Color::fromU8(35, 40, 50, 240), Color::fromU8(35, 40, 50, 240), Color::fromU8(35, 40, 50, 240), Color::fromU8(35, 40, 50, 240));

	setSize(WidgetElementId::MenuItemBody, 0, 20, 0);
	setElementColors(WidgetElementId::MenuItemBody, Color::black, cAccent, cAccent, cAccent, Color::black);

	setSize(WidgetElementId::SubMenuItemArrow, 22, 22, 0);
	setElementImage(WidgetElementId::SubMenuItemArrow, imgExpandableCollapsed); // Reuse right arrow
	
	setSize(WidgetElementId::MenuItemCheckMark, 22, 22, 0);
	setElementImage(WidgetElementId::MenuItemCheckMark, imgCheckMark);
	
	setSize(WidgetElementId::SelectableBody, 0, 20, 0);
	setElementColors(WidgetElementId::SelectableBody, Color::transparent, cAccent, cAccent, Color::transparent, Color::fromU8(255, 255, 255, 128));

	setSize(WidgetElementId::ProgressBack, 0, 20, 0);
	setElementColors(WidgetElementId::ProgressBack, Color::fromU8(35, 40, 50), Color::fromU8(35, 40, 50), Color::fromU8(35, 40, 50), Color::fromU8(35, 40, 50), Color::fromU8(35, 40, 50, 128));
	
	setSize(WidgetElementId::ProgressFill, 0, 19, 0);
	setElementColors(WidgetElementId::ProgressFill, cGreen, cGreen, cGreen, cGreen, Color::fromU8(55, 185, 100, 128));

	setSize(WidgetElementId::TableHeaderBodyLeft, 0, 25, 0);
	setElementColors(WidgetElementId::TableHeaderBodyLeft, cButtonNormal, cButtonNormal, cButtonNormal, cButtonNormal, cButtonNormal);
	
	setSize(WidgetElementId::TableHeaderBodyRight, 0, 25, 0);
	setElementColors(WidgetElementId::TableHeaderBodyRight, cButtonNormal, cButtonNormal, cButtonNormal, cButtonNormal, cButtonNormal);

	setSize(WidgetElementId::TableHeaderBody, 0, 25, 0);
	setElementColors(WidgetElementId::TableHeaderBody, cButtonNormal, cButtonNormal, cButtonNormal, cButtonNormal, cButtonNormal);

	setSize(WidgetElementId::TabBodyActive, 100, 30, 0);
	setElementColors(WidgetElementId::TabBodyActive, cButtonNormal, cButtonHovered, cButtonPressed, cButtonNormal, cButtonNormal);
	HUI_DEFAULT_STYLE(TabBodyActive).states[(u32)WidgetStateType::Normal].font = fontTab;
	
	setSize(WidgetElementId::TabBodyInactive, 100, 30, 0);
	setElementColors(WidgetElementId::TabBodyInactive, Color::fromU8(35, 40, 50), Color::fromU8(50, 55, 70), Color::fromU8(55, 60, 80), Color::fromU8(55, 60, 80), Color::fromU8(35, 40, 50));
	HUI_DEFAULT_STYLE(TabBodyInactive).states[(u32)WidgetStateType::Normal].font = fontTab;

	setSize(WidgetElementId::TooltipBody, 0, 0, 0);
	setElementColors(WidgetElementId::TooltipBody, Color::fromU8(30, 35, 40), Color::fromU8(30, 35, 40), Color::fromU8(30, 35, 40), Color::fromU8(30, 35, 40), Color::fromU8(30, 35, 40));

	setSize(WidgetElementId::PopupBody, 0, 0, 0);
	setElementColors(WidgetElementId::PopupBody, Color::fromU8(35, 40, 50), Color::fromU8(35, 40, 50), Color::fromU8(35, 40, 50), Color::fromU8(35, 40, 50), Color::fromU8(35, 40, 50));

	setSize(WidgetElementId::WindowBody, 0, 0, 0);
	setElementColors(WidgetElementId::WindowBody, Color::fromU8(25, 30, 35), Color::fromU8(25, 30, 35), Color::fromU8(25, 30, 35), Color::fromU8(25, 30, 35), Color::fromU8(25, 30, 35));

	setSize(WidgetElementId::BoxBody, 0, 0, 0);
	setElementColors(WidgetElementId::BoxBody, Color::fromU8(35, 40, 50), Color::fromU8(35, 40, 50), Color::fromU8(35, 40, 50), Color::fromU8(35, 40, 50), Color::fromU8(35, 40, 50));
	
	setSize(WidgetElementId::ColorPickerCheckers, 16, 16, 0);
	setElementImage(WidgetElementId::ColorPickerCheckers, imgCheckers);
	
	HUI_DEFAULT_STYLE(LinkBody).states[(u32)WidgetStateType::Normal].textColor = Color::fromU8(100, 160, 255);
	HUI_DEFAULT_STYLE(LinkBody).states[(u32)WidgetStateType::Hovered].textColor = Color::fromU8(140, 190, 255);
	HUI_DEFAULT_STYLE(LinkBody).states[(u32)WidgetStateType::Disabled].textColor = Color::fromU8(128, 128, 128);

	// Some specific widget text colors Overrides
	HUI_DEFAULT_STYLE(TabBodyActive).states[(u32)WidgetStateType::Normal].textColor = Color::fromU8(196, 196, 196);
	HUI_DEFAULT_STYLE(TabBodyInactive).states[(u32)WidgetStateType::Normal].textColor = Color::fromU8(150, 150, 160);

	theme->userSettings["sameLineHeight"] = "22";
	
	theme->build();

	return (HTheme)theme;
}

#undef HUI_DEFAULT_STYLE

}
