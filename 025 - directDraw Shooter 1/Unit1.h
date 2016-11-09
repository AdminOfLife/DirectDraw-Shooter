//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H

#include <ddraw.h>

extern LPDIRECTDRAW         lpDD;           // DirectDraw object
extern LPDIRECTDRAWSURFACE  lpDDSPrimary;   // DirectDraw primary surface
extern LPDIRECTDRAWSURFACE  lpDDSBack;      // DirectDraw back surface



// инициализация DirectDraw
bool init(HWND Handle, int Width, int Height)
{
    HRESULT         ddrval;
    DDSURFACEDESC   ddsd;
    DDSCAPS         ddscaps;
    HDC             DC;
    char            buf[256];

    ddrval = DirectDrawCreate(NULL, &lpDD, NULL);

    if(ddrval == DD_OK) {

      // Get exclusive mode
      ddrval = lpDD->SetCooperativeLevel(Handle, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);

      if(ddrval == DD_OK) {

            ddrval = lpDD->SetDisplayMode(Width, Height, 32);

            if(ddrval == DD_OK) {

                // Create the primary surface with 1 back buffer
                ddsd.dwSize = sizeof(ddsd);
                ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
                ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
                                      DDSCAPS_FLIP           |
                                      DDSCAPS_COMPLEX;
                ddsd.dwBackBufferCount = 1;
                ddrval = lpDD->CreateSurface(&ddsd, &lpDDSPrimary, NULL);

                if(ddrval == DD_OK) {

                    // Get a pointer to the back buffer
                    ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
                    ddrval = lpDDSPrimary->GetAttachedSurface(&ddscaps, &lpDDSBack);

                    if(ddrval == DD_OK)
                        return true;
                }
            }
        }
    }

    return false;
}

LPDIRECTDRAWSURFACE CreateSurface(LPDIRECTDRAW lpDD, DWORD w, DWORD h) {

    DDSURFACEDESC       desc;
    LPDIRECTDRAWSURFACE surf;

    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags  = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    desc.dwWidth  = w;
    desc.dwHeight = h;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN |   DDSCAPS_VIDEOMEMORY;

    if( lpDD->CreateSurface(&desc, &surf, 0) == DD_OK )
        return surf;

    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    if( lpDD->CreateSurface(&desc, &surf, 0) == DD_OK )
        return surf;

    return 0;
}

