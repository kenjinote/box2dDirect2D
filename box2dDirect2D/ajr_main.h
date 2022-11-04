// the main ajr test code header

#pragma once

#include "DebugDrawGDI.h"

#define DEGREESTORAD 0.01745329251994329f
#define RADTODEGREES 57.2957795130823208f

class CAjrMain
{
public:
	// construction and destruction
	CAjrMain(RECT* r, HWND hwnd);
	~CAjrMain();

	// time
	long long SetNextTime();
	long long GetNextTime() { return nextTime; }
	long long GetNowTime();
	long long GetOldNowTime() { return nowTime; }

protected:
	// box2d
	DebugDrawGDI* DebugDraw;
	b2Vec2 gravity;
	b2World* world;
	b2Body* b2box;

	// general
	RECT mainRect; // where we draw
	void SetMainRect(RECT* r, HWND hwnd, RECT* w);
	void SetMainRect(RECT* r, HWND hwnd, b2World* world);

private:
	// direct2d stuff
	ID2D1Factory* pD2DFactory;
	ID2D1HwndRenderTarget* renderTarget;
	ID2D1SolidColorBrush* pBlackBrush;
	ID2D1Bitmap* boxImage;

	// windows imaging
	IWICImagingFactory* pImageFactory;

	// startup
	void StartUpCode();

	// timing
	long long ticksPerFrame;
	long long nextTime;
	long long nowTime;
	int fps;
	float box2dStepSeconds;
	int velocityIterations;
	int positionIterations;

public:
	void SetMainRect(RECT* r, HWND hwnd);
	RECT* GetMainRect() { return &mainRect; }
	ID2D1SolidColorBrush* GetBlackBrush() { return pBlackBrush; }
	ID2D1HwndRenderTarget* GetRenderTarget() { return renderTarget; }
	ID2D1Factory* GetFactory() { return pD2DFactory; }
	int MainLogic();
	void MainDraw();
	void StartFrame();
	void DoneFrame();
	void CreateBox2dWorld();
	ID2D1Bitmap* LoadBitmapFromFile(PCWSTR fileName);
};

