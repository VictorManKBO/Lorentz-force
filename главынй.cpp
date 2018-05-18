//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: главный.cpp
// 
// Author: Victor Grinevich (C) All Rights Reserved
//
// System: Intel Pentium b970, 4096 DDR, Geforce 620m, Windows 7 Ultra, MSVC++ 12.0 
//
// Desc: The main program file.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include <string.h>
#include <math.h>
#include <vector>
#include <d3dx9core.h> 




//
// Globals
//

IDirect3DDevice9* pDevice = NULL; 

int width  = 0;// 0 - фулл скрин
int height = 0;// 0 - фулл скрин

HWND button[COUNT];
char butstate = UNPRESSED;

IDirect3DTexture9* pTexture = NULL;
IDirect3DVertexBuffer9* pBufferVershin = NULL;
IDirect3DIndexBuffer9* pBufferIndex = NULL;

ID3DXFont * pFont = NULL;      // Шрифт Diect3D
RECT Rec;       // Прямоугольник


d3d::BoundingSphere BSphere;
ID3DXMesh* pSphere = NULL;
ID3DXMesh* pCharge = NULL;
ID3DXMesh* pTailmsh = NULL;

//Стрелка вектора магнитной индукции
ID3DXMesh* pVecMagnInd = NULL;
std::vector<D3DMATERIAL9> vec_magnindMtrl( NULL );
std::vector<IDirect3DTexture9*> vec_magnindTextures( NULL );


D3DXVECTOR3 pos( 0.0f, 0.0f, 0.0f );
D3DXVECTOR3 target( 0.0f, 0.0f, 0.0f );
D3DXVECTOR3 up( 0.0f, 1.0f, 0.0f );

float deltaX; // прирощение координаты
float v0 = 0.0f;	// скорость заряда
D3DXVECTOR3 b( 1.0f, 0.0f, 0.0f );
D3DXVECTOR3 v( 1.0f, 0.0f, 0.0f );
float q = 1.6f * 100;
float m = 1.67f;
float time;
float chargeX = -300.0f;
float chargeY = 0.0f;
float chargeZ = 0.0f;
float dv = -1.0f, angle = 0.0f;
float dx, dy, dz;
float dx1, dy1, dz1;
#define TAILLEN 200
struct {
	float x, y, z;
	float time;
} tail[TAILLEN] ;

#define mx( y ) ( dx1 - dx )*( y - dy )/( dy1 - dy ) + dx
#define mz( y ) ( dz1 - dz )*( y - dy )/( dy1 - dy ) + dz

float angle_camX = 0.0f, angle_camY = 0.0f, rad_cam = 500.0f;


//
// Functions
//



int Sign( float a ){
	if( a > 0.0f ) return 1;
	else if( a < 0.0f ) return -1;
	else return 0;
}

VOID LoadXfile( LPCWSTR nameXfile, IDirect3DDevice9* device, ID3DXMesh** outMesh, 
		std::vector<D3DMATERIAL9>* outMtrl, std::vector<IDirect3DTexture9*>* outTextures )
{
	//Загрузка сетки
	
	HRESULT hr = NULL;
	ID3DXBuffer* adjBuffer = NULL;
	ID3DXBuffer* mtrljBuffer = NULL;
	DWORD numMtrls = NULL;

	hr = D3DXLoadMeshFromX( nameXfile, D3DXMESH_MANAGED, device, &adjBuffer, &mtrljBuffer, NULL, &numMtrls, &*outMesh ); 
	if( FAILED( hr ) )
	{
		::MessageBox( 0, L"D3DMeshFromFileX() - Failed", 0, 0 );
	}

	//Извлечение материала и загрузка текстур
	if ( mtrljBuffer != NULL && numMtrls != NULL ){
		D3DXMATERIAL * mtrls = (D3DXMATERIAL*)mtrljBuffer->GetBufferPointer();
		for( int i=0; i < numMtrls; i++		){
			mtrls[i].MatD3D.Ambient = mtrls[i].MatD3D.Diffuse;
			//Сохранение материала
			if ( outMtrl != NULL )
				outMtrl->push_back( mtrls[i].MatD3D );
			//Проверяем, связана ли с i-тым материалом текстура
			if( mtrls[i].pTextureFilename != NULL )
			{
				IDirect3DTexture9* tex = NULL;
				D3DXCreateTextureFromFile( device, LPCWSTR( mtrls[i].pTextureFilename ), &tex );
				//Соxраняем текстуру, если пользователь разрешил
				if( outTextures != NULL )
					outTextures->push_back( tex );
			}
			else if( outTextures != NULL )
					outTextures->push_back( NULL );

		}
	}
	//Уничтожаем неиспользуемые переменные
	d3d::Release<ID3DXBuffer*> ( mtrljBuffer );
	d3d::Release<ID3DXBuffer*> ( adjBuffer );
}


