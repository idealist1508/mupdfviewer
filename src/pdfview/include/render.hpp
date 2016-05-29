#ifndef RENDER_HPP
#define RENDER_HPP

#include <string>
#include <vector>
#include <pthread.h>
#include "inkview.h"
extern "C" {
#include "fitz-internal.h"
}
#include "utils.hpp"

using namespace std;

#define VF_TEXT 1
#define VF_PATH 2
#define VF_IMAG 4

struct cbox_params
{
    int filter;
    fz_rect bbox;
    fz_rect cbox;
    cbox_params(int _filter, fz_rect _bbox)
    {
        filter = _filter;
        bbox = _bbox;
        cbox = fz_empty_rect;
    }
};

fz_device* fz_new_cbox_device(fz_context* ctx, cbox_params* params);

struct pbook_params
{
    int filter;
    fz_device* canvas;
    pbook_params(int _filter, fz_device* _canvas)
    {
        filter = _filter;
        canvas = _canvas;
    }
};

fz_device* fz_new_pbook_device(fz_context* ctx, pbook_params* params);

struct TTOCEntry
{
    int Level;
    int PageNum;
    string Caption;
    TTOCEntry() { }
    TTOCEntry(int aLevel, int aPageNum, const string& aCaption)
    {
        Level = aLevel;
        PageNum = aPageNum;
        Caption = aCaption;
    }
};

class TRender
{
private:
    fz_cookie FCtl; //parser control structure
    fz_context* FCtx;
    fz_document* FDoc;
    int FBufWidth, FBufHeight;
    unsigned char* FBuf;
    fz_rect FCBox; //contents bounding rect
    fz_irect FIBox; //image rect

    int FPageCount;

    void Start() { memset(&FCtl, 0, sizeof(FCtl)); }
public:
    int GetPageCount() { return FPageCount; }

    void LoadFile(const string& aFileName);
    void CloseFile();
    void RenderPage(int aPageIdx, int W, int H, int A, int aFilter, bool aDropMargins);
    ibitmap* GetPage(TMemAlloc* aMalloc, irect *aCBox);
    void Abort() { FCtl.abort = true; }

    void LoadTOC(vector<TTOCEntry*>& aTOC);

    TRender();
    ~TRender();
};

struct TPrefetch
{
    pthread_t tid;
    TRender* Render;
    int PageIdx;
    int W;
    int H;
    int A;
    int Filter;
    bool DropMargins;
    bool Valid(int aPageIdx, int aW, int aH, int aA, int aFilter, bool aDropMargins);
    void Abort();
    void Wait();
    TPrefetch(TRender* aRender, int aPageIdx, int aW, int aH, int aA, int aFilter, bool aDropMargins);
};

#endif // RENDER_HPP
