#include <string.h>
#include <algorithm>
#include "context.h"
#include "renderer.h"
#include "atlas.h"
#include "font.h"
#include "theme.h"
#include "util.h"
#include "unicode_text_cache.h"

namespace hui
{
enum LineClipBit
{
	Inside = 0,
	Left = 1,
	Right = 2,
	Bottom = 4,
	Top = 8
};

// Function to compute region code for a point(x, y)
static i32 computeLineClipCode(const Point& p, const Rect& rect)
{
	// initialized as being inside
	i32 code = LineClipBit::Inside;

	if (p.x < rect.left())       // to the left of rectangle
		code |= LineClipBit::Left;
	else if (p.x > rect.right())  // to the right of rectangle
		code |= LineClipBit::Right;
	if (p.y < rect.top())       // below the rectangle
		code |= LineClipBit::Bottom;
	else if (p.y > rect.bottom())  // above the rectangle
		code |= LineClipBit::Top;

	return code;
}

static bool clipLineToRect(
	const Point& p1, const Point& p2,
	const Point& uv1, const Point& uv2,
	const Rect& rect,
	Point& newP1, Point& newP2,
	Point& newUv1, Point& newUv2)
{
	// Compute region codes for P1, P2
	i32 code1 = computeLineClipCode(p1, rect);
	i32 code2 = computeLineClipCode(p2, rect);

	// Initialize line as outside the rectangular window
	bool accept = false;

	newUv1 = uv1;
	newUv2 = uv2;

	while (true)
	{
		if ((code1 == 0) && (code2 == 0))
		{
			// If both endpoints lie within rectangle
			accept = true;
			newP1 = p1;
			newP2 = p2;
			newUv1 = uv1;
			newUv2 = uv2;
			break;
		}
		else if (code1 & code2)
		{
			// If both endpoints are outside rectangle,
			// in same region
			break;
		}
		else
		{
			// Some segment of line lies within the
			// rectangle
			i32 code_out = 0;
			f32 x = 0, y = 0;
			Point uv = uv1;

			// At least one endpoint is outside the
			// rectangle, pick it.
			if (code1 != 0)
				code_out = code1;
			else
				code_out = code2;

			// Find intersection point;
			// using formulas y = y1 + slope * (x - x1),
			// x = x1 + (1 / slope) * (y - y1)
			if (code_out & LineClipBit::Top)
			{
				// point is above the clip rectangle
				auto t = (rect.bottom() - p1.y) / (p2.y - p1.y);
				x = p1.x + (p2.x - p1.x) * t;
				y = rect.bottom();
				uv = uv1 + (uv2 - uv1) * t;
			}
			else if (code_out & LineClipBit::Bottom)
			{
				// point is below the rectangle
				auto t = (rect.top() - p1.y) / (p2.y - p1.y);
				x = p1.x + (p2.x - p1.x) * t;
				y = rect.top();
				uv = uv1 + (uv2 - uv1) * t;
			}
			else if (code_out & LineClipBit::Right)
			{
				// point is to the right of rectangle
				auto t = (rect.right() - p1.x) / (p2.x - p1.x);
				y = p1.y + (p2.y - p1.y) * t;
				x = rect.right();
				uv = uv1 + (uv2 - uv1) * t;
			}
			else if (code_out & LineClipBit::Left)
			{
				// point is to the left of rectangle
				auto t = (rect.left() - p1.x) / (p2.x - p1.x);
				y = p1.y + (p2.y - p1.y) * t;
				x = rect.left();
				uv = uv1 + (uv2 - uv1) * t;
			}

			// Now intersection point x,y is found
			// We replace point outside rectangle
			// by intersection point
			if (code_out == code1)
			{
				newP1.x = x;
				newP1.y = y;
				newUv1 = uv;
				code1 = computeLineClipCode(newP1, rect);
			}
			else
			{
				newP2.x = x;
				newP2.y = y;
				newUv2 = uv;
				code2 = computeLineClipCode(newP2, rect);
			}
		}
	}

	return accept;
}

static void clipLeft(const Rect& rect, Point* inPoints, Point* inUvPoints, Rgba32* inColors, u32 inCount, Point* outPoints, Point* outUvPoints, Rgba32* outColors, u32& outCount)
{
	Point* pp1;
	Point* pp2;
	Point* uvpp1;
	Point* uvpp2;
	Rgba32* cpp1;
	Rgba32* cpp2;

	for (u32 i = 0; i < inCount; i++)
	{
		if (i == inCount - 1)
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[0];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[0];
			cpp1 = &inColors[i];
			cpp2 = &inColors[0];
		}
		else
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[i + 1];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[i + 1];
			cpp1 = &inColors[i];
			cpp2 = &inColors[i + 1];
		}

		const Point& p1 = *pp1;
		const Point& p2 = *pp2;
		const Point& uvp1 = *uvpp1;
		const Point& uvp2 = *uvpp2;
		const Color cp1 = *cpp1;
		const Color cp2 = *cpp2;

		// inside
		if (p1.x >= rect.x && p2.x >= rect.x)
		{
			outUvPoints[outCount] = uvp2;
			outColors[outCount] = cp2;
			outPoints[outCount++] = p2;
		}

		// exit
		if (p1.x >= rect.x && p2.x < rect.x)
		{
			// point is to the left of rectangle
			auto t = (rect.x - p1.x) / (p2.x - p1.x);
			auto y = p1.y + (p2.y - p1.y) * t;
			auto x = rect.x;
			outUvPoints[outCount] = uvp1 + (uvp2 - uvp1) * t;
			outColors[outCount] = cp1 + (cp2 - cp1) * t;
			outPoints[outCount++] = { x, y };
		}

		// enter
		if (p1.x < rect.x && p2.x >= rect.x)
		{
			// point is to the left of rectangle
			auto t = (rect.x - p1.x) / (p2.x - p1.x);
			auto y = p1.y + (p2.y - p1.y) * t;
			auto x = rect.x;
			outUvPoints[outCount] = uvp1 + (uvp2 - uvp1) * t;
			outUvPoints[outCount + 1] = uvp2;
			outColors[outCount] = cp1 + (cp2 - cp1) * t;
			outColors[outCount + 1] = cp2;
			outPoints[outCount++] = { x, y };
			outPoints[outCount++] = p2;
		}
	}
}

static void clipRight(const Rect& rect, Point* inPoints, Point* inUvPoints, Rgba32* inColors, u32 inCount, Point* outPoints, Point* outUvPoints, Rgba32* outColors, u32& outCount)
{
	Point* pp1;
	Point* pp2;
	Point* uvpp1;
	Point* uvpp2;
	Rgba32* cpp1;
	Rgba32* cpp2;

	for (u32 i = 0; i < inCount; i++)
	{
		if (i == inCount - 1)
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[0];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[0];
			cpp1 = &inColors[i];
			cpp2 = &inColors[0];
		}
		else
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[i + 1];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[i + 1];
			cpp1 = &inColors[i];
			cpp2 = &inColors[i + 1];
		}

		const Point& p1 = *pp1;
		const Point& p2 = *pp2;
		const Point& uvp1 = *uvpp1;
		const Point& uvp2 = *uvpp2;
		const Color cp1 = *cpp1;
		const Color cp2 = *cpp2;

		// inside
		if (p1.x <= rect.right() && p2.x <= rect.right())
		{
			outUvPoints[outCount] = uvp2;
			outColors[outCount] = cp2;
			outPoints[outCount++] = p2;
		}

		// exit
		if (p1.x <= rect.right() && p2.x > rect.right())
		{
			// point is to the right of rectangle
			auto t = (rect.right() - p1.x) / (p2.x - p1.x);
			auto y = p1.y + (p2.y - p1.y) * t;
			auto x = rect.right();
			outUvPoints[outCount] = uvp1 + (uvp2 - uvp1) * t;
			outColors[outCount] = cp1 + (cp2 - cp1) * t;
			outPoints[outCount++] = { x, y };
		}

		// enter
		if (p1.x > rect.right() && p2.x <= rect.right())
		{
			// point is to the right of rectangle
			auto t = (rect.right() - p1.x) / (p2.x - p1.x);
			auto y = p1.y + (p2.y - p1.y) * t;
			auto x = rect.right();
			outUvPoints[outCount] = uvp1 + (uvp2 - uvp1) * t;
			outUvPoints[outCount + 1] = uvp2;
			outColors[outCount] = cp1 + (cp2 - cp1) * t;
			outColors[outCount + 1] = cp2;
			outPoints[outCount++] = { x, y };
			outPoints[outCount++] = p2;
		}
	}
}

static void clipTop(const Rect& rect, Point* inPoints, Point* inUvPoints, Rgba32* inColors, u32 inCount, Point* outPoints, Point* outUvPoints, Rgba32* outColors, u32& outCount)
{
	Point* pp1;
	Point* pp2;
	Point* uvpp1;
	Point* uvpp2;
	Rgba32* cpp1;
	Rgba32* cpp2;

	for (u32 i = 0; i < inCount; i++)
	{
		if (i == inCount - 1)
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[0];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[0];
			cpp1 = &inColors[i];
			cpp2 = &inColors[0];
		}
		else
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[i + 1];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[i + 1];
			cpp1 = &inColors[i];
			cpp2 = &inColors[i + 1];
		}

		const Point& p1 = *pp1;
		const Point& p2 = *pp2;
		const Point& uvp1 = *uvpp1;
		const Point& uvp2 = *uvpp2;
		const Color cp1 = *cpp1;
		const Color cp2 = *cpp2;

		// inside
		if (p1.y >= rect.top() && p2.y >= rect.top())
		{
			outUvPoints[outCount] = uvp2;
			outColors[outCount] = cp2;
			outPoints[outCount++] = p2;
		}

		// exit
		if (p1.y >= rect.top() && p2.y < rect.top())
		{
			// point is above the clip rectangle
			auto  t = (rect.top() - p1.y) / (p2.y - p1.y);
			auto x = p1.x + (p2.x - p1.x) * t;
			auto y = rect.top();
			outUvPoints[outCount] = uvp1 + (uvp2 - uvp1) * t;
			outColors[outCount] = cp1 + (cp2 - cp1) * t;
			outPoints[outCount++] = { x, y };
		}

		// enter
		if (p1.y < rect.top() && p2.y >= rect.top())
		{
			// point is above the clip rectangle
			auto t = (rect.top() - p1.y) / (p2.y - p1.y);
			auto x = p1.x + (p2.x - p1.x) * t;
			auto y = rect.top();
			outUvPoints[outCount] = uvp1 + (uvp2 - uvp1) * t;
			outUvPoints[outCount + 1] = uvp2;
			outColors[outCount] = cp1 + (cp2 - cp1) * t;
			outColors[outCount + 1] = cp2;
			outPoints[outCount++] = { x, y };
			outPoints[outCount++] = p2;
		}
	}
}