VOID WaitUnpressKey( char a ){
	do{
	}
	while ( ::GetAsyncKeyState( a ) & 0x8000f );
}

VOID Portate( float phi ){//Функция поворота вокруг оси y
#define cnt 3
			//ось у
			float matrix[cnt][cnt]= { cosf( D3DX_PI/2 - phi ), 0.0f, sinf( D3DX_PI/2 - phi ),
														 0.0f, 1.0f, 0.0f,
									-sinf( D3DX_PI/2 - phi ), 0.0f, cosf( D3DX_PI/2 - phi ) };
			float vec[cnt][1] = { chargeX,
								chargeZ,
								chargeY };
			float res[cnt][1] = { 0.0f };
			for( int j = 0; j < cnt; j++ )
					for( int k = 0; k < cnt; k++ )
							res[j][0] += matrix[j][k] * vec[k][0];
			chargeX = res[0][0];
			chargeZ = res[1][0];
			chargeY = res[2][0];
}

VOID InitFont( IDirect3DDevice9* pDevice )
{
    // Создаём и инициализируем шрифт
	D3DXCreateFont( pDevice, 
					30, //высота
					10, // ширина
					FW_NORMAL, //вес
					0, // кол-во уровне й mipmap
					FALSE, //курсив
					ANSI_CHARSET,// набор символов RUSSIAN_CHARSET
					OUT_DEFAULT_PRECIS, // соответсвие желаемого шрифта и фактического
					DEFAULT_QUALITY, //соответсвие желаемого шрифта с реальным
					DEFAULT_PITCH|FF_MODERN,//
					L"Arial", 
					&pFont);	
}

VOID DrawMyText( float x, float y, LPCWSTR strokaTexta  ){
	RECT r = { 600, 10 , 1300, 700 };

	DWORD color = d3d::BLACK;
	pFont->DrawText( NULL, strokaTexta, -1, &r, DT_WORDBREAK, color );

}

VOID MouseButton( float timeDelta ){

#define INITX chargeX = -300.0f; chargeY = 0.0f; chargeZ = 0.0f; 
#define INITZ chargeX = 0.0f; chargeY = 0.0f; chargeZ = -300.0f;

	if( v0 == 0.0f ) 
	switch ( STATE( butstate ) )
	{
		case UP: 
				b = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
				INITX
				break;
		case DOWN:
				b = D3DXVECTOR3( 0.0f, -1.0f, 0.0f );
				INITX
				break;
		case LEFT:
				b = D3DXVECTOR3( -1.0f, 0.0f, 0.0f );
				INITX
				break;
		case RIGHT:
				b = D3DXVECTOR3( 1.0f, 0.0f, 0.0f );
				INITX
				break;
		case Z:
				b = D3DXVECTOR3( 0.0f, 0.0f, 1.0f );				
				break;
		case ZINV:
				b = D3DXVECTOR3( 0.0f, 0.0f, -1.0f );
				break;
		case UPLEFT:
				b = D3DXVECTOR3( -1.0f, 1.0f, 0.0f );
				INITX
				break;
		case UPRIGHT:
				b = D3DXVECTOR3( 1.0f, 1.0f, 0.0f );
				INITX
				break;
		case DOWNLEFT:
				b = D3DXVECTOR3( -1.0f, -1.0f, 0.0f );
				INITX
				break;
		case DOWNRIGHT:
				b = D3DXVECTOR3( 1.0f, -1.0f, 0.0f );
				INITX
				break;

		default:
				break;
	}

	switch ( STATE( butstate ) ){
		case START:
				if( v0 == 0.0f ) v0 = 50.0f;
				else v0 = 0.0f;
				break;
		case RESETPOS:
				angle_camX = angle_camY  = 0.0f;
				break;
		case RESTART:
				v0 = 0.0f;
				angle = 0.0f;
				INITX
				break;
		case ROTATEX:
				angle_camX += 20.0f * timeDelta ;
				break;
		case ROTATEXINV:
				angle_camX -= 20.0f * timeDelta ;
				break;
		case ROTATEY:
				angle_camY += 20.0f * timeDelta ;
				break;
		case ROTATEYINV:
				angle_camY -= 20.0f * timeDelta ;
				break;

	default:
		break;
	}
	butstate = UNPRESSED;	
}

