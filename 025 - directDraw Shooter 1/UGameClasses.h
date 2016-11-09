//---------------------------------------------------------------------------

#ifndef UGameClassesH
#define UGameClassesH

#include <math.h>
#include <list.h>
#include "__UDDR_Sprite.h"

// extern global variables from main.cpp
extern const int MonsterSpd;
extern const int TankSpeed;



// --- Monster Class ----------------------------------------------------------
class DDR_Monster {

 #define animationSpeed 20

 friend class DDR_Bullet;

 public:
    // генерим монстра в заданной позиции
    DDR_Monster(DDR_Sprite *sprite, int x, int y)
        : Sprite(sprite), _X(x), _Y(y), _Alive(true) {
    }
    // генерим монстра в случайной позиции за пределами кадра
    DDR_Monster(DDR_Sprite *sprite, int maxX, int maxY, int Qty)
        : Sprite(sprite), _Alive(true) {

        if( random(2) ) {
            _X = random(maxX + 100) - 50;
            _Y = random(2) ? -50 : maxY + 50;
        }
        else {
            _X = random(2) ? -50 : maxX + 50;
            _Y = random(maxY + 100) - 50;
        }

        // добавляем скорости монстру в зависимости от их общего кол-ва
        _Speed = double(random(MonsterSpd)+20 + double(Qty/50)) / 100;
    }

    inline void __fastcall Draw(IDirectDrawSurface *surfDest, RECT *rectDest, const int &scrPosX, const int &scrPosY) {
        // отрисовываем картинку, переводя координаты из внутриэкранных в общие 
        Sprite->Draw(surfDest, rectDest, scrPosX + _X, scrPosY + _Y);
    }

    inline bool __fastcall isAlive() {
        return _Alive;
    }

    inline int __fastcall getX() {
        return _X;
    }

    inline int __fastcall getY() {
        return _Y;
    }

    void __fastcall Move(double x, double y) {

        double dX = x - _X;
        double dY = y - _Y;
        double Speed_Divided_By_Dist = _Speed / sqrt(dX*dX + dY*dY);

        dX = 0.1 * random(20) * Speed_Divided_By_Dist * dX;
        dY = 0.1 * random(10) * Speed_Divided_By_Dist * dY;

        _X += dX;
        _Y += dY;
    }

 private:
    DDR_Sprite *Sprite;
    double     _X, _Y, _Speed;
    bool       _Alive;

 public:
    static unsigned int FramesQty;
};

unsigned int DDR_Monster::FramesQty = 0;



// --- Player Class -----------------------------------------------------------
class DDR_Tank {
 public:
    DDR_Tank(DDR_Sprite* spr, DDR_Sprite* tower, int X, int Y)
        : TankSprite(spr), TowerSprite(tower), _X(X), _Y(Y),
         _LEFT(false), _RIGHT(false), _UP(false), _DOWN(false) {
    }
   ~DDR_Tank() {
        delete TankSprite;
        delete TowerSprite;
    }

    void __fastcall Show(IDirectDrawSurface *surfDest, RECT *rectDest) {
        TankSprite ->Draw(surfDest, rectDest, _X, _Y);
        TowerSprite->Draw(surfDest, rectDest, _X, _Y);
    }

    void __fastcall Move() {

        Step = TankSpeed;

        // в случае нажатия двух клавиш уменьшаем шаг в sqrt(2) раз, чтобы компенсировать сложение двух векторов движения
        if( (_UP && _LEFT) || (_UP && _RIGHT) || (_DOWN && _LEFT) || (_DOWN && _RIGHT) )
            Step /= 1.414214;

        if( DOWN  ) _Y += Step;
        if( LEFT  ) _X -= Step;
        if( UP    ) _Y -= Step;
        if( RIGHT ) _X += Step;
    }

    // по событию нажатия/отпускания клавиши выставляем эти свойства в true/false
    __property bool LEFT  = { read = _LEFT,  write = _LEFT  };
    __property bool RIGHT = { read = _RIGHT, write = _RIGHT };
    __property bool UP    = { read = _UP,    write = _UP    };
    __property bool DOWN  = { read = _DOWN,  write = _DOWN  };

