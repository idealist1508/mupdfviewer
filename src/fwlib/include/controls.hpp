#ifndef CONTROLS_HPP
#define CONTROLS_HPP

#include <string>
#include <vector>
#include <cassert>
#include "utils.hpp"

#define SET_PROP(aProp, aValue) if (aProp != aValue) { aProp = aValue; Repaint(); }
#define Application GetAppInst()

using namespace std;

class TPanel;
class TForm;

class TControl
{
friend class TPanel;
friend class TForm;
private:
    string FCaption;
    int FLeft;
    int FTop;
    int FWidth;
    int FHeight;
    bool FFocused;
    string FFontName;
    int FFontSize;
    int FFontColor;
    int FColor;
    bool FEnabled;
    bool FVisible;
    int FUpdateLock;

    TPanel* FParent;

    void SetFocused(bool aValue) { SET_PROP(FFocused, aValue) }
protected:

    virtual void Paint(irect aRect);
    virtual bool CanFocus() { return false; }
    virtual bool MouseDown(int x, int y) { if (!GetFocused() && CanFocus()) { SetFocused(true); return true; } return false; }
    virtual bool MouseUp(int x, int y) { Click(); return true; }
    virtual bool KeyDown(int aCode) { return false; }
    virtual bool KeyRepeat(int aCode, int aCount) { return false; }
    virtual bool KeyUp(int aCode, int aCount) { return false; }
public:
    int GetScreenLeft();
    int GetScreenTop();
    irect GetRect() { return MakeRect(GetScreenLeft(), GetScreenTop(), GetWidth(), GetHeight()); }
    TPanel* GetParent() { return FParent; }
    int GetLeft() { return FLeft; }
    int GetTop() { return FTop; }
    int GetWidth() { return FWidth; }
    int GetHeight() { return FHeight; }
    const string& GetCaption() { return FCaption; }
    bool GetFocused() { return FFocused; }
    bool GetEnabled() { return FEnabled; }
    bool GetVisible() { return FVisible; }
    const string& GetFontName() { return FFontName; }
    int GetFontSize() { return FFontSize; }
    int GetFontColor() { return FFontColor; }
    int GetColor() { return FColor; }

    virtual void SetCaption(const string& aValue) { SET_PROP(FCaption, aValue) }
    void SetPos(int aLeft, int aTop);
    void SetSize(int aWidth, int aHeight);
    void SetEnabled(bool aValue) { SET_PROP(FEnabled, aValue) }
    void SetVisible(bool aValue);
    void SetFontName(const string& aValue) { SET_PROP(FFontName, aValue) }
    void SetFontSize(int aValue) {SET_PROP(FFontSize, aValue) }
    void SetFontColor(int aValue) {SET_PROP(FFontColor, aValue) }
    void SetColor(int aValue) { SET_PROP(FColor, aValue) }

    bool Showing();
    virtual void Click() = 0;
    void Repaint();
    void BeginUpdate() { FUpdateLock++; }
    void EndUpdate(bool aRepaint = true);
    bool Updating();

    TControl();
    virtual ~TControl();
};

class TPanel : public TControl
{
private:
    vector<TControl*> FControls;
protected:
    virtual void Paint(irect aRect);
    virtual bool MouseDown(int x, int y);
    virtual bool MouseUp(int x, int y);
public:
    int GetControlCount() { return FControls.size(); }
    TControl* GetControl(int aIdx) { return FControls[aIdx]; }
    TControl* AddControl(TControl* aControl);
    void RemControl(TControl* aControl);
    void EnumControls(vector<TControl*>& aList);

    virtual void Click() { }

    virtual ~TPanel();
};

class TApplication;

class TForm : public TPanel
{
friend class TApplication;
private:
    TControl* FActiveControl;
protected:
    virtual bool MouseDown(int x, int y);
    virtual bool KeyDown(int aCode);
    virtual bool KeyRepeat(int aCode, int aCount);
    virtual bool KeyUp(int aCode, int aCount);
public:
    TControl* GetActiveControl() { return FActiveControl; }
    void SetActiveControl(TControl* aCtrl) { FActiveControl = aCtrl; }

    TForm();
};

class TApplication
{
private:
    TForm* FActiveForm;
protected:
    virtual string GetLogFileName() = 0;
public:
    TMemAlloc* BigMem;

    void DoShow();
    virtual void DoInit() { }
    virtual bool DoMouseDown(int x, int y);
    virtual bool DoMouseUp(int x, int y);
    virtual bool DoKeyDown(int aCode);
    virtual bool DoKeyRepeat(int aCode, int aCount);
    virtual bool DoKeyUp(int aCode, int aCount);
    virtual void DoFinit() { }