BOOL Setup()
{
	InitFont( pDevice );
	
	//
	//Загрузка X - объекта
	//

	LoadXfile( L"MagneticInduction.x", pDevice, &pVecMagnInd , &vec_magnindMtrl, &vec_magnindTextures );

	//
	// Построение сферы - магнитное поле, заряд, хвост заряда
	//

	BSphere._radius = 100;
	D3DXCreateSphere( pDevice, BSphere._radius, 50, 50, &pSphere, 0 );
	D3DXCreateSphere( pDevice, 10.0f, 20, 20, &pCharge, 0 );
	D3DXCreateSphere( pDevice, 3.0f, 20, 20, &pTailmsh, 0 );

	//
	// Настройка света
	// 

	pDevice->SetRenderState( D3DRS_NORMALIZENORMALS, true );
	pDevice->SetRenderState( D3DRS_SPECULARENABLE, false );	

	//
	// Установка матрицы вида
	//

	D3DXVECTOR3 pos( 0.0f, 0.0f, -500.0f );
	D3DXVECTOR3 target( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 up( 0.0f, 1.0f, 0.0f );

	D3DXMATRIX v;
	D3DXMatrixLookAtLH( &v, &pos, &target, &up );
	pDevice->SetTransform( D3DTS_VIEW, &v );
	
	

	//
	// Установка матрицы проекции
	//

	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(
			&proj,
			D3DX_PI * 0.25f, // 45 - градусов
			(float)width / (float)height,
			1.0f,
			1000.0f );
	pDevice->SetTransform( D3DTS_PROJECTION, &proj );

	return true;
}

VOID Cleanup()
{
	d3d::Release<ID3DXMesh*>( pSphere );
	d3d::Release<ID3DXMesh*>( pVecMagnInd );
	for( int i = 0; i < vec_magnindTextures.size(); i++ )
		d3d::Release<IDirect3DTexture9*>( vec_magnindTextures[i] );
	vec_magnindMtrl.resize( NULL );

	if( pTexture != NULL )
        pTexture->Release();

     if( pBufferIndex  != NULL )
        pBufferIndex->Release(); 

    if( pBufferVershin  != NULL )
        pBufferVershin->Release(); 
	if( pFont  != NULL )
        pFont->Release();
	d3d::Release<ID3DXMesh*>( pTailmsh );
	d3d::Release<ID3DXMesh*>( pCharge );
	
	for( int i = 0; i < vec_magnindTextures.size(); i++ )
		d3d::Release<IDirect3DTexture9*>( vec_magnindTextures[i] );
	vec_magnindMtrl.resize( NULL );
	d3d::Release<IDirect3DTexture9*>( pTexture );
}

bool Display( float timeDelta )
{
	if( pDevice )
	{
		


		//
		// Обновление точки наблюдателя на сфере радиуса rad_cam
		//
		
		pos.z = -rad_cam;
		
		if( ::GetAsyncKeyState( VK_UP ) & 0x8000f )
			angle_camY += 1.0f * timeDelta ;			
		
		if( ::GetAsyncKeyState( VK_DOWN ) & 0x8000f )
			angle_camY -= 1.0f * timeDelta ;			
		
		if( ::GetAsyncKeyState( VK_RIGHT ) & 0x8000f ) 
			angle_camX += 1.0f * timeDelta ;

		if( ::GetAsyncKeyState( VK_LEFT ) & 0x8000f )
			angle_camX -= 1.0f * timeDelta ;

		if( ::GetAsyncKeyState( 'Z' ) & 0x8000f )
			angle_camX = angle_camY = 0;
		
		if( ::GetAsyncKeyState( VK_ADD ) & 0x8000f )
			rad_cam -= 100.0f * timeDelta ;
		
		if( ::GetAsyncKeyState( VK_SUBTRACT ) & 0x8000f )
			rad_cam += 100.0f * timeDelta ;

		//
		// Проверка нажатия кнопок управления мышью
		//

		MouseButton( timeDelta );

		//
		// Клавиша перезапуска сцены
		//

		if( ::GetAsyncKeyState( 'R' ) & 0x8000f ){
			v0 = 0.0f;
			angle = 0.0f;
			chargeX = -300.0f;
			chargeY = 0.0f;
			chargeZ = 0.0f;
		}

		//
		// Клавиша запуска/останоки заряда
		//

		if( ::GetAsyncKeyState( VK_SPACE ) & 0x8000f ){
			WaitUnpressKey( VK_SPACE );
			if( v0 == 0.0f ) v0 = 50.0f;
			else v0 = 0.0f;
		}
		 
		//
		//Обновление матрицы вида для точки наблюдателя
		//

		D3DXMATRIX view;		
		D3DXMatrixLookAtLH( &view, &pos, &target, &up );
		D3DXMATRIX rotV;
		D3DXMatrixRotationY( &rotV, angle_camX );
		D3DXMatrixMultiply( &view, &rotV, &view );

		D3DXMatrixRotationX( &rotV, angle_camY );
		D3DXMatrixMultiply( &view, &rotV, &view );
		pDevice->SetTransform( D3DTS_VIEW, &view );

		//
		//Обновление позиции точечного источника света
		//

		D3DXCOLOR col( 1.0f, 1.0f, 1.0f, 1.0f );
		D3DXVECTOR3 poslight( rad_cam * sinf( angle_camX ), 0.0f, -rad_cam * cosf( angle_camX ) );
		D3DLIGHT9 light = d3d::InitPointLight( &poslight, &col );
		pDevice->SetLight( 0, &light );
		pDevice->LightEnable( 0, true );		
		 
		//
		// Просчёт углов и физичеких характеристик
		//

		D3DXVECTOR3 res;
		D3DXVec3Cross( &res, &v, &b );
		float f = q * D3DXVec3Length( &res );
		float a = f/m;
		a = 0.0f;
		
		if( v0 > 0.0f ) time = timeDelta;
		else 
			time = 0.0f;

		float sgn = 0.0f;
		if ( (b.z) || (b.y) == 1.0f )sgn = 1.0f;
		else if ( (b.z) || (b.y) == -1.0f )sgn = -1.0f;

		float phi = sgn * acos( D3DXVec3Dot( &v, & b )/( D3DXVec3Length( &v ) * D3DXVec3Length( &b ) ) );
		if( ( phi == 0 ) && ( b != v ) ) phi = D3DX_PI;
		
		//
		// Алгоритм движения частицы
		//

		if( ( chargeX > 400.0f ) || ( abs( chargeZ ) > 220.0f ) ) {
			chargeX = -300.0f; chargeY = 0.0f; chargeZ = 0.0f; v0 = 0.0f; angle = 0.0f;
		}
		
		if( ( chargeX < BSphere._center.x - 100.0f )||( b == D3DXVECTOR3( 1.0f, 0.0f, 0.0f ) )||( b == D3DXVECTOR3( -1.0f, 0.0f, 0.0f ) ) ){
					// Движение до магнтиного поля
					if( time > 0.09f ) time = 0.0f;
					chargeX += v0 * time ;
					chargeZ = 0.0f;
					chargeY = 0.0f;
		}
		else if( ( abs( abs( phi ) - D3DX_PI/2 ) < 0.000001 )&& ( b.z == 0.0f ) ){
				// Движение по окружности для b.z = 0
				if( angle > 6.28f ) angle = 0.0f;
				
				chargeX = -BSphere._radius * cosf( angle );
				chargeZ = BSphere._radius * sinf( angle * ( b.y ) ); // b.y направление вращения
				if( v0 > 0.0f ) angle += timeDelta;
			 }
			 else if ( abs( abs( phi ) - D3DX_PI/2 ) < 0.000001 ) {
				 // Движение по окружности для b.у = 0
					if( angle > 6.28f ) angle = 0.0f;
				
					chargeX = -BSphere._radius * cosf( angle );
					chargeY = BSphere._radius * sinf( angle * ( b.z ) ); // b.y направление вращения
					if( v0 > 0.0f ) angle += timeDelta;
			 }
			 else if ( ( abs( phi ) > 0.0f ) && ( abs( phi ) < D3DX_PI ) ){			
						
						if( ( chargeX*chargeX + chargeY*chargeY + chargeZ*chargeZ > BSphere._radius*BSphere._radius ) &&
							( v0 > 0.0f ) )	{
								// Движение за пределами магнитного поля
								chargeY += v0 * time * Sign( dy - dy1 );
								chargeX = mx( chargeY );
								chargeZ = mz( chargeY );
						}
						else{
							// Движение по спирали
							float r = abs( BSphere._radius * sinf( phi ) );
							r = (int) r;
							chargeX = -b.y * r * cosf( angle ) ; // b.y направление вращения
							chargeZ = r * sinf( angle * ( b.y ) )  ;// b.y направление вращения
							if( b.x > 0) chargeY = angle * 10 - r ;
							if( b.x < 0) chargeY = angle * 10 + r ;

							if( v0 > 0.0f ) angle += timeDelta;

							Portate( phi );		
							dx1 = dx; dy1 = dy; dz1 = dz;
							dx = chargeX; dy = chargeY; dz = chargeZ;
						}
				    }
		//
		// Поведение хвоста заряда
		//

		for( int u = TAILLEN - 1; ( u > 0 )&&( v0 != 0.0f ); u-- ) tail[u] = tail[u-1];
		if( chargeX == -300.0f ) 
			for( int u = TAILLEN - 1; u > 0 ; u-- ) tail[u].x =  0.0f; 
		tail[0].x = chargeX;
		tail[0].y = chargeY;	
		tail[0].z = chargeZ;
		
			
		//
		// Центр магнитного поля
		//

		BSphere._center = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );		

		//
		// Визуализация
		//

		pDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, d3d::WHITE, 1.0f, 0);
		pDevice->BeginScene();
		std::wstring s = L"С помощью стрелок можно направить вектор магнитной индукции. Процесс запускается кнопкой \"пуск\". Кнопка \"В начало\" вернёт исходную позиию камеры, а кнопка "; 
			s.append( L"\"Сначала\" перезапустит модель. С помощью стрелок можно вращать модель, так же это можно сделать с помощью стрелок на клавиатуре, удачи!");
		DrawMyText( 0.0f, 0.0f, (LPCWSTR)s.c_str() );
		
		//Установка мировой матрицы на координаты сферы для вектора магнитной индукции
		D3DXMATRIX w;		
		D3DXMatrixTranslation( &w,
			BSphere._center.x,
			BSphere._center.y,
			BSphere._center.z );

		//Установка направления вектора b
		D3DXMATRIX rot; 
		if ( b.z == 0.0f ){
			float anglebY = D3DX_PI/2 - phi; // 90 - phi, т.к phi - угол между b и оХ, а нужно оУ
			D3DXMatrixRotationZ( &rot, -anglebY );
		}
		else{
			float anglebY = phi;
			D3DXMatrixRotationX( &rot, -anglebY * b.z );
		}
		D3DXMatrixMultiply( &w, &rot, &w );
		pDevice->SetTransform( D3DTS_WORLD, &w );

		//Прорисовка стрелки вектора магнитной индукции		
		for( int i = 0; i < vec_magnindMtrl.size(); i++ ){
			pDevice->SetMaterial( &vec_magnindMtrl[i] );
			pDevice->SetTexture( NULL, vec_magnindTextures[i] );
			pVecMagnInd->DrawSubset( i );
		}		

		//Установка мировой матрицы на координаты заряда
		D3DXMatrixTranslation( &w, chargeX, chargeY, chargeZ );
		pDevice->SetTransform( D3DTS_WORLD, &w );

		//Установа материала и прорисовка заряда
		D3DMATERIAL9 red = d3d::RED_MTRL;
		pDevice->SetMaterial( &red );
		pCharge->DrawSubset( 0 );
		
		pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
		//Установка мировой матрицы на координаты хвоста заряда
		for(int i = 0; ( i < TAILLEN )&&( tail[i].x != 0.0f ); i++ ){
			D3DXMatrixTranslation( &w, tail[i].x, tail[i].y, tail[i].z );
			pDevice->SetTransform(D3DTS_WORLD, &w );	

			//Установа материала и прорисовка заряда
			D3DMATERIAL9 red = d3d::RED_MTRL;			
			red.Diffuse.a = 0.10f; // 10% прозрачность
			pDevice->SetMaterial( &red );
			
			pTailmsh->DrawSubset( 0 );
		}
		pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, false );

		//Установка мировой матрицы на координаты сферы
		if( D3DXVec3Length( &b ) > 0.0f ){
			D3DXMatrixTranslation( &w, BSphere._center.x, BSphere._center.y, BSphere._center.z );
			pDevice->SetTransform( D3DTS_WORLD, &w );		
		

			//Включение смешивания
			pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, true );
			pDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			pDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		
			//Установа материала и прорисовка сферы
			D3DMATERIAL9 blue = d3d::BLUE_MTRL;
			blue.Diffuse.a = 0.25f; // 25% прозрачность
			pDevice->SetMaterial( &blue );
			pSphere->DrawSubset( 0 );
		}
		
		pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, false );		
		pDevice->EndScene();
		pDevice->Present( NULL, NULL, NULL, NULL );
		
		
	}
	return true;
}

