#include <cassert>
#include <algorithm>
#include "inkview.h"
#include "utils.hpp"
#include "controls.hpp"

//**********************************************************************
//**********************************************************************
//**********************************************************************
void TControl::SetVisible(bool aValue)
{
    if (FVisible == aValue) return;

    FVisible = aValue;
    if (FParent != NULL) FParent->Repaint();
}
//**********************************************************************
void TControl::Paint(irect aRect)
{
    FillArea(aRect.x, aRect.y, aRect.w, aRect.h, FColor);
    DrawRect(aRect.x, aRect.y, aRect.w, aRect.h, BLACK);

    InflateRect(aRect, -1, -1);
    ifont* aFont = OpenFont(FFontName.c_str(), FFontSize, 0);
    SetFont(aFont, FFontColor);
    DrawTextRect(aRect.x, aRect.y, aRect.w, aRect.h, FCaption.c_str(), ALIGN_CENTER | VALIGN_MIDDLE);
    CloseFont(aFont);

    if (!FEnabled) DimArea(aRect.x, aRect.y, aRect.w, aRect.h, FColor);
}
//**********************************************************************
int TControl::GetScreenLeft()
{
    return FLeft + ((FParent == NULL) ? 0 : FParent->GetScreenLeft());
}
//**********************************************************************
int TControl::GetScreenTop()
{
    return FTop + ((FParent == NULL) ? 0 : FParent->GetScreenTop());
}
//**********************************************************************
void TControl::SetPos(int aLeft, int aTop)
{
    if (FLeft == aLeft && FTop == aTop) return;

    FLeft = aLeft;
    FTop = aTop;
    if (FParent != NULL) FParent->Repaint();
}
//**********************************************************************
void TControl::SetSize(int aWidth, int aHeight)
{
    if (FWidth == aWidth && FHeight == aHeight) return;

    FWidth = aWidth;
    FHeight = aHeight;
    if (FParent != NULL) FParent->Repaint();
}
//**********************************************************************
void TControl::Repaint()
{
    if (!Showing() || Updating()) return;

    irect aRect = GetRect();
    Paint(aRect);
    PartialUpdate(aRect.x, aRect.y, aRect.w, aRect.h);
}
//**********************************************************************
void TControl::EndUpdate(bool aRepaint)
{
    FUpdateLock--;
    if (FUpdateLock == 0 && aRepaint)
        Repaint();
}
//**********************************************************************
bool TControl::Updating()
{
    bool aResult = (FUpdateLock > 0);
    if (GetParent() != NULL)
        aResult = aResult || GetParent()->Updating();
    return  aResult;
}
//**********************************************************************
bool TControl::Showing()
{
    return GetVisible() &&
            ((GetParent() != NULL && GetParent()->Showing()) ||
             (dynamic_cast<TForm*>(this) != NULL));
}
//**********************************************************************
TControl::TControl()
{
    FLeft = 0;
    FTop = 0;
    FWidth = 0;
    FHeight = 0;
    FFocused = false;
    FColor = WHITE;
    FFontColor = BLACK;
    FFontSize = 12;
    FFontName = "LiberationSans";
    FEnabled = true;
    FVisible = true;
    FParent = NULL;
    FUpdateLock = 0;
}
//**********************************************************************
TControl::~TControl()
{
    if (FParent != NULL) FParent->RemControl(this);
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
void TPanel::Paint(irect aRect)
{
    TControl::Paint(aRect);

    for (int i = 0; i < GetControlCount(); i++)
    {
        TControl* aControl = GetControl(i);
        if (aControl->GetVisible())
            aControl->Paint(
                MakeRect(
                    aRect.x + aControl->GetLeft(),
                    aRect.y + aControl->GetTop(),
                    aControl->GetWidth(),
                    aControl->GetHeight()));
    }
}
//**********************************************************************
bool TPanel::MouseDown(int x, int y)
{
    irect r;
    for (int i = GetControlCount() - 1; i >= 0; i--)
    {
        TControl* aCtrl = GetControl(i);

        if (!aCtrl->GetVisible()) continue;
        if (!aCtrl->GetEnabled()) continue;
        r = MakeRect(aCtrl->GetLeft(), aCtrl->GetTop(), aCtrl->GetWidth(), aCtrl->GetHeight());
        if (!PtInRect(r, x, y)) continue;

        return aCtrl->MouseDown(x - aCtrl->GetLeft(), y - aCtrl->GetTop());
    }
    return false;
}
//**********************************************************************
bool TPanel::MouseUp(int x, int y)
{
    irect r;
    for (int i = GetControlCount() - 1; i >= 0; i--)
    {
        TControl* aCtrl = GetControl(i);

        if (!aCtrl->GetVisible()) continue;
        if (!aCtrl->GetEnabled()) continue;
        r = MakeRect(aCtrl->GetLeft(), aCtrl->GetTop(), aCtrl->GetWidth(), aCtrl->GetHeight());
        if (!PtInRect(r, x, y)) continue;

        return aCtrl->MouseUp(x - aCtrl->GetLeft(), y - aCtrl->GetTop());
    }
    return false;
}
//**********************************************************************
TControl* TPanel::AddControl(TControl* aControl)
{
    assert(aControl->FParent == NULL);

    aControl->FParent = this;
    FControls.push_back(aControl);
    aControl->Repaint();
    return aControl;
}
//**********************************************************************
void TPanel::RemControl(TControl* aControl)
{
    aControl->FParent = NULL;
    FControls.erase(find(FControls.begin(), FControls.end(), aControl));
    Repaint();
}
//**********************************************************************
void TPanel::EnumControls(vector<TControl*>& aList)
{
    for (int i = 0; i < GetControlCount(); i++)
    {
        TControl* aCtrl = GetControl(i);
        aList.push_back(aCtrl);
        TPanel* aPanel = dynamic_cast<TPanel*>(aCtrl);
        if (aPanel != NULL)
            aPanel->EnumControls(aList);
    }
}
//**********************************************************************
TPanel::~TPanel()
{
    for (int i = 0; i < GetControlCount(); i++)
    {
        TControl* aControl = GetControl(i);
        FControls[i] = NULL;
        aControl->FParent = NULL;
        delete aControl;
    }
    FControls.clear();
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
bool TForm::MouseDown(int x, int y)
{
    if (TPanel::MouseDown(x, y))
    {
        vector<TControl*> aList;
        EnumControls(aList);
        for (unsigned i = 0; i < aList.size(); i++)
            if (aList[i]->GetFocused())
                if (aList[i] != FActiveControl)
                {
                    if (FActiveControl != NULL)
                        FActiveControl->SetFocused(false);

                    FActiveControl = aList[i];
                    break;
                }

        return true;
    }
    return false;
}
//**********************************************************************
bool TForm::KeyDown(int aCode)
{
    return (FActiveControl == NULL) ? false : FActiveControl->KeyDown(aCode);
}
//**********************************************************************
bool TForm::KeyRepeat(int aCode, int aCount)
{
    return (FActiveControl == NULL) ? false : FActiveControl->KeyRepeat(aCode, aCount);
}
//**********************************************************************
bool TForm::KeyUp(int aCode, int aCount)
{
    return (FActiveControl == NULL) ? false : FActiveControl->KeyUp(aCode, aCount);
}
//**********************************************************************
TForm::TForm()
{
    FActiveControl = NULL;

    SetSize(ScreenWidth(), ScreenHeight());
    SetPos(0, 0);
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
void TApplication::DoShow()
{
    if (FActiveForm != NULL)
        FActiveForm->Repaint();
}

bool TApplication::DoMouseDown(int x, int y)
{
    return (FActiveForm == NULL) ? false : FActiveForm->MouseDown(x, y);
}

bool TApplication::DoMouseUp(int x, int y)
{
    return (FActiveForm == NULL) ? false : FActiveForm->MouseUp(x, y);
}

bool TApplication::DoKeyDown(int aCode)
{
    if (FActiveForm != NULL && FActiveForm->KeyDown(aCode)) return true;

    if (aCode == KEY_BACK)
    {
        AskTerminate();
        return true;
    }

    return false;
}

bool TApplication::DoKeyRepeat(int aCode, int aCount)
{
    return (FActiveForm == NULL) ? false : FActiveForm->KeyRepeat(aCode, aCount);
}

bool TApplication::DoKeyUp(int aCode, int aCount)
{
    return (FActiveForm == NULL) ? false : FActiveForm->KeyUp(aCode, aCount);
}

static void AppCloseDlgHandler(int aBtn)
{
    if (aBtn == 1)
        Application->Terminate();
}

void TApplication::AskTerminate()
{
    Dialog(ICON_WARNING, "Warning", "Exit appication?", "Yes", "No", AppCloseDlgHandler);
}

void TApplication::Terminate()
{
    CloseApp();
}

bool TApplication::HandleException(Exception* E)
{
    if (dynamic_cast<EAbort*>(E) != NULL) return true;
    ShowException(E);
    if (dynamic_cast<EUIError*>(E) != NULL) return true;
    WriteLog(E->GetMessage());

    return false;
}

void TApplication::ShowException(Exception* E)
{
    Message(ICON_ERROR, "Error", E->GetMessage().c_str(), 5000);
}

static int MainHandler(int aEvt, int aParam1, int aParam2);

void TApplication::Run()
{
    InitUtils(GetLogFileName());

    InkViewMain(MainHandler);
}

void TApplication::SetActiveForm(TForm* aValue, bool aRepaint)
{
    FActiveForm = aValue;
    if (FActiveForm != NULL && aRepaint)
        FActiveForm->Repaint();
}

TApplication::TApplication()
{
    FActiveForm = NULL;
}

TApplication::~TApplication()
{
    FinitUtils();
}

static TApplication* AppInst;
void SetAppInst(TApplication* aInst) { AppInst = aInst; }
TApplication* GetAppInst() { return AppInst; }
//**********************************************************************
//**********************************************************************
//**********************************************************************
static int MainHandler(int aEvt, int aParam1, int aParam2)
{
    assert(Application != NULL);
    try
    {
        #ifdef DEBUG
        WriteLog(Format("Msg = %d, p1 = %d, p2 = %d", aEvt, aParam1, aParam2));
        #endif
        switch (aEvt)
        {
        case EVT_INIT:
            ShowHourglass();
            Application->DoInit();
            HideHourglass();
            return 1;
        case EVT_SHOW:
        case EVT_ACTIVATE:
            Application->DoShow();
            return 1;
        case EVT_KEYDOWN:
            if (Application->DoKeyDown(aParam1)) return 1;
            break;
        case EVT_KEYREPEAT:
            if (Application->DoKeyRepeat(aParam1, aParam2)) return 1;
            break;
        case EVT_KEYUP:
            if (Application->DoKeyUp(aParam1, aParam2)) return 1;
            break;
        case EVT_POINTERDOWN:
            if (Application->DoMouseDown(aParam1, aParam2)) return 1;
            break;
        case EVT_POINTERUP:
            if (Application->DoMouseUp(aParam1, aParam2)) return 1;
            break;
        case EVT_EXIT:
            Application->DoFinit();
            return 1;
        }
    }
    catch (Exception* E)
    {
        if (!Application->HandleException(E))
            Application->Terminate();
    }
    catch (exception E)
    {
        Message(ICON_ERROR, "Error", E.what(), 5000);
        WriteLog(E.what());
        Application->Terminate();
    }
    catch (char* E)
    {
        Message(ICON_ERROR, "Error", E, 5000);
        WriteLog(E);
        Application->Terminate();
    }
    catch (...)
    {
        Message(ICON_ERROR, "Error", "Unknown error", 5000);
        WriteLog("Unknown error");
        Application->Terminate();
    }
    return 0;
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
void TLabel::AdjustSize()
{
    ifont* aFont = OpenFont(GetFontName().c_str(), GetFontSize(), 0);
    SetFont(aFont, GetFontColor());

    int W = StringWidth(GetCaption().c_str());
    int H = TextRectHeight(W, GetCaption().c_str(), 0);
    int X = GetLeft();
    int Y = GetTop();
    if (FHAlign == ALIGN_RIGHT)
        X = GetLeft() + GetWidth() - W;
    if (FVAlign == VALIGN_BOTTOM)
        Y = GetTop() + GetHeight() - H;

    SetPos(X, Y);
    SetSize(X, Y);

    CloseFont(aFont);
}
//**********************************************************************
TLabel::TLabel()
{
    FHAlign = ALIGN_LEFT;
    FVAlign = VALIGN_MIDDLE;
    SetCaption("Label");
    AdjustSize();
}
//**********************************************************************
void TLabel::Paint(irect aRect)
{
    FillArea(aRect.x, aRect.y, aRect.w, aRect.h, GetColor());

    ifont* aFont = OpenFont(GetFontName().c_str(), GetFontSize(), 1);
    SetFont(aFont, GetFontColor());
    DrawTextRect(aRect.x, aRect.y, aRect.w, aRect.h, GetCaption().c_str(), FHAlign | FVAlign);
    CloseFont(aFont);

    if (!GetEnabled()) DimArea(aRect.x, aRect.y, aRect.w, aRect.h, GetColor());
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
void TButton::Paint(irect aRect)
{
    FillArea(aRect.x, aRect.y, aRect.w, aRect.h, GetColor());
    if (FToolButton)
    {
        DrawRect(aRect.x, aRect.y, aRect.w, aRect.h, BLACK);
        InflateRect(aRect, -1, -1);
    }
    else
    {
        DrawSelection(aRect.x, aRect.y, aRect.w, aRect.h, BLACK);
        InflateRect(aRect, -3, -3);
    }
    ifont* aFont = OpenFont(GetFontName().c_str(), GetFontSize(), 1);
    SetFont(aFont, GetFontColor());
    DrawTextRect(aRect.x, aRect.y, aRect.w, aRect.h, GetCaption().c_str(), FAlign | VALIGN_MIDDLE | (FRotate ? ROTATE : 0) | DOTS);
    CloseFont(aFont);

    if (!GetEnabled()) DimArea(aRect.x, aRect.y, aRect.w, aRect.h, GetColor());
    if (FDown) InvertArea(aRect.x, aRect.y, aRect.w, aRect.h);
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
void TCheckBox::Paint(irect aRect)
{
    FillArea(aRect.x, aRect.y, aRect.w, aRect.h, GetColor());

    ifont* aFont = OpenFont(GetFontName().c_str(), GetFontSize(), 1);
    SetFont(aFont, GetFontColor());

    //Checkbox
    irect aTmpRect = MakeRect(aRect.x, aRect.y, 0, 0);
    aTmpRect.h = aTmpRect.w = TextRectHeight(100, "Ay", 0);
    OffsetRect(aTmpRect, 1, 1);
    InflateRect(aTmpRect, 1, 1);
    if (FAlign == ALIGN_RIGHT)
         aTmpRect.x = aRect.x + aRect.w - aTmpRect.w;
    DrawRect(aTmpRect.x, aTmpRect.y, aTmpRect.w, aTmpRect.h, BLACK);

    if (FChecked)
    {
        DrawLine(aTmpRect.x, aTmpRect.y, aTmpRect.x + aTmpRect.w, aTmpRect.y + aTmpRect.h, BLACK);
        DrawLine(aTmpRect.x, aTmpRect.y + aTmpRect.h, aTmpRect.x + aTmpRect.w, aTmpRect.y, BLACK);
    }

    InflateRect(aTmpRect, -1, -1);
    if (FDown)
        InvertArea(aTmpRect.x, aTmpRect.y, aTmpRect.w, aTmpRect.h);

    //Text label
    InflateRect(aTmpRect, 1, 1);
    if (FAlign == ALIGN_LEFT)
        aRect = MakeRect(aRect.x + aTmpRect.w + FSpacing, aRect.y, aRect.w - aTmpRect.w - FSpacing, aTmpRect.h);
    else
        aRect = MakeRect(aRect.x, aRect.y, aRect.w - aTmpRect.w - FSpacing, aTmpRect.h);

    DrawTextRect(aRect.x, aRect.y, aRect.w, aRect.h, GetCaption().c_str(), FAlign | VALIGN_MIDDLE);
    CloseFont(aFont);

    if (!GetEnabled()) DimArea(aRect.x, aRect.y, aRect.w, aRect.h, GetColor());
}
//**********************************************************************
void TCheckBox::AdjustSize()
{
    ifont* aFont = OpenFont(GetFontName().c_str(), GetFontSize(), 0);
    SetFont(aFont, GetFontColor());

    int H = TextRectHeight(100, "Ay", 0) + 2;
    int W = StringWidth(GetCaption().c_str()) + H + FSpacing;
    int X = GetLeft();
    int Y = GetTop();
    if (FAlign == ALIGN_RIGHT)
        X = GetLeft() + GetWidth() - W;
    SetPos(X, Y);
    SetSize(W, H);

    CloseFont(aFont);
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
irect TGridLayout::GetCellRect(int aRow, int aCol)
{
    irect R;

    R.x = 5 + aCol * (FColWidth + 4);
    R.y = 5 + aRow * (FRowHeight + 4);
    R.w = FColWidth;
    R.h = FRowHeight;

    return R;
}
//**********************************************************************
TControl* TGridLayout::GetControl(int aRow, int aCol)
{
    TControl* aResult = NULL;

    for (unsigned i = 0; i < FCells.size(); i++)
        if (FCells[i]->Row == aRow && FCells[i]->Col == aCol)
        {
            aResult = FCells[i]->Control;
            break;
        }

    return aResult;
}
//**********************************************************************
void TGridLayout::Paint(irect aRect)
{
    TPanel::Paint(aRect);

    irect R;
    if (FSelRow >=0 && FSelCol >= 0 && !FHideSel)
    {
        R = GetCellRect(FSelRow, FSelCol);
        OffsetRect(R, GetScreenLeft(), GetScreenTop());
        InflateRect(R, 4, 4);
        DrawSelection(R.x, R.y, R.w, R.h, BLACK);
    }

    R = GetRect();
    DrawRect(R.x, R.y, R.w, R.h, BLACK);
}
//**********************************************************************
bool TGridLayout::KeyDown(int aCode)
{
    switch (aCode)
    {
    case KEY_LEFT: SetSel(GetSelRow(), GetSelCol() - 1); return true;
    case KEY_RIGHT: SetSel(GetSelRow(), GetSelCol() + 1); return true;
    case KEY_UP: SetSel(GetSelRow() - 1, GetSelCol()); return true;
    case KEY_DOWN: SetSel(GetSelRow() + 1, GetSelCol()); return true;
    case KEY_OK: GetControl(GetSelRow(), GetSelCol())->Click(); return true;
    default:
        return false;
    }
}
//**********************************************************************
bool TGridLayout::KeyUp(int aCode, int aCount)
{
    switch (aCode)
    {
    case KEY_LEFT: return true;
    case KEY_RIGHT: return true;
    case KEY_UP: return true;
    case KEY_DOWN: return true;
    case KEY_OK: return true;
    default:
        return false;
    }
}
//**********************************************************************
void TGridLayout::SetSel(int aRow, int aCol)
{
    if (GetControl(aRow, aCol) == NULL) return;
    if (FSelRow == aRow && FSelCol == aCol) return;

    irect R1 = GetCellRect(FSelRow, FSelCol);
    irect R2 = GetCellRect(aRow, aCol);

    FSelRow = aRow;
    FSelCol = aCol;

    if (Showing() && !Updating())
    {
        OffsetRect(R1, GetScreenLeft(), GetScreenTop());
        InflateRect(R1, 4, 4);
        DrawSelection(R1.x, R1.y, R1.w, R1.h, WHITE);
        OffsetRect(R2, GetScreenLeft(), GetScreenTop());
        InflateRect(R2, 4, 4);
        DrawSelection(R2.x, R2.y, R2.w, R2.h, BLACK);

        irect R = UnionRect(R1, R2);
        PartialUpdate(R.x, R.y, R.w, R.h);
    }
}
//**********************************************************************
TControl* TGridLayout::AddControl(int aRow, int aCol, TControl* aControl)
{
    TPanel::AddControl(aControl);
    FCells.push_back(new TGridLayout::TGridCell(aRow, aCol, aControl));

    BeginUpdate();

    irect R = GetCellRect(aRow, aCol);
    aControl->SetPos(R.x, R.y);
    aControl->SetSize(R.w, R.h);

    EndUpdate(false);

    aControl->Repaint();
    return aControl;
}
//**********************************************************************
void TGridLayout::AdjustSize()
{
    irect r = MakeRect(0, 0, 0, 0);
    irect r1;
    for (unsigned i = 0; i < FCells.size(); i++)
    {
        r1 = GetCellRect(FCells[i]->Row, FCells[i]->Col);
        r = UnionRect(r, r1);
    }
    r.w += 5;
    r.h += 5;
    SetSize(r.w, r.h);
}
//**********************************************************************
TGridLayout::TGridLayout(int aRowHeight, int aColWidth)
{
    FRowHeight = aRowHeight;
    FColWidth = aColWidth;
    FHideSel = false;
    \
    FSelRow = -1;
    FSelCol = -1;
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