    void Terminate();
    void AskTerminate();
    virtual bool HandleException(Exception* E);
    void ShowException(Exception* E);
    void Run();

    TForm* GetActiveForm() { return FActiveForm; }
    void SetActiveForm(TForm* aValue, bool aRepaint = true);

    TApplication();
    virtual ~TApplication();
};

void SetAppInst(TApplication* aInst);
TApplication* GetAppInst();

class TLabel : public TControl
{
private:
    int FHAlign;
    int FVAlign;
protected:
    virtual void Paint(irect aRect);
public:
    int GetHAlign() { return FHAlign; }
    int GetVAlign() { return FVAlign; }
    void SetHAlign(int aValue) { SET_PROP(FHAlign, aValue) }
    void SetVAlign(int aValue) { SET_PROP(FVAlign, aValue) }

    void AdjustSize();
    virtual void Click() { }

    TLabel();
};

class TButton : public TControl
{
private:
    bool FDown;
    bool FToolButton;
    int FAlign;
    bool FRotate;
protected:
    virtual void Paint(irect aRect);
    virtual bool MouseDown(int x, int y) { /*FDown = true; Repaint();*/ return true; }
    virtual bool MouseUp(int x, int y) { /*FDown = false; Repaint();*/ Click(); return true; }
public:
    bool GetToolButton() { return FToolButton; }
    int GetAlign() { return FAlign; }
    bool GetRotate() { return FRotate; }
    bool GetDown() { return FDown; }
    void SetToolButton(bool aValue) { SET_PROP(FToolButton, aValue) }
    void SetAlign(int aValue) { SET_PROP(FAlign, aValue) }
    void SetRotate(bool aValue) { SET_PROP(FRotate, aValue) }
    void SetDown(bool aValue) { SET_PROP(FDown, aValue) }

    TButton()
    {
        FDown = false;
        FToolButton = false;
        FAlign = ALIGN_CENTER;
        FRotate = false;
        SetCaption("Button");
        SetSize(50, 25);
        SetColor(LGRAY);
    }
};

class TToolButton : public TButton
{
public:
    TToolButton()
    {
        SetToolButton(true);
        SetSize(25, 25);
    }
};

class TCheckBox : public TControl
{
private:
    static const int FSpacing = 2;
    int FAlign;
    bool FChecked;
    bool FDown;
protected:
    virtual void Paint(irect aRect);
    virtual bool MouseDown(int x, int y) { /*FDown = true; Repaint();*/ return true; }
    virtual bool MouseUp(int x, int y) { FChecked = !FChecked; FDown = false; Repaint(); Click(); return true; }
public:
    int GetAlign() { return FAlign; }
    bool GetChecked() { return FChecked; }
    void SetAlign(int aValue) { assert(aValue == ALIGN_LEFT || aValue == ALIGN_RIGHT); SET_PROP(FAlign, aValue) }
    void SetChecked(bool aValue) { SET_PROP(FChecked, aValue) }

    void AdjustSize();
    virtual void Click() { }

    TCheckBox()
    {
        FAlign = ALIGN_LEFT;
        FChecked = false;
        FDown = false;
        SetCaption("CheckBox");
    }
};

class TPaintBox : public TControl
{
protected:
    virtual void Paint(irect aRect) { /*need to override*/ }
public:
    virtual void Click() { }

    TPaintBox() { }
};

class TGridLayout : public TPanel
{
    struct TGridCell
    {
        int Row;
        int Col;
        TControl* Control;

        TGridCell(int aRow, int aCol, TControl* aControl)
        {
            Row = aRow;
            Col = aCol;
            Control = aControl;
        }
    };

private:
    vector<TGridCell*> FCells;
    int FSelRow, FSelCol;
    int FRowHeight, FColWidth;
    bool FHideSel;

    irect GetCellRect(int aRow, int aCol);
    TControl* GetControl(int aRow, int aCol);

protected:
    virtual void Paint(irect aRect);
    virtual bool KeyDown(int aCode);
    virtual bool KeyUp(int aCode, int aCount);

public:
    TControl* GetControl(int aIdx) { return TPanel::GetControl(aIdx); }
    int GetSelRow() { return FSelRow; }
    int GetSelCol() { return FSelCol; }
    void SetSel(int aRow, int aCol);
    TControl* AddControl(int aRow, int aCol, TControl* aControl);
    void AdjustSize();
    void ShowSel() { SET_PROP(FHideSel, false); }
    void HideSel() { SET_PROP(FHideSel, true); }

    TGridLayout(int aRowHeight, int aColWidth);
    virtual ~TGridLayout() { ClrObjList(FCells); }
};

#endif // CONTROLS_HPP