//
// Функция обработки сообщений
//

LRESULT CALLBACK d3d::WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_DESTROY:
		::PostQuitMessage( 0 );
		break;
		
	case WM_KEYDOWN:
		if( wParam == VK_ESCAPE )
			::DestroyWindow( hwnd );

		break;
	case WM_COMMAND:
		for( int i = 0; i < COUNT; i++ )
			if ( (HWND)lParam == button[i] ){
				butstate = STATE( i );
        }
		break;
	
	}
	return ::DefWindowProc( hwnd, msg, wParam, lParam );
}

//
// WinMain
//

int WINAPI WinMain( HINSTANCE hinstance,
				   HINSTANCE prevInstance, 
				   PSTR cmdLine,
				   int showCmd )
{
	if( ( width == 0 ) || ( height == 0 ) ){
		width = GetSystemMetrics( SM_CXSCREEN );//Получить ширину экрана
		height = GetSystemMetrics( SM_CYSCREEN );//Получить высоту экрана
	}

	if( !d3d::InitD3D( hinstance,
		width, height, true, D3DDEVTYPE_HAL, &pDevice, button ) )
	{
		::MessageBox( 0, L"InitD3D() - FAILED", 0, 0 );
		return 0;
	}
		
	if( !Setup() )
	{
		::MessageBox( 0, L"Setup() - FAILED", 0, 0 );
		return 0;
	}

	d3d::EnterMsgLoop( Display );

	Cleanup();

	pDevice->Release();

	return 0;
}