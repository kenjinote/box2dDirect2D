// DebugDrawGDI.cpp
// 
// 
// Created by PavlAl on 06.05.2013
// Copyright (c) 2013 tatalata, http://talatala.com
// 
// updated to use directx 2d by Alex Russell, 2013/09/25
//

#include "stdafx.h"
#include "DebugDrawGDI.h"
#include <vector>

// set w to the box2D world AABB
// use this to help scale/transform our world
void DebugDrawGDI::GetBoundBox2DBounds(RECT* w, b2World* world)
{
	// iterate over ALL the bodies, and set the w to max/min
	b2Body* b;
	b2Fixture* fix;
	b2AABB bound;
	float minX, maxX, minY, maxY;

	minX = minY = 1000000.0;
	maxX = maxY = -1000000.0;

	b = world->GetBodyList();
	while (b)
	{
		fix = b->GetFixtureList();
		while (fix)
		{
			bound = fix->GetAABB(0);
			if (bound.lowerBound.x < minX)
				minX = bound.lowerBound.x;
			if (bound.upperBound.x > maxX)
				maxX = bound.upperBound.x;
			if (bound.lowerBound.y < minY)
				minY = bound.lowerBound.y;
			if (bound.upperBound.y > maxY)
				maxY = bound.upperBound.y;

			fix = fix->GetNext();
		}

		b = b->GetNext();
	}

	maxX += 2.0;
	maxY += 2.0;
	minX -= 2.0;
	minY -= 2.0;
	w->left = (long)minX;
	w->right = (long)maxX;
	w->top = (long)maxY;
	w->bottom = (long)minY;

}

DebugDrawGDI::DebugDrawGDI()
{
	ResetScale();
}

DebugDrawGDI::DebugDrawGDI(RECT* r, b2World* world)
{
	ScaleWorldCalculate(r, world);
}

// turn off scaling
void DebugDrawGDI::ResetScale()
{
	// default to no scaling, no moving
	scaleX = 1;
	scaleY = 1;
	scale = 1;
	offsetX = 0;
	offsetY = 0;
	yAdjust = 0;
	matrixTransform = D2D1::Matrix3x2F::Identity();
}

DebugDrawGDI::~DebugDrawGDI()
{
	// nothing to do
}

// have to call this before each call to debugdraw, send in the one you make in on_paint
void DebugDrawGDI::SetRenderTarget(ID2D1HwndRenderTarget* rt, ID2D1Factory* f)
{
	renderTarget = rt;
	factory = f;
	renderTarget->SetTransform(matrixTransform);
}


// r is the rect for the windows drawing window, w is the extent of the box2d world
// r is assumed to have y increasing down, w is assumed to have y increasing up
// r is in pixels, w is in metres
// you can use this function to select any part of the box2d world
// to scale instead of the whole thing, which is the default
void DebugDrawGDI::ScaleWorldCalculate(RECT* r, RECT* w)
{
	int outputWidth = r->right - r->left;
	int outputHeight = r->bottom - r->top;
	int boundsWidth = w->right - w->left;
	int boundsHeight = w->top - w->bottom;
	D2D1::Matrix3x2F scaleM, translateM;

	// ratio of the windows size to the world size
	scaleX = (float)outputWidth / (float)boundsWidth;
	scaleY = (float)outputHeight / (float)boundsHeight;
	scale = scaleX > scaleY ? scaleY : scaleX;

	// move things over if required
	offsetX = r->left - (int)((float)w->left * scale);
	offsetY = r->top - (int)((float)w->bottom * scale);

	// used to flip the y values
	yAdjust = (r->bottom - r->top) - r->top;

	// make a transform matrix
	// scale (-y as part of our y flip)
	scaleM = D2D1::Matrix3x2F::Scale(scale, -scale);
	// translate (+yAdjust is part of the y flip)
	translateM = D2D1::Matrix3x2F::Translation((float)offsetX, (float)(yAdjust - offsetY));
	matrixTransform = scaleM * translateM;  // make one matrix with both operations

	// note: near the bottom of this file are ScaleX functions that
	// do the same thing as the transform, but to one point
}

