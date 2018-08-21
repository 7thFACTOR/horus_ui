#include "horus.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#include <GL/gl.h>
#pragma execution_character_set("utf-8")
#include <vector>
#include <string>

using namespace hui;
WidgetElementInfo inf;

void curveEditor(f32 height, u32 maxPoints, Point* points, u32& pointCount, const Color& lineColor)
{
    beginCustomWidget(height);
    setBackColor(isPressed() ? Color::red : Color::white);
    drawBorderedImage(inf.image, inf.border, getWidgetRect());
    setLineStyle({ Color::orange, 1.5f });
    drawLine({ 10,10 }, { 10,110 });
    SplineControlPoint p[4];
    p[0].center = {10, 20};
    p[0].rightTangent = {10,20};
    p[1].center = { 210, 120 };
    p[1].leftTangent = { -10,520 };
    p[1].rightTangent = { 10,20 };

    p[2].center = { 310, 320 };
    p[2].isLine = false;
    p[2].leftTangent = { -10,20 };
    p[2].rightTangent = { 10,20 };

    p[3].center = getMousePosition();
    p[3].isLine = false;
    p[3].leftTangent = { -10,20 };
    p[3].rightTangent = { 10,20 };
    drawSpline(p, 4);
    endCustomWidget();
}

int main(int argc, char** args)
{
	hui::initializeSDL("Custom Widgets", { 0, 0, 1200, 600 });
	auto theme = hui::loadTheme("../themes/default.theme");

	hui::setTheme(theme);
    auto largeFnt = hui::getFont(theme, "title");

    WidgetElementInfo el;
    getThemeWidgetElementInfo(WidgetElementId::ButtonBody, WidgetStateType::Normal, inf);

    while (true)
    {
        hui::processEvents();
        hui::setWindow(hui::getMainWindow());
        hui::beginWindow(hui::getMainWindow());
        hui::clearBackground();

        hui::beginFrame();
        hui::Rect panelRect = { 50, 50, 350, 500 };
        hui::beginContainer(panelRect);
        hui::WidgetElementInfo elemInfo;
        hui::getThemeWidgetElementInfo(hui::WidgetElementId::PopupBody, hui::WidgetStateType::Normal, elemInfo);

        const int maxPts = 32;
        Point pts[maxPts] = { 0 };
        u32 ptCount = 0;

        curveEditor(55, maxPts, pts, ptCount, Color::red);

        if (hui::button("Exit"))
            hui::quitApplication();

        hui::popPadding();
        hui::endContainer();
        hui::endFrame();
        hui::endWindow();
        hui::presentWindow(hui::getMainWindow());

        if (hui::wantsToQuit() || hui::mustQuit())
            break;
    }

	hui::shutdown();

	return 0;
}
