#ifdef _WINDOWS
#include <windows.h>
#endif

#include <string.h>
#include <algorithm>
#include "renderer.h"
#include "ui_atlas.h"
#include "ui_font.h"
#include "ui_theme.h"
#include "ui_context.h"
#include "util.h"
#include "unicode_text_cache.h"

namespace hui
{
//TODO: make them grow dynamically like the vertex buffer
const u32 textBufferMaxSize = 1024 * 1024 * 5;//!<< 5MB of text on screen at once its more than enough for now
const u32 pointBufferMaxSize = 500000; //!<< more than enough for a full screen of lines, around 5MB

enum LineClipBit
{
	Inside = 0,
	Left = 1,
	Right = 2,
	Bottom = 4,
	Top = 8
};

// Function to compute region code for a point(x, y) 
int computeLineClipCode(const Point& p, const Rect& rect)
{
	// initialized as being inside  
	int code = LineClipBit::Inside;

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

bool clipLineToRect(
	const Point& p1, const Point& p2,
	const Point& uv1, const Point& uv2,
	const Rect& rect,
	Point& newP1, Point& newP2,
	Point& newUv1, Point& newUv2)
{
	// Compute region codes for P1, P2 
	int code1 = computeLineClipCode(p1, rect);
	int code2 = computeLineClipCode(p2, rect);

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
			int code_out;
			f32 x, y;
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

void clipLeft(const Rect& rect, Point* inPoints, Point* inUvPoints, u32 inCount, Point* outPoints, Point* outUvPoints, u32& outCount)
{
	Point* pp1;
	Point* pp2;
	Point* uvpp1;
	Point* uvpp2;

	for (u32 i = 0; i < inCount; i++)
	{
		if (i == inCount - 1)
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[0];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[0];
		}
		else
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[i + 1];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[i + 1];
		}

		const Point& p1 = *pp1;
		const Point& p2 = *pp2;
		const Point& uvp1 = *uvpp1;
		const Point& uvp2 = *uvpp2;

		// inside
		if (p1.x >= rect.x && p2.x >= rect.x)
		{
			outUvPoints[outCount] = uvp2;
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
			outPoints[outCount++] = { x, y };
			outPoints[outCount++] = p2;
		}
	}
}

void clipRight(const Rect& rect, Point* inPoints, Point* inUvPoints, u32 inCount, Point* outPoints, Point* outUvPoints, u32& outCount)
{
	Point* pp1;
	Point* pp2;
	Point* uvpp1;
	Point* uvpp2;

	for (u32 i = 0; i < inCount; i++)
	{
		if (i == inCount - 1)
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[0];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[0];
		}
		else
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[i + 1];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[i + 1];
		}

		const Point& p1 = *pp1;
		const Point& p2 = *pp2;
		const Point& uvp1 = *uvpp1;
		const Point& uvp2 = *uvpp2;

		// inside
		if (p1.x <= rect.right() && p2.x <= rect.right())
		{
			outUvPoints[outCount] = uvp2;
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
			outPoints[outCount++] = { x, y };
			outPoints[outCount++] = p2;
		}
	}
}

void clipTop(const Rect& rect, Point* inPoints, Point* inUvPoints, u32 inCount, Point* outPoints, Point* outUvPoints, u32& outCount)
{
	Point* pp1;
	Point* pp2;
	Point* uvpp1;
	Point* uvpp2;

	for (u32 i = 0; i < inCount; i++)
	{
		if (i == inCount - 1)
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[0];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[0];
		}
		else
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[i + 1];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[i + 1];
		}

		const Point& p1 = *pp1;
		const Point& p2 = *pp2;
		const Point& uvp1 = *uvpp1;
		const Point& uvp2 = *uvpp2;

		// inside
		if (p1.y >= rect.top() && p2.y >= rect.top())
		{
			outUvPoints[outCount] = uvp2;
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
			outPoints[outCount++] = { x, y };
			outPoints[outCount++] = p2;
		}
	}
}

void clipBottom(const Rect& rect, Point* inPoints, Point* inUvPoints, u32 inCount, Point* outPoints, Point* outUvPoints, u32& outCount)
{
	Point* pp1;
	Point* pp2;
	Point* uvpp1;
	Point* uvpp2;

	for (u32 i = 0; i < inCount; i++)
	{
		if (i == inCount - 1)
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[0];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[0];
		}
		else
		{
			pp1 = &inPoints[i];
			pp2 = &inPoints[i + 1];
			uvpp1 = &inUvPoints[i];
			uvpp2 = &inUvPoints[i + 1];
		}

		const Point& p1 = *pp1;
		const Point& p2 = *pp2;
		const Point& uvp1 = *uvpp1;
		const Point& uvp2 = *uvpp2;

		// inside
		if (p1.y <= rect.bottom() && p2.y <= rect.bottom())
		{
			outUvPoints[outCount] = uvp2;
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
			outPoints[outCount++] = { x, y };
			outPoints[outCount++] = p2;
		}
	}
}

bool clipTriangleToRect(
	const Point& p1, const Point& p2, const Point& p3,
	const Point& uv1, const Point& uv2, const Point& uv3,
	const Rect& rect,
	Point* outPoints, Point* outUvPoints, u32& outCount)
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
		outCount = 3;
		return true;
	}

	Point inPoints[] = {p1, p2, p3, Point(), Point(), Point(), Point(), Point(), Point(), Point(), Point(), Point() };
	Point inUvPoints[] = { uv1, uv2, uv3, Point(), Point(), Point(), Point(), Point(), Point(), Point(), Point(), Point() };
	u32 inCount = 3;

	outCount = 0;
	clipLeft(rect, inPoints, inUvPoints, inCount, outPoints, outUvPoints, outCount);
	inCount = 0;
	clipTop(rect, outPoints, outUvPoints, outCount, inPoints, inUvPoints, inCount);
	outCount = 0;
	clipRight(rect, inPoints, inUvPoints, inCount, outPoints, outUvPoints, outCount);
	inCount = 0;
	clipBottom(rect, outPoints, outUvPoints, outCount, inPoints, inUvPoints, inCount);
	
	for (u32 i = 0; i < inCount; i++)
	{
		outPoints[i] = inPoints[i];
		outUvPoints[i] = inUvPoints[i];
	}

	outCount = inCount;

	return true;
}

