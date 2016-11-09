#include <vcl.h>
#include <vector.h>
#include <list.h>
#include <math.h>
#include <mmsystem.h>
#include <iostream>

#pragma hdrstop
#include "Main.h"
#include "Unit1.h"
#pragma resource "*.dfm"

#include "__UDDR_Sprite.h"
#include "UGameClasses.h"

#include "d3d9.h"
//#pragma comment (lib, "d3d9.lib")




// Global Variables
TFormMain *FormMain;

HDC DC;                              // контекст первичной и задней поверхности
HBITMAP hbDesktop;                   // current desktop image handle

int scrWidth, scrHeight,             // размер экрана (текущее разрешение)
    playerPosX, playerPosY,          // координаты игрока
    playerPosXold, playerPosYold,    // координаты игрока до просчета
    BgrWidth, BgrHeight,             // размер фона (может превышать размеры экрана)
    scrPosX, scrPosY;                // верхняя левая позиция окна в пределах фонового изображения

// Direct Draw and Surfaces
LPDIRECTDRAW         lpDD;           // DirectDraw object
LPDIRECTDRAWSURFACE  lpDDSPrimary;   // DirectDraw primary surface
LPDIRECTDRAWSURFACE  lpDDSBack;      // DirectDraw back surface
RECT                 rectPrimary;

char ptsArray[1600*900];

const int MonsterSpd = 333;          // модификатор скорости монстров
const int TankSpeed  = 10;           // шаг перемещения игрока

#define  TIMER_RATE   10             // интервал таймера просчета
#define  BulletRate   50             // ограничитель скорости генерации пуль
#undef   BulletRate
#define  BulletSpeed  50.0           // шаг перемещения пули
#define  MonsterCnt   3333           // начальное число монстров
#define  BorderDist   300            // величина зазора, при котором игрок начинает сдвигать экран, подходя к краю
#define  useList                     // использовать список вместо вектора для всех массивов
//#undef   useList
#define  showInfo                    // отображать инфо на экране
#undef   showInfo
#define  logInfo                     // выводить лог просчета и рендеринга в файл
#undef   logInfo


DDR_Sprite      *Back,
                *Sprite,
                *TankSprite, *TowerSprite,
                *BulletSprite;
DDR_Tank        *Player;
DDR_Collection  *AnimateExpl, *AnimateMnstr;

#if defined useList
    list   <DDR_Bullet    *>  bulletList;
    list   <DDR_Explosion *>  explosionList;
    list   <DDR_Monster   *>  monsterList;
    int    monsterListSize = 0;
#else
    vector <DDR_Bullet    *>  bulletVec;
    vector <DDR_Explosion *>  explosionVec;
    vector <DDR_Monster   *>  monsterVec;
#endif


// Управление процессом
bool    LMB_Pressed = false;
int     LMB_Counter = 0;
int     MouseX, MouseY;

bool    isCalculating, isDrawing;

precisionTimer precTimer;
unsigned long framesCnt;

// simple logger
void msgLog(AnsiString str) {

    FILE *f = fopen("zzz___msgLog.log", "a");

    if( f != NULL) {
        fputs((str + "\n").c_str(), f);
        fclose(f);
    }
}

void showMsg(AnsiString str, bool nextLine = true) {

    static int yPos = -10;

    if( lpDDSPrimary->GetDC(&DC) == DD_OK ) {

        HBRUSH Brush, OldBrush;

        SetBkColor(DC, RGB(0, 0, 0));
        SetTextColor(DC, RGB(100, 200, 100));

        if( nextLine )
            yPos += 20;

        TextOut(DC, 10, yPos, str.c_str(), str.Length());

        // release context
        lpDDSPrimary->ReleaseDC(DC);
    }
}
// ----------------------------------------------------------------------------

__fastcall TFormMain::TFormMain(TComponent* Owner)
        : TForm(Owner)
{
    lpDD = NULL;

    // Получаем хэндл на битмап с текущим десктопом. Если его не получить, то будет загружена другая картинка
    //hbDesktop = getDesktop(Screen->Width, Screen->Height);

    randomize();

    msgLog("----- app Started ----- \n");
}
// ----------------------------------------------------------------------------