// загрузка битмапа из памяти в поверхность
extern "C" HRESULT DDCopyBitmap(IDirectDrawSurface *pdds, HBITMAP hbm, int x, int y, int dx, int dy)
{
    HDC                 hdcImage;
    HDC                 hdc;
    BITMAP              bm;
    DDSURFACEDESC       ddsd;
    HRESULT             hr;

    if( hbm == NULL || pdds == NULL )
    	return E_FAIL;

    // make sure this surface is restored.
    pdds->Restore();

    //  select bitmap into a memoryDC so we can use it.
    hdcImage = CreateCompatibleDC(NULL);
    if( !hdcImage )
    	OutputDebugString("createcompatible dc failed\n");
        
    SelectObject(hdcImage, hbm);

    // get size of the bitmap
    GetObject(hbm, sizeof(bm), &bm);    // get size of bitmap
    dx = dx == 0 ? bm.bmWidth  : dx;    // use the passed size, unless zero
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

// другой вариант загрузки битмапа из памяти в поверхность
HRESULT DDCopyBitmap(IDirectDrawSurface *pdds, HBITMAP hbm, int dx, int dy)
{
    HDC hdcImage;
    HDC hdc;
    HRESULT hr;
    HBITMAP hbmOld;

    hdcImage = CreateCompatibleDC(NULL);
    hbmOld = (HBITMAP)SelectObject(hdcImage, hbm);

    if( (hr = pdds->GetDC(&hdc)) == DD_OK ) {
    
        BitBlt(hdc, 0, 0, dx, dy, hdcImage, 0, 0, SRCCOPY);
        pdds->ReleaseDC(hdc);
    }

    SelectObject(hdcImage, hbmOld);
    DeleteDC(hdcImage);

    return hr;
}

// загрузка битмапа из файла в существующую поверхность
extern "C" IDirectDrawSurface * DDLoadBitmap(IDirectDraw *pdd, LPCSTR szBitmap, int dx, int dy)
{
    HBITMAP             hbm;
    BITMAP              bm;
    DDSURFACEDESC       ddsd;
    IDirectDrawSurface *pdds;

    //  try to load the bitmap as a resource, if that fails, try it as a file
    hbm = (HBITMAP)LoadImage(GetModuleHandle(NULL), szBitmap, IMAGE_BITMAP, dx, dy, LR_CREATEDIBSECTION);

    if (hbm == NULL)
	hbm = (HBITMAP)LoadImage(NULL, szBitmap, IMAGE_BITMAP, dx, dy, LR_LOADFROMFILE|LR_CREATEDIBSECTION);

    if (hbm == NULL)
	return NULL;

    // get size of the bitmap
    GetObject(hbm, sizeof(bm), &bm);      // get size of bitmap

    // create a DirectDrawSurface for this bitmap
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT |DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = bm.bmWidth;
    ddsd.dwHeight = bm.bmHeight;

    if (pdd->CreateSurface(&ddsd, &pdds, NULL) != DD_OK)
	return NULL;

    DDCopyBitmap(pdds, hbm, 0, 0, 0, 0);

    DeleteObject(hbm);

    return pdds;
}

// копирование одной поверхности на другую по указанным координатам
bool BltSurface(LPDIRECTDRAWSURFACE surfDest, LPDIRECTDRAWSURFACE surfSrc, int x, int y, bool srcColorKey)
{
    if( surfDest != 0 && surfSrc != 0 ) {

        bool use_fastblt = true;

        DDSURFACEDESC surfdescDest, surfdescSrc;

        ZeroMemory(&surfdescDest, sizeof(surfdescDest));
        ZeroMemory(&surfdescSrc,  sizeof(surfdescSrc) );

        surfdescDest.dwSize = sizeof(surfdescDest);
        surfdescSrc.dwSize  = sizeof(surfdescSrc);

        surfDest->GetSurfaceDesc(&surfdescDest);
        surfSrc ->GetSurfaceDesc(&surfdescSrc );

        RECT rectDest, rectSrc;
        rectDest.left   = 0;
        rectDest.top    = 0;
        rectDest.right  = surfdescDest.dwWidth;
        rectDest.bottom = surfdescDest.dwHeight;

        rectSrc.left   = 0;
        rectSrc.top    = 0;
        rectSrc.right  = surfdescSrc.dwWidth;
        rectSrc.bottom = surfdescSrc.dwHeight;

        // если поверхность Source не видна в пределах поверхности Dest, возвращаемся 
        if( x + rectSrc.left   >= rectDest.right  ||
            y + rectSrc.top    >= rectDest.bottom ||
            x + rectSrc.right  <= rectDest.left   ||
            y + rectSrc.bottom <= rectDest.top
          )
            return false;

        // если поверхность Source видна, но выходит за пределы поверхности Dest, обрезаем размеры выводимой поверхности
        if( x + rectSrc.right  > rectDest.right  )
            rectSrc.right -= x + rectSrc.right - rectDest.right;
        if( y + rectSrc.bottom > rectDest.bottom )
            rectSrc.bottom -= y + rectSrc.bottom - rectDest.bottom;

        RECT dr;
        if( x < 0 ) {
            rectSrc.left =- x;
            x = 0;
            dr.left   = x;
            dr.top    = y;
            dr.right  = x + (rectSrc.right  - rectSrc.left);
            dr.bottom = y + (rectSrc.bottom - rectSrc.top );
            use_fastblt = false;
        }

        if( y < 0 ) {
            rectSrc.top =- y;
            y = 0;
            dr.left   = x;
            dr.top    = y;
            dr.right  = x + (rectSrc.right  - rectSrc.left);
            dr.bottom = y + (rectSrc.bottom - rectSrc.top );
            use_fastblt = false;
        }

        DWORD flags;

        if( use_fastblt ) {
            flags = DDBLTFAST_WAIT;

            if( srcColorKey )
                flags |= DDBLTFAST_SRCCOLORKEY;

            surfDest->BltFast(x, y, surfSrc, &rectSrc, flags);

        }
        else {
            flags = DDBLT_WAIT;

            if( srcColorKey )
                flags |= DDBLT_KEYSRC;

            surfDest->Blt(&dr, surfSrc, &rectSrc, flags, 0);
        }

        return true;
    }

    return false;
}

// очищаем поверхность, заливая ее сплошным цветом
bool ClearSurface(LPDIRECTDRAWSURFACE Surface, DWORD clr, RECT *rect)
{
    if( Surface == 0 )
        return false;

    DDBLTFX bltfx;
    ZeroMemory(&bltfx, sizeof(bltfx));
    bltfx.dwSize = sizeof(bltfx);
    bltfx.dwFillColor = clr;

    HRESULT res;
    res = Surface->Blt(rect, 0, 0, DDBLT_COLORFILL | DDBLT_WAIT, &bltfx);

    return res == DD_OK;
}

// сохраняем битмап из памяти в файл на диске
BOOL StoreBitmapFile(LPCTSTR lpszFileName, HBITMAP HBM)
{
  BITMAP BM;  
  BITMAPFILEHEADER BFH;
  LPBITMAPINFO BIP;  
  HDC DC;      
  LPBYTE Buf;  
  DWORD ColorSize, DataSize;
  WORD BitCount;  
  HANDLE FP;  
  DWORD dwTemp;
 
  GetObject(HBM, sizeof(BITMAP), (LPSTR)&BM);
 
  BitCount = (WORD)BM.bmPlanes * BM.bmBitsPixel;
 
  if(BitCount > 8)
    ColorSize = 0;  
  else
    ColorSize = sizeof(RGBQUAD) * (1 << BitCount);
 
  DataSize = (BM.bmWidth * BitCount + 15)/8 * BM.bmHeight;
 
  BIP=(LPBITMAPINFO)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(BITMAPINFOHEADER)+ColorSize);
  
  BIP->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  BIP->bmiHeader.biWidth = BM.bmWidth;
  BIP->bmiHeader.biHeight = BM.bmHeight;
  BIP->bmiHeader.biPlanes = 1;
  BIP->bmiHeader.biBitCount = BitCount;
  BIP->bmiHeader.biCompression = 0;
  BIP->bmiHeader.biSizeImage = DataSize;
  BIP->bmiHeader.biXPelsPerMeter = 0;
  BIP->bmiHeader.biYPelsPerMeter = 0;
 
  if (ColorSize)
    BIP->bmiHeader.biClrUsed = (1<<BitCount);
  else
    BIP->bmiHeader.biClrUsed = 0;
 
  BIP->bmiHeader.biClrImportant = 0;
 
  BFH.bfType = 0x4d42;      
  BFH.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER) + ColorSize;
  BFH.bfReserved1 = 0;
  BFH.bfReserved2 = 0;
  BFH.bfSize = BFH.bfOffBits + DataSize;            
  
  Buf = (LPBYTE)GlobalAlloc(GMEM_FIXED, DataSize);
 
 
  DC = GetDC(0);
  GetDIBits(DC, HBM, 0,(WORD)BM.bmHeight, Buf, BIP, DIB_RGB_COLORS);    
  ReleaseDC(0, DC);
 
  FP = CreateFile(lpszFileName,GENERIC_READ | GENERIC_WRITE, 0, NULL,
            CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);      
  WriteFile(FP,&BFH,sizeof(BITMAPFILEHEADER),&dwTemp,NULL);        
  WriteFile(FP,BIP,sizeof(BITMAPINFOHEADER) + BIP->bmiHeader.biClrUsed * sizeof(RGBQUAD),&dwTemp,NULL);            
  WriteFile(FP,Buf,DataSize,&dwTemp,NULL);
 
  CloseHandle(FP);      
  GlobalFree((HGLOBAL)Buf);
  HeapFree(GetProcessHeap(),0,(LPVOID)BIP);
 
  return(true);
}