static void clipBottom(const Rect& rect, Point* inPoints, Point* inUvPoints, Rgba32* inColors, u32 inCount, Point* outPoints, Point* outUvPoints, Rgba32* outColors, u32& outCount)
{
	Point* pp1;
	Point* pp2;
	Point* uvpp1;
	Point* uvpp2;
	Rgba32* cpp1;
	Rgba32* cpp2;

	for (u32 i = 0; i < inCount; i++)
	{
		if (i == inCount - 1)
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[0];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[0];
			cpp1 = &inColors[i];
			cpp2 = &inColors[0];
		}
		else
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[i + 1];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[i + 1];
			cpp1 = &inColors[i];
			cpp2 = &inColors[i + 1];
		}

		const Point& p1 = *pp1;
		const Point& p2 = *pp2;
		const Point& uvp1 = *uvpp1;
		const Point& uvp2 = *uvpp2;
		const Color cp1 = *cpp1;
		const Color cp2 = *cpp2;

		// inside
		if (p1.y <= rect.bottom() && p2.y <= rect.bottom())
		{
			outUvPoints[outCount] = uvp2;
			outColors[outCount] = cp2;
			outPoints[outCount++] = p2;
		}

		// exit
		if (p1.y <= rect.bottom() && p2.y > rect.bottom())
		{
			// point is below the rectangle
			auto t = (rect.bottom() - p1.y) / (p2.y - p1.y);
			auto x = p1.x + (p2.x - p1.x) * t;
			auto y = rect.bottom();
			outUvPoints[outCount] = uvp1 + (uvp2 - uvp1) * t;
			outColors[outCount] = cp1 + (cp2 - cp1) * t;
			outPoints[outCount++] = { x, y };
		}

		// enter
		if (p1.y > rect.bottom() && p2.y <= rect.bottom())
		{
			// point is below the rectangle
			auto t = (rect.bottom() - p1.y) / (p2.y - p1.y);
			auto x = p1.x + (p2.x - p1.x) * t;
			auto y = rect.bottom();
			outUvPoints[outCount] = uvp1 + (uvp2 - uvp1) * t;
			outUvPoints[outCount + 1] = uvp2;
			outColors[outCount] = cp1 + (cp2 - cp1) * t;
			outColors[outCount + 1] = cp2;
			outPoints[outCount++] = { x, y };
			outPoints[outCount++] = p2;
		}
	}
}