void __fastcall TFormMain::FormDestroy(TObject *Sender)
{
    if( hbDesktop )
        DeleteObject(hbDesktop);

    if(lpDD != NULL) {

        lpDD->RestoreDisplayMode();
        lpDD->SetCooperativeLevel(Handle, DDSCL_NORMAL);

        if(lpDDSPrimary != NULL) {
            lpDDSPrimary->Release();
            lpDDSPrimary = NULL;
        }

        lpDD->Release();
        lpDD = NULL;
    }

    msgLog("----- app Closed ----- \n\n\n");    

    // вынес удаление всех созданных поверхностей и объектов в отдельный метод destroyAll(), который вызывается
    // _до_ метода Close(), т.к. при уничтожении объектов в FormDestroy() возникали странные артефакты
    // вроде того, что в векторе монстров объект с индексом 1 не существовал
}
// ----------------------------------------------------------------------------

void __fastcall TFormMain::destroyAll()
{
#if defined useList

    {
        list<DDR_Bullet *>::iterator iter = bulletList.begin(), end = bulletList.end();
        while( iter != end ) {
            if( iter != NULL )
                delete *iter;
            ++iter;
        }
    }

    {
        list<DDR_Explosion *>::iterator iter = explosionList.begin(), end = explosionList.end();
        while( iter != end ) {
            if( iter != NULL )
                delete *iter;
            ++iter;
        }
    }

    {
        list<DDR_Monster *>::iterator iter = monsterList.begin(), end = monsterList.end();
        while( iter != end ) {
            if( iter != NULL )
                delete *iter;
            ++iter;
        }
    }

#else

    unsigned int i;

    if( bulletVec.size() )
        for(i = 0; i < bulletVec.size(); i++)
            if(bulletVec[i] != NULL)
                delete bulletVec[i];

    if( explosionVec.size() )
        for(i = 0; i < explosionVec.size(); i++)
            if(explosionVec[i] != NULL)
                delete explosionVec[i];

    if( monsterVec.size() )
        for(i = 0; i < monsterVec.size(); i++)
            if(monsterVec[i] != NULL)
                delete monsterVec[i];

#endif

    delete Back;
    delete Sprite;
    delete TankSprite;
    delete TowerSprite;
    delete Player;
    delete BulletSprite;
    delete AnimateExpl;
    delete AnimateMnstr;
}

