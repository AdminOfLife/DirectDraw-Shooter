#ifndef MainH
#define MainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Menus.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
#define WM_RUNAPP WM_USER

class TFormMain : public TForm
{
__published:
    TTimer *Timer1;
    void __fastcall FormDestroy(TObject *Sender);
    void __fastcall FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
    void __fastcall FormPaint(TObject *Sender);
    void __fastcall Timer1Timer(TObject *Sender);
    void __fastcall Redraw();
    void __fastcall FormKeyUp(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall FormMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
    void __fastcall FormMouseUp(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
    void __fastcall FormMouseMove(TObject *Sender, TShiftState Shift,
          int X, int Y);

private:
    BOOL FActive;                           // is application active?
    MESSAGE void Run(TMessage &Message);
    void __fastcall initDD();
    void __fastcall destroyAll();    

public:
    __fastcall TFormMain(TComponent* Owner);

    BEGIN_MESSAGE_MAP
        MESSAGE_HANDLER(WM_RUNAPP, TMessage, Run);
    END_MESSAGE_MAP(TForm);
};
//---------------------------------------------------------------------------
extern TFormMain *FormMain;
//---------------------------------------------------------------------------
#endif