    inline int __fastcall getX() {
        return _X;
    }

    inline int __fastcall getY() {
        return _Y;
    }

    inline void __fastcall setX(int x) {
        _X = x;
    }

    inline void __fastcall setY(int y) {
        _Y = y;
    }

 private:
    DDR_Sprite *TankSprite, *TowerSprite;
    double    _X, _Y, Step;
    bool      _LEFT, _RIGHT, _UP, _DOWN;
};



// --- Bullet Class -----------------------------------------------------------
class DDR_Bullet {
 public:
    DDR_Bullet(DDR_Sprite* spr, int x0, int y0, int X, int Y, float speed = 1.0)
        : Sprite(spr), _x(x0), _y(y0), _X(X), _Y(Y), Speed(speed), explosionCounter(0), MonsterIsHit(false) {

        // вычислим dX и dY:
/*
        double Dist = sqrt((_x - X)*(_x - X) + (_y - Y)*(_y - Y));
        dX = Speed * (X - _x) / Dist;
        dY = Speed * (Y - _y) / Dist;
*/
        // немного ускоряем вычисление
        dX = X - _x;
        dY = Y - _y;
        double Speed_Divided_By_Dist = Speed / sqrt(dX*dX + dY*dY);

        dX = Speed_Divided_By_Dist * dX;
        dY = Speed_Divided_By_Dist * dY;

        Collision = (_X < _x);
    }

   ~DDR_Bullet() {
        // спрайт не удаляем, т.к. он реюзабле, его удалит основная программа
        // delete Sprite;
    }

    void Draw(IDirectDrawSurface *surfDest, RECT *rectDest, const int &scrPosX, const int &scrPosY) {
    
        Sprite->Draw(surfDest, rectDest, scrPosX + _x, scrPosY + _y);

        if( explosionCounter == 1 ) {
/*
            Graphics::TBitmap *b = new Graphics::TBitmap();
            b->Width  = 24;
            b->Height = 24;

            b->Canvas->Brush->Color = TColor(RGB(255, 100, 255));
            b->Canvas->FillRect(Rect(0, 0, 24, 24));

            for(int i = 0; i < 5 + random(5); i++)
                b->Canvas->Pixels[random(24)][random(24)] = clBlack;

            if( !MonsterIsHit ) {
                b->Canvas->Brush->Color = clGray;
                b->Canvas->Ellipse(Rect(9, 9, 15, 15));
                b->Canvas->Brush->Color = clLtGray;
                b->Canvas->Ellipse(Rect(10, 10, 14, 14));
            }

            // оставим след взрыва на главной копии фона
            DibSprite s(Form1->Canvas->Handle, b);
            s.drawSprite(Back, _x, _y, true);
*/            
        }
    }

    // просчитываем движение пули, столкновение ее с монстром или конец траектории
    // возвращаем ноль, если столкновения не происходит, или счетчик анимации взрыва, если столкновение произошло
    int Move(vector<DDR_Monster *> &vecMon) {

        if( !explosionCounter ) {

            for(unsigned int i = 0; i < vecMon.size(); i++) {

                if( commonSectionCircle(_x, _y, _x + dX, _y + dY, vecMon[i]->getX() + 24, vecMon[i]->getY() + 24, 24) ) {
                    MonsterIsHit = true;
                    vecMon[i]->_Alive = false;
                    _X = vecMon[i]->getX()+12;
                    _Y = vecMon[i]->getY()+12;
                    return ++explosionCounter;
                }
            }
        }

        _x += dX;
        _y += dY;

        // ??? нужно проверять еще и по игреку, а то по вертикали пули уходят в вечность
        // проверяем, достигли ли цели
        if( (_X < _x) != Collision || explosionCounter ) {
            _x = _X;
            _y = _Y;
            dX = dY = 0.0;
            return ++explosionCounter;
        }

        return 0;
    }

