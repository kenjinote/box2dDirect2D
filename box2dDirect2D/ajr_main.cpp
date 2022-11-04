// the main ajr test code

#include "stdafx.h"
#include "ajr_main.h"

void CAjrMain::StartUpCode()
{
	unsigned int flags;

	// create the render factory
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);

	// basic box2d world setup
	gravity.x = 0.0f;
	gravity.y = -10.0f;
	world = new b2World(gravity);
	DebugDraw = new DebugDrawGDI();

	// timers
	fps = 60; // our goal, not the actual fps
	LARGE_INTEGER perf;
	if (QueryPerformanceFrequency(&perf))
	{
		ticksPerFrame = (long long)perf.QuadPart / fps;
	}
	else
	{
		// wow, your machine is old!
	}

	// more box2d setup
	// choose what to show
	flags = 0;
	flags |= b2Draw::e_shapeBit;
	flags |= b2Draw::e_jointBit;
	//flags |= b2Draw::e_aabbBit;
	flags |= b2Draw::e_pairBit;
	flags |= b2Draw::e_centerOfMassBit;
	DebugDraw->SetFlags(flags);
	box2dStepSeconds = 1.0f / (float)fps;  //how many seconds to step each calculation
	velocityIterations = 8; // reccommended defaults
	positionIterations = 3;

	// com stuff for loading bitmaps
	CoInitialize(NULL);
	// Create the COM imaging factory
	hr = CoCreateInstance(CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pImageFactory)
	);
}

CAjrMain::CAjrMain(RECT* r, HWND hwnd)
{
	StartUpCode();

	SetMainRect(r, hwnd);
}

CAjrMain::~CAjrMain()
{
	delete DebugDraw;
	delete world;

	SafeRelease(&pD2DFactory);
}

// setup where we are drawing, and the offscreen buffer (bitmap)
void CAjrMain::SetMainRect(RECT* r, HWND hwnd)
{
	HRESULT hr;

	mainRect = *r;
	hr = pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(r->right - r->left, r->bottom - r->top)),
		&renderTarget
	);
	renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &pBlackBrush);
}

void CAjrMain::SetMainRect(RECT* r, HWND hwnd, RECT* w)
{
	SetMainRect(r, hwnd);
	DebugDraw->ScaleWorldCalculate(r, w);
}

void CAjrMain::SetMainRect(RECT* r, HWND hwnd, b2World* world)
{
	SetMainRect(r, hwnd);
	DebugDraw->ScaleWorldCalculate(r, world);
}

long long CAjrMain::SetNextTime()
{
	LARGE_INTEGER perf;

	QueryPerformanceCounter(&perf);
	nowTime = perf.QuadPart;
	nextTime = nowTime + ticksPerFrame;

	return nextTime;
}

long long CAjrMain::GetNowTime()
{
	LARGE_INTEGER perf;

	QueryPerformanceCounter(&perf);

	return perf.QuadPart;
}

void CAjrMain::CreateBox2dWorld()
{
	// will be used to add each item to the world, can re-use safely
	b2BodyDef def;
	b2PolygonShape poly;
	b2CircleShape circle;
	b2FixtureDef fixture;
	b2Body* body;

	// add box2d objects
	// a box to hold every thing
	// box2d will clean up all the bodies for us

	// top
	def.position.Set(0.0, 50);
	body = world->CreateBody(&def);
	// half width, half height, will be centred at position
	poly.SetAsBox(50.0, 1.0);
	body->CreateFixture(&poly, 0.0);
	// bottom
	def.position.Set(0.0, -50.0);
	body = world->CreateBody(&def);
	poly.SetAsBox(50.0, 1.0);
	body->CreateFixture(&poly, 0.0);
	// left
	def.position.Set(-50, 0);
	body = world->CreateBody(&def);
	poly.SetAsBox(1.0, 50.0);
	body->CreateFixture(&poly, 0.0);
	// right
	def.position.Set(50, 0);
	body = world->CreateBody(&def);
	poly.SetAsBox(1.0, 50.0);
	body->CreateFixture(&poly, 0.0);

	// a falling box
	def.type = b2_dynamicBody;
	def.position.Set(0.0, 45.0);
	b2box = body = world->CreateBody(&def);
	poly.SetAsBox(3.55f, 3.60f);
	fixture.shape = &poly;
	fixture.density = 2.0;
	fixture.friction = 0.3f;
	fixture.restitution = 0.9f;
	body->CreateFixture(&fixture);

	// a falling ball
	def.position.Set(-3.0, 35.0);
	body = world->CreateBody(&def);
	circle.m_radius = 3.0;
	fixture.shape = &circle;
	body->CreateFixture(&fixture);

	// setup transform
	DebugDraw->ScaleWorldCalculate(&mainRect, world);
	// tell world to use this class to debug draw
	world->SetDebugDraw(DebugDraw);
	SetNextTime();

	boxImage = LoadBitmapFromFile(_T("test.bmp"));
}

