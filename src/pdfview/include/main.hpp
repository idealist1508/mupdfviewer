#ifndef MAIN_HPP
#define MAIN_HPP

#include <deque>
#include <string>
#include <vector>
#include "inkview.h"
#include "controls.hpp"
#include "render.hpp"

#define CFGFILE CONFIGPATH "/mupdfview.cfg"

using namespace std;

static int BtnSize = 25;

enum TViewMode {
    vmTPI = VF_TEXT | VF_PATH | VF_IMAG,
    vmTP = VF_TEXT | VF_PATH,
    vmT = VF_TEXT
};

struct TCacheEntry
{
    int PageIdx;
    ibitmap* Image;
    irect CBox;
    TViewMode ViewMode;
    bool DropMargins;
    int Angle;
    int Scroll;
    string PlainText;

    TCacheEntry()
    {
        PageIdx = -1;
        Scroll = 0;
        Image = NULL;
    }
    ~TCacheEntry()
    {
        if (Image != NULL)
            Application->BigMem->Free(Image);
    }
};

class TMainForm : public TForm
{
    class TMainFormToolButton : public TToolButton
    {
    public:
        TMainForm* GetForm()
        {
            TPanel* aResult = GetParent();
            while (aResult->GetParent() != NULL)
                aResult = aResult->GetParent();

            return dynamic_cast<TMainForm*>(aResult);
        }

        TMainFormToolButton()
        {
            SetSize(BtnSize, BtnSize);
            SetColor(WHITE);
            SetFontSize(16);
            SetFontName("LiberationSans-Bold.ttf");
        }
    };

    class TAppCloseBtn : public TMainFormToolButton
    {
    public:
        virtual void Click() { Application->AskTerminate(); }
        TAppCloseBtn() { SetCaption("X"); }
    };

    class TPageNumDigitBtn : public TMainFormToolButton
    {
    private:
        string FDigit;
    public:
        virtual void Click() { GetForm()->PageNumDigit(FDigit); }
        TPageNumDigitBtn(const string& aDigit) { FDigit = aDigit; SetCaption(aDigit); }
    };

    class TPageNumFixBtn : public TMainFormToolButton
    {
    public:
        virtual void Click() { GetForm()->PageNumFix(); }
        TPageNumFixBtn() { SetCaption("<"); }
    };


    class TFirstPageBtn : public TMainFormToolButton
    {
    public:
        virtual void Click() { GetForm()->ShowPage(0); }
        TFirstPageBtn() { SetCaption("|<"); }
    };

    class TLastPageBtn : public TMainFormToolButton
    {
    public:
        virtual void Click() { GetForm()->ShowPage(GetForm()->GetPageCount() - 1); }
        TLastPageBtn() { SetCaption(">|"); }
    };

    class TPageNumGotoBtn : public TMainFormToolButton
    {
    public:
        virtual void Click() { GetForm()->PageNumGoto(); }
        TPageNumGotoBtn() { SetCaption("V"); }
    };

    class TPageNumResetBtn : public TMainFormToolButton
    {
    public:
        virtual void Click() { GetForm()->PageNumReset(); }
        TPageNumResetBtn() { SetCaption("X"); }
    };

    class TPageImageBox : public TPaintBox
    {
    protected:
        virtual bool MouseUp(int x, int y)
        {
            irect r = MakeRect(GetWidth() / 2, GetHeight() / 2, 0, 0);
            InflateRect(r, 100, 100);
            if (PtInRect(r, x, y))
            {
                ((TMainForm*)GetParent())->EnterMenu();
                return true;
            }
            return false;
        }
        virtual void Paint(irect aRect) { ((TMainForm*)GetParent())->PaintCurrPageImage(aRect); }
    };

    class TTOCBtn : public TMainFormToolButton
    {
    public:
        virtual void Click() { GetForm()->ShowTOC(); }
        TTOCBtn() { SetCaption("C"); }
    };

    class TViewModeBtn : public TMainFormToolButton
    {
    private:
        TViewMode FMode;
    public:
        virtual void Click() { GetForm()->SetViewMode(FMode); }
        TViewModeBtn(TViewMode aMode)
        {
            FMode = aMode;
            switch (FMode)
            {
            case vmTPI: SetCaption("V3"); break;
            case vmTP: SetCaption("V2"); break;
            case vmT: SetCaption("V1"); break;
            }
        }
    };

    class TMarginsBtn : public TMainFormToolButton
    {
    public:
        virtual void Click() { GetForm()->ToggleMargins(); }
        TMarginsBtn() { SetCaption("M"); }
    };

    class TRotateBtn : public TMainFormToolButton
    {
    public:
        virtual void Click() { GetForm()->ToggleRotate(); }
        TRotateBtn() { SetCaption("R"); }
    };

