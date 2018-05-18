//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: d3dutility.cpp
// 
// Author: Victor Grinevich (C) All Rights Reserved
//
// System: Intel Pentium b970, 4096 DDR, Geforce 620m, Windows 7 Ultra, MSVC++ 12.0
//
// Desc: Provides utility functions for simplifying common tasks.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"

#define TITLE  L"Движение электрического заряда в магнитном поле"



// vertex formats
const DWORD d3d::Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;


bool d3d::InitD3D(
	HINSTANCE hInstance,
	int width, int height,
	bool windowed,	
	D3DDEVTYPE deviceType,
	IDirect3DDevice9** device, HWND button[] )
{
	//
	// Создание главного окна приложения.
	//

	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = (WNDPROC)d3d::WndProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon( 0, IDI_APPLICATION );
	wc.hCursor       = LoadCursor( 0, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)GetStockObject( WHITE_BRUSH );
	wc.lpszMenuName  = 0;
	wc.lpszClassName = L"Direct3D9App";

	if( !RegisterClass( &wc ) ) 
	{
		::MessageBox( 0, L"RegisterClass() - FAILED", 0, 0 );
		return false;
	}
		
	HWND hwnd = 0;
	hwnd = ::CreateWindow( L"Direct3D9App", TITLE, 
		WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		0, 0, width, height,
		0 /*parent hwnd*/, 0 /* menu */, hInstance, 0 /*extra*/ ); 

	if( !hwnd )
	{
		::MessageBox( 0, L"CreateWindow() - FAILED", 0, 0 );
		return false;
	}
	HWND h = CreateWindowEx( WS_EX_TOPMOST, L"Direct3D9App", L"Окно управления", 
		  WS_BORDER, 
		0, 0, 588, 139,
		0 /*parent hwnd*/, 0 /* menu */, 0, 0 /*extra*/ );
	//← ↑ → ↓ ↖ ↗ ↘ ↙ × ∙
	button[UP] =		CreateWindow( L"button", L"↑",	WS_VISIBLE|WS_CHILD , 0,	0,	50, 50, h, NULL, NULL, NULL );
	button[DOWN] =		CreateWindow( L"button", L"↓",	WS_VISIBLE|WS_CHILD , 51,	0,	50, 50, h, NULL, NULL, NULL );
	button[UPLEFT] =	CreateWindow( L"button", L"↖", WS_VISIBLE|WS_CHILD , 102,	0,	50, 50, h, NULL, NULL, NULL );
	button[UPRIGHT] =	CreateWindow( L"button", L"↗", WS_VISIBLE|WS_CHILD , 153,	0,	50, 50, h, NULL, NULL, NULL );
	button[LEFT] =		CreateWindow( L"button", L"←",	WS_VISIBLE|WS_CHILD , 0,	51, 50, 50, h, NULL, NULL, NULL );
	button[RIGHT] =		CreateWindow( L"button", L"→",	WS_VISIBLE|WS_CHILD , 51,	51, 50, 50, h, NULL, NULL, NULL );
	button[DOWNLEFT] =	CreateWindow( L"button", L"↙", WS_VISIBLE|WS_CHILD , 102,	51,	50, 50, h, NULL, NULL, NULL );
	button[DOWNRIGHT] = CreateWindow( L"button", L"↘", WS_VISIBLE|WS_CHILD , 153,	51, 50, 50, h, NULL, NULL, NULL );
	button[Z] =			CreateWindow( L"button", L"∙",	WS_VISIBLE|WS_CHILD , 204,	0,	50, 50, h, NULL, NULL, NULL );
	button[ZINV] =		CreateWindow( L"button", L"×",	WS_VISIBLE|WS_CHILD , 204,	51, 50, 50, h, NULL, NULL, NULL );

	button[START] =			CreateWindow( L"button", L"ПУСК/ПАУЗА",	WS_VISIBLE|WS_CHILD , 260,	0,	141,50,	h, NULL, NULL ,NULL );
	button[RESETPOS] =		CreateWindow( L"button", L"В начало",	WS_VISIBLE|WS_CHILD , 260,	51,	70,	50,	h, NULL, NULL, NULL );
	button[RESTART] =		CreateWindow( L"button", L"Сначала",	WS_VISIBLE|WS_CHILD , 331,	51,	70,	50,	h, NULL, NULL, NULL );
	button[ROTATEY] =		CreateWindow( L"button", L"Вверх",		WS_VISIBLE|WS_CHILD , 465,	0,	50,	50,	h, NULL, NULL, NULL );
	button[ROTATEYINV] =	CreateWindow( L"button", L"Вниз",		WS_VISIBLE|WS_CHILD , 465,	51,	50,	50,	h, NULL, NULL, NULL );
	button[ROTATEX] =		CreateWindow( L"button", L"Влево",		WS_VISIBLE|WS_CHILD , 409,	25,	55,	50,	h, NULL, NULL, NULL );
	button[ROTATEXINV] =	CreateWindow( L"button", L"Вправо",		WS_VISIBLE|WS_CHILD , 516,	25,	55,	50,	h, NULL, NULL, NULL );

	::ShowWindow( hwnd, SW_SHOW );
	::ShowWindow( h, SW_SHOW );
	::UpdateWindow( hwnd );
	::UpdateWindow( h );

	//
	// Инициализация D3D: 
	//

	HRESULT hr = 0;

	// Шаг 1: Создание IDirect3D9 объекта.  

	IDirect3D9* d3d9 = 0;
    d3d9 = Direct3DCreate9( D3D_SDK_VERSION );

    if( !d3d9 )
	{
		::MessageBox( 0, L"Direct3DCreate9() - FAILED", 0, 0 );
		return false;
	}

	
	// Шаг 2: Проверка настроек gpu.

	D3DCAPS9 caps;
	d3d9->GetDeviceCaps( D3DADAPTER_DEFAULT, deviceType, &caps );

	int vp = 0;
	if( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT )
		vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// Шаг 3: Заполнение D3DPRESENT_PARAMETERS структуры.
	
	D3DPRESENT_PARAMETERS d3dpp;
	d3dpp.BackBufferWidth            = width;
	d3dpp.BackBufferHeight           = height;
	d3dpp.BackBufferFormat           = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount            = 2;
	d3dpp.MultiSampleType            = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality         = NULL;
	d3dpp.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow              = hwnd;
	d3dpp.Windowed                   = windowed;
	d3dpp.EnableAutoDepthStencil     = true; 
	d3dpp.AutoDepthStencilFormat     = D3DFMT_D24S8;
	d3dpp.Flags                      = 0;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE;

	// Шаг 4: Создание устройства.

	hr = d3d9->CreateDevice(
		D3DADAPTER_DEFAULT, // первичный адаптер
		deviceType,         // тип устройства
		hwnd,               // свзязь окна с устройством
		vp,                 // вершинная обработка
	    &d3dpp,             // параметры показа
	    device);            // созданной устройство

	if( FAILED(hr) )
	{
		
		// Попытка использования  16-битный глубинный буффер

		d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
		
		hr = d3d9->CreateDevice(
			D3DADAPTER_DEFAULT,
			deviceType,
			hwnd,
			vp,
			&d3dpp,
			device );

		if( FAILED( hr ) )
		{
			d3d9->Release(); 
			::MessageBox( 0, L"CreateDevice() - FAILED", 0, 0 );
			return false;
		}
	}

	d3d9->Release(); 
	
	return true;
}