static bool clipTriangleToRect(
	const Point& p1, const Point& p2, const Point& p3,
	const Point& uv1, const Point& uv2, const Point& uv3,
	const Rgba32 c1, const Rgba32 c2, const Rgba32 c3,
	const Rect& rect,
	Point* outPoints, Point* outUvPoints, Rgba32* outColors, u32& outCount)
{
	outCount = 0;
	auto clip1 = computeLineClipCode(p1, rect);
	auto clip2 = computeLineClipCode(p2, rect);
	auto clip3 = computeLineClipCode(p3, rect);

	if (clip1 == LineClipBit::Inside
		&& clip2 == LineClipBit::Inside
		&& clip3 == LineClipBit::Inside)
	{
		outPoints[0] = p1;
		outPoints[1] = p2;
		outPoints[2] = p3;
		outUvPoints[0] = uv1;
		outUvPoints[1] = uv2;
		outUvPoints[2] = uv3;
		outColors[0] = c1;
		outColors[1] = c2;
		outColors[2] = c3;
		outCount = 3;

		return true;
	}

	Point inPoints[] = {p1, p2, p3, Point(), Point(), Point(), Point(), Point(), Point(), Point(), Point(), Point() };
	Point inUvPoints[] = { uv1, uv2, uv3, Point(), Point(), Point(), Point(), Point(), Point(), Point(), Point(), Point() };
	Rgba32 inColors[] = { c1, c2, c3, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	u32 inCount = 3;

	outCount = 0;
	clipLeft(rect, inPoints, inUvPoints, inColors, inCount, outPoints, outUvPoints, outColors, outCount);
	inCount = 0;
	clipTop(rect, outPoints, outUvPoints, outColors, outCount, inPoints, inUvPoints, inColors, inCount);
	outCount = 0;
	clipRight(rect, inPoints, inUvPoints, inColors, inCount, outPoints, outUvPoints, outColors, outCount);
	inCount = 0;
	clipBottom(rect, outPoints, outUvPoints, outColors, outCount, inPoints, inUvPoints, inColors, inCount);

	for (u32 i = 0; i < inCount; i++)
	{
		outPoints[i] = inPoints[i];
		outUvPoints[i] = inUvPoints[i];
		outColors[i] = inColors[i];
	}

	outCount = inCount;

	return true;
}

DrawCmdLayerSplitter::DrawCmdLayerSplitter()
{}

void DrawCmdLayerSplitter::clear()
{
	for (auto& layer : layers)
	{
		layer.clear();
	}

	currentLayerIndex = 0;
}

void DrawCmdLayerSplitter::split(u32 layerCount)
{
	auto prevCount = layers.size();

	if (layers.size() != layerCount)
	{
		layers.resize(layerCount);
	}

	for (size_t i = 0; i < layers.size(); i++)
	{
		layers[i].clear();
	}
}

void DrawCmdLayerSplitter::merge()
{
	if (layers.empty())
		return;

	setLayer(0);

	for (u32 i = 1; i < layers.size(); i++)
	{
		auto& layer = layers[i];
		layers[0].insert(layers[0].end(), layer.begin(), layer.end());
		layer.clear();
	}

	HORUS_ASSERT(ctx->renderer.currentDrawCmdLayer);
	ctx->renderer.currentDrawCmdLayer->insert(ctx->renderer.currentDrawCmdLayer->end(), layers[0].begin(), layers[0].end());
}

void DrawCmdLayerSplitter::setLayer(u32 index)
{
	if (index == currentLayerIndex)
		return;

	HORUS_ASSERT(ctx->renderer.currentDrawCmdLayer);
	layers[currentLayerIndex].swap(*ctx->renderer.currentDrawCmdLayer);
	currentLayerIndex = index;
	ctx->renderer.currentDrawCmdLayer->swap(layers[currentLayerIndex]);
}

Renderer::Renderer()
{}

Renderer::~Renderer()
{}

void Renderer::setCurrentNativeWindow(HNativeWindow wnd)
{
	currentWindow = wnd;
	auto iter = windowContexts.find(wnd);

	if (iter == windowContexts.end())
	{
		windowContexts.insert({ wnd, NativeWindowRenderContext() });
		auto& wndCtx = windowContexts[wnd];
		wndCtx.textBuffer.resize(ctx->settings.textBufferMaxSize);
		wndCtx.pointBuffer.resize(ctx->settings.pointBufferMaxSize);
	}

	currentWindowContext = &windowContexts[wnd];
	currentDrawCmdLayer = &currentWindowContext->drawCmdLayers[(u32)currentWindowContext->currentDrawCmdLayer];
}

void Renderer::executeDrawCommands(HNativeWindow wnd)
{
	//TODO: should these be per window ?
	if (disableRendering || skipRender)
		return;

	currentWindowContext = &windowContexts[wnd];
	currentTexture = nullptr;
	currentTextureWidth = 0;
	currentTextureHeight = 0;
	currentBatch = nullptr;
	currentWindowContext->batches.clear();
	vertexBufferData.drawVertexCount = 0;
	currentWindow = wnd;

	for (auto& layerCmds : currentWindowContext->drawCmdLayers)
	{
		for (auto& cmd : layerCmds)
		{
			switch (cmd.type)
			{
			case DrawCommand::Type::DrawImageBordered:
				drawImageBordered(cmd.data.drawImageBordered.image, cmd.data.drawImageBordered.border, cmd.data.drawImageBordered.rect, cmd.data.drawImageBordered.scale);
				break;
			case DrawCommand::Type::DrawQuad:
				drawQuad(cmd.data.drawQuad.image, cmd.data.drawQuad.corners[0], cmd.data.drawQuad.corners[1], cmd.data.drawQuad.corners[2], cmd.data.drawQuad.corners[3]);
				break;
			case DrawCommand::Type::DrawRect:
			{
				if (cmd.data.drawRect.texture && cmd.data.drawRect.texture != currentTexture)
				{
					addBatch();
					currentTexture = cmd.data.drawRect.texture;
				}
				else
				{
					currentTexture = ctx->theme->atlas->texture;
					currentTextureWidth = ctx->theme->atlas->width;
					currentTextureHeight = ctx->theme->atlas->height;
				}

				if (clipRect(cmd.data.drawRect.rotated, cmd.data.drawRect.rect, cmd.data.drawRect.uvRect))
				{
					if (cmd.data.drawRect.rotated)
					{
						drawQuadRot90(cmd.data.drawRect.rect, cmd.data.drawRect.uvRect);
					}
					else
					{
						drawQuad(cmd.data.drawRect.rect, cmd.data.drawRect.uvRect);
					}
				}
				break;
			}
			case DrawCommand::Type::DrawText:
				computeSizeOrDrawText(cmd.data.drawText.text, cmd.data.drawText.rect, cmd.data.drawText.horizAlign, cmd.data.drawText.vertAlign, true, currentFont, cmd.data.drawText.singleLineEllipsis, cmd.data.drawText.noWordWrap);
				break;
			case DrawCommand::Type::SetColor:
				currentColor = cmd.data.color;
				break;
			case DrawCommand::Type::SetFont:
				currentFont = cmd.data.font;
				break;
			case DrawCommand::Type::ClipRect:
				currentClipRect = cmd.data.clipRect;
				break;
			case DrawCommand::Type::SetTextStyle:
				currentTextStyle = cmd.data.textStyle;
				break;
			case DrawCommand::Type::SetLineStyle:
				currentLineStyle = cmd.data.lineStyle;
				break;
			case DrawCommand::Type::SetFillStyle:
				currentFillStyle = cmd.data.fillStyle;
				break;
			case DrawCommand::Type::DrawLine:
				currentColor = currentLineStyle.color;
				drawLine(cmd.data.drawLine.a, cmd.data.drawLine.b);
				break;
			case DrawCommand::Type::DrawPolyLine:
				currentColor = currentLineStyle.color;
				drawPolyLine(cmd.data.drawPolyLine.points, cmd.data.drawPolyLine.count, cmd.data.drawPolyLine.closed);
				break;
			case DrawCommand::Type::DrawSolidTriangle:
				drawTriangle(
					cmd.data.drawTriangle.p1,
					cmd.data.drawTriangle.p2,
					cmd.data.drawTriangle.p3,
					cmd.data.drawTriangle.uv1,
					cmd.data.drawTriangle.uv2,
					cmd.data.drawTriangle.uv3,
					cmd.data.drawTriangle.c1,
					cmd.data.drawTriangle.c2,
					cmd.data.drawTriangle.c3,
					cmd.data.drawTriangle.image);
				break;
			case DrawCommand::Type::DrawQuad4Colors:
				drawQuad4Colors(
					cmd.data.drawQuad4Colors.rect,
					cmd.data.drawQuad4Colors.uvRect.contract({ ctx->settings.whiteImageUvBorder, ctx->settings.whiteImageUvBorder }),
					cmd.data.drawQuad4Colors.topLeft,
					cmd.data.drawQuad4Colors.topRight,
					cmd.data.drawQuad4Colors.bottomRight,
					cmd.data.drawQuad4Colors.bottomLeft);
				break;
			case DrawCommand::Type::SetTexture:
				HORUS_ASSERT(cmd.data.setTexture.texture);
				HORUS_ASSERT(cmd.data.setTexture.width);
				HORUS_ASSERT(cmd.data.setTexture.height);

				if (currentTexture != cmd.data.setTexture.texture && cmd.data.setTexture.texture)
				{
					currentTexture = cmd.data.setTexture.texture;
					currentTextureWidth = cmd.data.setTexture.width;
					currentTextureHeight = cmd.data.setTexture.height;
					addBatch();
				}
				break;
			case DrawCommand::Type::ClearBackground:
				ctx->settings.services.clearBackbuffer(cmd.data.color);
				break;
			case DrawCommand::Type::Callback:
				cmd.data.callback(currentWindow);
				break;
			default:
				break;
			}
		}

		layerCmds.clear();
	}

	HORUS_ASSERT(currentWindowContext->batches.size());
	ctx->settings.services.draw(vertexBufferData.vertices.data(), vertexBufferData.drawVertexCount, currentWindowContext->batches.data(), currentWindowContext->batches.size());
}

void Renderer::cmdCallback(RenderCallback callback)
{
	DrawCommand cmd(DrawCommand::Type::Callback);

	cmd.data.callback = callback;
	addDrawCommand(cmd);
}

void Renderer::cmdClearBackground(const Rgba32 color)
{
	DrawCommand cmd(DrawCommand::Type::ClearBackground);

	cmd.data.color = color;
	addDrawCommand(cmd);
}

Rect Renderer::pushClipRect(const Rect& rect, bool clipToParent)
{
	auto oldRect = currentClipRect;
	currentWindowContext->clipRectStack.push_back(currentClipRect);
	auto newRect = clipToParent ? rect.clipInside(oldRect) : rect;
	currentClipRect = newRect;

	DrawCommand cmd(DrawCommand::Type::ClipRect);

	cmd.data.clipRect = currentClipRect;
	cmd.data.clipToParent = clipToParent;
	addDrawCommand(cmd);

	return newRect;
}

void Renderer::popClipRect()
{
	if (currentWindowContext->clipRectStack.empty())
	{
		return;
	}

	currentClipRect = currentWindowContext->clipRectStack.back();
	currentWindowContext->clipRectStack.pop_back();

	DrawCommand cmd(DrawCommand::Type::ClipRect);

	cmd.data.clipRect = currentClipRect;
	cmd.data.popClipRect = true;
	addDrawCommand(cmd);
}

void Renderer::setWindowSize(const Point& size)
{
	windowSize = size;
	currentClipRect = { 0, 0, windowSize.x, windowSize.y };
	ctx->settings.services.setViewport(windowSize, currentClipRect);
}

void Renderer::pushWindowDrawCmdLayer(DrawCmdLayerType type)
{
	HORUS_ASSERT(currentWindowContext);
	currentWindowContext->drawCmdLayerTypeStack.push_back(currentWindowContext->currentDrawCmdLayer);
	currentWindowContext->currentDrawCmdLayer = type;
	currentDrawCmdLayer = &currentWindowContext->drawCmdLayers[(u32)type];
}

void Renderer::popWindowDrawCmdLayer()
{
	HORUS_ASSERT(currentWindowContext);
	currentWindowContext->currentDrawCmdLayer = currentWindowContext->drawCmdLayerTypeStack.back();
	currentWindowContext->drawCmdLayerTypeStack.pop_back();
	currentDrawCmdLayer = &currentWindowContext->drawCmdLayers[(u32)currentWindowContext->currentDrawCmdLayer];
}

void Renderer::resetWindowContexts()
{
	for (auto& wc : windowContexts)
	{
		wc.second.batches.clear();
		wc.second.clipRectStack.clear();
		wc.second.currentDrawCmdLayer = DrawCmdLayerType::Normal;
		wc.second.pointBufferPosition = 0;
		wc.second.textBufferPosition = 0;
		wc.second.textBuffer.resize(ctx->settings.textBufferMaxSize);
		wc.second.pointBuffer.resize(ctx->settings.pointBufferMaxSize);

		for (auto& layer : wc.second.drawCmdLayers)
		{
			layer.clear();
		}
	}
}

void Renderer::begin()
{
	cmdSetTexture(ctx->theme->atlas->texture, ctx->theme->atlas->width, ctx->theme->atlas->height);
}

void Renderer::end()
{
}

void Renderer::cmdSetColor(const Rgba32 newColor)
{
	DrawCommand cmd(DrawCommand::Type::SetColor);

	currentColor = newColor;
	cmd.data.color = newColor;
	addDrawCommand(cmd);
}

void Renderer::cmdSetTexture(HTexture textureHandle, u32 width, u32 height)
{
	DrawCommand cmd(DrawCommand::Type::SetTexture);

	cmd.data.setTexture.texture = textureHandle;
	cmd.data.setTexture.width = width;
	cmd.data.setTexture.height = height;
	addDrawCommand(cmd);
}

void Renderer::cmdSetFont(Font* font)
{
	DrawCommand cmd(DrawCommand::Type::SetFont);

	cmd.data.font = font;
	currentFont = font;
	addDrawCommand(cmd);
}

void Renderer::cmdSetTextUnderline(bool underline)
{
	DrawCommand cmd(DrawCommand::Type::SetTextStyle);

	currentTextStyle.underline = underline;
	cmd.data.textStyle = currentTextStyle;
	addDrawCommand(cmd);
}

void Renderer::cmdSetTextBackfill(bool backfill)
{
	DrawCommand cmd(DrawCommand::Type::SetTextStyle);

	currentTextStyle.backFill = backfill;
	cmd.data.textStyle = currentTextStyle;
	addDrawCommand(cmd);
}

void Renderer::cmdSetTextBackfillColor(const Rgba32 color)
{
	DrawCommand cmd(DrawCommand::Type::SetTextStyle);

	currentTextStyle.backFillColor = color;
	cmd.data.textStyle = currentTextStyle;
	addDrawCommand(cmd);
}

void Renderer::cmdSetLineStyle(const LineStyle& style)
{
	DrawCommand cmd(DrawCommand::Type::SetLineStyle);

	currentLineStyle = cmd.data.lineStyle = style;
	addDrawCommand(cmd);
}

void Renderer::cmdSetFillStyle(const FillStyle& style)
{
	DrawCommand cmd(DrawCommand::Type::SetFillStyle);

	currentFillStyle = cmd.data.fillStyle = style;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawImage(Image* image, const Point& position, f32 scale)
{
	DrawCommand cmd(DrawCommand::Type::DrawRect);

	cmd.data.drawRect.rect = Rect(position.x, position.y, image->rect.width * scale, image->rect.height * scale);
	cmd.data.drawRect.uvRect = image->uvRect;
	cmd.data.drawRect.rotated = image->rotated;
	cmd.data.drawRect.texture = 0;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawImage(Image* image, const Rect& rect)
{
	DrawCommand cmd(DrawCommand::Type::DrawRect);

	cmd.data.drawRect.rect = rect;
	cmd.data.drawRect.uvRect = image->uvRect;
	cmd.data.drawRect.rotated = image->rotated;
	cmd.data.drawRect.texture = 0;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawImage(Image* image, const Rect& rect, const Rect& uvRect)
{
	DrawCommand cmd(DrawCommand::Type::DrawRect);

	cmd.data.drawRect.rect = rect;
	cmd.data.drawRect.uvRect = uvRect;
	cmd.data.drawRect.rotated = image->rotated;
	cmd.data.drawRect.texture = 0;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawQuad(Image* image, const Point& p1, const Point& p2, const Point& p3, const Point& p4)
{
	DrawCommand cmd(DrawCommand::Type::DrawQuad);

	cmd.data.drawQuad.corners[0] = p1;
	cmd.data.drawQuad.corners[1] = p2;
	cmd.data.drawQuad.corners[2] = p3;
	cmd.data.drawQuad.corners[3] = p4;
	cmd.data.drawQuad.image = image;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawImageBordered(Image* image, u32 border, const Rect& rect, f32 scale)
{
	DrawCommand cmd(DrawCommand::Type::DrawImageBordered);

	cmd.data.drawImageBordered.rect = rect;
	cmd.data.drawImageBordered.image = image;
	cmd.data.drawImageBordered.border = border;
	cmd.data.drawImageBordered.scale = scale;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawImageScaledAligned(Image* image, const Rect& rect, HAlignType halign, VAlignType valign, f32 scale)
{
	f32 newWidth = image->rect.width * scale;
	f32 newHeight = image->rect.height * scale;
	Rect newRect = { rect.x, rect.y, newWidth, newHeight };

	switch (halign)
	{
	case hui::HAlignType::Left:
		break;
	case hui::HAlignType::Right:
		newRect.x = rect.right() - newWidth;
		break;
	case hui::HAlignType::Center:
		newRect.x = newRect.x + (rect.width - newWidth) / 2.0f;
		break;
	default:
		break;
	}

	switch (valign)
	{
	case hui::VAlignType::Top:
		break;
	case hui::VAlignType::Bottom:
		newRect.y = rect.bottom() - newHeight;
		break;
	case hui::VAlignType::Center:
		newRect.y = newRect.y + (rect.height - newHeight) / 2.0f;
		break;
	default:
		break;
	}

	cmdDrawImage(image, newRect);
}

void Renderer::cmdDrawImageTiled(Image* image, const Rect& destRect, const Point& offset, const Point& scale)
{
    f32 imageWidth = image->rect.width * scale.x;
    f32 imageHeight = image->rect.height * scale.y;

    if (imageWidth <= 0.0f || imageHeight <= 0.0f)
        return;

    // Normalize offset into [0, imageWidth) / [0, imageHeight)
    f32 ox = fmodf(offset.x, imageWidth);
    f32 oy = fmodf(offset.y, imageHeight);
    if (ox < 0) ox += imageWidth;
    if (oy < 0) oy += imageHeight;

    // Start tiling so pattern is shifted by offset
    f32 startX = destRect.x - ox;
    f32 startY = destRect.y - oy;

    for (f32 y = startY; y < destRect.bottom(); y += imageHeight)
    {
        for (f32 x = startX; x < destRect.right(); x += imageWidth)
        {
            // full tile rect (may extend outside destRect)
            Rect tileRect = { x, y, imageWidth, imageHeight };

            // compute intersection with destRect -> this is the visible portion we need to draw
            Rect visibleRect = tileRect.clipInside(destRect);

            if (visibleRect.width <= 0.0f || visibleRect.height <= 0.0f)
                continue;

            // Compute UV mapping for visibleRect.
            // tileRect maps to the whole image UV; visibleRect is an offset sub-rect of tileRect.
            // compute fraction of tile that is visible on each axis
            f32 visOffsetX = visibleRect.x - tileRect.x; // pixels into the tile
            f32 visOffsetY = visibleRect.y - tileRect.y;
            f32 visW = visibleRect.width;
            f32 visH = visibleRect.height;

            // base UV for the tile (full tile)
            f32 u0 = image->uvRect.x;
            f32 v0 = image->uvRect.y;
            f32 uScale = image->uvRect.width / imageWidth;
            f32 vScale = image->uvRect.height / imageHeight;

            Rect tileUvRect = {
                u0 + visOffsetX * uScale,
                v0 + visOffsetY * vScale,
                visW * uScale,
                visH * vScale
            };

            // Issue clipped tile using computed UVs
            cmdDrawImage(image, visibleRect, tileUvRect);
        }
    }
}

void Renderer::cmdDrawRectangle(const Rect& rect)
{
	Point pts[4] = {
		rect.topLeft(),
		rect.topRight(),
		rect.bottomRight(),
		rect.bottomLeft()
	};

	cmdDrawPolyLine(pts, 4, true);
}

void Renderer::cmdDrawFilledRectangle(const Rect& rect)
{
	auto image = ctx->theme->atlas->whiteImage;
	auto uvRect = image->uvRect;
	uvRect = uvRect.contract(ctx->settings.whiteImageUvBorder);
	cmdDrawImage(image, rect, uvRect);
}

void Renderer::cmdDrawRectangle4Colors(const Rect& rect, const Rgba32 topLeft, const Rgba32 topRight, const Rgba32 bottomRight, const Rgba32 bottomLeft)
{
	DrawCommand cmd(DrawCommand::Type::DrawQuad4Colors);

	cmd.data.drawQuad4Colors.rect = rect;
	cmd.data.drawQuad4Colors.uvRect = ctx->theme->atlas->whiteImage->uvRect.contract({ ctx->settings.whiteImageUvBorder, ctx->settings.whiteImageUvBorder });
	cmd.data.drawQuad4Colors.image = ctx->theme->atlas->whiteImage;
	cmd.data.drawQuad4Colors.bottomLeft = bottomLeft;
	cmd.data.drawQuad4Colors.bottomRight = bottomRight;
	cmd.data.drawQuad4Colors.topLeft = topLeft;
	cmd.data.drawQuad4Colors.topRight = topRight;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawLine(const Point& a, const Point& b)
{
	DrawCommand cmd(DrawCommand::Type::DrawLine);

	cmd.data.drawLine.a = a;
	cmd.data.drawLine.b = b;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawPolyLine(const Point* points, u32 pointCount, bool closed)
{
	DrawCommand cmd(DrawCommand::Type::DrawPolyLine);

	cmd.data.drawPolyLine.count = pointCount;
	cmd.data.drawPolyLine.closed = closed;
	cmd.data.drawPolyLine.points = &currentWindowContext->pointBuffer[currentWindowContext->pointBufferPosition];
	memcpy(currentWindowContext->pointBuffer.data() + currentWindowContext->pointBufferPosition, points, pointCount * sizeof(Point));
	currentWindowContext->pointBufferPosition += pointCount;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawSolidTriangle(const Point& p1, const Point& p2, const Point& p3, const Rgba32 c1, const Rgba32 c2, const Rgba32 c3)
{
	DrawCommand cmd(DrawCommand::Type::DrawSolidTriangle);

	cmd.data.drawTriangle.p1 = p1;
	cmd.data.drawTriangle.p2 = p2;
	cmd.data.drawTriangle.p3 = p3;
	auto uvRc =  ctx->theme->atlas->whiteImage->uvRect;
	uvRc = uvRc.contract(ctx->settings.whiteImageUvBorder);
	cmd.data.drawTriangle.uv1 = uvRc.topLeft();
	cmd.data.drawTriangle.uv2 = uvRc.topRight();
	cmd.data.drawTriangle.uv3 = uvRc.bottomRight();
	cmd.data.drawTriangle.c1 = c1;
	cmd.data.drawTriangle.c2 = c2;
	cmd.data.drawTriangle.c3 = c3;
	cmd.data.drawTriangle.image = ctx->theme->atlas->whiteImage;
	addDrawCommand(cmd);
}

FontTextSize Renderer::computeSizeOrDrawText(
	const char* text,
	const Rect& rect,
	HAlignType horizAlign,
	VAlignType vertAlign,
	bool doDraw,
	Font* font,
	bool singleLineEllipsis,
	bool noWordWrap)
{
	if (!text || !strcmp(text, ""))
	{
		return FontTextSize();
	}

	// reuse text cache to get utf32 string
	const Utf32String& utext = *ctx->textCache.getText(text);
	return computeSizeOrDrawText(utext.data(), (u32)utext.size(), rect, horizAlign, vertAlign, doDraw, font, singleLineEllipsis, noWordWrap);
}

FontTextSize Renderer::computeSizeOrDrawText(
	const GlyphCode* const text,
	u32 size,
	const Rect& rect,
	HAlignType horizAlign,
	VAlignType vertAlign,
	bool doDraw,
	Font* font,
	bool singleLineEllipsis,
	bool noWordWrap)
{
	FontTextSize fsize;
	Font* fnt = font ? font : currentFont;

	if (!fnt || size == 0)
	{
		return fsize;
	}

	// Helper: compute ellipsis width (prefer single U+2026 glyph, fall back to three dots)
	auto computeEllipsisWidth = [&](Font* ff) -> f32 {
		const GlyphCode uniEll = 0x2026;
		auto gEll = ff->getGlyph(uniEll);

		if (gEll)
			return gEll->advanceX;

		// fallback to three ASCII dots, include basic kerning conservatively
		auto gDot = ff->getGlyph((GlyphCode)'.');

		if (!gDot) return 0.0f;

		// approximate three dots advance (simple approximation)
		return gDot->advanceX * 3.0f;
	};

	// If singleLineEllipsis is requested, produce measurement/draw for exactly one line
	if (singleLineEllipsis)
	{
		// find end of first logical line (stop at \n or end)
		u32 lineEnd = 0;

		while (lineEnd < size && text[lineEnd] != '\n') ++lineEnd;

		// defensive: zero width rect -> nothing to draw
		if (rect.width <= 0.0f)
		{
			// still provide height for one line
			fsize.height = fnt->getMetrics().height;
			fsize.width = 0.0f;

			return fsize;
		}

		// measure how many glyphs fit when appending an ellipsis if truncated
		f32 ellWidth = computeEllipsisWidth(fnt);
		u32 lastChr = 0;
		f32 currWidth = 0.0f;
		u32 fitCount = 0;
		bool forceTruncationLogic = false;

		// 1. Check if the entire line fits without truncation.
		{
			f32 w = 0.0f;
			u32 lChr = 0;
			u32 cnt = 0;

			for (u32 i = 0; i < lineEnd; ++i)
			{
				auto chr = text[i];
				auto glyph = fnt->getGlyph(chr);

				if (!glyph) continue;

				auto kern = fnt->getKerning(lChr, chr);
				w += glyph->advanceX + kern;
				lChr = chr;
				cnt++;
			}

			// Tolerance for floating point precision issues
			if (w <= rect.width + 0.001f)
			{
				currWidth = w;
				fitCount = cnt;
			}
			else
			{
				forceTruncationLogic = true;
			}
		}

		if (forceTruncationLogic)
		{
			currWidth = 0.0f;
			fitCount = 0;
			lastChr = 0;

			// Determine how many glyphs can be drawn while leaving room for ellipsis if needed
			for (u32 i = 0; i < lineEnd; ++i)
			{
				auto chr = text[i];
				auto glyph = fnt->getGlyph(chr);

				if (!glyph)
					continue;

				auto kern = fnt->getKerning(lastChr, chr);
				f32 adv = glyph->advanceX + kern;

				// If entire text fits without ellipsis, accept it.
				// If not, ensure we leave space for ellipsis.
				bool wouldExceed = (currWidth > rect.width);
				bool needsEllipsis = (lineEnd > 0 && (lineEnd - 0) > (i + 1)); // more glyphs after this one

				if (wouldExceed)
				{
					// can't accept this glyph; stop
					break;
				}

				// If there are remaining glyphs after this and adding them would later overflow,
				// ensure we have room for ellipsis now. Conservative check: if next glyph would push us
				// over and we don't have ellipsis room, stop before adding current glyph.
				if (i + 1 < lineEnd)
				{
					// estimate minimal remaining (we don't know exactly next widths) - ensure ellipsis fits after adding this glyph
					if (currWidth + adv + ellWidth > rect.width)
					{
						// if even zero glyphs fit but ellipsis itself fits, show only ellipsis
						if (fitCount == 0)
						{
							// if ellipsis itself doesn't fit, we'll clamp width to rect.width and return
							if (ellWidth > rect.width)
							{
								currWidth = rect.width;
								fitCount = 0;
								break;
							}
						}
						// stop before adding this glyph so we can append ellipsis
						break;
					}
				}

				currWidth += adv;
				lastChr = chr;
				++fitCount;
			}
		}

		// Decide final displayed width:
		bool didTruncate = fitCount < lineEnd;
		f32 displayedWidth = currWidth;

		if (didTruncate)
		{
			// if nothing fits but ellipsis fits, display only ellipsis
			if (fitCount == 0)
			{
				displayedWidth = std::min(ellWidth, rect.width);
			}
			else
			{
				displayedWidth = currWidth + ellWidth;
				if (displayedWidth > rect.width) displayedWidth = rect.width;
			}
		}
		else
		{
			// no truncation, displayedWidth is full measured width (currWidth) but ensure not exceeding rect
			if (displayedWidth > rect.width) displayedWidth = rect.width;
		}

		// fill FontTextSize results
		fsize.width = displayedWidth;
		fsize.height = fnt->getMetrics().height;
		fsize.maxLength = fitCount;

		// DRAW pass: draw the single aligned line with optional ellipsis
		if (doDraw)
		{
			Point pos;

			// vertical align: place baseline based on requested vertAlign (single line)
			switch (vertAlign)
			{
			case hui::VAlignType::Top:
				pos.y = rect.y + fnt->getMetrics().ascender;
				break;
			case hui::VAlignType::Bottom:
				pos.y = rect.bottom() + fnt->getMetrics().descender;
				break;
			case hui::VAlignType::Center:
				pos.y = rect.y + (rect.height - (fnt->getMetrics().ascender - fnt->getMetrics().descender)) * .5f + fnt->getMetrics().ascender;
				break;
			default:
				pos.y = rect.y + fnt->getMetrics().ascender;
				break;
			}

			// horizontal align based on displayedWidth (includes ellipsis if truncated)
			switch (horizAlign)
			{
			case hui::HAlignType::Left:
				pos.x = rect.x;
				break;
			case hui::HAlignType::Right:
				pos.x = rect.right() - displayedWidth;
				break;
			case hui::HAlignType::Center:
				pos.x = rect.x + (rect.width - displayedWidth) / 2.0f;
				break;
			default:
				pos.x = rect.x;
				break;
			}

			pos.x = round(pos.x);
			pos.y = round(pos.y);

			// draw the fitted glyphs
			GlyphCode lastDrawChr = 0;
			for (u32 j = 0; j < fitCount; ++j)
			{
				auto chr = text[j];
				auto glyph = fnt->getGlyph(chr);
				auto img = fnt->getGlyphImage(chr);

				if (!glyph) continue;
				auto kern = fnt->getKerning(lastDrawChr, chr);
				pos.x += kern;
				if (img) drawTextGlyph(img, { pos.x + glyph->bitmapLeft, pos.y - glyph->bitmapTop });
				pos.x += glyph->advanceX;
				lastDrawChr = chr;
			}

			// draw ellipsis if truncated
			if (didTruncate)
			{
				const GlyphCode uniEll = 0x2026;
				auto gEll = fnt->getGlyph(uniEll);
				if (gEll && fnt->getGlyphImage(uniEll))
				{
					auto kern = fnt->getKerning(lastDrawChr, uniEll);
					pos.x += kern;
					drawTextGlyph(fnt->getGlyphImage(uniEll), { pos.x + gEll->bitmapLeft, pos.y - gEll->bitmapTop });
					// advance x not needed further
				}
				else
				{
					// fallback: draw up to three '.' glyphs as ellipsis
					auto gDot = fnt->getGlyph((GlyphCode)'.');
					auto imgDot = fnt->getGlyphImage((GlyphCode)'.');
					for (int d = 0; d < 3; ++d)
					{
						if (!gDot) break;
						auto kern = fnt->getKerning(lastDrawChr, (GlyphCode)'.');
						pos.x += kern;
						if (imgDot) drawTextGlyph(imgDot, { pos.x + gDot->bitmapLeft, pos.y - gDot->bitmapTop });
						pos.x += gDot->advanceX;
						lastDrawChr = (GlyphCode)'.';
					}
				}
			}
		}

		return fsize;
	}

	// measurement with wrapping (port of original Font::computeTextSize)
	u32 lastChr = 0;
	u32 lineCount = 1;
	f32 crtLineWidth = 0.0f;
	f32 crtWordWidth = 0.0f;
	u32 currentLineChars = 0;
	u32 longestLineChars = 0;
	u32 lastWordIndex = 0;
	u32 lineStart = 0;

	lines.clear();

	for (u32 i = 0; i < size; ++i)
	{
		auto chr = text[i];

		// explicit newline -> finish current line and start new one
		// Skip newline handling if noWordWrap is enabled - treat as space instead
		if (!noWordWrap && chr == '\n')
		{
			// finalize this line
			f32 segmentWidth = crtLineWidth;
			lines.push_back({ lineStart, currentLineChars, segmentWidth });

			if (fsize.width < segmentWidth)
				fsize.width = segmentWidth;

			if (longestLineChars < currentLineChars)
				longestLineChars = currentLineChars;

			// reset for next line
			crtLineWidth = 0.0f;
			crtWordWidth = 0.0f;
			currentLineChars = 0;
			lastChr = 0;
			lastWordIndex = i + 1;
			lineStart = i + 1;
			++lineCount;
			continue;
		}
		else if (noWordWrap && chr == '\n')
		{
			// When noWordWrap is enabled, treat newlines as spaces
			chr = ' ';
		}

		auto glyph = fnt->getGlyph(chr);
		if (!glyph)
			continue;

		// track word boundaries
		if (chr == ' ')
		{
			lastWordIndex = i + 1;
			crtWordWidth = 0.0f;
		}

		// compute advance including kerning with previous glyph on same line
		auto kern = fnt->getKerning(lastChr, chr);
		f32 glyphAdvance = glyph->advanceX + kern;
		f32 projectedLineWidth = crtLineWidth + glyphAdvance;
		f32 projectedWordWidth = crtWordWidth + glyphAdvance;

		// wrapping when maxWidth specified (rect.width used as constraint)
		// Skip wrapping if noWordWrap is enabled - render as single line
		if (!noWordWrap && projectedLineWidth > rect.width)
		{
			// If we are at start of line we must break inside word (force at least one glyph)
			if (currentLineChars == 0 || projectedWordWidth >= rect.width)
			{
				// find break position inside the word (from lastWordIndex to i)
				f32 wordSize = 0.0f;
				u32 breakPos = lastWordIndex;
				GlyphCode localLast = 0;

				for (u32 k = lastWordIndex; k <= i; ++k)
				{
					auto g2 = fnt->getGlyph(text[k]);
					if (!g2) continue;

					auto kern2 = fnt->getKerning(localLast, text[k]);
					f32 cw = g2->advanceX + kern2;
					wordSize += cw;

					if (wordSize >= (f32)rect.width)
					{
						// break AFTER k so that we place glyphs up to k in this line
						breakPos = k + 1;
						break;
					}

					localLast = text[k];
				}

				// ensure we advance at least one glyph if breakPos didn't move
				if (breakPos <= lineStart)
					breakPos = lineStart + 1;

				// finalize current line [lineStart .. breakPos-1]
				u32 lineLen = breakPos > lineStart ? breakPos - lineStart : 0;

				// compute actual width for the pushed segment to update fsize.width correctly
				f32 segmentWidth = 0.0f;
				GlyphCode segLast = 0;
				for (u32 k = lineStart; k < lineStart + lineLen && k < size; ++k)
				{
					auto g2 = fnt->getGlyph(text[k]);
					if (!g2) continue;
					auto kern2 = fnt->getKerning(segLast, text[k]);
					segmentWidth += g2->advanceX + kern2;
					segLast = text[k];
				}

				lines.push_back({ lineStart, lineLen, segmentWidth });

				if (fsize.width < segmentWidth) fsize.width = segmentWidth;

				if (longestLineChars < currentLineChars) longestLineChars = currentLineChars;
				++lineCount;

				// start new line at breakPos
				crtLineWidth = 0.0f;
				crtWordWidth = 0.0f;
				currentLineChars = 0;
				lastChr = 0;
				lineStart = breakPos;
				lastWordIndex = breakPos;

				// advance lineStart to first non-space to avoid leading spaces
				while (lineStart < size && text[lineStart] == ' ')
					++lineStart;

				lastWordIndex = lineStart;

				// set iterator so loop will process lineStart next
				if (lineStart > 0) i = lineStart - 1;
				else i = lineStart;

				continue;
			}
			else
			{
				// move whole word to next line (only valid when there's already content on current line)
				// avoid pushing zero-length lines
				// Previously we pushed `currentLineChars` which could include a partial word.
				// Instead push only up to the last word boundary (lastWordIndex).
				u32 pushLen = 0;
				if (lastWordIndex > lineStart)
					pushLen = lastWordIndex - lineStart;

				if (pushLen > 0)
				{
					// compute actual width for the pushed segment (lineStart .. lineStart+pushLen-1)
					f32 segmentWidth = 0.0f;
					GlyphCode segLast = 0;
					for (u32 k = lineStart; k < lineStart + pushLen && k < size; ++k)
					{
						auto g2 = fnt->getGlyph(text[k]);
						if (!g2) continue;
						auto kern2 = fnt->getKerning(segLast, text[k]);
						segmentWidth += g2->advanceX + kern2;
						segLast = text[k];
					}

					lines.push_back({ lineStart, pushLen, segmentWidth });

					if (fsize.width < segmentWidth) fsize.width = segmentWidth;

					if (longestLineChars < pushLen) longestLineChars = pushLen;
					++lineCount;
				}

				// start new line at lastWordIndex (skip leading spaces)
				crtLineWidth = 0.0f;
				crtWordWidth = 0.0f;
				currentLineChars = 0;
				lastChr = 0;

				u32 newStart = lastWordIndex;
				while (newStart < size && text[newStart] == ' ')
					++newStart;

				lineStart = newStart;
				lastWordIndex = newStart;

				if (lineStart > 0) i = lineStart - 1;
				else i = lineStart;

				continue;
			}
		}

		// accept glyph into current line
		crtLineWidth = projectedLineWidth;
		crtWordWidth = projectedWordWidth;
		++currentLineChars;
		lastChr = chr;

		// keep vertical metrics for centering
		f32 top = glyph->bearingY;
		f32 bottom = -(glyph->pixelHeight - glyph->bearingY);
		f32 glyphHeight = fabs(top - bottom);

		if (fsize.maxGlyphHeight < glyphHeight) fsize.maxGlyphHeight = glyphHeight;
		if (fsize.maxBearingY < glyph->bearingY) fsize.maxBearingY = glyph->bearingY;
	}

	// finalize last line
	if (fsize.width < crtLineWidth) fsize.width = crtLineWidth;
	lines.push_back({ lineStart, currentLineChars, crtLineWidth });

	// If text ends with a trailing newline, remove the artificially pushed empty line
	if (size > 0 && text[size - 1] == '\n')
	{
		if (lineCount > 0) --lineCount;
	}

	// finalize longest line char count with the last line
	if (longestLineChars < currentLineChars) longestLineChars = currentLineChars;

	// total height = number of lines * metrics.height
	fsize.height = (f32)lineCount * fnt->getMetrics().height;
	fsize.maxLength = longestLineChars;

	// DRAW pass (if requested) - iterate lines and draw glyphs per-line
	if (doDraw)
	{
		Point pos;

		switch (vertAlign)
		{
		case hui::VAlignType::Top:
			pos.y = rect.y + fnt->getMetrics().ascender;
			break;
		case hui::VAlignType::Bottom:
			pos.y = rect.bottom() + fnt->getMetrics().descender;
			break;
		case hui::VAlignType::Center:
			pos.y = rect.y + (rect.height - (fnt->getMetrics().ascender - fnt->getMetrics().descender) * (f32)lineCount) * .5f + fnt->getMetrics().ascender;
			break;
		default:
			pos.y = rect.y;
			break;
		}

		pos.y = round(pos.y);

		const f32 lineHeight = fnt->getMetrics().height;

		for (size_t li = 0; li < lines.size(); ++li)
		{
			auto start = lines[li].start;
			auto len = lines[li].len;
			auto lineWidth = lines[li].width;

			switch (horizAlign)
			{
			case hui::HAlignType::Left:
				pos.x = rect.x;
				break;
			case hui::HAlignType::Right:
				pos.x = rect.right() - lineWidth;
				break;
			case hui::HAlignType::Center:
				pos.x = rect.x + (rect.width - lineWidth) / 2.0f;
				break;
			default:
				pos.x = rect.x;
				break;
			}

			pos.x = round(pos.x);
			const f32 startX = pos.x;

			// if the last line can be an empty line created by trailing newline, skip drawing glyphs for zero len
			if (len == 0)
			{
				pos.x = startX;
				pos.y += lineHeight;
				continue;
			}

			GlyphCode lastDrawChr = 0;

			// draw glyphs in the line
			for (u32 j = 0; j < len; ++j)
			{
				u32 idx = start + j;
				if (idx >= size) break;
				auto chr = text[idx];

				auto glyph = fnt->getGlyph(chr);
				auto img = fnt->getGlyphImage(chr);

				if (!glyph)
				{
					continue;
				}

				auto kern = fnt->getKerning(lastDrawChr, chr);
				pos.x += kern;
				if (img) drawTextGlyph(img, { pos.x + glyph->bitmapLeft, pos.y - glyph->bitmapTop });
				pos.x += glyph->advanceX;
				lastDrawChr = chr;
			}

			// move to next line baseline
			pos.y += lineHeight;
		}

		// render underline (single continuous underline across computed width)
		if (currentTextStyle.underline)
		{
			auto image = ctx->theme->atlas->whiteImage;

			// underline spans the whole measured width (max line width)
			Rect underlineRect(
				rect.x,
				rect.y - fnt->getMetrics().underlinePosition,
				fsize.width,
				fnt->getMetrics().underlineThickness);

			if (!image->rotated)
			{
				drawQuad(underlineRect, image->uvRect);
			}
			else
			{
				drawQuadRot90(
					{
						rect.x,
						rect.y - fnt->getMetrics().underlinePosition,
						fsize.width,
						fnt->getMetrics().underlineThickness
					},
					image->uvRect);
			}
		}
	}

	return fsize;
}

void Renderer::drawAtlasRegion(bool rotated, const Rect& rect, const Rect& uvRect)
{
	Rect newRect = rect, newUvRect = uvRect;

	if (!clipRect(rotated, newRect, newUvRect))
	{
		return;
	}

	if (rotated)
		drawQuadRot90(newRect, newUvRect);
	else
		drawQuad(newRect, newUvRect);
}

void Renderer::drawTextGlyph(Image* image, const Point& position)
{
	Rect rect = Rect(
		position.x,
		position.y,
		image->rect.width,
		image->rect.height);
	Rect uvRect = image->uvRect;

	if (!clipRect(image->rotated, rect, uvRect))
		return;

	if (image->rotated)
		drawQuadRot90(rect, uvRect);
	else
		drawQuad(rect, uvRect);
}

void Renderer::drawQuad(Image* image, const Point& p1, const Point& p2, const Point& p3, const Point& p4)
{
	drawTriangle(p1, p2, p3, image->uvRect.topLeft(), image->uvRect.topRight(), image->uvRect.bottomRight(), currentColor, currentColor, currentColor, image);
	drawTriangle(p1, p3, p4, image->uvRect.topLeft(), image->uvRect.bottomRight(), image->uvRect.bottomLeft(), currentColor, currentColor, currentColor, image);
}

void Renderer::drawQuad(const Rect& rect, const Rect& uvRect)
{
	needToAddVertexCount(6);

	u32 i = vertexBufferData.drawVertexCount;

	vertexBufferData.vertices[i].position = rect.topLeft();
	vertexBufferData.vertices[i].uv = uvRect.topLeft();
	vertexBufferData.vertices[i].color = currentColor;
	i++;

	vertexBufferData.vertices[i].position = rect.topRight();
	vertexBufferData.vertices[i].uv = uvRect.topRight();
	vertexBufferData.vertices[i].color = currentColor;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomLeft();
	vertexBufferData.vertices[i].uv = uvRect.bottomLeft();
	vertexBufferData.vertices[i].color = currentColor;
	i++;

	// 2nd triangle
	vertexBufferData.vertices[i].position = rect.topRight();
	vertexBufferData.vertices[i].uv = uvRect.topRight();
	vertexBufferData.vertices[i].color = currentColor;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomRight();
	vertexBufferData.vertices[i].uv = uvRect.bottomRight();
	vertexBufferData.vertices[i].color = currentColor;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomLeft();
	vertexBufferData.vertices[i].uv = uvRect.bottomLeft();
	vertexBufferData.vertices[i].color = currentColor;
	i++;

	vertexBufferData.drawVertexCount = i;
	currentBatch->vertexCount += 6;
}

void Renderer::drawQuad4Colors(const Rect& rect, const Rect& uvRect, const Rgba32 colTopLeft, const Rgba32 colTopRight, const Rgba32 colBottomRight, const Rgba32 colBottomLeft)
{
	drawTriangle(rect.topLeft(), rect.topRight(), rect.bottomRight(),
		uvRect.topLeft(), uvRect.topRight(), uvRect.bottomRight(),
		colTopLeft, colTopRight, colBottomRight, ctx->theme->atlas->whiteImage);
	drawTriangle(rect.topLeft(), rect.bottomRight(), rect.bottomLeft(),
		uvRect.topLeft(), uvRect.bottomRight(), uvRect.bottomLeft(),
		colTopLeft, colBottomRight, colBottomLeft, ctx->theme->atlas->whiteImage);
}

void Renderer::drawQuadRot90(const Rect& rect, const Rect& uvRect)
{
	needToAddVertexCount(6);

	u32 i = vertexBufferData.drawVertexCount;
	Point t0(uvRect.topLeft());
	Point t1(uvRect.topRight());
	Point t2(uvRect.bottomRight());
	Point t3(uvRect.bottomLeft());

	// t3-------t0
	//  |     /  |
	//  |  /     |
	// t2-------t1

	vertexBufferData.vertices[i].position = rect.topLeft();
	vertexBufferData.vertices[i].uv = t3;
	vertexBufferData.vertices[i].color = currentColor;
	i++;

	vertexBufferData.vertices[i].position = rect.topRight();
	vertexBufferData.vertices[i].uv = t0;
	vertexBufferData.vertices[i].color = currentColor;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomLeft();
	vertexBufferData.vertices[i].uv = t2;
	vertexBufferData.vertices[i].color = currentColor;
	i++;

	// 2nd triangle

	vertexBufferData.vertices[i].position = rect.topRight();
	vertexBufferData.vertices[i].uv = t0;
	vertexBufferData.vertices[i].color = currentColor;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomRight();
	vertexBufferData.vertices[i].uv = t1;
	vertexBufferData.vertices[i].color = currentColor;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomLeft();
	vertexBufferData.vertices[i].uv = t2;
	vertexBufferData.vertices[i].color = currentColor;
	i++;

	vertexBufferData.drawVertexCount = i;
	currentBatch->vertexCount += 6;
}

void Renderer::drawImageBordered(Image* image, u32 border, const Rect& rect, f32 scale)
{
	Rect screenRect = rect;

	screenRect.x = round(screenRect.x);
	screenRect.y = round(screenRect.y);
	screenRect.width = round(screenRect.width);
	screenRect.height = round(screenRect.height);

	if (screenRect.width < 1
		|| screenRect.height < 1)
	{
		return;
	}

	f32 borderW = border * scale;
	f32 borderH = border * scale;

	// resize border if rect is smaller than the border x 2
	if (screenRect.width < borderW * 2.f)
	{
		borderW = (f32)screenRect.width / 2.0f;
	}

	if (screenRect.height < borderH * 2.f)
	{
		borderH = (f32)screenRect.height / 2.0f;
	}

	//TODO: optimize this, maybe special shader for 9 cell?
	// compute the UV sizes for the border corners
	f32 fborder = (f32)border;
	f32 borderU = fborder / (f32)currentTextureWidth;
	f32 borderV = fborder / (f32)currentTextureHeight;
	HORUS_ASSERT(currentTextureWidth);
	HORUS_ASSERT(currentTextureHeight);
	// this is the double size, two borders used in computations
	f32 borderU2 = borderU * 2.0f;
	f32 borderV2 = borderV * 2.0f;

	Rect topLeftUV;
	Rect topMiddleUV;
	Rect topRightUV;
	Rect middleLeftUV;
	Rect middleCenterUV;
	Rect middleRightUV;
	Rect bottomLeftUV;
	Rect bottomMiddleUV;
	Rect bottomRightUV;

	if (!image->rotated)
	{
		topLeftUV = { image->uvRect.x, image->uvRect.y, borderU, borderV };
		topMiddleUV = { image->uvRect.x + borderU, image->uvRect.y, image->uvRect.width - borderU2, borderV };
		topRightUV = { image->uvRect.right() - borderU, image->uvRect.y, borderU, borderV };

		middleLeftUV = { image->uvRect.x, image->uvRect.y + borderV, borderU, image->uvRect.height - borderV2 };
		middleCenterUV = { image->uvRect.x + borderU, image->uvRect.y + borderV, image->uvRect.width - borderU2, image->uvRect.height - borderV2 };
		middleRightUV = { image->uvRect.right() - borderU, image->uvRect.y + borderV, borderU, image->uvRect.height - borderV2 };

		bottomLeftUV = { image->uvRect.x, image->uvRect.bottom() - borderV, borderU, borderV };
		bottomMiddleUV = { image->uvRect.x + borderU, image->uvRect.bottom() - borderV, image->uvRect.width - borderU2, borderV };
		bottomRightUV = { image->uvRect.right() - borderU, image->uvRect.bottom() - borderV, borderU, borderV };
	}
	else
	{
		auto texCoords = image->uvRect;

		topLeftUV = { texCoords.x, texCoords.bottom() - borderV, borderU, borderV };
		topMiddleUV = { texCoords.x, texCoords.y + borderV, borderU, texCoords.height - borderV2 };
		topRightUV = { texCoords.x, texCoords.y, borderU, borderV };

		middleLeftUV = { texCoords.x + borderU, texCoords.bottom() - borderV, texCoords.width - borderU2, borderV };
		middleCenterUV = { texCoords.x + borderU, texCoords.y + borderV, texCoords.width - borderU2, texCoords.height - borderV2 };
		middleRightUV = { texCoords.x + borderU, texCoords.y, texCoords.width - borderU2, borderV };

		bottomLeftUV = { texCoords.right() - borderU, texCoords.bottom() - borderV, borderU, borderV };
		bottomMiddleUV = { texCoords.right() - borderU, texCoords.y + borderV, borderU, texCoords.height - borderV2 };
		bottomRightUV = { texCoords.right() - borderU, texCoords.y, borderU, borderV };
	}

	f32 borderW2 = borderW * 2.0f;
	f32 borderH2 = borderH * 2.0f;

	Rect topLeft = { 0, 0, borderW, borderH };
	Rect topMiddle = { 0, 0, screenRect.width - borderW2, borderH };
	Rect topRight = { 0, 0, borderW, borderH };

	Rect middleLeft = { 0, 0, borderW, screenRect.height - borderW2 };
	Rect middleCenter = { 0, 0, screenRect.width - borderW2, screenRect.height - borderH2 };
	Rect middleRight = { 0, 0, borderW, screenRect.height - borderH2 };

	Rect bottomLeft = { 0, 0, borderW, borderH };
	Rect bottomMiddle = { 0, 0, screenRect.width - borderW2, borderH };
	Rect bottomRight = { 0, 0, borderW, borderH };

	// top row
	drawAtlasRegion(
		image->rotated,
		Rect(
			screenRect.x, screenRect.y,
			topLeft.width, topLeft.height),
		topLeftUV);
	drawAtlasRegion(
		image->rotated,
		Rect(
			screenRect.x + topLeft.width,
			screenRect.y,
			screenRect.width - topLeft.width - topRight.width,
			topMiddle.height),
		topMiddleUV);
	drawAtlasRegion(
		image->rotated,
		Rect(
			screenRect.right() - topRight.width,
			screenRect.y,
			topRight.width,
			topRight.height),
		topRightUV);

	// middle row

	{
		drawAtlasRegion(
			image->rotated,
			Rect(
				screenRect.x,
				screenRect.y + topLeft.height,
				middleLeft.width,
				screenRect.height - topLeft.height - bottomLeft.height),
			middleLeftUV);

		drawAtlasRegion(
			image->rotated,
			Rect(
				screenRect.x + topLeft.width,
				screenRect.y + topLeft.height,
				screenRect.width - middleLeft.width - middleRight.width,
				screenRect.height - topMiddle.height - bottomMiddle.height),
			middleCenterUV);

		drawAtlasRegion(
			image->rotated,
			Rect(
				screenRect.right() - middleRight.width,
				screenRect.y + topRight.height,
				middleRight.width,
				screenRect.height - topRight.height - bottomRight.height),
			middleRightUV);
	}

	// bottom row
	drawAtlasRegion(
		image->rotated,
		Rect(
			screenRect.x,
			screenRect.bottom() - bottomLeft.height,
			bottomLeft.width, bottomLeft.height),
		bottomLeftUV);
	drawAtlasRegion(
		image->rotated,
		Rect(
			screenRect.x + bottomLeft.width,
			screenRect.bottom() - bottomMiddle.height,
			screenRect.width - bottomLeft.width - bottomRight.width,
			bottomMiddle.height),
		bottomMiddleUV);
	drawAtlasRegion(
		image->rotated,
		Rect(
			screenRect.right() - bottomRight.width,
			screenRect.bottom() - bottomRight.height,
			bottomRight.width,
			bottomRight.height),
		bottomRightUV);
}

void Renderer::drawLine(const Point& a, const Point& b)
{
	Point pts[] = { a, b };
	drawPolyLine(pts, 2, false);
}

void Renderer::drawPolyLine(const Point* points, u32 pointCount, bool closed)
{
	std::vector<Point> stippleLines;
	std::vector<bool> stippleLinesSkip;
	Point* pts = (Point*)points;

	if (currentLineStyle.useStipple)
	{
		stippleLines.clear();
		stippleLinesSkip.clear();
		f32 remainder = 0;
		u32 oldJ = 0;
		bool skip = false;
		f32 totalsize = 0;

		for (u32 j = 0; j < currentLineStyle.stipplePatternCount; j++)
		{
			totalsize += currentLineStyle.stipplePattern[j];
		}

		f32 offs = (int)currentLineStyle.stipplePhase % (int)totalsize;
		f32 tot = 0;

		if (offs > 0.0f)
		{
			for (u32 j = 0; j < currentLineStyle.stipplePatternCount; j++)
			{
				// if its in this stipple cell
				if (offs >= tot && offs < (tot + currentLineStyle.stipplePattern[j]))
				{
					oldJ = j;
					remainder = (tot + currentLineStyle.stipplePattern[j]) - offs;
					break;
				}

				tot += currentLineStyle.stipplePattern[j];
				skip = !skip;
			}
		}

		bool bail = false;
		Point line;
		f32 totalLineLength;
		f32 currentLength;
		u32 idx;

		for (u32 i = 0; i < pointCount; i++)
		{
			if (stippleLines.empty() ||
				(stippleLines.size() && points[i] != stippleLines.back()))
			{
				stippleLines.push_back(points[i]);
				stippleLinesSkip.push_back(skip);
			}

			idx = i + 1;

			if (i == pointCount - 1)
			{
				if (!closed) break;
				idx = 0;
			}

			line = points[idx] - points[i];
			totalLineLength = line.getLength();
			currentLength = remainder;
			bail = false;

			while (!bail)
			{
				for (u32 j = oldJ; j < currentLineStyle.stipplePatternCount; j++)
				{
					f32 stippleSize = currentLineStyle.stipplePattern[j];

					// if we're outside the line
					if (currentLength + stippleSize > totalLineLength)
					{
						// remember the stipple index, we'll use it on the next segment
						oldJ = j;
						remainder = (currentLength + stippleSize) - totalLineLength;
						bail = true;
						break;
					}

					if (remainder > 0)
					{
						remainder = 0;
					}
					else
					{
						currentLength += stippleSize;
					}

					f32 t = currentLength / totalLineLength;
					Point pt = points[i] + line * t;

					if (stippleLines.empty()
						|| (stippleLines.size() && pt != stippleLines.back()))
					{
						stippleLines.push_back(pt);
						// toggle pattern skip
						skip = !skip;
						stippleLinesSkip.push_back(skip);
					}
				}

				if (!bail)
					oldJ = 0;
			}

			if (bail && ((i == pointCount - 2 && !closed) || (i == pointCount - 1 && closed)) )
			{
				break;
			}
		}

		pts = stippleLines.data();
		pointCount = stippleLines.size();
	}

	Point d1;
	Point d2;
	Point n1;
	Point n2;
	Point lastP11, lastP12;
	Point lastN2;
	Point p11;
	Point p12;
	Point p21;
	Point p22;
	auto lineImage = ctx->theme->atlas->whiteImage;
	const auto color = currentLineStyle.color;
	auto rcUv = lineImage->uvRect;

	rcUv.x += ctx->settings.whiteImageUvBorder;
	rcUv.y += ctx->settings.whiteImageUvBorder;
	rcUv.width -= ctx->settings.whiteImageUvBorder * 2.0f;
	rcUv.height -= ctx->settings.whiteImageUvBorder * 2.0f;

	const auto uv11 = rcUv.topLeft();
	const auto uv12 = rcUv.topRight();
	const auto uv22 = rcUv.bottomRight();
	const auto uv21 = rcUv.bottomLeft();
	const f32 half = currentLineStyle.width  / 2.0f;

	// P11----------P21
	//  |            |
	// P12----------P22
	f32 extrudeScale1 = 1;
	f32 extrudeScale2 = 1;
	f32 lastExtrudeScale1 = 1;
	f32 lastExtrudeScale2 = 1;
	f32 firstExtrudeScale = 1;
	f32 sinAngle = 0;
	Point firstN;
	Point seg1, seg2;
	bool stippleToggle = true;

	for (auto p = 0; p < pointCount; p++)
	{
		extrudeScale1 = 1;
		extrudeScale2 = 1;

		if (!closed && p == pointCount - 1)
			break;

		if (p == 0)
		{
			if (closed)
			{
				seg1 = Point(pts[pointCount - 1].x - pts[p].x, pts[pointCount - 1].y - pts[p].y);
				seg2 = Point(pts[p + 1].x - pts[p].x, pts[p + 1].y - pts[p].y);
				seg1.normalize();
				seg2.normalize();
				d1 = seg1 + seg2;
				d1.normalize();
				sinAngle = (d1.x * seg2.y - d1.y * seg2.x);
				extrudeScale1 = 1.0f / sinAngle;
				auto a = seg1.dot(seg2);

				if (a < -0.9f)
				{
					d1 = Point(pts[p + 1].x - pts[p].x, pts[p + 1].y - pts[p].y);
					n1 = Point(d1.y, -d1.x).getNormalized();
					extrudeScale1 = 1;
				}
				else
				{
					n1 = d1;
				}

				firstN = n1;
				firstExtrudeScale = extrudeScale1;
			}
			else
			{
				d1 = Point(pts[1].x - pts[0].x, pts[1].y - pts[0].y);
				n1 = Point(d1.y, -d1.x);
				n1.normalize();
			}

			seg1 = Point(pts[p].x - pts[p + 1].x, pts[p].y - pts[p + 1].y);
			seg2 = Point(pts[p + 2].x - pts[p + 1].x, pts[p + 2].y - pts[p + 1].y);
			seg1.normalize();
			seg2.normalize();
			d1 = seg1 + seg2;
			d1.normalize();
			sinAngle = (d1.x * seg2.y - d1.y * seg2.x);
			auto a = seg1.dot(seg2);
			extrudeScale2 = 1.0f / sinAngle;
			n2 = d1;
			lastN2 = n2;

			if (a < -0.9f)
			{
				d1 = Point(pts[1].x - pts[0].x, pts[1].y - pts[0].y);
				n2 = Point(d1.y, -d1.x);
				n2.normalize();
				lastN2 = n2;
				extrudeScale2 = 1;
			}

			if (pointCount == 2)
			{
				n2 = n1;
				extrudeScale2 = extrudeScale1;
			}
		}
		// if last point and its closed
		else if (p == pointCount - 1 && closed)
		{
			n1 = lastN2;
			extrudeScale1 = lastExtrudeScale2;

			n2 = firstN;
			extrudeScale2 = firstExtrudeScale;
		}
		// if almost last one
		else if (p == pointCount - 2)
		{
			n1 = lastN2;
			extrudeScale1 = lastExtrudeScale2;

			if (closed)
			{
				seg1 = Point(pts[p].x - pts[p + 1].x, pts[p].y - pts[p + 1].y);
				seg2 = Point(pts[0].x - pts[p + 1].x, pts[0].y - pts[p + 1].y);
				seg1.normalize();
				seg2.normalize();
				d1 = seg1 + seg2;
				d1.normalize();
				sinAngle = (d1.x * seg2.y - d1.y * seg2.x);
				extrudeScale2 = 1.0f / sinAngle;
				auto a = seg1.dot(seg2);

				if (a < -0.9f)
				{
					d1 = Point(pts[p + 1].x - pts[p].x, pts[p + 1].y - pts[p].y);
					n2 = Point(d1.y, -d1.x).getNormalized();
					extrudeScale2 = 1;
				}
				else
				{
					n2 = d1;
				}

				lastN2 = n2;
			}
			else
			{
				d1 = Point(pts[p + 1].x - pts[p].x, pts[p + 1].y - pts[p].y);
				n2 = Point(d1.y, -d1.x).getNormalized();
				lastN2 = n2;
				extrudeScale2 = 1;
			}
		}
		else if (p < pointCount - 2)
		{
			n1 = lastN2;
			extrudeScale1 = lastExtrudeScale2;

			seg1 = Point(pts[p].x - pts[p + 1].x, pts[p].y - pts[p + 1].y);
			seg2 = Point(pts[p + 2].x - pts[p + 1].x, pts[p + 2].y - pts[p + 1].y);
			seg1.normalize();
			seg2.normalize();
			d1 = seg1 + seg2;
			d1.normalize();
			sinAngle = (d1.x * seg2.y - d1.y * seg2.x);
			extrudeScale2 = 1.0f / sinAngle;
			auto a = seg1.dot(seg2);

			if (a < -0.9f)
			{
				d1 = Point(pts[p + 1].x - pts[p].x, pts[p + 1].y - pts[p].y);
				n2 = Point(d1.y, -d1.x).getNormalized();
				extrudeScale2 = 1;
			}
			else
			{
				n2 = d1;
			}

			lastN2 = n2;
		}

		n1 *= half * extrudeScale1;
		n2 *= half * extrudeScale2;
		lastExtrudeScale1 = extrudeScale1;
		lastExtrudeScale2 = extrudeScale2;

		if (p > 0)
		{
			p11 = lastP11;
			p12 = lastP12;
		}
		else
		{
			p11 = Point(pts[p].x + n1.x, pts[p].y + n1.y);
			p12 = Point(pts[p].x - n1.x, pts[p].y - n1.y);
		}

		if (p == pointCount - 1 && closed)
		{
			p21 = Point(pts[0].x + n2.x, pts[0].y + n2.y);
			p22 = Point(pts[0].x - n2.x, pts[0].y - n2.y);
		}
		else
		{
			p21 = Point(pts[p + 1].x + n2.x, pts[p + 1].y + n2.y);
			p22 = Point(pts[p + 1].x - n2.x, pts[p + 1].y - n2.y);
		}

		lastP11 = p21;
		lastP12 = p22;

		bool drawIt = true;

		if (currentLineStyle.useStipple)
		{
			drawIt = !stippleLinesSkip[p];
		}

		if (drawIt)
		{
			drawTriangle(p11, p21, p22, uv11, uv21, uv22, currentColor, currentColor, currentColor, lineImage);
			drawTriangle(p11, p22, p12, uv11, uv22, uv12, currentColor, currentColor, currentColor, lineImage);
		}
	}
}

void Renderer::drawTriangle(
	const Point& p1, const Point& p2, const Point& p3,
	const Point& uv1, const Point& uv2, const Point& uv3,
	const Rgba32 c1, const Rgba32 c2, const Rgba32 c3,
	Image* image)
{
	//TODO: not thread safe, but we dont support MT anyway, so should be fine for now, just avoid recursive calls to drawTriangle
	static Point pts[12];
	static Point uvPts[12];
	static Rgba32 colors[12];
	static u32 pointCount;
	static Point newUv1, newUv2, newUv3;

	newUv1 = uv1;
	newUv2 = uv2;
	newUv3 = uv3;

	clipTriangleToRect(
		p1, p2, p3, newUv1, newUv2, newUv3, c1, c2, c3,
		currentClipRect, pts, uvPts, colors, pointCount);

	if (!pointCount)
		return;

	Point& firstPoint = pts[0];
	Point& firstUv = uvPts[0];
	Rgba32 firstColor = colors[0];

	needToAddVertexCount((pointCount - 2) * 3);
	u32 i = vertexBufferData.drawVertexCount;

	for (int k = 1; k < pointCount - 1; k++)
	{
		vertexBufferData.vertices[i].position = firstPoint;
		vertexBufferData.vertices[i].color = firstColor;
		vertexBufferData.vertices[i].uv = firstUv;
		i++;

		vertexBufferData.vertices[i].position = pts[k];
		vertexBufferData.vertices[i].color = colors[k];
		vertexBufferData.vertices[i].uv = uvPts[k];
		i++;

		vertexBufferData.vertices[i].position = pts[k+1];
		vertexBufferData.vertices[i].color = colors[k+1];
		vertexBufferData.vertices[i].uv = uvPts[k+1];
		i++;
	}

	currentBatch->vertexCount += (pointCount - 2) * 3;
	vertexBufferData.drawVertexCount = i;
}

bool Renderer::clipRectNoRot(Rect& rect, Rect& uvRect, Rgba32* colors) const
{
	if (rect.outside(currentClipRect))
		return false;

	auto newRect = rect.clipInside(currentClipRect);
	auto oldUvRect = uvRect;

	// clip left and top UVs
	auto tx = (newRect.x - rect.x) / rect.width;
	auto ty = (newRect.y - rect.y) / rect.height;

	// left clip
	uvRect.x += uvRect.width * tx;
	uvRect.width -= uvRect.width * tx;

	// top clip
	uvRect.y += uvRect.height * ty;
	uvRect.height -= uvRect.height * ty;

	// clip right and bottom UVs
	tx = (rect.right() - newRect.right()) / rect.width;
	ty = (rect.bottom() - newRect.bottom()) / rect.height;

	uvRect.width -= oldUvRect.width * tx;
	uvRect.height -= oldUvRect.height * ty;

	// If a color array is provided, treat it as in/out and update corners.
	// colors layout: [0]=topLeft, [1]=topRight, [2]=bottomRight, [3]=bottomLeft
	if (colors && rect.width > 0.0f && rect.height > 0.0f)
	{
		// normalized X positions of new left/right within original rect
		f32 leftT = (newRect.x - rect.x) / rect.width;
		f32 rightT = (newRect.right() - rect.x) / rect.width;

		// read input colors into Color for interpolation
		Color tl = colors[0];
		Color tr = colors[1];
		Color br = colors[2];
		Color bl = colors[3];

		// interpolate top edge colors at new left/right
		Color newTL = tl + (tr - tl) * leftT;
		Color newTR = tl + (tr - tl) * rightT;

		// interpolate bottom edge colors at new left/right
		Color newBL = bl + (br - bl) * leftT;
		Color newBR = bl + (br - bl) * rightT;

		// write back (Rgba32 supports assignment from Color)
		colors[0] = newTL;
		colors[1] = newTR;
		colors[2] = newBR;
		colors[3] = newBL;
	}

	rect = newRect;

	return true;
}

bool Renderer::clipRectRot(Rect& rect, Rect& uvRect, Rgba32* colors) const
{
	if (rect.outside(currentClipRect))
		return false;

	auto newRect = rect.clipInside(currentClipRect);

	// clip left and top UVs (rotated handling)
	auto tx = (newRect.x - rect.x) / rect.width;
	auto ty = (newRect.y - rect.y) / rect.height;
	uvRect.x += uvRect.width * ty;
	uvRect.width -= uvRect.width * ty;
	uvRect.height -= uvRect.height * tx;

	// clip right and bottom UVs
	tx = (rect.right() - newRect.right()) / rect.width;
	ty = (rect.bottom() - newRect.bottom()) / rect.height;
	uvRect.y += uvRect.height * tx;
	uvRect.width -= uvRect.width * ty;
	uvRect.height -= uvRect.height * tx;

	// If a color array is provided, treat it as in/out and update corners.
	// colors layout: [0]=topLeft, [1]=topRight, [2]=bottomRight, [3]=bottomLeft
	if (colors && rect.width > 0.0f && rect.height > 0.0f)
	{
		// normalized X positions of new left/right within original rect
		f32 leftT = (newRect.x - rect.x) / rect.width;
		f32 rightT = (newRect.right() - rect.x) / rect.width;

		Color tl = colors[0];
		Color tr = colors[1];
		Color br = colors[2];
		Color bl = colors[3];

		// interpolate across X for top and bottom edges
		Color newTL = tl + (tr - tl) * leftT;
		Color newTR = tl + (tr - tl) * rightT;
		Color newBL = bl + (br - bl) * leftT;
		Color newBR = bl + (br - bl) * rightT;

		colors[0] = newTL;
		colors[1] = newTR;
		colors[2] = newBR;
		colors[3] = newBL;
	}

	rect = newRect;

	return true;
}

bool Renderer::clipRect(bool rotated, Rect& rect, Rect& uvRect, Rgba32* colors) const
{
	if (!rotated)
	{
		return clipRectNoRot(rect, uvRect, colors);
	}

	return clipRectRot(rect, uvRect, colors);
}

void Renderer::needToAddVertexCount(u32 count)
{
	if (vertexBufferData.drawVertexCount + count < (u32)vertexBufferData.vertices.size())
	{
		// already have space available
		return;
	}

	vertexBufferData.vertices.resize(vertexBufferData.vertices.size() * vertexBufferData.vertexCountGrowFactor + count);
}

char* Renderer::addUtf8TextToBuffer(const char* text, u32 sizeBytes)
{
	if (currentWindowContext->textBufferPosition + sizeBytes + 1 >= (u32)currentWindowContext->textBuffer.size()) return nullptr;

	auto textAddr = currentWindowContext->textBuffer.data() + currentWindowContext->textBufferPosition;

	memcpy(textAddr, text, sizeBytes + 1); // and zero
	currentWindowContext->textBufferPosition += sizeBytes + 1;

	return textAddr;
}

void Renderer::addBatch()
{
	currentWindowContext->batches.push_back(RenderBatch());
	currentBatch = &currentWindowContext->batches.back();
	currentBatch->primitiveType = RenderBatch::PrimitiveType::TriangleList;
	currentBatch->startVertexIndex = vertexBufferData.drawVertexCount;
	currentBatch->texture = currentTexture;
}

void Renderer::addDrawCommand(const DrawCommand& cmd)
{
	if (disableRendering || skipRender)
		return;

	HORUS_ASSERT(currentDrawCmdLayer);
	currentDrawCmdLayer->push_back(cmd);
}

void Renderer::cmdDrawTextAt(
	const char* text,
	const Point& position)
{
	DrawCommand cmd(DrawCommand::Type::DrawText);
	cmd.data.drawText.rect = Rect(position.x, position.y, 0, 0);
	cmd.data.drawText.horizAlign = HAlignType::Left;
	cmd.data.drawText.vertAlign = VAlignType::Top;
	cmd.data.drawText.text = addUtf8TextToBuffer(text, (u32)strlen(text));

	if (cmd.data.drawText.text)
	{
		addDrawCommand(cmd);
	}
}

void Renderer::cmdDrawTextInBox(
	const char* text,
	const Rect& rect,
	HAlignType horizAlign,
	VAlignType vertAlign,
	bool singleLineEllipsis,
	bool noWordWrap)
{
	DrawCommand cmd(DrawCommand::Type::DrawText);

	cmd.data.drawText.rect = rect;
	cmd.data.drawText.horizAlign = horizAlign;
	cmd.data.drawText.vertAlign = vertAlign;
	cmd.data.drawText.text = addUtf8TextToBuffer(text, (u32)strlen(text));
	cmd.data.drawText.singleLineEllipsis = singleLineEllipsis;
	cmd.data.drawText.noWordWrap = noWordWrap;

	if (cmd.data.drawText.text)
	{
		addDrawCommand(cmd);
	}
}

}