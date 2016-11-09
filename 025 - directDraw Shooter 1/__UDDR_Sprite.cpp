//---------------------------------------------------------------------------

#pragma hdrstop
#include "__UDDR_Sprite.h"
#include <ddraw.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)

extern void msgLog(AnsiString str);

// --- DDR_SpriteBase ---------------------------------------------------------

// конструктор для инициализации bmp-файлом с диска
__fastcall DDR_SpriteBase::DDR_SpriteBase(IDirectDraw *pdd, AnsiString fileName)
    : pDD(pdd), isColorKeySet(false)
{
    HBITMAP       hbm;
    BITMAP        bm;
    DDSURFACEDESC surfDesc;

    //  try to load the bitmap as a resource, if that fails, try it as a file
    hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), fileName.c_str(), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    if( hbm == NULL )
	    hbm = (HBITMAP)LoadImage(NULL, fileName.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);

    if( hbm != NULL ) {

        // get size of the bitmap
        GetObject(hbm, sizeof(bm), &bm);

        // create a DirectDrawSurface for this bitmap
        ZeroMemory(&surfDesc, sizeof(surfDesc));
        surfDesc.dwSize         = sizeof(surfDesc);
        surfDesc.dwFlags        = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        surfDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
        surfDesc.dwWidth        = bm.bmWidth;
        surfDesc.dwHeight       = bm.bmHeight;

        surfRect.left   = 0;
        surfRect.top    = 0;
        surfRect.right  = bm.bmWidth;
        surfRect.bottom = bm.bmHeight;

        if( pDD->CreateSurface(&surfDesc, &pDDSurf, NULL) == DD_OK ) {
              DDCopyBitmap(pDDSurf, hbm, 0, 0, 0, 0);
              DeleteObject(hbm);
        }
    }
}

// конструктор для инициализации объектом HBITMAP
__fastcall DDR_SpriteBase::DDR_SpriteBase(IDirectDraw *pdd, HBITMAP *hBitmap)
    : pDD(pdd), isColorKeySet(false)
{
    BITMAP        bm;
    DDSURFACEDESC surfDesc;

    if( *hBitmap != NULL ) {

        // get size of the bitmap
        GetObject(*hBitmap, sizeof(BITMAP), &bm);

        // create a DirectDrawSurface for this bitmap
        ZeroMemory(&surfDesc, sizeof(surfDesc));
        surfDesc.dwSize         = sizeof(surfDesc);
        surfDesc.dwFlags        = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
        surfDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
        surfDesc.dwWidth        = bm.bmWidth;
        surfDesc.dwHeight       = bm.bmHeight;

        surfRect.left   = 0;
        surfRect.top    = 0;
        surfRect.right  = bm.bmWidth;
        surfRect.bottom = bm.bmHeight;

        if( pDD->CreateSurface(&surfDesc, &pDDSurf, NULL) == DD_OK ) {
              DDCopyBitmap(pDDSurf, *hBitmap, 0, 0, 0, 0);
        }
        else {
            surfDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

            if( pDD->CreateSurface(&surfDesc, &pDDSurf, NULL) == DD_OK )
                DDCopyBitmap(pDDSurf, *hBitmap, 0, 0, 0, 0);
        }
    }
}

__fastcall DDR_SpriteBase::~DDR_SpriteBase() {
    if( pDDSurf != NULL ) {
        pDDSurf->Release();
        pDDSurf = NULL;
    }
}

void __fastcall DDR_SpriteBase::init() {
}

HRESULT __fastcall DDR_SpriteBase::DDCopyBitmap(IDirectDrawSurface *pdds, HBITMAP hbm, int x, int y, int dx, int dy)
{
    HDC                 hdcImage;
    HDC                 hdc;
    BITMAP              bm;
    DDSURFACEDESC       ddsd;
    HRESULT             hr;

    if (hbm == NULL || pdds == NULL)
    	return E_FAIL;

    // make sure this surface is restored.
    pdds->Restore();

    //  select bitmap into a memoryDC so we can use it.
    hdcImage = CreateCompatibleDC(NULL);
    if( !hdcImage )
    	msgLog("createcompatible dc failed\n");
        
    SelectObject(hdcImage, hbm);

    // get size of the bitmap
    GetObject(hbm, sizeof(bm), &bm);

    // use the passed size, unless zero    
    dx = dx == 0 ? bm.bmWidth  : dx;
    dy = dy == 0 ? bm.bmHeight : dy;

    // get size of surface.
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
    pdds->GetSurfaceDesc(&ddsd);

    if( (hr = pdds->GetDC(&hdc)) == DD_OK ) {
    	StretchBlt(hdc, 0, 0, ddsd.dwWidth, ddsd.dwHeight, hdcImage, x, y, dx, dy, SRCCOPY);
	    pdds->ReleaseDC(hdc);
    }

    DeleteDC(hdcImage);

    return hr;
}