int d3d::EnterMsgLoop( bool (*ptr_display)( float timeDelta ) )
{
	MSG msg;
	::ZeroMemory( &msg, sizeof( MSG ) );
	
	static float lastTime = (float)timeGetTime(); 

	while( msg.message != WM_QUIT )
	{
		if( ::PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
		{
			::TranslateMessage( &msg );
			::DispatchMessage( &msg );
		}
		else
        {	
			float currTime  = (float)timeGetTime();
			float timeDelta = ( currTime - lastTime ) * 0.001f;

			ptr_display( timeDelta );
			
			lastTime = currTime;
        }
    }
    return msg.wParam;
}

D3DLIGHT9 d3d::InitDirectionalLight( D3DXVECTOR3* direction, D3DXCOLOR* color )
{
	D3DLIGHT9 light;
	::ZeroMemory( &light, sizeof( light ) );

	light.Type      = D3DLIGHT_DIRECTIONAL;
	light.Ambient   = *color * 0.4f;
	light.Diffuse   = *color;
	light.Specular  = *color * 0.6f;
	light.Direction = *direction;

	return light;
}

D3DLIGHT9 d3d::InitPointLight( D3DXVECTOR3* position, D3DXCOLOR* color )
{
	D3DLIGHT9 light;
	::ZeroMemory( &light, sizeof( light ) );

	light.Type      = D3DLIGHT_POINT;
	light.Ambient   = *color * 0.4f;
	light.Diffuse   = *color;
	light.Specular  = *color * 0.6f;
	light.Position  = *position;
	light.Range        = 1000.0f;
	light.Falloff      = 1.0f;
	light.Attenuation0 = 1.0f;
	light.Attenuation1 = 0.0f;
	light.Attenuation2 = 0.0f;

	return light;
}

D3DLIGHT9 d3d::InitSpotLight( D3DXVECTOR3* position, D3DXVECTOR3* direction, D3DXCOLOR* color )
{
	D3DLIGHT9 light;
	::ZeroMemory( &light, sizeof( light ) );

	light.Type      = D3DLIGHT_SPOT;
	light.Ambient   = *color * 0.4f;
	light.Diffuse   = *color;
	light.Specular  = *color * 0.6f;
	light.Position  = *position;
	light.Direction = *direction;
	light.Range        = 1000.0f;
	light.Falloff      = 1.0f;
	light.Attenuation0 = 1.0f;
	light.Attenuation1 = 0.0f;
	light.Attenuation2 = 0.0f;
	light.Theta        = 0.5f;
	light.Phi          = 0.7f;

	return light;
}

D3DMATERIAL9 d3d::InitMtrl( D3DXCOLOR a, D3DXCOLOR d, D3DXCOLOR s, D3DXCOLOR e, float p )
{
	D3DMATERIAL9 mtrl;
	mtrl.Ambient  = a;
	mtrl.Diffuse  = d;
	mtrl.Specular = s;
	mtrl.Emissive = e;
	mtrl.Power    = p;
	return mtrl;
}


d3d::BoundingSphere::BoundingSphere()
{
	_radius = 0.0f;
}


