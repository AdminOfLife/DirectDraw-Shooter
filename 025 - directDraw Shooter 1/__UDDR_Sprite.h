//---------------------------------------------------------------------------

#ifndef UDDR_SpriteH
#define UDDR_SpriteH

#include <vcl.h>
#include <ddraw.h>
#include <vector.h>

// ----------------------------------------------------------------------------
// --- ����� ������� ��� ������ �� ��������� � ������� Direct Draw Surface ----
// ----------------------------------------------------------------------------

// '����������' ���� ������� - RGB(255, 100, 255) - ������� ����� ����� �� ��������� �� �������� �����������
#define TransparentColor 231

extern int scrPosX;
extern int scrPosY;

// ----------------------------------------------------------------------------
// -------------- ������� ����� ��� �������� � ������� ���� -------------------
// ----------------------------------------------------------------------------

class DDR_SpriteBase {

 public:
    __fastcall  DDR_SpriteBase(IDirectDraw *, AnsiString);  // ������������� bmp-������
    __fastcall  DDR_SpriteBase(IDirectDraw *, HBITMAP *);   // ������������� �������� HBITMAP
    __fastcall ~DDR_SpriteBase();

    // ��� ����������� ������ ���������� ����, ������� �� ����� ������������
    void __fastcall setColorKey(DDCOLORKEY *);

 private:
    // ������ ����� ����������� 
    virtual void __fastcall Draw(IDirectDrawSurface *, RECT *, int = 0, int = 0, int = 0) = 0;

    void    __fastcall init();
    HRESULT __fastcall DDCopyBitmap(IDirectDrawSurface *pdds, HBITMAP hbm, int, int, int, int);

 protected:
    IDirectDraw          *pDD;
    IDirectDrawSurface   *pDDSurf;
    RECT                  surfRect;
    bool                  isColorKeySet;
};



// ----------------------------------------------------------------------------
// --- ����� ��� ������� ------------------------------------------------------
// ----------------------------------------------------------------------------

class DDR_Sprite : public DDR_SpriteBase {
 public:
    __fastcall  DDR_Sprite(IDirectDraw *, AnsiString);
    __fastcall  DDR_Sprite(IDirectDraw *, HBITMAP *);   // ������������� �������� HBITMAP
    __fastcall ~DDR_Sprite();

  virtual void __fastcall Draw(IDirectDrawSurface *, RECT *, int = 0, int = 0, int = 0);
    inline int __fastcall getWidth();
    inline int __fastcall getHeight();

    inline IDirectDrawSurface* getSurf() {
        return pDDSurf;
    }

 private:
    void __fastcall init();
};

int __fastcall DDR_Sprite::getWidth() {
    return surfRect.right;
}

int __fastcall DDR_Sprite::getHeight() {
    return surfRect.bottom;
}



// ----------------------------------------------------------------------------
// --- ����� ��� ��������� ������ ---------------------------------------------
// ----------------------------------------------------------------------------

class DDR_Collection {

 typedef unsigned int uint;

 public:

    enum AnimationStyle { FORWARD = 0, BACKWARD, FORWARD_ONCE, BACKWARD_ONCE, THERE_AND_BACK };

    __fastcall  DDR_Collection(IDirectDraw *, AnsiString, DDCOLORKEY *);
    __fastcall ~DDR_Collection();

    // ������������ ����� � �������� ������� �� ��������� ��� �� ��������� �����������
    inline void __fastcall Draw(IDirectDrawSurface *, RECT *, int, int, int);
    // �������� ����� ����������� �������
    inline uint __fastcall getFramesQty();

 protected:
    vector<DDR_Sprite *> _vec;           // ������ � ��������
    unsigned short       _FramesQty;     // ����� ����� ����������� �������
};

// ���������� ������ ���������� � ���� �� �����!
// ������������ ����� � �������� ������� �� ��������� ��� �� ��������� �����������
inline void __fastcall DDR_Collection::Draw(IDirectDrawSurface *surfDest, RECT *rect, int frameNo, int X, int Y) {
    if( frameNo >= 0 && frameNo < _FramesQty )
        _vec[frameNo]->Draw(surfDest, rect, X, Y);
}

inline unsigned int __fastcall DDR_Collection::getFramesQty() {
    return _FramesQty;
}

//---------------------------------------------------------------------------
#endif