    int Move(list<DDR_Monster *> &vecList) {

        if( !explosionCounter ) {

            list<DDR_Monster *>::iterator
                iter = vecList.begin(), end = vecList.end();

            while(iter != end ) {

                if( commonSectionCircle(_x, _y, _x + dX, _y + dY, (*iter)->getX() + 24, (*iter)->getY() + 24, 24) ) {
                    MonsterIsHit = true;
                    (*iter)->_Alive = false;
                    _X = (*iter)->getX()+12;
                    _Y = (*iter)->getY()+12;
                    return ++explosionCounter;
                }

                ++iter;                
            }
        }

        _x += dX;
        _y += dY;

        // ??? нужно проверять еще и по игреку, а то по вертикали пули уходят в вечность
        // проверяем, достигли ли цели
        if( (_X < _x) != Collision || explosionCounter ) {
            _x = _X;
            _y = _Y;
            dX = dY = 0.0;
            return ++explosionCounter;
        }

        return 0;
    }

    inline int getX() { return _X; }
    inline int getY() { return _Y; }

    // заменяем спрайт
    void setSprite(DDR_Sprite *spr) {
        Sprite = spr;
    }

 private:

    // пересечение отрезка с окружностью ( http://www.cyberforum.ru/cpp-beginners/thread853799.html )
    bool __fastcall commonSectionCircle (double x1, double y1, double x2, double y2, double xC, double yC, double R) {

        x1 -= xC;
        y1 -= yC;
        x2 -= xC;
        y2 -= yC;

        dx = x2 - x1;
        dy = y2 - y1;

        // составляем коэффициенты квадратного уравнения на пересечение прямой и окружности.
        // если на отрезке [0..1] есть отрицательные значения, значит отрезок пересекает окружность
        a = dx*dx + dy*dy;
        b = 2.0 * (x1*dx + y1*dy);
        c = x1*x1 + y1*y1 - R*R;

        // а теперь проверяем, есть ли на отрезке [0..1] решения
        if( -b < 0 )
            return c < 0;
        if( -b < (2.0 * a) )
            return (4.0 * a*c - b*b) < 0;

        return (a + b + c) < 0 ;
    }

 private:
    DDR_Sprite *Sprite;     // спрайт для пули

    double    _x, _y,       // текущая координата нашей пули
              _X, _Y,       // конечная точка, в которую пуля летит
               dX, dY;      // смещения по x и по y для нахождения новой позиции пули
    int        explosionCounter;
    float      Speed;
    bool       Collision;   // определяем знак (X < X0) и потом, как только знак этого выражения меняется, понимаем, что достигли цели
    bool       MonsterIsHit;

    double dx, dy, a, b, c; // переменные для вычисления пересечения пули с монстром, чтобы не объявлять их каждый раз в теле функции
};



// --- Explosion Class --------------------------------------------------------
class DDR_Explosion {
 public:
    __fastcall DDR_Explosion(DDR_Collection *frames, int x, int y)
        : _Frames(frames), _X(x), _Y(y), _Active(true), _Counter(0) {
    }
    __fastcall ~DDR_Explosion() {
    }

    void __fastcall Draw(IDirectDrawSurface *surfDest, RECT *rect, const int &scrPosX, const int &scrPosY) {

        if( _Counter < FramesQty ) {
            _Frames->Draw(surfDest, rect, _Counter, scrPosX + _X, scrPosY + _Y);
            _Cnt++;

            if( _Cnt > 10 ) {
                _Counter++;
                _Cnt = 0;
            }
        }
        else
            _Active = false;
    }

    inline __fastcall bool isActive() {
        return _Active;
    }

 private:
    DDR_Collection *_Frames;
    int             _X, _Y;
    unsigned int    _Counter, _Cnt;
    bool            _Active;

 public:
    static unsigned int FramesQty;
};

unsigned int DDR_Explosion::FramesQty = 0;



//---------------------------------------------------------------------------
#endif