void DebugDrawGDI::ScaleWorldCalculate(RECT* r, b2World* world)
{
	RECT w;

	GetBoundBox2DBounds(&w, world);
	ScaleWorldCalculate(r, &w);
}


// all these drawing functions assume that gdi->SetTransform()
// has been called with the correct transformation matrix

/// Draw a closed polygon provided in CCW order
void DebugDrawGDI::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	int i;
	ID2D1PathGeometry* geo;
	ID2D1GeometrySink* sink;
	ID2D1SolidColorBrush* brush;
	D2D1::ColorF dColor(color.r, color.g, color.b);
	D2D1_POINT_2F* points = new D2D1_POINT_2F[vertexCount + 1];
	HRESULT hr;

	// create a direct2d pathGeometry
	hr = factory->CreatePathGeometry(&geo);
	hr = geo->Open(&sink);
	sink->SetFillMode(D2D1_FILL_MODE_WINDING);
	// first point
	sink->BeginFigure(D2D1::Point2F(vertices[0].x, vertices[0].y), D2D1_FIGURE_BEGIN_FILLED);
	// middle points
	vertices++;
	vertexCount--;
	for (i = 0; i < vertexCount; i++, vertices++)
	{
		points[i].x = vertices->x;
		points[i].y = vertices->y;
	}
	points[vertexCount].x = points[0].x;
	points[vertexCount].y = points[0].y;
	sink->AddLines(points, vertexCount);
	// close it
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();
	SafeRelease(&sink);

	renderTarget->CreateSolidColorBrush(dColor, &brush);
	renderTarget->DrawGeometry(geo, brush, DBD_LineWidth);

	delete points;
	SafeRelease(&geo);
}

/// Draw a solid closed polygon provided in CCW order.
void DebugDrawGDI::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	int i;
	ID2D1PathGeometry* geo;
	ID2D1GeometrySink* sink;
	ID2D1SolidColorBrush* brush;
	D2D1::ColorF dColor(color.r, color.g, color.b);
	D2D1_POINT_2F* points = new D2D1_POINT_2F[vertexCount + 1];
	HRESULT hr;

	// create a direct2d pathGeometry
	hr = factory->CreatePathGeometry(&geo);
	hr = geo->Open(&sink);
	sink->SetFillMode(D2D1_FILL_MODE_WINDING);
	// first point
	sink->BeginFigure(D2D1::Point2F(vertices[0].x, vertices[0].y), D2D1_FIGURE_BEGIN_FILLED);
	// middle points
	vertices++;
	vertexCount--;
	for (i = 0; i < vertexCount; i++, vertices++)
	{
		points[i].x = vertices->x;
		points[i].y = vertices->y;
	}
	points[vertexCount].x = points[0].x;
	points[vertexCount].y = points[0].y;
	sink->AddLines(points, vertexCount);
	// close it
	sink->EndFigure(D2D1_FIGURE_END_CLOSED);
	sink->Close();
	SafeRelease(&sink);

	renderTarget->CreateSolidColorBrush(dColor, &brush);
	renderTarget->FillGeometry(geo, brush);

	delete points;
	SafeRelease(&geo);
}

/// Draw a circle. outline
void DebugDrawGDI::DrawCircle(const b2Vec2& center, float radius, const b2Color& color)
{
	D2D1::ColorF dColor(color.r, color.g, color.b);
	ID2D1EllipseGeometry* geo;
	ID2D1SolidColorBrush* brush;

	factory->CreateEllipseGeometry(D2D1::Ellipse(D2D1::Point2F(center.x, center.y), radius, radius), &geo);
	renderTarget->CreateSolidColorBrush(dColor, &brush);
	renderTarget->DrawGeometry(geo, brush, DBD_LineWidth);
	SafeRelease(&geo);
}