// ----------------------------------------------------------------------------
void __fastcall TFormMain::initDD()
{
//    if( init(this->Handle, 1280, 720) ) {
    if( init(this->Handle, 1600, 900) ) {

        msgLog("Direct Draw Init() - success");
        showMsg("Direct Draw Init() - OK");

        rectPrimary.left   = 0;
        rectPrimary.top    = 0;
        rectPrimary.right  = Width;
        rectPrimary.bottom = Height;

        scrWidth   = Width;
        scrHeight  = Height;
        playerPosX = scrWidth  / 2;
        playerPosY = scrHeight / 2;

        AnsiString Path = ExtractFileDir(Application->ExeName) + "\\img\\";

        // создаем необходимые поверхности
        // не забываем добавлять созданные повехности в деструктор формы
        Back = hbDesktop ? new DDR_Sprite(lpDD, &hbDesktop) : new DDR_Sprite(lpDD, (Path + "Bgr3.bmp").c_str());
        BgrWidth  = Back->getWidth();
        BgrHeight = Back->getHeight();

        // позиционируем окно в центр задника
        scrPosX = scrWidth / 2 - BgrWidth / 2;
        scrPosY = scrHeight/ 2 - BgrHeight/ 2;

        BulletSprite = new DDR_Sprite(lpDD, (Path + "Bullet.bmp"   ).c_str());
        Sprite       = new DDR_Sprite(lpDD, (Path + "pic2.bmp"     ).c_str());
        TankSprite   = new DDR_Sprite(lpDD, (Path + "TankBody.bmp" ).c_str());
        TowerSprite  = new DDR_Sprite(lpDD, (Path + "TankTower.bmp").c_str());

        msgLog("sprites created");
        showMsg("sprites creation - OK");

        // для поверхности lpDDSTmp задаем прозрачный цвет, который не будет отображаться
        DDCOLORKEY ddColorKey;
        ddColorKey.dwColorSpaceLowValue  = RGB(255, 100, 255);
        ddColorKey.dwColorSpaceHighValue = RGB(255, 100, 255);

        BulletSprite->setColorKey(&ddColorKey);
        Sprite      ->setColorKey(&ddColorKey);
        TankSprite  ->setColorKey(&ddColorKey);
        TowerSprite ->setColorKey(&ddColorKey);

        Player = new DDR_Tank(TankSprite, TowerSprite, playerPosX, playerPosY);

        AnimateExpl = new DDR_Collection(lpDD, "img\\expl\\", &ddColorKey);
        DDR_Explosion::FramesQty = AnimateExpl->getFramesQty();

        AnimateMnstr = new DDR_Collection(lpDD, "img\\monster\\", &ddColorKey);
        DDR_Monster::FramesQty = AnimateMnstr->getFramesQty();

        msgLog("animations created");
        showMsg("animations creation - OK");

        msgLog("generating monsters...");
        showMsg("generating monsters...");                

        // генерим начальных монстров
        for(int i = 0; i < MonsterCnt; i++) {
            #if defined useList
                //monsterList.push_back(new DDR_Monster(Sprite, BgrWidth, BgrHeight, monsterList.size()));
                monsterListSize++;
                monsterList.push_back(new DDR_Monster(Sprite, BgrWidth, BgrHeight, monsterListSize));
            #else
                monsterVec.push_back(new DDR_Monster(Sprite, BgrWidth, BgrHeight, i));
            #endif

            if( !((i+1) % 1000) )
                showMsg("generating monsters... " + IntToStr(i+1), false);
        }

        msgLog("monsters generation done");
        showMsg("generating monsters - OK");
        Sleep(300);

        // запускаем вечный цикл
        framesCnt = 0;
        precTimer.Start();
        FActive          = true;
        Timer1->Interval = TIMER_RATE;
        PostMessage(Handle, WM_USER, 0, 0);
        Timer1->Enabled  = true;

        msgLog("endless timer started");
        showMsg("App Started - OK");
        Sleep(300);
    }

    DeleteObject(hbDesktop);
    hbDesktop = NULL;
}
// ----------------------------------------------------------------------------

void __fastcall TFormMain::FormKeyDown(TObject *Sender, WORD &Key,
        TShiftState Shift)
{
    switch( Key ) {

        case VK_ESCAPE:
            Timer1->Enabled = false;
            FActive = false;
            precTimer.End();
            destroyAll();
            Close();
            break;

        case 37:
        case 65:
            Player->LEFT = true;
            break;

        case 38:
        case 87:
            Player->UP = true;
            break;

        case 39:
        case 68:
            Player->RIGHT = true;
            break;

        case 40:
        case 83:
            Player->DOWN = true;
            break;
    }
}
// ----------------------------------------------------------------------------

void __fastcall TFormMain::FormKeyUp(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    switch( Key ) {
        case 37:
        case 65:
            Player->LEFT = false;
        break;

        case 38:
        case 87:
            Player->UP = false;
        break;

        case 39:
        case 68:
            Player->RIGHT = false;
        break;

        case 40:
        case 83:
            Player->DOWN = false;
        break;
    }

}
// ----------------------------------------------------------------------------