    class TBookmarkBtn : public TMainFormToolButton
    {
    private:
        int FPageNum;
    public:
        int GetPageNum() { return FPageNum; }
        void SetPageNum(int aValue)
        {
            FPageNum = aValue;
            BeginUpdate();
            SetCaption(FPageNum == -1 ? "" : Format("%d", FPageNum));
            SetDown(FPageNum != -1);
            EndUpdate();
        }
        virtual void Click() { GetForm()->HandleBookmark(this); }
        TBookmarkBtn()
        {
            SetRotate(true);
            SetAlign(ALIGN_LEFT);
            SetColor(WHITE);
            SetFontSize(20);
            SetPageNum(-1);
        }
    };

    class TToggleBookmarkBtn : public TMainFormToolButton
    {
    public:
        virtual void Click()
        {
            GetForm()->ToggleBookmarks();
        }
        TToggleBookmarkBtn() { SetCaption("B"); }
    };

    class TAddBookmarkBtn : public TMainFormToolButton
    {
    public:
        virtual void Click()
        {
            GetForm()->AddBookmark();
        }
        TAddBookmarkBtn() { SetCaption("B+"); }
    };

    class TDelBookmarkBtn : public TMainFormToolButton
    {
    public:
        virtual void Click()
        {
            GetForm()->DelBookmark();
        }
        TDelBookmarkBtn() { SetCaption("B-"); }
    };

    class TConfigBtn : public TMainFormToolButton
    {
    public:
        virtual void Click()
        {
            GetForm()->EditConfig();
        }
        TConfigBtn() { SetCaption("S"); }
    };

private:
    iconfig* FConfig;
    TRender* FRender;
    //info from bookmarks file
    FILE* FTocFile;
    int FPageNumOffset;
    vector<TTOCEntry*> FTOC;

    unsigned int FMaxCacheSize;
    int FUpdateInterval, FUpdateCounter;
    int FHWMargin;
    deque<TCacheEntry*> FCache;

    string FPageNumStr;
    tocentry* FTOCMenu;
    int FTOCSize;

    TViewMode FViewMode;
    bool FDropMargins;
    int FAngle;
    int FScroll;

    TPanel* FToolBar;
    TPageImageBox* FPageImage;
    TLabel* FPageNumLbl;
    TButton* FMarginsBtn;
    TButton* FTBtn;
    TButton* FTPBtn;
    TButton* FTPIBtn;
    TLabel* FInfoLbl;
    TLabel* FRepeatLbl;

    TGridLayout* FBookmarksBar;
    vector<TBookmarkBtn*> FBookmarkBtns;
    bool FAddBkm, FDelBkm;

    TPrefetch* FPrefetch;

    TGridLayout* FMenu;

    time_t FGestStart, FGestEnd;
    irect FGestPts;

    int GetPageCount() { return FRender->GetPageCount(); }

    void PageNumDigit(const string& aDigit);
    void PageNumFix();
    void PageNumGoto();
    void PageNumReset();

    void ShowTOC();
    void FreeTOC(tocentry* aMenu, int aSize);

    TButton* AddToolBtn(TButton* aBtn, TPanel* aBar, int aSize, int& aPos, bool aVert);

    bool Cached(int aPageIdx);

    void PaintCurrPageImage(irect aRect);
    TCacheEntry* LoadPage(int aPageIdx);
    void StepPage(int aDelta);
    void ScrollPage(int aDelta);
    void SetViewMode(TViewMode aMode);
    void ToggleMargins();
    void ToggleRotate();
    void CloseFile();

    void HandleBookmark(TBookmarkBtn* aSender);
    void StepBookmark(int aDelta);
    void ToggleBookmarks();
    void AddBookmark();
    void DelBookmark();
    void EnterBookmarks();
    void ExitBookmarks();

    bool InMenu() { return FMenu != NULL; }
    void EnterMenu();
    void ExitMenu(bool aRepaint = true);

protected:
    virtual void Paint(irect aRect);
    virtual bool KeyDown(int aCode);
    virtual bool KeyUp(int aCode, int aCount);
    virtual bool KeyRepeat(int aCode, int aCount);
    virtual bool MouseDown(int x, int y);
    virtual bool MouseUp(int x, int y);

public:
    void ShowPage(int aPageIdx);
    void UpdateInfo();
    void LoadFile(const string& aFileName);
    void LoadConfig();
    void EditConfig();
    void Layout();

    TMainForm();
    ~TMainForm();
};

class TPdfViewApp : public TApplication
{
private:
    string FFileName;
    TMainForm* FMainForm;
protected:
    virtual string GetLogFileName();

public:
    virtual void DoInit();
    virtual void DoFinit();

    TPdfViewApp(const string& aFileName);
};

#endif // MAIN_HPP
