// DebugDrawGDI.h
// 
// 
// Created by PavlAl on 06.05.2013
// Copyright (c) 2013 tatalata, http://talatala.com
// 
// updated to use directx 2d by Alex Russell 2013/09/25

#include <Box2D/Box2D.h>

#pragma once

#define DBD_LineWidth 0.3f

template <class T> void SafeRelease(T** ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

#define DEGREESTORAD 0.01745329251994329f
#define RADTODEGREES 57.2957795130823208f

class DebugDrawGDI : public b2Draw
{
public:
	DebugDrawGDI();
	DebugDrawGDI(RECT* r, b2World* world);
	~DebugDrawGDI();

	// these are the box2d virtual functions we have to implement -------------------------------------------------------------------------------
	/// Draw a closed polygon provided in CCW order.
	virtual void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);

	/// Draw a solid closed polygon provided in CCW order.
	virtual void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);

	/// Draw a circle.
	virtual void DrawCircle(const b2Vec2& center, float radius, const b2Color& color);

	/// Draw a solid circle.
	virtual void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color);

	/// Draw a line segment.
	virtual void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);

	/// Draw a transform. Choose your own length scale.
	/// @param xf a transform.
	virtual void DrawTransform(const b2Transform& xf);
	// -------------------------------------------------------------------------------------------------------------------------------------------

	// a test function, draw a circle at a box2d world coordinate with radius r
	void DrawTestCircle(float x, float y, float r);

	void DrawPoint(const b2Vec2& p, float size, const b2Color& color);

public:
	// set the gdi to a valid gdi class
	void SetRenderTarget(ID2D1HwndRenderTarget* rt, ID2D1Factory* f);

	// setup the transformation matrixto use when drawing
	// also saves the scaling factors
	void ScaleWorldCalculate(RECT* r, RECT* w);
	void ScaleWorldCalculate(RECT* r, b2World* world);

	// turn off scaling / transforming
	void ResetScale();
	// used to scale the box2d coords to windows coords
	// can be used instead of the transformMatrix
	int ScaleX(float fx);
	int ScaleY(float fy);
	float ScaleXF(float fx);
	float ScaleYF(float fy);
	int Scale(float fr);

private:
	// used to scale if not using the transform
	float scaleX, scaleY, scale;
	int offsetX, offsetY;
	int yAdjust;
	// debugdraw assumes this transform is in effect
	D2D1::Matrix3x2F matrixTransform;
	ID2D1HwndRenderTarget* renderTarget;
	ID2D1Factory* factory;
	void GetBoundBox2DBounds(RECT* w, b2World* world);
};