int CAjrMain::MainLogic()
{
	int doDraw = false;

	if (GetNowTime() >= nextTime)
	{
		doDraw = true;
		SetNextTime();

		// box2d solver, yup, that is all you do
		world->Step(box2dStepSeconds, velocityIterations, positionIterations);
	}

	return doDraw;
}

void CAjrMain::MainDraw()
{
	D2D1_RECT_F ru;
	D2D1_POINT_2F p;
	D2D1_SIZE_F sizeF;
	D2D1::Matrix3x2F mTrans, mRotate, mAll;

	// clear the window
	StartFrame();
	renderTarget->Clear();

	// draw the box2d world using debugdraw functions
	DebugDraw->SetRenderTarget(renderTarget, pD2DFactory); // this sets the transform too
	world->DebugDraw();

	//DebugDraw->DrawTestCircle(0,0,5);
	//DebugDraw->DrawTestCircle(10,-10,5);

	// draw the bitmap on top of the box --- --- -- --
	float angle = b2box->GetAngle();
	b2Vec2 pos = b2box->GetPosition();
	// move our bitmap to where the box is and rotate it to match
	angle *= -RADTODEGREES; // negative so it rotates the right direction
	sizeF = boxImage->GetSize();
	// box2d position is at the center of its object, move our object to match
	mTrans = D2D1::Matrix3x2F::Translation(-(sizeF.width / 2.0f), -(sizeF.height / 2.0f));
	// rotate to match box2d, get centre of rotation
	p.x = DebugDraw->ScaleXF(pos.x);
	p.y = DebugDraw->ScaleYF(pos.y);
	mRotate = D2D1::Matrix3x2F::Rotation(angle, p);
	mAll = mTrans * mRotate;
	renderTarget->SetTransform(&mAll);
	// now draw the box, at the scaled position, the transform looks after the rotation
	ru.left = p.x;
	ru.top = p.y;
	ru.right = ru.left + sizeF.width;
	ru.bottom = ru.top + sizeF.height;
	renderTarget->DrawBitmap(boxImage, &ru);

	// no transform, to undo the box2d transforms
	renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

	DoneFrame();
}

void CAjrMain::DoneFrame()
{
	renderTarget->EndDraw();
}

void CAjrMain::StartFrame()
{
	renderTarget->BeginDraw();
}

ID2D1Bitmap* CAjrMain::LoadBitmapFromFile(PCWSTR fileName)
{
	IWICBitmapDecoder* pDecoder = NULL;
	IWICBitmapFrameDecode* pSource = NULL;
	IWICStream* pStream = NULL;
	IWICFormatConverter* pConverter = NULL;
	IWICBitmapScaler* pScaler = NULL;
	ID2D1Bitmap* bitmap = NULL;

	HRESULT hr = pImageFactory->CreateDecoderFromFilename(
		fileName,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		&pDecoder
	);


	if (SUCCEEDED(hr))
	{
		// Create the initial frame.
		hr = pDecoder->GetFrame(0, &pSource);
	}

	if (SUCCEEDED(hr))
	{

		// Convert the image format to 32bppPBGRA
		// (DXGI_FORMAT_B8G8R8A8_UNORM + D2D1_ALPHA_MODE_PREMULTIPLIED).
		hr = pImageFactory->CreateFormatConverter(&pConverter);
	}

	if (SUCCEEDED(hr))
	{
		hr = pConverter->Initialize(
			pSource,
			GUID_WICPixelFormat32bppPBGRA,
			WICBitmapDitherTypeNone,
			NULL,
			0.f,
			WICBitmapPaletteTypeMedianCut
		);
	}

	if (SUCCEEDED(hr))
	{
		// Create a Direct2D bitmap from the WIC bitmap.
		hr = renderTarget->CreateBitmapFromWicBitmap(
			pConverter,
			NULL,
			&bitmap
		);
	}

	SafeRelease(&pDecoder);
	SafeRelease(&pSource);
	SafeRelease(&pStream);
	SafeRelease(&pConverter);
	SafeRelease(&pScaler);

	return bitmap;
}