void __fastcall TFormMain::FormMouseDown(TObject *Sender, TMouseButton Button,
      TShiftState Shift, int X, int Y)
{
    LMB_Pressed = true;
    LMB_Counter = 0;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::FormMouseUp(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    LMB_Pressed = false;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::FormMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
    MouseX = X;
    MouseY = Y;
}
//---------------------------------------------------------------------------

void __fastcall TFormMain::FormPaint(TObject *Sender)
{
    initDD();
}
// ----------------------------------------------------------------------------

void TFormMain::Run(TMessage &Message)
{
    msgLog("Run()");

    isCalculating = true;    

    do {
        Redraw();
        Application->ProcessMessages();
        Sleep(0);
    } while( FActive );
}
// ----------------------------------------------------------------------------

void __fastcall TFormMain::Timer1Timer(TObject *Sender)
{
    while( isDrawing )
        Application->ProcessMessages();

#if defined logInfo
    precTimer.getTime();
    int ms1 = precTimer.get_mSec();
#endif

    isCalculating = true;

    playerPosXold = Player->getX();
    playerPosYold = Player->getY();
    Player->Move();
    playerPosX = Player->getX();
    playerPosY = Player->getY();

    // Просчитываем перемещение окна вдоль заднего фона
    if( playerPosX != playerPosXold ) {

        if( playerPosX > scrWidth - BorderDist ) {

            if( -scrPosX < BgrWidth - scrWidth - 1 ) {
                // если можем сместить экран вправо, смещаем его, а игрок остается на краю зоны
                scrPosX += scrWidth - BorderDist - playerPosX;
                playerPosX = scrWidth - BorderDist;
            }
            else {
                // если экран уже смещать некуда, замораживаем его на месте, а игроку позволяем дойти до края экрана
                scrPosX = scrWidth - BgrWidth;
                if( playerPosX > scrWidth - 25 )
                    playerPosX = scrWidth - 25;
            }
        }

        if( playerPosX < BorderDist ) {

            if( scrPosX < 0 ) {
                // если можем сместить экран влево, смещаем его, а игрок остается на краю зоны
                scrPosX += BorderDist - playerPosX;
                playerPosX = BorderDist;
            }
            else {
                // если экран уже смещать некуда, замораживаем его на месте, а игроку позволяем дойти до края экрана
                scrPosX = 0;
                if( playerPosX < 0 )
                    playerPosX = 0;
            }
        }

        Player->setX(playerPosX);
    }

    if( playerPosY != playerPosYold ) {

        if( playerPosY > scrHeight - BorderDist ) {

            if( -scrPosY < BgrHeight - scrHeight - 1 ) {
                // если можем сместить экран вниз, смещаем его, а игрок остается на краю зоны
                scrPosY += scrHeight - BorderDist - playerPosY;
                playerPosY = scrHeight - BorderDist;
            }
            else {
                // если экран уже смещать некуда, замораживаем его на месте, а игроку позволяем дойти до края экрана
                scrPosY = scrHeight - BgrHeight;
                if( playerPosY > scrHeight - 25 )
                    playerPosY = scrHeight - 25;
            }
        }

        if( playerPosY < BorderDist ) {

            if( scrPosY < 0 ) {
                // если можем сместить экран влево, смещаем его, а игрок остается на краю зоны
                scrPosY += BorderDist - playerPosY;
                playerPosY = BorderDist;
            }
            else {
                // если экран уже смещать некуда, замораживаем его на месте, а игроку позволяем дойти до края экрана
                scrPosY = 0;
                if( playerPosY < 0 )
                    playerPosY = 0;
            }
        }

        Player->setY(playerPosY);
    }



#if defined useList

    // список с пулями
    {
        int Hit;
        list<DDR_Bullet *>::iterator iter = bulletList.begin(), end = bulletList.end();

        while( iter != end ) {

            Hit = (*iter)->Move(monsterList);

            // дошли до точки, куда стреляли
            if( Hit ) {

                if( Hit == 1 ) {
                    explosionList.push_back(new DDR_Explosion(AnimateExpl, (*iter)->getX(), (*iter)->getY()));

                    delete *iter;
                    iter = bulletList.erase( iter );
                    continue;
                }
            }

            ++iter;
        }
    }

    // список со взрывами
    {
        list<DDR_Explosion *>::iterator iter = explosionList.begin(), end = explosionList.end();

        while( iter != end ) {

            if( !(*iter)->isActive() ) {
                delete *iter;
                iter = explosionList.erase( iter );
                continue;
            }

            ++iter;
        }
    }

    // список с монстрами
    {
        list<DDR_Monster *>::iterator iter = monsterList.begin(), end = monsterList.end();

        while( iter != end ) {

            if( (*iter)->isAlive() ) {
                (*iter)->Move(playerPosX - scrPosX, playerPosY - scrPosY);
            }
            else {

                delete *iter;
                iter = monsterList.erase( iter );
                monsterListSize--;

                for(int i = 0; i < random(3); i++) {
                    monsterListSize++;
                    monsterList.push_back(new DDR_Monster(Sprite, BgrWidth, BgrHeight, monsterListSize));
                }

                continue;
            }

            ++iter;
        }
    }

#else

    // вектор с пулями
    int Hit;
    for(unsigned int i = 0; i < bulletVec.size(); i++) {
        Hit = bulletVec[i]->Move(monsterVec);

        // дошли до точки, куда стреляли
        if( Hit ) {

            if( Hit == 1) {
                explosionVec.push_back(new DDR_Explosion(AnimateExpl, bulletVec[i]->getX(), bulletVec[i]->getY()));

                DDR_Bullet *ptr = bulletVec[i];
                bulletVec.erase(&bulletVec[i]);
                delete ptr;
            }
        }
    }

    // вектор со взрывами
    for(unsigned int i = 0; i < explosionVec.size(); i++) {

        if( !explosionVec[i]->isActive() ) {
            DDR_Explosion *ptr = explosionVec[i];
            explosionVec.erase(&explosionVec[i]);
            delete ptr;
        }
    }


    // вектор с монстрами
    for(unsigned int i = 0; i < monsterVec.size(); i++) {

        if( monsterVec[i]->isAlive() ) {
            monsterVec[i]->Move(playerPosX, playerPosY);
        }
        else {

            DDR_Monster *ptr = monsterVec[i];
            monsterVec.erase(&monsterVec[i]);
            delete ptr;

            //DDR_Sprite *Monster = NULL;
            //monsterVec.push_back(new DDR_Monster(Monster, AnimateMnstr, maxX, maxY, monsterVec.size()));
        }
    }

#endif



    if( LMB_Pressed ) {

        #if defined BulletRate
        LMB_Counter++;

        if( LMB_Counter * TIMER_RATE > BulletRate )
        #endif
        {
            #if defined useList
                bulletList.push_back(new DDR_Bullet(
            #else
                bulletVec.push_back(new DDR_Bullet(
            #endif
                    BulletSprite, playerPosX - scrPosX, playerPosY - scrPosY, MouseX - scrPosX + random(47) - 23, MouseY - scrPosY + random(47) - 23, BulletSpeed));

            #if defined BulletRate
            LMB_Counter = 0;
            #endif
        }
    }

    isCalculating = false;

#if defined logInfo
    precTimer.getTime();
    int ms2 = precTimer.get_mSec();
    msgLog("calc time (ms): " + IntToStr(ms2 - ms1));
#endif
}
// ----------------------------------------------------------------------------

void __fastcall TFormMain::Redraw()
{
    while( isCalculating )
        Application->ProcessMessages();

#if defined logInfo
    precTimer.getTime();
    int ms1 = precTimer.get_mSec();
#endif

    framesCnt++;

    isDrawing = true;

    // выводим чистый задний фон
    // rectPrimary - содержит размеры экрана для сравнения на выход за пределы
    Back->Draw(lpDDSBack, &rectPrimary, scrPosX, scrPosY, 123);

    // на фон начинаем выводить всю нашу сцену
    Player->Show(lpDDSBack, &rectPrimary);



#if defined useList

{
    list<DDR_Monster *>::iterator iter = monsterList.begin(), end = monsterList.end();
    while( iter != end ) {
        if( (*iter)->isAlive() ) {
            //(*iter)->Draw(lpDDSBack, &rectPrimary, scrPosX+3000, scrPosY+3000);
            (*iter)->Draw(lpDDSBack, &rectPrimary, scrPosX, scrPosY);
        }
        ++iter;
    }
}

{
    list<DDR_Bullet *>::iterator iter = bulletList.begin(), end = bulletList.end();
    while( iter != end ) {
        (*iter)->Draw(lpDDSBack, &rectPrimary, scrPosX, scrPosY);
        ++iter;
    }
}

{
    list<DDR_Explosion *>::iterator iter = explosionList.begin(), end = explosionList.end();
    while( iter != end ) {
        if( (*iter)->isActive() )
            (*iter)->Draw(lpDDSBack, &rectPrimary, scrPosX, scrPosY);
        ++iter;
    }
}

#else

    for(unsigned int i = 0; i < bulletVec.size(); i++)
        bulletVec[i]->Draw(lpDDSBack, &rectPrimary, scrPosX, scrPosY);

    for(unsigned int i = 0; i < explosionVec.size(); i++)
        if( explosionVec[i]->isActive() )
            explosionVec[i]->Draw(lpDDSBack, &rectPrimary, scrPosX, scrPosY);

    for(unsigned int i = 0; i < monsterVec.size(); i++)
        if( monsterVec[i]->isAlive() )
            monsterVec[i]->Draw(lpDDSBack, &rectPrimary, scrPosX, scrPosY);

#endif



#if defined showInfo
    // Don't step through code that has lock on it!
    // Getting a DC may put a lock on video memory.
    if( lpDDSBack->GetDC(&DC) == DD_OK ) {

        HBRUSH Brush, OldBrush;

        Brush = CreateSolidBrush(RGB(10, 10, 10));
        OldBrush = (HBRUSH)SelectObject(DC, Brush);

        //Rectangle(DC, playerPosX, playerPosY, playerPosX+33, playerPosY+33);
        Rectangle(DC, scrPosX + 100, scrPosY + 200, scrPosX + 100+10, scrPosY + 200+10);

        SelectObject(DC, OldBrush);
        DeleteObject(Brush);


        SetBkColor(DC, RGB(0, 0, 0));
        SetTextColor(DC, RGB(100, 200, 100));

#if defined useList
        AnsiString str = " bulletsList.size = " + IntToStr(bulletList.size()) + " ";
#else
        AnsiString str = " bulletsVec.size = " + IntToStr(bulletVec.size()) + " ";
#endif

        TextOut(DC, 10, 10, str.c_str(), str.Length());

#if defined useList
        str = " monsterList.size = " + IntToStr(monsterListSize) + " ";
#else
        str = " monsterVec.size = " + IntToStr(monsterVec.size()) + " ";
#endif

        TextOut(DC, 10, 30, str.c_str(), str.Length());

        precTimer.getTime();
        float fSec = 10 * precTimer.get_Sec();
        int iSec = (int)fSec;
        str = "Time elapsed: " + FloatToStr(iSec/(float)10);
        TextOut(DC, 10, 50, str.c_str(), str.Length());
        str = "Frame rate: " + IntToStr(int(10 * framesCnt / fSec));
        TextOut(DC, 10, 70, str.c_str(), str.Length());



        // release context
        lpDDSBack->ReleaseDC(DC);
    }
#endif



    while( true ) {

        HRESULT ddrval = lpDDSPrimary->Flip(NULL, 0);

        if( ddrval == DD_OK )
            break;

        if( ddrval == DDERR_SURFACELOST ) {

            ddrval = lpDDSPrimary->Restore();
            if( ddrval != DD_OK )
                break;
        }

        if( ddrval != DDERR_WASSTILLDRAWING ) {
            FActive = False;
            destroyAll();
            Close();
        }
    }

    isDrawing = false;

#if defined logInfo
    precTimer.getTime();
    int ms2 = precTimer.get_mSec();
    msgLog("render time (ms): " + IntToStr(ms2 - ms1));
#endif
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
//---------------------------------------------------------------------------


