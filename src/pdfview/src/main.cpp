#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "inkview.h"
#include "utils.hpp"
#include "main.hpp"

using namespace std;

iconfigedit CfgDesc[] = {
    {CFG_NUMBER, NULL, "FullUpdate Interval", "Number of pages between full update", "update_interval", "0", NULL, NULL},
    {CFG_NUMBER, NULL, "Hardware Margins", "Number of pixels hidden under bezel", "hw_margin", "0", NULL, NULL},
    {CFG_NUMBER, NULL, "Cache Size", "Number of pages cached in memory", "cache_size", "10", NULL, NULL},
    {0, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

void TOCHandler(long long aPageIdx)
{
    TMainForm* aForm = dynamic_cast<TMainForm*>(Application->GetActiveForm());
    if (aForm != NULL)
        aForm->ShowPage(aPageIdx);
}

void CfgHandler()
{
    TMainForm* aForm = dynamic_cast<TMainForm*>(Application->GetActiveForm());
    if (aForm != NULL)
    {
        aForm->LoadConfig();
        aForm->Layout();
    }
}

//**********************************************************************
//**********************************************************************
//**********************************************************************
void TMainForm::PageNumDigit(const string& aDigit)
{
    FPageNumStr += aDigit;
    FPageNumLbl->SetCaption(FPageNumStr);
}
//**********************************************************************
void TMainForm::PageNumFix()
{
    FPageNumStr.erase(FPageNumStr.size() - 1, 1);
    FPageNumLbl->SetCaption(FPageNumStr);
}
//**********************************************************************
void TMainForm::PageNumGoto()
{
    int aNum = atoi(FPageNumStr.c_str());
    ShowPage(aNum + FPageNumOffset);
}
//**********************************************************************
void TMainForm::PageNumReset()
{
    FPageNumStr.clear();
    if (!FCache.empty())
        FPageNumLbl->SetCaption(Format("%d of %d", FCache.front()->PageIdx - FPageNumOffset, GetPageCount() - (FPageNumOffset + 1)));

    if (InMenu()) ExitMenu();
}
//**********************************************************************
void TMainForm::ShowTOC()
{
    if (FTOC.empty())
        throw new EUIError("Not table of contents found");

    FreeTOC(FTOCMenu, FTOCSize);
    FTOCMenu = (tocentry*)malloc(sizeof(tocentry) * FTOC.size());
    FTOCSize = FTOC.size();

    for (unsigned i = 0; i < FTOC.size(); i++)
    {
        FTOCMenu[i].level = FTOC[i]->Level;
        FTOCMenu[i].page = FTOC[i]->PageNum;
        FTOCMenu[i].position = FTOC[i]->PageNum + FPageNumOffset;
        FTOCMenu[i].text = MakeStr(FTOC[i]->Caption);
    }

    OpenContents(FTOCMenu, FTOCSize, FCache.front()->PageIdx, TOCHandler);
}
//**********************************************************************
void TMainForm::FreeTOC(tocentry* aMenu, int aSize)
{
    if (aMenu == NULL) return;

    for (int i = 0; i < aSize; i++)
        if (aMenu[i].text != NULL)
            free(aMenu[i].text);
    free(aMenu);
}
//**********************************************************************
TButton* TMainForm::AddToolBtn(TButton* aBtn, TPanel* aBar, int aSize, int &aPos, bool aVert)
{
    if (!aVert)
    {
        aBtn->SetPos(aPos, 0);
        aBtn->SetSize(aSize, aBtn->GetHeight());
        aPos += aBtn->GetWidth();
    }
    else
    {
        aBtn->SetPos(0, aPos);
        aBtn->SetSize(aBtn->GetWidth(), aSize);
        aPos += aBtn->GetHeight();
    }
    aPos--;
    aBar->AddControl(aBtn);
    return aBtn;
}
//**********************************************************************
bool TMainForm::Cached(int aPageIdx)
{
    for (deque<TCacheEntry*>::iterator aPtr = FCache.begin(); aPtr != FCache.end(); ++aPtr)
        if ((*aPtr)->PageIdx == aPageIdx)
            return true;

    return false;
}
//**********************************************************************
void TMainForm::Paint(irect aRect)
{
    TForm::Paint(aRect);
}
//**********************************************************************
bool TMainForm::KeyDown(int aCode)
{
    if (TForm::KeyDown(aCode)) return true;

    switch (aCode)
    {
    case KEY_LEFT:
        if (FAngle == 0)
            StepPage(-1);
        else
            ScrollPage(1);
        return true;
    case KEY_RIGHT:
        if (FAngle == 0)
            StepPage(1);
        else
            ScrollPage(-1);
        return true;
    case KEY_MENU:
        ShowTOC();
        return true;
    case KEY_ZOOMIN:
        ToggleRotate();
        return true;
    case KEY_ZOOMOUT:
        ClearScreen();
        PartialUpdate(0, 0, ScreenWidth(), ScreenHeight());
        Paint(GetRect());
        FullUpdate();
        return true;
    case KEY_OK:
        EnterMenu();
        return true;
    case KEY_BACK:
        if (InMenu())
        {
            ExitMenu();
            return true;
        }
        if (GetActiveControl() == FBookmarksBar)
        {
            ExitBookmarks();
            return true;
        }
    default:
        return false;
    }
}
//**********************************************************************
bool TMainForm::KeyUp(int aCode, int aCount)
{
    if (TForm::KeyUp(aCode, aCount)) return true;

    FRepeatLbl->SetCaption("");
    switch (aCode)
    {
    case KEY_PREV: StepPage(-(aCount + 1)); return true;
    case KEY_NEXT: StepPage(aCount + 1); return true;
    case KEY_UP: StepBookmark(-(aCount + 1)); return true;
    case KEY_DOWN: StepBookmark(aCount + 1); return true;
    case KEY_OK:
        if (aCount > 0)
        {
            if (FViewMode == vmTP)
                SetViewMode(vmTPI);
            else
                SetViewMode(vmTP);
        }
        return true;
    default:
        return false;
    }
}
//**********************************************************************
bool TMainForm::KeyRepeat(int aCode, int aCount)
{
    if (TForm::KeyRepeat(aCode, aCount)) return true;

    FRepeatLbl->SetCaption(Format("%d", aCount + 1));
    return true;
}
//**********************************************************************
bool TMainForm::MouseDown(int x, int y)
{
    FGestStart = time(NULL);
    FGestPts.x = x;
    FGestPts.y = y;

    if (TForm::MouseDown(x, y)) return true;

    return false;
}
//**********************************************************************
double sqr(double x)
{
    return x * x;
}

double hypot(const irect& r)
{
    return sqrt(sqr(r.w - r.x) + sqr(r.h - r.y));
}

double i2f(int x)
{
    return x;
}

bool TMainForm::MouseUp(int x, int y)
{
    //Check if we can recognize a gesture
    FGestEnd = time(NULL);
    FGestPts.w = x;
    FGestPts.h = y;

    irect r = FGestPts;
    double t = (FGestEnd - FGestStart);
    double l = hypot(r);
    double a = atan(fabs(i2f(r.h - r.y) / (r.w - r.x))) * 180 / 3.1415926;
    int aKey = 0;

    if (r.h >= r.y && a > 80) aKey = KEY_DOWN;
    if (r.y >= r.h && a > 80) aKey = KEY_UP;
    if (r.w >= r.x && a < 10) aKey = KEY_RIGHT;
    if (r.x >= r.w && a < 10) aKey = KEY_LEFT;

    if (aKey != 0 && t <= 2 && l >= 50)
    {
        KeyDown(aKey);
        KeyUp(aKey, 0);
        return true;
    }

    //Check if it's an UI element
    if (TForm::MouseUp(x, y)) return true;

    return false;
}
//**********************************************************************
void TMainForm::PaintCurrPageImage(irect aRect)
{
    if (FCache.empty()) return;

    SetClip(aRect.x, aRect.y, aRect.w, aRect.h);

    TCacheEntry* aPage = FCache.front();

    irect aSrcRect;
    aSrcRect.w = min(FPageImage->GetWidth(), (int)aPage->Image->width);
    aSrcRect.h = aPage->Image->height;
    aSrcRect.x = max(0, (int)(aPage->Image->width - (FScroll + aSrcRect.w)));
    aSrcRect.y = 0;

    DrawBitmapArea(aRect.x, aRect.y, aPage->Image, aSrcRect.x, aSrcRect.y, aSrcRect.w, aSrcRect.h);
    if (!FDropMargins)
        DrawRect(aPage->CBox.x + aRect.x, aPage->CBox.y + aRect.y, aPage->CBox.w, aPage->CBox.h, BLACK);

    SetClip(0, 0, ScreenWidth(), ScreenHeight());
}
//**********************************************************************
TCacheEntry* TMainForm::LoadPage(int aPageIdx)
{
    if (aPageIdx < 0)
        throw new EUIError(Format("Page index out of range (%d < 0)", aPageIdx));
    if (aPageIdx >= GetPageCount())
        throw new EUIError(Format("Page index out of range (%d > %d)", aPageIdx, GetPageCount() - 1));

    TCacheEntry* aResult = new TCacheEntry();
    try
    {
        aResult->PageIdx = aPageIdx;

        if (FPrefetch != NULL)
        {
            if (!FPrefetch->Valid(aPageIdx, FPageImage->GetWidth(), FPageImage->GetHeight(), FAngle,
                                  FViewMode, FDropMargins))
            {
                FPrefetch->Abort();
                FreeAndNil(FPrefetch);
            }
            else
                FPrefetch->Wait();
        }

        if (FPrefetch == NULL)
            FRender->RenderPage(
                aPageIdx, FPageImage->GetWidth(), FPageImage->GetHeight(), FAngle,
                FViewMode, FDropMargins);
        else
        {
            FreeAndNil(FPrefetch);
            fprintf(stderr, "Using pre-rendered page\n");
        }

        aResult->Image = FRender->GetPage(Application->BigMem, &aResult->CBox);
        aResult->ViewMode = FViewMode;
        aResult->DropMargins = FDropMargins;
        aResult->Angle = FAngle;

        return aResult;
    }
    catch (...)
    {
        delete aResult;
        throw;
    }
}
//**********************************************************************
void TMainForm::ShowPage(int aPageIdx)
{
    ShowHourglass();
    TCacheEntry* aPage = NULL;
    for (deque<TCacheEntry*>::iterator aPtr = FCache.begin(); aPtr != FCache.end(); ++aPtr)
        if ((*aPtr)->PageIdx == aPageIdx)
        {
            aPage = *aPtr;
            FCache.erase(aPtr);
            break;
        }

    if (aPage != NULL && (aPage->ViewMode != FViewMode || aPage->DropMargins != FDropMargins || aPage->Angle != FAngle))
        FreeAndNil(aPage);

    if (aPage == NULL)
        aPage = LoadPage(aPageIdx);

    //Update cache
    while (FCache.size() > (unsigned)FMaxCacheSize)
    {
        delete FCache.back();
        FCache.pop_back();
    }
    if (!FCache.empty())
        FCache.front()->Scroll = FScroll;
    FCache.push_front(aPage);
    FScroll = FCache.front()->Scroll;

    //Update screen
    if (InMenu())
        ExitMenu(false);

    if (FUpdateInterval > 0 && FUpdateCounter >= FUpdateInterval)
    {
        ClearScreen();
        PartialUpdate(0, 0, ScreenWidth(), ScreenHeight());

        BeginUpdate();
        PageNumReset();
        UpdateInfo();
        for (unsigned i = 0; i < FBookmarkBtns.size(); i++)
            FBookmarkBtns[i]->SetDown(FBookmarkBtns[i]->GetPageNum() + FPageNumOffset == aPageIdx);
        EndUpdate(false);

        Paint(GetRect());

        FullUpdate();
        HideHourglass();
        FUpdateCounter = 1;
    }
    else
    {
        irect aRect = FPageImage->GetRect();
        FillArea(aRect.x, aRect.y, aRect.w, aRect.h, WHITE);
        PartialUpdate(aRect.x, aRect.y, aRect.w, aRect.h);
        FPageImage->Repaint();
        HideHourglass();

        FToolBar->BeginUpdate();
        PageNumReset();
        UpdateInfo();
        FToolBar->EndUpdate();

        FBookmarksBar->BeginUpdate();
        for (unsigned i = 0; i < FBookmarkBtns.size(); i++)
            FBookmarkBtns[i]->SetDown(FBookmarkBtns[i]->GetPageNum() + FPageNumOffset == aPageIdx);
        FBookmarksBar->EndUpdate();

        FUpdateCounter++;
    }
}
//**********************************************************************
void TMainForm::UpdateInfo()
{
    struct mallinfo aMemInfo = mallinfo();
    double aMemSize = (aMemInfo.uordblks + aMemInfo.hblkhd) / (1024 * 1024);

    FInfoLbl->SetCaption(
        Format("%d%% %dC %s %.1fM", GetBatteryPower(), GetTemperature(), FormatDateTime("%H:%M", time(NULL)).c_str(), aMemSize));
}
//**********************************************************************
void TMainForm::StepPage(int aDelta)
{
    if (FCache.empty()) return;

    int aIdx = max(0, min(FCache.front()->PageIdx + aDelta, GetPageCount() - 1));
    ShowPage(aIdx);

    //Try to prefetch next page in the same direction
    aIdx = max(0, min(aIdx + aDelta / abs(aDelta), GetPageCount() - 1));
    if (!Cached(aIdx))
    {
        if (FPrefetch != NULL)
            if (!FPrefetch->Valid(aIdx, FPageImage->GetWidth(), FPageImage->GetHeight(), FAngle,
                                  FViewMode, FDropMargins))
            {
                FPrefetch->Abort();
                FreeAndNil(FPrefetch);
            }

        if (FPrefetch == NULL)
        {
            fprintf(stderr, "Prefetching page %d\n", aIdx);
            FPrefetch = new TPrefetch(FRender, aIdx, FPageImage->GetWidth(), FPageImage->GetHeight(), FAngle,
                                      FViewMode, FDropMargins);
        }
    }
}
//**********************************************************************
void TMainForm::ScrollPage(int aDelta)
{
    int aOldScroll = FScroll;

    int W = FCache.front()->Image->width;
    int w = FPageImage->GetWidth();
    FScroll = max(0, min(FScroll + aDelta * w * 9 / 10, W - w));

    if (FScroll != aOldScroll)
    {
        irect aRect = MakeRect(FPageImage->GetScreenLeft(), FPageImage->GetScreenTop(), FPageImage->GetWidth(), FPageImage->GetHeight());
        FillArea(aRect.x, aRect.y, aRect.w, aRect.h, WHITE);
        PartialUpdate(aRect.x, aRect.y, aRect.w, aRect.h);

        FPageImage->Repaint();
    }
    else if (aDelta < 0)
        StepPage(-1);
    else if (aDelta > 0)
        StepPage(1);
}
//**********************************************************************
void TMainForm::StepBookmark(int aDelta)
{
    if (!FBookmarksBar->GetVisible()) return;

    for (int i = 0; (unsigned)i < FBookmarkBtns.size(); i++)
        if (FBookmarkBtns[i]->GetDown())
        {
            TBookmarkBtn* aLastBkm = NULL;
            while (aDelta != 0)
            {
                i = (aDelta > 0) ? i + 1 : i - 1;
                if (i < 0 || (unsigned)i >= FBookmarkBtns.size())
                    break;
                if (FBookmarkBtns[i]->GetPageNum() >= 0)
                {
                    aDelta = (aDelta > 0) ? aDelta - 1 : aDelta + 1;
                    aLastBkm = FBookmarkBtns[i];
                }
            }
            if (aLastBkm != NULL) aLastBkm->Click();
            return;
        }

    EnterBookmarks();
}
//**********************************************************************
void TMainForm::HandleBookmark(TMainForm::TBookmarkBtn* aSender)
{
    if (aSender->GetPageNum() == -1 || FAddBkm)
    {
        int aPageNum = FCache.front()->PageIdx - FPageNumOffset;
        for (unsigned i = 0; i < FBookmarkBtns.size(); i++)
            if (FBookmarkBtns[i]->GetPageNum() == aPageNum)
            {
                FBookmarkBtns[i]->SetDown(true);
                throw new EUIError("Page already bookmarked");
            }
        aSender->SetPageNum(aPageNum);
        FAddBkm = false;
    }
    else if (FDelBkm)
    {
        aSender->SetPageNum(-1);
        FDelBkm = false;
    }
    else
    {
        int aPageIdx = aSender->GetPageNum() + FPageNumOffset;
        if (aPageIdx == FCache.front()->PageIdx)
            aSender->SetPageNum(-1);
        else
            ShowPage(aPageIdx);
    }

    if (GetActiveControl() == FBookmarksBar)
        ExitBookmarks();
}
//**********************************************************************
void TMainForm::ToggleBookmarks()
{
    ShowHourglass();
    BeginUpdate();

    FBookmarksBar->SetVisible(!FBookmarksBar->GetVisible());
    Layout();

    EndUpdate();
    ExitMenu();
    HideHourglass();
}
//**********************************************************************
void TMainForm::AddBookmark()
{
    FAddBkm = true;
    EnterBookmarks();
}
//**********************************************************************
void TMainForm::DelBookmark()
{
    FDelBkm = true;
    EnterBookmarks();
}
//**********************************************************************
void TMainForm::EnterBookmarks()
{
    FBookmarksBar->ShowSel();

    ExitMenu();
    SetActiveControl(FBookmarksBar);
}
//**********************************************************************
void TMainForm::ExitBookmarks()
{
    SetActiveControl(NULL);

    FBookmarksBar->HideSel();
    FAddBkm = false;
    FDelBkm = false;
}
//**********************************************************************
void TMainForm::SetViewMode(TViewMode aMode)
{
    FViewMode = aMode;
    if (!FCache.empty())
        ShowPage(FCache.front()->PageIdx);

    if (FTBtn != NULL)
        FTBtn->SetDown(false);
    if (FTPBtn != NULL)
        FTPBtn->SetDown(false);
    if (FTPIBtn != NULL)
        FTPIBtn->SetDown(false);
    switch (aMode)
    {
    case vmT: if (FTBtn != NULL) FTBtn->SetDown(true); break;
    case vmTP: if (FTPBtn != NULL) FTPBtn->SetDown(true); break;
    case vmTPI: if (FTPIBtn != NULL) FTPIBtn->SetDown(true); break;
    }
}
//**********************************************************************
void TMainForm::ToggleMargins()
{
    FDropMargins = !FDropMargins;
    if (!FCache.empty())
        ShowPage(FCache.front()->PageIdx);

    if (FMarginsBtn != NULL)
        FMarginsBtn->SetDown(FDropMargins);
}
//**********************************************************************
void TMainForm::ToggleRotate()
{
    FAngle = (FAngle == 0 ? 90 : 0);
    if (!FCache.empty())
        ShowPage(FCache.front()->PageIdx);
}
//**********************************************************************
void TMainForm::EnterMenu()
{
    if (InMenu()) return;

    BeginUpdate();

    TToolButton* aBtn;
    FMenu = new TGridLayout(50, 50);

    FMenu->AddControl(0, 0, new TMainForm::TPageNumResetBtn());
    FMenu->AddControl(0, 1, new TMainForm::TPageNumGotoBtn());
    FMenu->AddControl(0, 2, new TMainForm::TPageNumFixBtn());

    FMenu->AddControl(1, 0, new TMainForm::TPageNumDigitBtn("1"));
    FMenu->AddControl(1, 1, new TMainForm::TPageNumDigitBtn("2"));
    FMenu->AddControl(1, 2, new TMainForm::TPageNumDigitBtn("3"));
    FMenu->AddControl(2, 0, new TMainForm::TPageNumDigitBtn("4"));
    FMenu->AddControl(2, 1, new TMainForm::TPageNumDigitBtn("5"));
    FMenu->AddControl(2, 2, new TMainForm::TPageNumDigitBtn("6"));
    FMenu->AddControl(3, 0, new TMainForm::TPageNumDigitBtn("7"));
    FMenu->AddControl(3, 1, new TMainForm::TPageNumDigitBtn("8"));
    FMenu->AddControl(3, 2, new TMainForm::TPageNumDigitBtn("9"));

    FMenu->AddControl(4, 0, new TMainForm::TFirstPageBtn());
    FMenu->AddControl(4, 1, new TMainForm::TPageNumDigitBtn("0"));
    FMenu->AddControl(4, 2, new TMainForm::TLastPageBtn());

    FMenu->AddControl(0, 4, new TMainForm::TAppCloseBtn());
    FMenu->AddControl(1, 4, new TMainForm::TTOCBtn());
    aBtn = (TToolButton*)FMenu->AddControl(2, 4, new TMainForm::TToggleBookmarkBtn());
    if (FBookmarksBar->GetVisible())
        aBtn->SetDown(true);
    FMenu->AddControl(3, 4, new TMainForm::TAddBookmarkBtn());
    FMenu->AddControl(4, 4, new TMainForm::TDelBookmarkBtn());

    FMenu->AddControl(5, 0, new TMainForm::TConfigBtn());

    aBtn = (TToolButton*)FMenu->AddControl(0, 3, new TMainForm::TMarginsBtn());
    if (FDropMargins) aBtn->SetDown(true);
    FMenu->AddControl(1, 3, new TMainForm::TRotateBtn());
    aBtn = (TToolButton*)FMenu->AddControl(2, 3, new TMainForm::TViewModeBtn(vmT));
    if (FViewMode == vmT) aBtn->SetDown(true);
    aBtn = (TToolButton*)FMenu->AddControl(3, 3, new TMainForm::TViewModeBtn(vmTP));
    if (FViewMode == vmTP) aBtn->SetDown(true);
    aBtn = (TToolButton*)FMenu->AddControl(4, 3, new TMainForm::TViewModeBtn(vmTPI));
    if (FViewMode == vmTPI) aBtn->SetDown(true);

    for (int i = 0; i < FMenu->GetControlCount(); i++)
        FMenu->GetControl(i)->SetFontSize(30);

    FMenu->SetSel(2, 2);

    FMenu->AdjustSize();
    irect r = FMenu->GetRect();
    r.x = (ScreenWidth() - r.w) / 2;
    r.y = (ScreenHeight() - r.h) / 2;
    FMenu->SetPos(r.x, r.y);

    AddControl(FMenu);
    SetActiveControl(FMenu);

    EndUpdate(false);

    r = FPageImage->GetRect();
    DimArea(r.x, r.y, r.w, r.h, BLACK);
    PartialUpdate(r.x, r.y, r.w, r.h);
    FMenu->Repaint();
}
//**********************************************************************
void TMainForm::ExitMenu(bool aRepaint)
{
    if (!InMenu()) return;

    BeginUpdate();

    SetActiveControl(NULL);
    RemControl(FMenu);
    FreeAndNil(FMenu);

    EndUpdate(false);
    if (aRepaint)
    {
        irect r = FPageImage->GetRect();
        FillArea(r.x, r.y, r.w, r.h, WHITE);
        PartialUpdate(r.x, r.y, r.w, r.h);
        FPageImage->Repaint();
    }
}
//**********************************************************************
void TMainForm::Layout()
{
    BeginUpdate();

    FToolBar->SetPos(FHWMargin, FHWMargin);
    FToolBar->SetSize(ScreenWidth() - 2 * FHWMargin, BtnSize);
    FInfoLbl->SetSize(FToolBar->GetWidth() - FInfoLbl->GetLeft() - 1, BtnSize - 2);

    FBookmarksBar->SetPos(FHWMargin, BtnSize + FHWMargin - 1);
    FBookmarksBar->SetSize(BtnSize + 10, ScreenHeight() - BtnSize - 2 * FHWMargin);

    int x, w;
    if (FBookmarksBar->GetVisible())
    {
        x = FBookmarksBar->GetLeft() + FBookmarksBar->GetWidth();
        w = ScreenWidth() - x - FHWMargin;
    }
    else
    {
        x = FHWMargin;
        w = ScreenWidth() - 2 * FHWMargin;
    }
    FPageImage->SetPos(x, BtnSize + FHWMargin);
    FPageImage->SetSize(w, ScreenHeight() - BtnSize - 2 * FHWMargin);

    EndUpdate(true);
}
//**********************************************************************
void TMainForm::LoadFile(const string& aFileName)
{
    BeginUpdate();

    FRender->LoadFile(aFileName);

    //TODO: read TOC, bookmks and etc from file
    int aLastPageIdx = 0;
    FPageNumOffset = 0;
    string aTocFileName;
    int aPos = 0;
    for (int i = aFileName.size() - 1; i >= 0; i--)
        if (aFileName[i] == '.')
        {
            aPos = i;
            break;
        }
        else if (aFileName[i] == '/')
            break;
    if (aPos == 0)
        aTocFileName = aFileName + ".toc";
    else
        aTocFileName = aFileName.substr(0, aPos) + ".toc";
    fprintf(stderr, "TOC file: %s\n", aTocFileName.c_str());
    FTocFile = fopen(aTocFileName.c_str(), "a+");
    if (FTocFile != NULL)
    {
        char* aLine = NULL;
        size_t aLen = 0;
        int aVal;
        while (getline(&aLine, &aLen, FTocFile) != -1)
        {
            if (sscanf(aLine, "Page offset\t%d", &aVal) > 0)
                FPageNumOffset = aVal;
            if (sscanf(aLine, "Last page\t%d", &aVal) > 0)
                aLastPageIdx = aVal;
            if (sscanf(aLine, "Margins\t%d", &aVal) > 0)
                FDropMargins = (aVal != 0);
            if (sscanf(aLine, "Rotate\t%d", &aVal) > 0)
                FAngle = (aVal != 0 ? 90 : 0);
            if (sscanf(aLine, "BookmarksBar\t%d", &aVal) > 0)
                FBookmarksBar->SetVisible(aVal != 0);
            if (sscanf(aLine, "View mode\t%d", &aVal) > 0)
                switch (aVal)
                {
                case VF_TEXT: FViewMode = vmT; break;
                case VF_TEXT | VF_PATH: FViewMode = vmTP; break;
                case VF_TEXT | VF_PATH | VF_IMAG: FViewMode = vmTPI; break;
                default:
                    FViewMode = vmTPI;
                }
            if (sscanf(aLine, "TOC\t%d", &aVal) > 0)
                while (aVal > 0)
                {
                    if (getline(&aLine, &aLen, FTocFile) == -1) break;

                    int aLevel, aPageNum;
                    if (sscanf(aLine, "%d\t%d\t", &aLevel, &aPageNum) != 2) break;

                    int i = strlen(aLine) - 1;
                    while (i >= 0 && aLine[i] != '\n') i--;
                    if (i >= 0)
                        aLine[i] = 0;
                    else
                        i = strlen(aLine) - 1;

                    while (i >= 0 && aLine[i] != '\t') i--;
                    if (i >= 0)
                    {
                        FTOC.push_back(new TTOCEntry(aLevel, aPageNum, &aLine[i + 1]));
                    }

                    aVal--;
                }
            if (sscanf(aLine, "Bookmarks\t%d", &aVal) > 0)
            {
                unsigned i = 0;
                while (aVal > 0 && i < FBookmarkBtns.size())
                {
                    if (getline(&aLine, &aLen, FTocFile) == -1) break;

                    int aPageNum;
                    if (sscanf(aLine, "%d", &aPageNum) != 1) break;
                    FBookmarkBtns[i]->SetPageNum(aPageNum);

                    i++;
                    aVal--;
                }
            }
        }
        free(aLine);
    }
    //*******************************************
    Layout();
    if (FTOC.empty()) FRender->LoadTOC(FTOC);    
    FPageNumOffset--;
    FDropMargins = !FDropMargins; ToggleMargins();
    ShowPage(aLastPageIdx);

    EndUpdate();

    AddLastOpen(aFileName.c_str());
}
//**********************************************************************
void TMainForm::CloseFile()
{
    FRender->CloseFile();

    if (FTocFile != NULL)
    {
        ftruncate(FTocFile->_fileno, 0);
        fprintf(FTocFile, "Page offset\t%d\n", FPageNumOffset + 1);
        fprintf(FTocFile, "Last page\t%d\n", FCache.front()->PageIdx);
        fprintf(FTocFile, "Margins\t%d\n", FDropMargins ? 1 : 0);
        fprintf(FTocFile, "Rotate\t%d\n", FAngle == 90 ? 1 : 0);
        fprintf(FTocFile, "BookmarksBar\t%d\n", FBookmarksBar->GetVisible() ? 1 : 0);
        fprintf(FTocFile, "View mode\t%d\n", FViewMode);
        fprintf(FTocFile, "TOC\t%d\n", FTOC.size());
        for (int i = 0; (unsigned)i < FTOC.size(); i++)
            fprintf(FTocFile, "%d\t%d\t%s\n", FTOC[i]->Level, FTOC[i]->PageNum, FTOC[i]->Caption.c_str());
        fprintf(FTocFile, "Bookmarks\t%d\n", FBookmarkBtns.size());
        for (int i = 0; (unsigned)i < FBookmarkBtns.size(); i++)
            fprintf(FTocFile, "%d\n", FBookmarkBtns[i]->GetPageNum());
        fclose(FTocFile);
        FTocFile = NULL;
    }

    ClrObjList(FTOC);
    ClrObjList(FCache);
}
//**********************************************************************
void TMainForm::LoadConfig()
{
    FUpdateInterval = ReadInt(FConfig, "update_interval", 0);
    FMaxCacheSize = ReadInt(FConfig, "cache_size", 10);
    FHWMargin = ReadInt(FConfig, "hw_margin", 0);
}
//**********************************************************************
void TMainForm::EditConfig()
{
    ExitMenu(false);
    OpenConfigEditor("muPDF settings", FConfig, CfgDesc, CfgHandler, NULL);
}
//**********************************************************************
TMainForm::TMainForm()
{
    FConfig = OpenConfig(CFGFILE, CfgDesc);
    //Read from config
    FUpdateInterval = 0;
    FMaxCacheSize = 10;
    FHWMargin = 0;
    LoadConfig();

    FUpdateCounter = 1;
    FAngle = 0;

    FTOCMenu = NULL;
    FTOCSize = 0;

    FViewMode = vmTPI;
    FDropMargins = true;

    FRender = new TRender();
    FPrefetch = NULL;

    bool aOSDMenu = (QueryTouchpanel() != 0 && ScreenWidth() >= 800);

    FMenu = NULL;
    FAddBkm = false;
    FDelBkm = false;

    BeginUpdate();
    //Toolbar
    FToolBar = new TPanel();
    AddControl(FToolBar);

    int aPos = 0;    

    aPos++;
    FRepeatLbl = new TLabel();
    FRepeatLbl->SetPos(aPos, 1);
    FRepeatLbl->SetSize(BtnSize, BtnSize - 2);
    FRepeatLbl->SetHAlign(ALIGN_CENTER);
    aPos += FRepeatLbl->GetWidth();
    FToolBar->AddControl(FRepeatLbl);
    FRepeatLbl->SetCaption("");
    FRepeatLbl->SetFontName("LiberationSans-Bold.ttf");
    FRepeatLbl->SetFontSize(20);

    if (aOSDMenu)
    {
        AddToolBtn(new TFirstPageBtn(), FToolBar, BtnSize, aPos, false);
        for (int i = 0; i <= 9; i++)
            AddToolBtn(new TPageNumDigitBtn(Format("%d", i)), FToolBar, BtnSize, aPos, false);
        AddToolBtn(new TLastPageBtn(), FToolBar, BtnSize, aPos, false);
    }

    aPos++;
    FPageNumLbl = new TLabel();
    FPageNumLbl->SetPos(aPos, 1);
    FPageNumLbl->SetSize(4 * BtnSize, BtnSize - 2);
    FPageNumLbl->SetHAlign(ALIGN_CENTER);
    aPos += FPageNumLbl->GetWidth();
    FToolBar->AddControl(FPageNumLbl);
    FPageNumLbl->SetCaption("");
    FPageNumLbl->SetFontName("LiberationSans-Bold.ttf");
    FPageNumLbl->SetFontSize(20);

    if (aOSDMenu)
    {
        AddToolBtn(new TPageNumGotoBtn(), FToolBar, BtnSize, aPos, false);
        AddToolBtn(new TPageNumResetBtn(), FToolBar, BtnSize, aPos, false);
        AddToolBtn(new TTOCBtn(), FToolBar, BtnSize, aPos, false);
        FMarginsBtn = AddToolBtn(new TMarginsBtn(), FToolBar, BtnSize, aPos, false);
        AddToolBtn(new TRotateBtn(), FToolBar, BtnSize, aPos, false);

        FTBtn = AddToolBtn(new TViewModeBtn(vmT), FToolBar, BtnSize, aPos, false);
        FTPBtn = AddToolBtn(new TViewModeBtn(vmTP), FToolBar, BtnSize, aPos, false);
        FTPIBtn = AddToolBtn(new TViewModeBtn(vmTPI), FToolBar, BtnSize, aPos, false);
    }
    else
    {
        FMarginsBtn = NULL;
        FTBtn = NULL;
        FTPBtn = NULL;
        FTPIBtn = NULL;
    }

    aPos++;
    FInfoLbl = new TLabel();
    FInfoLbl->SetPos(aPos, 1);
    FInfoLbl->SetHAlign(ALIGN_CENTER);
    FToolBar->AddControl(FInfoLbl);
    UpdateInfo();
    FInfoLbl->SetFontName("LiberationSans-Bold.ttf");
    FInfoLbl->SetFontSize(20);

    //Bookmark buttons bar
    FBookmarksBar = new TGridLayout(50, 25);
    AddControl(FBookmarksBar);

    int aCount = (ScreenHeight() - 5 - 2 * FHWMargin) / (50 + 5);
    for (int i = 0; i < aCount; i++)
        FBookmarkBtns.push_back(
            (TBookmarkBtn*)FBookmarksBar->AddControl(i, 0, new TBookmarkBtn()));
    FBookmarksBar->SetSel(0, 0);
    FBookmarksBar->HideSel();

    FPageImage = new TPageImageBox();
    AddControl(FPageImage);

    Layout();

    EndUpdate(false);
}
//**********************************************************************
TMainForm::~TMainForm()
{
    if (FConfig != NULL)
    {
        SaveConfig(FConfig);
        CloseConfig(FConfig);
        FConfig = NULL;
    }

    if (FPrefetch != NULL)
    {
        FPrefetch->Abort();
        FreeAndNil(FPrefetch);
    }

    CloseFile();

    FreeAndNil(FRender);
    FreeTOC(FTOCMenu, FTOCSize);
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
string TPdfViewApp::GetLogFileName()
{
    return string(SDCARDDIR) + string("/mupdfview_log.txt");
}
//**********************************************************************
void TPdfViewApp::DoInit()
{
    if (FFileName.empty())
        throw new Exception("Book file not specified");

    BigMem = new THeapAlloc();
    FMainForm = new TMainForm();
    SetActiveForm(FMainForm, false);

    FMainForm->BeginUpdate();
    FMainForm->LoadFile(FFileName);
    FMainForm->EndUpdate(false);
}
//**********************************************************************
void TPdfViewApp::DoFinit()
{
    SetActiveForm(NULL);
    FreeAndNil(FMainForm);
    FreeAndNil(BigMem);
}
//**********************************************************************
TPdfViewApp::TPdfViewApp(const string& aFileName)
{
    FFileName = aFileName;
    FMainForm = NULL;
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
void usage(int argc, char* argv[])
{
    fprintf(stderr, "Usage: %s filename", argv[0]);
}

int main (int argc, char* argv[])
{
    string aFileName;
    int aOpt;
    while ((aOpt = getopt(argc, argv, "m:")) != -1)
        switch (aOpt)
        {
        default:
            usage(argc, argv);
            exit(EXIT_FAILURE);
        }

    if (optind < argc)
    {
        aFileName = argv[optind];
    }

    SetAppInst(new TPdfViewApp(aFileName));
    Application->Run();
    delete Application;
    return 0;
}