// получаем битмап в памяти, содержащий текущий десктоп
HBITMAP getDesktop(int width, int height)
{
    // get the desktop device context
    HDC hdc = GetDC(NULL);
    // create a device context to use yourself
    HDC hDest = CreateCompatibleDC(hdc);
    
    // create a bitmap
    HBITMAP hbDesktop = CreateCompatibleBitmap(hdc, width, height);
    // use the previously created device context with the bitmap
    SelectObject(hDest, hbDesktop);

    // copy from the desktop device context to the bitmap device context
    // call this once per 'frame'
    BitBlt(hDest, 0,0, width, height, hdc, 0, 0, SRCCOPY);

    // можем сохранить полученный битмап в файл
    //StoreBitmapFile("C:\\_maxx\\__asd2.bmp", hbDesktop);

    // after the recording is done, release the desktop context you got..
    ReleaseDC(NULL, hdc);
    // ..and delete the context you created
    DeleteDC(hDest);

    return hbDesktop;
}

// High Precision Timer
class precisionTimer {
 public:
    precisionTimer() {
        running      = false;
		milliseconds = 0.0;
		seconds      = 0.0;
		start_t      = 0;
		end_t        = 0;
    }

    inline void Start() {
        if(running)
            return;

        running = true;
        start_t = timeGetTime();
    }

    inline void End() {
        if( !running )
            return;

        running = false;
        end_t   = timeGetTime();
        milliseconds = end_t - start_t;
        seconds      = milliseconds / (float)1000;
    }

    float get_Sec() {
        return seconds;
    }

    inline float get_mSec() {
        return milliseconds;
    }

    inline void getTime() {
        end_t        = timeGetTime();
        milliseconds = end_t - start_t;
        seconds      = milliseconds / (float)1000;
    }

	private:
        unsigned long   start_t;
		unsigned long   end_t;
        float           milliseconds;
        float           seconds;
		bool            running;
};

//---------------------------------------------------------------------------
#endif