Renderer::Renderer()
{
	textBuffer.resize(textBufferMaxSize);
	pointBuffer.resize(pointBufferMaxSize);
	vertexBuffer = ctx->gfx->createVertexBuffer();
}

Renderer::~Renderer()
{
	delete vertexBuffer;
}

void Renderer::clear(const Color& color)
{
	ctx->gfx->clear(color);
}

Rect Renderer::pushClipRect(const Rect& rect, bool clipToParent)
{
	auto oldRect = currentClipRect;
	clipRectStack.push_back(currentClipRect);
	auto newRect = clipToParent ? rect.clipInside(oldRect) : rect;
	currentClipRect = newRect;

	DrawCommand cmd(DrawCommand::Type::ClipRect);

	cmd.zOrder = zOrder;
	cmd.clipRect = currentClipRect;
	cmd.clipToParent = clipToParent;
	addDrawCommand(cmd);

	return newRect;
}

void Renderer::popClipRect()
{
	if (clipRectStack.empty())
	{
		return;
	}

	currentClipRect = clipRectStack.back();
	clipRectStack.pop_back();

	DrawCommand cmd(DrawCommand::Type::ClipRect);

	cmd.clipRect = currentClipRect;
	cmd.zOrder = zOrder;
	cmd.popClipRect = true;
	addDrawCommand(cmd);
}

void Renderer::setWindowSize(const Point& size)
{
	windowSize = size;
	currentClipRect = { 0, 0, windowSize.x, windowSize.y };
	ctx->gfx->setViewport(windowSize, currentClipRect);
}

void Renderer::beginFrame()
{
	zOrder = 0;
	skipRender = false;
	disableRendering = false;
	drawCommands.clear();
	batches.clear();
	vertexBufferData.drawVertexCount = 0;
	textBufferPosition = 0;
	pointBufferPosition = 0;
	currentAtlas = nullptr;
	currentBatch = nullptr;
	cmdSetAtlas(ctx->theme->atlas);
}

void Renderer::endFrame()
{
	if (disableRendering || skipRender)
		return;

	auto sortDrawCommands = [](const DrawCommand& a, const DrawCommand& b) -> bool
	{
		if (a.zOrder < b.zOrder)
			return true;

		return false;
	};

	std::stable_sort(drawCommands.begin(), drawCommands.end(), sortDrawCommands);
	currentAtlas = nullptr;
	currentBatch = nullptr;

	// generate the batches
	for (auto& cmd : drawCommands)
	{
		switch (cmd.type)
		{
		case DrawCommand::Type::DrawImageBordered:
			drawImageBordered(cmd.drawImageBordered.image, cmd.drawImageBordered.border, cmd.drawImageBordered.rect, cmd.drawImageBordered.scale);
			break;
		case DrawCommand::Type::DrawQuad:
			drawQuad(cmd.drawQuad.image, cmd.drawQuad.corners[0], cmd.drawQuad.corners[1], cmd.drawQuad.corners[2], cmd.drawQuad.corners[3]);
			break;
		case DrawCommand::Type::DrawRect:
		{
			atlasTextureIndex = cmd.drawRect.textureIndex;

			if (clipRect(cmd.drawRect.rotated, cmd.drawRect.rect, cmd.drawRect.uvRect))
			{
				if (cmd.drawRect.rotated)
					drawQuadRot90(cmd.drawRect.rect, cmd.drawRect.uvRect);
				else
					drawQuad(cmd.drawRect.rect, cmd.drawRect.uvRect);
			}
			break;
		}
		case DrawCommand::Type::DrawText:
			drawTextInternal(cmd.drawText.text, cmd.drawText.position);
			break;
		case DrawCommand::Type::SetColor:
			currentColor = cmd.setColor.getRgba();
			break;
		case DrawCommand::Type::SetFont:
			currentFont = cmd.setFont;
			break;
		case DrawCommand::Type::ClipRect:
			currentClipRect = cmd.clipRect;
			break;
		case DrawCommand::Type::SetTextStyle:
			currentTextStyle = cmd.setTextStyle;
			break;
		case DrawCommand::Type::SetLineStyle:
			currentLineStyle = cmd.setLineStyle;
			break;
		case DrawCommand::Type::DrawLine:
			drawLine(cmd.drawLine.a, cmd.drawLine.b);
			break;
		case DrawCommand::Type::DrawPolyLine:
			drawPolyLine(cmd.drawPolyLine.points, cmd.drawPolyLine.count, cmd.drawPolyLine.closed);
			break;
		case DrawCommand::Type::DrawTriangle:
			drawTriangle(
				cmd.drawTriangle.p1,
				cmd.drawTriangle.p2,
				cmd.drawTriangle.p3,
				cmd.drawTriangle.uv1,
				cmd.drawTriangle.uv2,
				cmd.drawTriangle.uv3,
				cmd.drawTriangle.image);
			break;
		case DrawCommand::Type::SetAtlas:
			if (currentAtlas != cmd.setAtlas)
			{
				currentAtlas = cmd.setAtlas;
				addBatch();
			}
			break;
		}
	}

	vertexBuffer->updateData(vertexBufferData.vertices.data(), 0, vertexBufferData.drawVertexCount);
	// render the batches
	ctx->gfx->draw(batches.data(), batches.size());
}