// для поверхности задаем прозрачный цвет, который не будет отображаться
void __fastcall DDR_SpriteBase::setColorKey(DDCOLORKEY *colorKey) {

    pDDSurf->SetColorKey(DDCKEY_SRCBLT, colorKey);
    isColorKeySet = true;
}


// --- DDR_Sprite -------------------------------------------------------------

__fastcall DDR_Sprite::DDR_Sprite(IDirectDraw *pdd, AnsiString fileName)
    : DDR_SpriteBase(pdd, fileName) {
}

__fastcall  DDR_Sprite::DDR_Sprite(IDirectDraw *pdd, HBITMAP *hBitmap)
    : DDR_SpriteBase(pdd, hBitmap) {
}

__fastcall DDR_Sprite::~DDR_Sprite() {
}

// use_fastblt станет false, если X или Y текущего изображения < 0, но изображение все равно выглядывает на экран
void __fastcall DDR_Sprite::Draw(IDirectDrawSurface *surfDest, RECT *rectDest, int X, int Y, int test) {

    if( surfDest != 0 ) {

        bool  use_fastblt = true;
        DWORD flags;
        RECT  destPartRect;

        RECT rectSrc;       // координаты текущей поверхности (просто размеры ее битмапа)
        rectSrc.left   = this->surfRect.left;
        rectSrc.top    = this->surfRect.top;
        rectSrc.right  = this->surfRect.right;
        rectSrc.bottom = this->surfRect.bottom;

        // если текущая поверхность Source не видна в пределах поверхности rectDest, возвращаемся
        if( X + rectSrc.left   >= rectDest->right  ||
            Y + rectSrc.top    >= rectDest->bottom ||
            X + rectSrc.right  <= rectDest->left   ||
            Y + rectSrc.bottom <= rectDest->top
          )
            return;

        // если поверхность Source видна, но выходит за пределы поверхности Dest, обрезаем размеры выводимой поверхности
        if( X + rectSrc.right  > rectDest->right )
            rectSrc.right -= X + rectSrc.right - rectDest->right;
        if( Y + rectSrc.bottom > rectDest->bottom )
            rectSrc.bottom -= Y + rectSrc.bottom - rectDest->bottom;

        // если выводимое изображение помещается на конечную поверхность не целиком, то надо вычислить её прямоугольник, который ложится на конечную поверхность - для ф-ции Blt()
        // ??? - почему учитывается только X/Y < 0, но не X/Y > maxX ???
        if( X < 0 ) {
            rectSrc.left =- X;
            X = 0;
            destPartRect.left   = X;
            destPartRect.top    = Y;
            destPartRect.right  = X + (rectSrc.right  - rectSrc.left);
            destPartRect.bottom = Y + (rectSrc.bottom - rectSrc.top );
            use_fastblt = false;
        }

        if( Y < 0 ) {
            rectSrc.top =- Y;
            Y = 0;
            destPartRect.left   = X;
            destPartRect.top    = Y;
            destPartRect.right  = X + (rectSrc.right  - rectSrc.left);
            destPartRect.bottom = Y + (rectSrc.bottom - rectSrc.top );
            use_fastblt = false;
        }

        if( use_fastblt ) {
            flags = DDBLTFAST_WAIT;

            if( isColorKeySet )
                flags |= DDBLTFAST_SRCCOLORKEY;

            surfDest->BltFast(X, Y, pDDSurf, &rectSrc, flags);
        }
        else {
            flags = DDBLT_WAIT;

            if( isColorKeySet )
                flags |= DDBLT_KEYSRC;

            surfDest->Blt(&destPartRect, pDDSurf, &rectSrc, flags, 0);
        }
    }
}

void __fastcall DDR_Sprite::init() {
}



// --- DibCollection ----------------------------------------------------------

__fastcall DDR_Collection::DDR_Collection(IDirectDraw *pdd, AnsiString fileMask, DDCOLORKEY *colorKey)
    : _FramesQty(0) {

    // читаем файлы из указанной локации по указанной маске
    while( FileExists( ExtractFileDir(Application->ExeName) + "\\" + fileMask + IntToStr(_FramesQty) + ".bmp" ) ) {

        DDR_Sprite *ptr = new DDR_Sprite(pdd, fileMask + IntToStr(_FramesQty) + ".bmp");
        ptr->setColorKey(colorKey);
        _vec.push_back(ptr);
        _FramesQty++;
    }
}

__fastcall DDR_Collection::~DDR_Collection() {
    if( _vec.size() )
        for(unsigned int i = 0; i < _vec.size(); i++)
            delete _vec[i];
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