/// Draw a solid circle.
void DebugDrawGDI::DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color)
{
	D2D1::ColorF dColor(color.r, color.g, color.b);
	ID2D1EllipseGeometry* geo;
	ID2D1SolidColorBrush* brush;

	factory->CreateEllipseGeometry(D2D1::Ellipse(D2D1::Point2F(center.x, center.y), radius, radius), &geo);
	renderTarget->CreateSolidColorBrush(dColor, &brush);
	renderTarget->FillGeometry(geo, brush);
	SafeRelease(&geo);
}

/// Draw a line segment.
void DebugDrawGDI::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
	D2D1::ColorF dColor(color.r, color.g, color.b);
	ID2D1SolidColorBrush* brush;
	D2D_POINT_2F dp0, dp1;

	dp0.x = p1.x;
	dp0.y = p1.y;
	dp1.x = p2.x;
	dp1.y = p2.y;
	renderTarget->CreateSolidColorBrush(dColor, &brush);
	renderTarget->DrawLine(dp0, dp1, brush, DBD_LineWidth);
}

/// Draw a transform. Choose your own length scale.
/// @param xf a transform.
void DebugDrawGDI::DrawTransform(const b2Transform& xf)
{
	ID2D1SolidColorBrush* brush;
	D2D_POINT_2F dp0, dp1;

	renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &brush);
	const float length = 1.0f * scale;
	b2Vec2 start = xf.p;
	b2Vec2 axis = xf.q.GetXAxis();
	b2Vec2 end = start + b2Vec2(axis.x * length, axis.y * length);

	dp0.x = start.x;
	dp0.y = start.y;
	dp1.x = end.x;
	dp1.y = end.y;
	renderTarget->DrawLine(dp0, dp1, brush, DBD_LineWidth);
}

// simple transform from box2d coords to windows coords
// use for x xcoords
int DebugDrawGDI::ScaleX(float fx)
{
	int x;

	fx *= scaleX;
	x = (int)fx;
	x += offsetX;

	return x;
}

// simple transform from box2d coords to windows coords
// does the same thing as the matrixTransform
// use for y coords
int DebugDrawGDI::ScaleY(float fy)
{
	int y;

	fy *= scaleY;
	y = (int)fy;
	y += offsetY;
	y = yAdjust - y;

	return y;
}

// simple transform from box2d coords to windows coords
// use for length, eg a circle's radius
int DebugDrawGDI::Scale(float fr)
{
	int r;

	fr *= scale;
	r = (int)fr;
	if (r < 1)
		r = 1;

	return r;
}

// little helper function for testing
// draw a circle at a fixed location
void DebugDrawGDI::DrawTestCircle(float fx, float fy, float fr)
{
	ID2D1SolidColorBrush* brush;
	D2D1_ELLIPSE elp;

	renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &brush);
	elp.point.x = fx;
	elp.point.y = fy;
	elp.radiusX = fr;
	elp.radiusY = fr;
	renderTarget->FillEllipse(elp, brush);
}

void DebugDrawGDI::DrawPoint(const b2Vec2& p, float size, const b2Color& color)
{
	D2D1::ColorF dColor(color.r, color.g, color.b);
	ID2D1EllipseGeometry* geo;
	ID2D1SolidColorBrush* brush;

	factory->CreateEllipseGeometry(D2D1::Ellipse(D2D1::Point2F(p.x, p.y), size, size), &geo);
	renderTarget->CreateSolidColorBrush(dColor, &brush);
	renderTarget->DrawGeometry(geo, brush, DBD_LineWidth);
	SafeRelease(&geo);
}

float DebugDrawGDI::ScaleXF(float fx)
{
	fx *= scale;
	fx += offsetX;

	return fx;
}

float DebugDrawGDI::ScaleYF(float fy)
{
	fy *= scale;
	fy += offsetY;
	fy = yAdjust - fy;

	return fy;
}