void Renderer::cmdSetColor(const Color& newColor)
{
	DrawCommand cmd(DrawCommand::Type::SetColor);
	cmd.zOrder = zOrder;
	cmd.setColor = newColor;
	addDrawCommand(cmd);
}

void Renderer::cmdSetAtlas(UiAtlas* newAtlas)
{
	DrawCommand cmd(DrawCommand::Type::SetAtlas);
	cmd.zOrder = zOrder;
	cmd.setAtlas = newAtlas;
	currentAtlas = newAtlas;
	addDrawCommand(cmd);
}

void Renderer::cmdSetFont(UiFont* font)
{
	DrawCommand cmd(DrawCommand::Type::SetFont);
	cmd.zOrder = zOrder;
	cmd.setFont = font;
	currentFont = font;
	addDrawCommand(cmd);
}

void Renderer::cmdSetTextUnderline(bool underline)
{
	DrawCommand cmd(DrawCommand::Type::SetTextStyle);
	currentTextStyle.underline = underline;
	cmd.zOrder = zOrder;
	cmd.setTextStyle = currentTextStyle;
	addDrawCommand(cmd);
}

void Renderer::cmdSetTextBackfill(bool backfill)
{
	DrawCommand cmd(DrawCommand::Type::SetTextStyle);
	currentTextStyle.backFill = backfill;
	cmd.zOrder = zOrder;
	cmd.setTextStyle = currentTextStyle;
	addDrawCommand(cmd);
}

void Renderer::cmdSetTextBackfillColor(const Color& color)
{
	DrawCommand cmd(DrawCommand::Type::SetTextStyle);
	currentTextStyle.backFillColor = color;
	cmd.zOrder = zOrder;
	cmd.setTextStyle = currentTextStyle;
	addDrawCommand(cmd);
}

void Renderer::cmdSetLineStyle(const LineStyle& style)
{
	DrawCommand cmd(DrawCommand::Type::SetLineStyle);
	cmd.zOrder = zOrder;
	cmd.setLineStyle = style;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawImage(UiImage* image, const Point& position, f32 scale)
{
	DrawCommand cmd(DrawCommand::Type::DrawRect);
	cmd.zOrder = zOrder;
	cmd.drawRect.rect = Rect(position.x, position.y, image->rect.width * scale, image->rect.height * scale);
	cmd.drawRect.uvRect = image->uvRect;
	cmd.drawRect.rotated = image->rotated;
	cmd.drawRect.textureIndex = image->atlasTexture->textureIndex;

	addDrawCommand(cmd);
}

void Renderer::cmdDrawImage(UiImage* image, const Rect& rect)
{
	DrawCommand cmd(DrawCommand::Type::DrawRect);
	cmd.zOrder = zOrder;
	cmd.drawRect.rect = rect;
	cmd.drawRect.uvRect = image->uvRect;
	cmd.drawRect.rotated = image->rotated;
	cmd.drawRect.textureIndex = image->atlasTexture->textureIndex;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawQuad(UiImage* image, const Point& p1, const Point& p2, const Point& p3, const Point& p4)
{
	DrawCommand cmd(DrawCommand::Type::DrawQuad);
	cmd.zOrder = zOrder;
	cmd.drawQuad.corners[0] = p1;
	cmd.drawQuad.corners[1] = p2;
	cmd.drawQuad.corners[2] = p3;
	cmd.drawQuad.corners[3] = p4;
	cmd.drawQuad.image = image;

	addDrawCommand(cmd);
}

void Renderer::cmdDrawImageBordered(UiImage* image, u32 border, const Rect& rect, f32 scale)
{
	DrawCommand cmd(DrawCommand::Type::DrawImageBordered);
	cmd.zOrder = zOrder;
	cmd.drawImageBordered.rect = rect;
	cmd.drawImageBordered.image = image;
	cmd.drawImageBordered.border = border;
	cmd.drawImageBordered.scale = scale;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawImageScaledAligned(UiImage* image, const Rect& rect, HAlignType halign, VAlignType valign, f32 scale)
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
		newRect.x = newRect.x + (rect.width - newWidth) / 2.f;
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
		newRect.y = newRect.y + (rect.height - newHeight) / 2.f;
		break;
	default:
		break;
	}

	cmdDrawImage(image, newRect);
}

void Renderer::cmdDrawSolidColor(const Rect& rect)
{
	auto image = currentAtlas->whiteImage;
	cmdDrawImage(image, rect);
}

void Renderer::cmdDrawInterpolatedColors(const Rect& rect, const Color& topLeft, const Color& bottomLeft, const Color& topRight, const Color& bottomRight)
{
	DrawCommand cmd(DrawCommand::Type::DrawInterpolatedColors);
	cmd.zOrder = zOrder;
	cmd.drawInterpolatedColors.rect = rect;
	cmd.drawInterpolatedColors.bottomLeft = bottomLeft;
	cmd.drawInterpolatedColors.bottomRight = bottomRight;
	cmd.drawInterpolatedColors.topLeft = topLeft;
	cmd.drawInterpolatedColors.topRight = topRight;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawSpectrumColors(const Rect& rect, DrawSpectrumBrightness brightness, DrawSpectrumDirection dir)
{
	//TODO
}

void Renderer::cmdDrawInterpolatedColorsTopBottom(const Rect& rect, const Color& top, const Color& bottom)
{
	cmdDrawInterpolatedColors(rect, top, bottom, top, bottom);
}

void Renderer::cmdDrawInterpolatedColorsLeftRight(const Rect& rect, const Color& left, const Color& right)
{
	cmdDrawInterpolatedColors(rect, left, left, right, right);
}

void Renderer::cmdDrawLine(const Point& a, const Point& b)
{
	DrawCommand cmd(DrawCommand::Type::DrawLine);
	cmd.zOrder = zOrder;
	cmd.drawLine.a = a;
	cmd.drawLine.b = b;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawPolyLine(const Point* points, u32 pointCount, bool closed)
{
	DrawCommand cmd(DrawCommand::Type::DrawPolyLine);
	cmd.zOrder = zOrder;
	cmd.drawPolyLine.count = pointCount;
	cmd.drawPolyLine.closed = closed;
	cmd.drawPolyLine.points = &pointBuffer[pointBufferPosition];
	memcpy(pointBuffer.data() + pointBufferPosition, points, pointCount * sizeof(Point));
	pointBufferPosition += pointCount;
	addDrawCommand(cmd);
}

void Renderer::cmdDrawTriangle(const Point& p1, const Point& p2, const Point& p3, const Point& uv1, const Point& uv2, const Point& uv3, UiImage* image)
{
	DrawCommand cmd(DrawCommand::Type::DrawTriangle);
	cmd.zOrder = zOrder;
	cmd.drawTriangle.p1 = p1;
	cmd.drawTriangle.p2 = p2;
	cmd.drawTriangle.p3 = p3;
	cmd.drawTriangle.uv1 = uv1;
	cmd.drawTriangle.uv2 = uv2;
	cmd.drawTriangle.uv3 = uv3;
	cmd.drawTriangle.image = image;
	addDrawCommand(cmd);
}

FontTextSize Renderer::cmdDrawTextAt(
	const char* text,
	const Point& position)
{
	FontTextSize fsize = currentFont->computeTextSize(text);
	DrawCommand cmd(DrawCommand::Type::DrawText);
	cmd.zOrder = zOrder;
	cmd.drawText.position = position;
	cmd.drawText.text = addUtf8TextToBuffer(text, strlen(text));
	addDrawCommand(cmd);
	return fsize;
}

FontTextSize Renderer::cmdDrawTextInBox(
	const char* text,
	const Rect& rect,
	HAlignType horizontal,
	VAlignType vertical)
{
	FontTextSize fsize = currentFont->computeTextSize(text);
	DrawCommand cmd(DrawCommand::Type::DrawText);
	Point pos;

	switch (vertical)
	{
	case hui::VAlignType::Top:
		pos.y = rect.y + currentFont->getMetrics().ascender;
		break;
	case hui::VAlignType::Bottom:
		pos.y = rect.bottom() + currentFont->getMetrics().descender;
		break;
	case hui::VAlignType::Center:
		pos.y = rect.y + (rect.height - fsize.maxGlyphHeight) / 2.0f + fsize.maxBearingY;
		break;
	default:
		pos.y = rect.y;
		break;
	}

	switch (horizontal)
	{
	case hui::HAlignType::Left:
		pos.x = rect.x;
		break;
	case hui::HAlignType::Right:
		pos.x = rect.right() - fsize.width;
		break;
	case hui::HAlignType::Center:
		pos.x = rect.x + (rect.width - fsize.width) / 2.0f;
		break;
	default:
		pos.x = rect.x;
		break;
	}

	cmd.zOrder = zOrder;
	cmd.drawText.position = pos;
	cmd.drawText.text = addUtf8TextToBuffer(text, strlen(text));
	addDrawCommand(cmd);
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

void Renderer::drawTextGlyph(UiImage* image, const Point& position)
{
	atlasTextureIndex = image->atlasTexture->textureIndex;
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

void Renderer::drawQuad(UiImage* image, const Point& p1, const Point& p2, const Point& p3, const Point& p4)
{
	drawTriangle(p1, p2, p3, image->uvRect.topLeft(), image->uvRect.topRight(), image->uvRect.bottomRight(), image);
	drawTriangle(p1, p3, p4, image->uvRect.topLeft(), image->uvRect.bottomRight(), image->uvRect.bottomLeft(), image);

	//atlasTextureIndex = image->atlasTexture->textureIndex;
	//needToAddVertexCount(6);

	//u32 i = vertexBufferData.drawVertexCount;

	//vertexBufferData.vertices[i].position = p1;
	//vertexBufferData.vertices[i].uv = image->uvRect.topLeft();
	//vertexBufferData.vertices[i].color = currentColor;
	//vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	//i++;

	//vertexBufferData.vertices[i].position = p2;
	//vertexBufferData.vertices[i].uv = image->uvRect.bottomLeft();
	//vertexBufferData.vertices[i].color = currentColor;
	//vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	//i++;

	//vertexBufferData.vertices[i].position = p3;
	//vertexBufferData.vertices[i].uv = image->uvRect.bottomRight();
	//vertexBufferData.vertices[i].color = currentColor;
	//vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	//i++;

	//// 2nd triangle
	//vertexBufferData.vertices[i].position = p3;
	//vertexBufferData.vertices[i].uv = image->uvRect.bottomRight();
	//vertexBufferData.vertices[i].color = currentColor;
	//vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	//i++;

	//vertexBufferData.vertices[i].position = p4;
	//vertexBufferData.vertices[i].uv = image->uvRect.topRight();
	//vertexBufferData.vertices[i].color = currentColor;
	//vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	//i++;

	//vertexBufferData.vertices[i].position = p1;
	//vertexBufferData.vertices[i].uv = image->uvRect.topLeft();
	//vertexBufferData.vertices[i].color = currentColor;
	//vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	//i++;

	//vertexBufferData.drawVertexCount = i;
	//currentBatch->vertexCount += 6;
}

void Renderer::drawQuad(const Rect& rect, const Rect& uvRect)
{
	needToAddVertexCount(6);

	u32 i = vertexBufferData.drawVertexCount;

	vertexBufferData.vertices[i].position = rect.topLeft();
	vertexBufferData.vertices[i].uv = uvRect.topLeft();
	vertexBufferData.vertices[i].color = currentColor;
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.topRight();
	vertexBufferData.vertices[i].uv = uvRect.topRight();
	vertexBufferData.vertices[i].color = currentColor;
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomLeft();
	vertexBufferData.vertices[i].uv = uvRect.bottomLeft();
	vertexBufferData.vertices[i].color = currentColor;
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	// 2nd triangle
	vertexBufferData.vertices[i].position = rect.topRight();
	vertexBufferData.vertices[i].uv = uvRect.topRight();
	vertexBufferData.vertices[i].color = currentColor;
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomRight();
	vertexBufferData.vertices[i].uv = uvRect.bottomRight();
	vertexBufferData.vertices[i].color = currentColor;
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomLeft();
	vertexBufferData.vertices[i].uv = uvRect.bottomLeft();
	vertexBufferData.vertices[i].color = currentColor;
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.drawVertexCount = i;
	currentBatch->vertexCount += 6;
}

void Renderer::drawQuadRot90(const Rect& rect, const Rect& uvRect)
{
	needToAddVertexCount(6);

	u32 i = vertexBufferData.drawVertexCount;
	Point t0(uvRect.topLeft());
	Point t1(uvRect.topRight());
	Point t2(uvRect.right(), uvRect.bottom());
	Point t3(uvRect.bottomLeft());

	// t3-------t0
	//  |     /  |
	//  |  /     |
	// t2-------t1

	vertexBufferData.vertices[i].position = rect.topLeft();
	vertexBufferData.vertices[i].uv = t3;
	vertexBufferData.vertices[i].color = currentColor;
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.topRight();
	vertexBufferData.vertices[i].uv = t0;
	vertexBufferData.vertices[i].color = currentColor;
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomLeft();
	vertexBufferData.vertices[i].uv = t2;
	vertexBufferData.vertices[i].color = currentColor;
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	// 2nd triangle

	vertexBufferData.vertices[i].position = rect.topRight();
	vertexBufferData.vertices[i].uv = t0;
	vertexBufferData.vertices[i].color = currentColor;
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomRight();
	vertexBufferData.vertices[i].uv = t1;
	vertexBufferData.vertices[i].color = currentColor;
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomLeft();
	vertexBufferData.vertices[i].uv = t2;
	vertexBufferData.vertices[i].color = currentColor;
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.drawVertexCount = i;
	currentBatch->vertexCount += 6;
}

void Renderer::drawInterpolatedColors(
	const Rect& rect,
	const Rect& uvRect,
	const Color& topLeft,
	const Color& bottomLeft,
	const Color& topRight,
	const Color& bottomRight)
{
	//TODO: optimize for speed and multiple textures
	u32 width = rect.width;
	u32 height = rect.height;

	std::vector<u32> pixels;
	auto t = 0.0f;
	Color xcTop;
	Color xcBottom;
	Color c;

	pixels.resize(width * height);

	for (size_t x = 0; x < width; x++)
	{
		t = (f32)x / width;
		xcTop = topLeft + (topRight - topLeft) * t;
		xcBottom = bottomLeft + (bottomRight - bottomLeft) * t;

		for (size_t y = 0; y < height; y++)
		{
			c = xcTop + (xcBottom - xcTop) * ((f32)y / height);
			pixels[x + y * width] = c.getRgba();
		}
	}

	//if (pixels.data())
	//{
	//	currentBatch->textureArray->updateRectData( pixels.data());
	//}

	//beginBatch(coloredQuadAtlas);
	//atlasTextureIndex = 0;
	//drawQuad(clippedUvRect, clippedRect);
	//endBatch();
}

void Renderer::drawSpectrumColors(
	const Rect& rect,
	DrawSpectrumBrightness brightness,
	DrawSpectrumDirection dir)
{
	//TODO: optimize for speed and multiple textures
	u32 width = rect.width;
	u32 height = rect.height;

	//spectrumQuadAtlas->textureArray->resize(1, width, height);

	std::vector<u32> pixels;
	auto t = 0.0f;
	Color xcTop;
	Color xcBottom;
	Color c;
	Color spectrum[] = { Color::red, Color::yellow, Color::green, Color::cyan, Color::blue, Color::magenta };
	const int spectrumCount = 6;
	Color top = Color::white;
	Color bottom = Color::black;

	pixels.resize(width * height);

	if (dir == DrawSpectrumDirection::Horizontal)
	{
		for (size_t x = 0; x < width; x++)
		{
			t = (f32)x / width;
			size_t index = t * spectrumCount;
			size_t indexNext = index + 1;
			f32 indexF = t * (f32)spectrumCount;
			f32 fraction = indexF - (f32)index;

			if (indexNext == spectrumCount)
				indexNext = 0;

			c = spectrum[index] + (spectrum[indexNext] - spectrum[index]) * fraction;
			c.a = 1;
			Color cf = c;

			for (size_t y = 0; y < height; y++)
			{
				if (brightness == DrawSpectrumBrightness::On)
				{
					f32 b = (f32)y / height;

					if (b <= 0.5f)
						cf = Color::white + (c - Color::white) * (b / 0.5f);
					else
						cf = c + (Color::black - c) * ((b - 0.5f) / 0.5f);

					cf.a = 1;
				}

				pixels[x + y * width] = cf.getRgba();
			}
		}
	}
	else
	{
		for (size_t y = 0; y < height; y++)
		{
			t = (f32)y / height;
			size_t index = t * spectrumCount;
			size_t indexNext = index + 1;
			f32 indexF = t * (f32)spectrumCount;
			f32 fraction = indexF - (f32)index;

			if (indexNext == spectrumCount)
				indexNext = 0;

			c = spectrum[index] + (spectrum[indexNext] - spectrum[index]) * fraction;
			c.a = 1;
			u32 color = c.getRgba();
			auto offs = y * width;

			for (size_t x = 0; x < width; x++)
			{
				pixels[x + offs] = color;
			}
		}
	}

	//if (pixels.data())
	//{
	//	spectrumQuadAtlas->textureArray->updateData(pixels.data());
	//}

	//beginBatch(spectrumQuadAtlas);
	//atlasTextureIndex = 0;
	//drawQuad(clippedUvRect, clippedRect);
	//endBatch();
}

void Renderer::drawInterpolatedColorsTopBottom(
	const Rect& rect,
	const Rect& uvRect,
	const Color& top,
	const Color& bottom)
{
	auto whiteImg = currentAtlas->whiteImage;
	atlasTextureIndex = whiteImg->atlasTexture->textureIndex;

	Color clippedTopColor = top;
	Color clippedBottomColor = bottom;

	if (uvRect.y > 0.0f)
		clippedTopColor = top + (bottom - top) * uvRect.y;

	if (uvRect.height < 1.0f && uvRect.y == 0.0f)
		clippedBottomColor = top + (bottom - top) * uvRect.height;

	needToAddVertexCount(6);

	u32 i = vertexBufferData.drawVertexCount;

	vertexBufferData.vertices[i].position = rect.topLeft();
	vertexBufferData.vertices[i].color = clippedTopColor.getRgba();
	vertexBufferData.vertices[i].uv = whiteImg->uvRect.topLeft();
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.topRight();
	vertexBufferData.vertices[i].color = clippedTopColor.getRgba();
	vertexBufferData.vertices[i].uv = whiteImg->uvRect.topRight();
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomLeft();
	vertexBufferData.vertices[i].color = clippedBottomColor.getRgba();
	vertexBufferData.vertices[i].uv = whiteImg->uvRect.bottomLeft();
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	// 2nd tri
	vertexBufferData.vertices[i].position = rect.topRight();
	vertexBufferData.vertices[i].color = clippedTopColor.getRgba();
	vertexBufferData.vertices[i].uv = whiteImg->uvRect.topRight();
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomRight();
	vertexBufferData.vertices[i].color = clippedBottomColor.getRgba();
	vertexBufferData.vertices[i].uv = whiteImg->uvRect.bottomRight();
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomLeft();
	vertexBufferData.vertices[i].color = clippedBottomColor.getRgba();
	vertexBufferData.vertices[i].uv = whiteImg->uvRect.bottomLeft();
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.drawVertexCount = i;
	currentBatch->vertexCount += 6;
}

void Renderer::drawInterpolatedColorsLeftRight(
	const Rect& rect,
	const Rect& uvRect,
	const Color& left,
	const Color& right)
{
	auto whiteImg = currentAtlas->whiteImage;

	atlasTextureIndex = whiteImg->atlasTexture->textureIndex;
	needToAddVertexCount(6);

	u32 i = vertexBufferData.drawVertexCount;

	vertexBufferData.vertices[i].position = rect.topLeft();
	vertexBufferData.vertices[i].color = left.getRgba();
	vertexBufferData.vertices[i].uv = whiteImg->uvRect.topLeft();
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.topRight();
	vertexBufferData.vertices[i].color = right.getRgba();
	vertexBufferData.vertices[i].uv = whiteImg->uvRect.topRight();
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomLeft();
	vertexBufferData.vertices[i].color = left.getRgba();
	vertexBufferData.vertices[i].uv = whiteImg->uvRect.bottomLeft();
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	// 2nd tri

	vertexBufferData.vertices[i].position = rect.topRight();
	vertexBufferData.vertices[i].color = right.getRgba();
	vertexBufferData.vertices[i].uv = whiteImg->uvRect.topRight();
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomRight();
	vertexBufferData.vertices[i].color = right.getRgba();
	vertexBufferData.vertices[i].uv = whiteImg->uvRect.bottomRight();
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.vertices[i].position = rect.bottomLeft();
	vertexBufferData.vertices[i].color = left.getRgba();
	vertexBufferData.vertices[i].uv = whiteImg->uvRect.bottomLeft();
	vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
	i++;

	vertexBufferData.drawVertexCount = i;
	currentBatch->vertexCount += 6;
}

void Renderer::drawImageBordered(UiImage* image, u32 border, const Rect& rect, f32 scale)
{
	Rect screenRect = rect;

	screenRect.x = round(screenRect.x);
	screenRect.y = round(screenRect.y);
	screenRect.width = round(screenRect.width);
	screenRect.height = round(screenRect.height);

	atlasTextureIndex = image->atlasTexture->textureIndex;

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
	f32 borderU = fborder / (f32)currentBatch->textureArray->getWidth();
	f32 borderV = fborder / (f32)currentBatch->textureArray->getHeight();

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

void Renderer::drawPolyLine1Pixel(const Point* points, u32 pointCount, bool closed)
{
	auto atlas = (UiAtlas*)currentBatch->atlas;
	auto lineImage = atlas->whiteImage;
	const auto color = currentLineStyle.color.getRgba();
	auto rcUv = lineImage->uvRect;
	auto atlasTextureIndex = lineImage->atlasTexture->textureIndex;

	rcUv.x += ctx->settings.whiteImageUvBorder;
	rcUv.y += ctx->settings.whiteImageUvBorder;
	rcUv.width -= ctx->settings.whiteImageUvBorder * 2.0f;
	rcUv.height -= ctx->settings.whiteImageUvBorder * 2.0f;

	const auto uv11 = rcUv.topLeft();
	const auto uv12 = rcUv.topRight();
	const auto uv22 = rcUv.bottomRight();
	const auto uv21 = rcUv.bottomLeft();
	Point p1, p2, uv1, uv2;

	for (int p = 0; p < pointCount; p++)
	{
		if (p == pointCount - 1)
		{
			if (!clipLineToRect(points[p], points[0], uv11, uv21, currentClipRect, p1, p2, uv1, uv2))
				break;
		}
		else if (!clipLineToRect(points[p], points[p + 1], uv11, uv21, currentClipRect, p1, p2, uv1, uv2))
			continue;

		needToAddVertexCount(3);
		u32 i = vertexBufferData.drawVertexCount;

		vertexBufferData.vertices[i].position = p1;
		vertexBufferData.vertices[i].color = currentLineStyle.color.getRgba();
		vertexBufferData.vertices[i].uv = uv11;
		vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
		i++;

		vertexBufferData.vertices[i].position = p2;
		vertexBufferData.vertices[i].color = currentLineStyle.color.getRgba();
		vertexBufferData.vertices[i].uv = uv12;
		vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
		i++;

		vertexBufferData.vertices[i].position = p1;
		vertexBufferData.vertices[i].color = currentLineStyle.color.getRgba();
		vertexBufferData.vertices[i].uv = uv11;
		vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
		i++;

		currentBatch->vertexCount += 3;
		vertexBufferData.drawVertexCount = i;
	}
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

		for (u32 i = 0; i < pointCount; i++)
		{
			stippleLines.push_back(points[i]);
			stippleLinesSkip.push_back(skip);

			u32 idx = i + 1;

			if (i == pointCount - 1)
			{
				if (!closed) break;
				idx = 0;
			}

			Point line = points[idx] - points[i];
			f32 totalLen = line.getLength();
			f32 len = remainder;
			bool bail = false;

			while (!bail)
			{
				for (u32 j = oldJ; j < currentLineStyle.stipplePatternCount; j++)
				{
					f32 size = currentLineStyle.stipplePattern[j];

					// if we're outside the line
					if (len + size >= totalLen)
					{
						// remember the stipple index
						oldJ = j;
						remainder = len + size - totalLen;
						bail = true;
						break;
					}

					if (remainder > 0)
					{
						remainder = 0;
					}
					else
					{
						len += size;
					}

					f32 t = len / totalLen;

					stippleLines.push_back(points[i] + line * t);
					skip = !skip;
					stippleLinesSkip.push_back(skip);
				}

				if (!bail)
					oldJ = 0;
			}

			if (bail && ((i == pointCount - 2 && !closed) || (i == pointCount - 1 && closed)) )
			{
				stippleLines.push_back(points[idx]);
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
	auto atlas = (UiAtlas*)currentBatch->atlas;
	auto lineImage = atlas->whiteImage;
	const auto color = currentLineStyle.color.getRgba();
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

	for (int p = 0; p < pointCount; p++)
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
					n1 = d1;

				firstN = n1;
				firstExtrudeScale = extrudeScale1;
			}
			else
			{
				d1 = Point(pts[1].x - pts[0].x, pts[1].y - pts[0].y);
				n1 = Point(d1.y, -d1.x);
				n1.normalize();

				seg1 = Point(pts[p].x - pts[p + 1].x, pts[p].y - pts[p + 1].y);
				seg2 = Point(pts[p + 2].x - pts[p + 1].x, pts[p + 2].y - pts[p + 1].y);
				seg1.normalize();
				seg2.normalize();
				d1 = seg1 + seg2;
				d1.normalize();
				sinAngle = (d1.x * seg2.y - d1.y * seg2.x);
				extrudeScale2 = 1.0f / sinAngle;
				n2 = d1;
				lastN2 = n2;
				auto a = seg1.dot(seg2);

				if (a < -0.9f)
				{
					n2 = n1;
					extrudeScale2 = extrudeScale1;
				}
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
					n2 = d1;

				lastN2 = n2;
			}
			else
			{
				d1 = Point(pts[p + 1].x - pts[p].x, pts[p + 1].y - pts[p].y);
				n2 = Point(d1.y, -d1.x).getNormalized();
				lastN2 = n2;
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
			drawTriangle(p11, p21, p22, uv11, uv21, uv22, lineImage);
			drawTriangle(p11, p22, p12, uv11, uv22, uv12, lineImage);
		}
	}
}

void Renderer::drawTriangle(
	const Point& p1, const Point& p2, const Point& p3,
	const Point& uv1, const Point& uv2, const Point& uv3,
	UiImage* image)
{
	// not thread safe
	static Point pts[12];
	static Point uvPts[12];
	static u32 pointCount;
	static Point newUv1, newUv2, newUv3;
	auto img = image ? image : currentAtlas->whiteImage;

	if (image)
	{
		newUv1 = uv1;
		newUv2 = uv2;
		newUv3 = uv3;
	}
	else
	{
		newUv1 = img->uvRect.topLeft();
		newUv1.x += ctx->settings.whiteImageUvBorder;
		newUv1.y += ctx->settings.whiteImageUvBorder;
		newUv2 = img->uvRect.topRight();
		newUv2.x -= ctx->settings.whiteImageUvBorder;
		newUv2.y += ctx->settings.whiteImageUvBorder;
		newUv3 = img->uvRect.bottomRight();
		newUv3.x -= ctx->settings.whiteImageUvBorder;
		newUv3.y -= ctx->settings.whiteImageUvBorder;
	}

	clipTriangleToRect(
		p1, p2, p3, newUv1, newUv2, newUv3,
		currentClipRect, pts, uvPts, pointCount);

	if (!pointCount)
		return;

	Point& fp = pts[0];
	Point& uvFp = uvPts[0];

	atlasTextureIndex = img->atlasTexture->textureIndex;

	needToAddVertexCount((pointCount - 2) * 3);
	u32 i = vertexBufferData.drawVertexCount;

	for (int k = 1; k < pointCount - 1; k++)
	{
		vertexBufferData.vertices[i].position = fp;
		vertexBufferData.vertices[i].color = currentLineStyle.color.getRgba();
		vertexBufferData.vertices[i].uv = uvFp;
		vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
		i++;

		vertexBufferData.vertices[i].position = pts[k];
		vertexBufferData.vertices[i].color = currentLineStyle.color.getRgba();
		vertexBufferData.vertices[i].uv = uvPts[k];
		vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
		i++;

		vertexBufferData.vertices[i].position = pts[k+1];
		vertexBufferData.vertices[i].color = currentLineStyle.color.getRgba();
		vertexBufferData.vertices[i].uv = uvPts[k+1];
		vertexBufferData.vertices[i].textureIndex = atlasTextureIndex;
		i++;
	}

	currentBatch->vertexCount += (pointCount - 2) * 3;
	vertexBufferData.drawVertexCount = i;
}

void Renderer::drawTextInternal(
	const char* text,
	const Point& position)
{
	if (!strcmp(text, ""))
	{
		return;
	}

	Point pos = position;

	pos.x = round(pos.x);
	pos.y = round(pos.y);

	GlyphCode lastChr = 0;
	Point underlineStartPos = pos;

	/////////////////////////////
	// DRAW CHARS
	/////////////////////////////
	const UnicodeString& utext = *ctx->textCache->getText(text);

	for (int i = 0; i < utext.size(); i++)
	{
		auto chr = utext[i];

		if (chr == '\n')
		{
			continue;
		}

		auto glyph = currentFont->getGlyph(chr);
		auto img = currentFont->getGlyphImage(chr);

		if (!glyph || !img)
		{
			continue;
		}

		auto kern = currentFont->getKerning(lastChr, chr);
		pos.x += kern;
		drawTextGlyph(img, { pos.x + glyph->bitmapLeft, pos.y - glyph->bitmapTop });
		pos.x += glyph->advanceX;
		lastChr = chr;
	}

	// render underline
	if (currentTextStyle.underline)
	{
		auto fsize = currentFont->computeTextSize(utext);
		auto image = currentAtlas->whiteImage;

		if (!image->rotated)
		{
			drawQuad(
				{
					underlineStartPos.x,
					underlineStartPos.y - currentFont->getMetrics().underlinePosition,
					fsize.width,
					currentFont->getMetrics().underlineThickness
				},
				image->uvRect);
		}
		else
		{
			drawQuadRot90(
				{
					underlineStartPos.x,
					underlineStartPos.y - currentFont->getMetrics().underlinePosition,
					fsize.width,
					currentFont->getMetrics().underlineThickness
				},
				image->uvRect);
		}
	}
}

bool Renderer::clipRectNoRot(Rect& rect, Rect& uvRect)
{
	if (rect.outside(currentClipRect))
		return false;

	auto newRect = rect.clipInside(currentClipRect);

	// clip left and top UVs
	auto tx = (newRect.x - rect.x) / rect.width;
	auto ty = (newRect.y - rect.y) / rect.height;
	uvRect.x += uvRect.width * tx;
	uvRect.y += uvRect.height * ty;
	uvRect.width -= uvRect.width * tx;
	uvRect.height -= uvRect.height * ty;

	// clip right and bottom UVs
	tx = (rect.right() - newRect.right()) / rect.width;
	ty = (rect.bottom() - newRect.bottom()) / rect.height;
	uvRect.width -= uvRect.width * tx;
	uvRect.height -= uvRect.height * ty;
	rect = newRect;

	return true;
}

bool Renderer::clipRectRot(Rect& rect, Rect& uvRect)
{
	if (rect.outside(currentClipRect))
		return false;

	auto newRect = rect.clipInside(currentClipRect);

	// clip left and top UVs
	auto tx = (newRect.x - rect.x) / rect.width;
	auto ty = (newRect.y - rect.y) / rect.height;
	uvRect.x += uvRect.width * ty;
	uvRect.width -= uvRect.width * ty;
	uvRect.height -= uvRect.height * tx;

	// clip right and bottom UVs
	tx = (rect.right() - newRect.right()) / rect.width;
	ty = (rect.bottom() - newRect.bottom()) / rect.height;
	uvRect.width -= uvRect.width * ty;
	uvRect.height -= uvRect.height * tx;
	rect = newRect;

	return true;
}

bool Renderer::clipRect(bool rotated, Rect& rect, Rect& uvRect)
{
	if (!rotated)
	{
		return clipRectNoRot(rect, uvRect);
	}

	return clipRectRot(rect, uvRect);
}

void Renderer::needToAddVertexCount(u32 count)
{
	if (vertexBufferData.drawVertexCount + count < vertexBufferData.vertices.size())
	{
		// already have space available
		return;
	}

	std::vector<Vertex> verts = vertexBufferData.vertices;

	vertexBufferData.vertices.resize(vertexBufferData.vertices.size() * vertexBufferData.vertexCountGrowFactor + count);
	vertexBufferData.vertices.insert(vertexBufferData.vertices.begin(), verts.begin(), verts.end());
	vertexBuffer->resize(vertexBufferData.vertices.size());
}

char* Renderer::addUtf8TextToBuffer(const char* text, u32 sizeBytes)
{
	auto pos = textBuffer.data() + textBufferPosition;
	memcpy(pos, text, sizeBytes + 1); // and zero
	textBufferPosition += sizeBytes + 1;
	return pos;
}

void Renderer::addBatch()
{
	batches.push_back(RenderBatch());
	currentBatch = &batches.back();
	currentBatch->atlas = currentAtlas;
	currentBatch->primitiveType = RenderBatch::PrimitiveType::TriangleList;
	currentBatch->startVertexIndex = vertexBufferData.drawVertexCount;
	currentBatch->vertexBuffer = vertexBuffer;
	currentBatch->textureArray = currentAtlas->textureArray;
}

void Renderer::addDrawCommand(const DrawCommand& cmd)
{
	if (drawCmdNextInsertIndex == ~0)
	{
		drawCommands.push_back(cmd);
	}
	else
	{
		drawCommands.insert(drawCommands.begin() + drawCmdNextInsertIndex, cmd);
		drawCmdNextInsertIndex++;
	}
}

}
