#include "utils.hpp"
#include "controls.hpp"
#include "render.hpp"

//**********************************************************************
//**********************************************************************
//**********************************************************************
bool TPrefetch::Valid(int aPageIdx, int aW, int aH, int aA, int aFilter, bool aDropMargins)
{
    return PageIdx == aPageIdx && W == aW && H == aH && A == aA &&
            Filter == aFilter && DropMargins == aDropMargins;
}
//**********************************************************************
void TPrefetch::Abort()
{
    Render->Abort();
    pthread_join(tid, NULL);
}
//**********************************************************************
void TPrefetch::Wait()
{
    pthread_join(tid, NULL);
}
//**********************************************************************
void* DoPrefetch(void* arg)
{
    TPrefetch* aParams = (TPrefetch*)arg;
    aParams->Render->RenderPage(
        aParams->PageIdx, aParams->W, aParams->H, aParams->A,
        aParams->Filter, aParams->DropMargins);

    return NULL;
}
//**********************************************************************
TPrefetch::TPrefetch(TRender* aRender, int aPageIdx, int aW, int aH, int aA, int aFilter, bool aDropMargins)
{
    Render = aRender;
    PageIdx = aPageIdx;
    W = aW;
    H = aH;
    A = aA;
    Filter = aFilter;
    DropMargins = aDropMargins;

    pthread_create(&tid, NULL, DoPrefetch, this);
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
void TRender::LoadFile(const string& aFileName)
{
    assert(FDoc == NULL);

    FDoc = fz_open_document(FCtx, aFileName.c_str());
    FPageCount = fz_count_pages(FDoc);
}
//**********************************************************************
void TRender::CloseFile()
{
    if (FDoc != NULL)
    {
        fz_close_document(FDoc);
        FDoc = NULL;
    }
}
//**********************************************************************
void TRender::RenderPage(int aPageIdx, int W, int H, int A, int aFilter, bool aDropMargins)
{
    tic(clock_t t);
    fz_page* aPage = fz_load_page(FDoc, aPageIdx);
    toc(t, "Loading time");

    fz_rect aBounds;
    fz_bound_page(FDoc, aPage, &aBounds);
    fprintf(stderr, "Page box: %f, %f, %f, %f\n", aBounds.x0, aBounds.y0, aBounds.x1, aBounds.y1);

    //Parse page elements
    fz_display_list* aCmds = fz_new_display_list(FCtx);
    fz_device* aParser = fz_new_list_device(FCtx, aCmds);
    Start();
    tic(t);
    fz_run_page(FDoc, aPage, aParser, &fz_identity, &FCtl);
    toc(t, "Parsing time");
    fz_free_device(aParser);

    //Determine contents bounds
    cbox_params aCBParams(aFilter, aBounds);
    fz_device* aCBoxer = fz_new_cbox_device(FCtx, &aCBParams);
    Start();
    fz_run_display_list(aCmds, aCBoxer, &fz_identity, &aBounds, &FCtl);
    FCBox = aCBParams.cbox;
    fprintf(stderr, "Contents box: %f, %f, %f, %f\n", FCBox.x0, FCBox.y0, FCBox.x1, FCBox.y1);
    if (fz_is_empty_rect(&FCBox) ||
        fz_is_infinite_rect(&FCBox) ||
        FCBox.x1 - FCBox.x0 > aBounds.x1 - aBounds.x0 ||
        FCBox.y1 - FCBox.y0 > aBounds.y1 - aBounds.y0)
        FCBox = aBounds;
    fz_free_device(aCBoxer);

    fz_rect aDBox = aBounds; //Drawbox
    if (aDropMargins)
        aDBox = FCBox;
    //Setup transformation to crop margins and fit to screen
    fz_matrix aTfm;
    float aScale;
    if (A == 90)
    {
        //aScale = min(W * 1.0f / (aDBox.y1 - aDBox.y0), H  * 1.0f / (aDBox.x1 - aDBox.x0));
        aScale = H  * 1.0f / (aDBox.x1 - aDBox.x0);
        fz_scale(&aTfm, aScale, aScale);
        fz_pre_translate(&aTfm, aDBox.y1 - aDBox.y0, 0);
        fz_pre_rotate(&aTfm, A);
        fz_pre_translate(&aTfm, -aDBox.x0, -aDBox.y0);
    }
    else
    {
        aScale = min(W * 1.0f / (aDBox.x1 - aDBox.x0), H  * 1.0f / (aDBox.y1 - aDBox.y0));
        fz_scale(&aTfm, aScale, aScale);
        fz_pre_translate(&aTfm, -aDBox.x0, -aDBox.y0);
    }

    fz_transform_rect(&aDBox, &aTfm);
    fz_transform_rect(&FCBox, &aTfm);

    fz_round_rect(&FIBox, &aDBox);
    fz_translate_irect(&FIBox, -FIBox.x0, -FIBox.y0);
    fprintf(stderr, "Image box: %d, %d, %d, %d\n", FIBox.x0, FIBox.y0, FIBox.x1, FIBox.y1);

    //Check buffer size
    if (FIBox.x1 - FIBox.x0 > FBufWidth || FIBox.y1 - FIBox.y0 > FBufHeight)
    {
        fprintf(stderr, "Resizing muPDF render buffer (W: %d->%d, H: %d->%d)\n", FBufWidth, FIBox.x1 - FIBox.x0, FBufHeight, FIBox.y1 - FIBox.y0);
        Application->BigMem->Free(FBuf);
        FBufWidth = FIBox.x1 - FIBox.x0;
        FBufHeight = FIBox.y1 - FIBox.y0;
        FBuf = (unsigned char*)Application->BigMem->Alloc(FBufWidth * FBufHeight * 2);
    }

    //Render page
    fz_pixmap* aMap = fz_new_pixmap_with_bbox_and_data(FCtx, fz_device_gray, &FIBox, FBuf);
    fz_clear_pixmap_with_value(FCtx, aMap, 0xff);
    fz_device* aCanvas = fz_new_draw_device(FCtx, aMap);
    pbook_params aPBParams(aFilter, aCanvas);
    fz_device* aProxy = fz_new_pbook_device(FCtx, &aPBParams);
    Start();
    tic(t);
    fz_run_display_list(aCmds, aProxy, &aTfm, &fz_infinite_rect, &FCtl);
    toc(t, "Drawing time");

    fz_free_device(aCanvas);
    fz_free_device(aProxy);
    fz_drop_pixmap(FCtx, aMap);

    fz_free_display_list(FCtx, aCmds);
    fz_free_page(FDoc, aPage);
}
//**********************************************************************
ibitmap* TRender::GetPage(TMemAlloc* aMalloc, irect* aCBox)
{
    ibitmap* aResult;
    int S = (FIBox.x1 - FIBox.x0) / 2 + 1;
    int aSize = (FIBox.y1 - FIBox.y0) * S;
    unsigned char* p = (unsigned char*)aMalloc->Alloc(aSize + sizeof(ibitmap));
    aResult = (ibitmap*)p;
    aResult->width = (FIBox.x1 - FIBox.x0);
    aResult->height = (FIBox.y1 - FIBox.y0);
    aResult->depth = 4;
    aResult->scanline = S;
    p += sizeof(ibitmap);
    memmove(&(aResult->data), &p, sizeof(p));
    unsigned char* s = FBuf;
    unsigned char* d = aResult->data;
    tic(clock_t t);
    for (int y = 0; y < aResult->height; y++)
    {
        d = p;
        for (int x = 0; x < aResult->width; x += 2)
        {
            *d = (*s >> 4) << 4;
            s += 2;
            if (x < aResult->width - 1)
            {
                *d += *s >> 4;
                s += 2;
            }
            d++;
        }
        p += S;
    }
    toc(t, "Copy time");

    if (aCBox != NULL)
    {
        fz_irect aBuf;
        fz_round_rect(&aBuf, &FCBox);
        aCBox->x = (int)FCBox.x0;
        aCBox->y = (int)FCBox.y0;
        aCBox->w = (int)(FCBox.x1 - FCBox.x0);
        aCBox->h = (int)(FCBox.y1 - FCBox.y0);
    }

    return aResult;
}
//**********************************************************************
void DoLoadTOC(vector<TTOCEntry*>& aTOC, fz_outline* aItem, int aLevel)
{
    while (aItem != NULL)
    {
        TTOCEntry* aRec = new TTOCEntry();
        aTOC.push_back(aRec);
        aRec->Level = aLevel;
        aRec->PageNum = aItem->dest.kind == FZ_LINK_GOTO ? aItem->dest.ld.gotor.page + 1 : 0;
        aRec->Caption = aItem->title;

        if (aItem->down != NULL)
            DoLoadTOC(aTOC, aItem->down, aLevel + 1);

        aItem = aItem->next;
    }
}

void TRender::LoadTOC(vector<TTOCEntry*>& aTOC)
{
    fz_outline* aItem = fz_load_outline(FDoc);

    DoLoadTOC(aTOC, aItem, 0);

    fz_free_outline(FCtx, aItem);
}
//**********************************************************************
TRender::TRender()
{
    FCtx = fz_new_context(NULL, NULL, 16 * 1024 * 1024);
    FDoc = NULL;
    fz_set_aa_level(FCtx, 8);

    FIBox = fz_empty_irect;
    FIBox.x1 = ScreenWidth();
    FIBox.y1 = ScreenHeight();
    FBufWidth = ScreenWidth();
    FBufHeight = ScreenHeight();
    FBuf = (unsigned char*)Application->BigMem->Alloc(FBufWidth * FBufHeight * 2);
}
//**********************************************************************
TRender::~TRender()
{
    CloseFile();

    if (FBuf != NULL)
    {
        Application->BigMem->Free(FBuf);
        FBuf = NULL;
    }

    if (FCtx != NULL)
    {
        fz_free_context(FCtx);
        FCtx = NULL;
    }
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
bool fz_good_rect(fz_rect* r, fz_rect* b)
{
    return (r->x0 >= b->x0 && r->x1 <= b->x1 && r->y0 >= b->y0 && r->y1 <= b->y1) &&
            !fz_is_empty_rect(r) &&
            !fz_is_infinite_rect(r);
}

static void
fz_cbox_fill_path(fz_device *dev, fz_path *path, int even_odd, const fz_matrix *ctm,
    fz_colorspace *colorspace, float *color, float alpha)
{
    cbox_params* params = (cbox_params*)dev->user;
    if (!(params->filter & VF_PATH)) return;

    fz_rect r;
    if (fz_good_rect(fz_bound_path(dev->ctx, path, NULL, ctm, &r), &params->bbox))
        fz_union_rect(&(params->cbox), &r);
}

static void
fz_cbox_stroke_path(fz_device *dev, fz_path *path, fz_stroke_state *stroke,
    const fz_matrix *ctm, fz_colorspace *colorspace, float *color, float alpha)
{
    cbox_params* params = (cbox_params*)dev->user;
    if (!(params->filter & VF_PATH)) return;

    fz_rect r;
    if (fz_good_rect(fz_bound_path(dev->ctx, path, stroke, ctm, &r), &params->bbox))
        fz_union_rect(&(params->cbox), &r);
}

static void
fz_cbox_fill_text(fz_device *dev, fz_text *text, const fz_matrix *ctm,
    fz_colorspace *colorspace, float *color, float alpha)
{
    cbox_params* params = (cbox_params*)dev->user;
    if (!(params->filter & VF_TEXT)) return;

    fz_rect r;
    if (fz_good_rect(fz_bound_text(dev->ctx, text, ctm, &r), &params->bbox))
        fz_union_rect(&(params->cbox), &r);
}

static void
fz_cbox_stroke_text(fz_device *dev, fz_text *text, fz_stroke_state *stroke,
    const fz_matrix *ctm, fz_colorspace *colorspace, float *color, float alpha)
{
    cbox_params* params = (cbox_params*)dev->user;
    if (!(params->filter & VF_TEXT)) return;

    fz_rect r;
    if (fz_good_rect(fz_bound_text(dev->ctx, text, ctm, &r), &params->bbox))
        fz_union_rect(&(params->cbox), &r);
}

static void
fz_cbox_fill_shade(fz_device *dev, fz_shade *shade, const fz_matrix *ctm, float alpha)
{
    cbox_params* params = (cbox_params*)dev->user;
    if (!(params->filter & VF_IMAG)) return;

    fz_rect r;
    if (fz_good_rect(fz_bound_shade(dev->ctx, shade, ctm, &r), &params->bbox))
        fz_union_rect(&(params->cbox), &r);
}

static void
fz_cbox_fill_image(fz_device *dev, fz_image *image, const fz_matrix *ctm, float alpha)
{
    cbox_params* params = (cbox_params*)dev->user;
    if (!(params->filter & VF_IMAG)) return;

    fz_rect r = fz_unit_rect;
    if (fz_good_rect(fz_transform_rect(&r, ctm), &params->bbox))
        fz_union_rect(&(params->cbox), &r);
}

static void
fz_cbox_fill_image_mask(fz_device *dev, fz_image *image, const fz_matrix *ctm,
    fz_colorspace *colorspace, float *color, float alpha)
{
    cbox_params* params = (cbox_params*)dev->user;
    if (!(params->filter & VF_IMAG)) return;

    fz_rect r = fz_unit_rect;
    if (fz_good_rect(fz_transform_rect(&r, ctm), &params->bbox))
        fz_union_rect(&(params->cbox), &r);
}

fz_device *
fz_new_cbox_device(fz_context *ctx, cbox_params *params)
{
    fz_device *dev;

    dev = fz_new_device(ctx, params);

    dev->fill_path = fz_cbox_fill_path;
    dev->stroke_path = fz_cbox_stroke_path;
    dev->fill_text = fz_cbox_fill_text;
    dev->stroke_text = fz_cbox_stroke_text;
    dev->fill_shade = fz_cbox_fill_shade;
    dev->fill_image = fz_cbox_fill_image;
    dev->fill_image_mask = fz_cbox_fill_image_mask;

    params->cbox = fz_empty_rect;

    return dev;
}
//**********************************************************************
//**********************************************************************
//**********************************************************************
static float gblack = 0;
static float gwhite = 1;
static float tmp_rgb[3];
static float glevel;

void color2bw(fz_context *ctx, fz_colorspace *colorspace, float *color)
{
    colorspace->to_rgb(ctx, colorspace, color, tmp_rgb);
    fz_device_gray->from_rgb(ctx, fz_device_gray, tmp_rgb, &glevel);
    if (glevel < 0.5)
        glevel = gblack;
}

static void
fz_pbook_fill_path(fz_device *dev, fz_path *path, int even_odd, const fz_matrix *ctm,
    fz_colorspace *colorspace, float *color, float alpha)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_PATH) && params->canvas->fill_path)
    {
        params->canvas->fill_path(params->canvas, path, even_odd, ctm, colorspace, color, alpha);
    }
}

static void
fz_pbook_stroke_path(fz_device *dev, fz_path *path, fz_stroke_state *stroke, const fz_matrix *ctm,
    fz_colorspace *colorspace, float *color, float alpha)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_PATH) && params->canvas->stroke_path)
    {
        params->canvas->stroke_path(params->canvas, path, stroke, ctm, colorspace, color, alpha);
    }
}

static void
fz_pbook_clip_path(fz_device *dev, fz_path *path, const fz_rect *rect, int even_odd, const fz_matrix *ctm)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_PATH) && params->canvas->clip_path)
    {
        params->canvas->clip_path(params->canvas, path, rect, even_odd, ctm);
    }
}

static void
fz_pbook_clip_stroke_path(fz_device *dev, fz_path *path, const fz_rect *rect, fz_stroke_state *stroke, const fz_matrix *ctm)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_PATH) && params->canvas->clip_stroke_path)
    {
        params->canvas->clip_stroke_path(params->canvas, path, rect, stroke, ctm);
    }
}

static void
fz_pbook_fill_text(fz_device *dev, fz_text *text, const fz_matrix *ctm,
    fz_colorspace *colorspace, float *color, float alpha)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_TEXT) && params->canvas->fill_text)
    {
        if (params->filter == VF_TEXT)
            params->canvas->fill_text(params->canvas, text, ctm, fz_device_gray, &gblack, alpha);
        else
            params->canvas->fill_text(params->canvas, text, ctm, colorspace, color, alpha);
    }
}

static void
fz_pbook_stroke_text(fz_device *dev, fz_text *text, fz_stroke_state *stroke, const fz_matrix *ctm,
    fz_colorspace *colorspace, float *color, float alpha)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_TEXT) && params->canvas->stroke_text)
    {
        if (params->filter == VF_TEXT)
            params->canvas->stroke_text(params->canvas, text, stroke, ctm, fz_device_gray, &gblack, alpha);
        else
            params->canvas->stroke_text(params->canvas, text, stroke, ctm, colorspace, color, alpha);
    }
}

static void
fz_pbook_clip_text(fz_device *dev, fz_text *text, const fz_matrix *ctm, int accumulate)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_TEXT) && params->canvas->clip_text)
    {
        params->canvas->clip_text(params->canvas, text, ctm, accumulate);
    }
}

static void
fz_pbook_clip_stroke_text(fz_device *dev, fz_text *text, fz_stroke_state *stroke, const fz_matrix *ctm)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_TEXT) && params->canvas->clip_stroke_text)
    {
        params->canvas->clip_stroke_text(params->canvas, text, stroke, ctm);
    }
}

static void
fz_pbook_ignore_text(fz_device *dev, fz_text *text, const fz_matrix *ctm)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_TEXT) && params->canvas->ignore_text)
    {
        params->canvas->ignore_text(params->canvas, text, ctm);
    }
}

static void
fz_pbook_fill_image(fz_device *dev, fz_image *image, const fz_matrix *ctm, float alpha)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_IMAG) && params->canvas->fill_image)
    {
        params->canvas->fill_image(params->canvas, image, ctm, alpha);
    }
}

static void
fz_pbook_fill_shade(fz_device *dev, fz_shade *shade, const fz_matrix *ctm, float alpha)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_IMAG) && params->canvas->fill_shade)
    {
        params->canvas->fill_shade(params->canvas, shade, ctm, alpha);
    }
}

static void
fz_pbook_fill_image_mask(fz_device *dev, fz_image *image, const fz_matrix *ctm,
    fz_colorspace *colorspace, float *color, float alpha)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_IMAG) && params->canvas->fill_image_mask)
    {
        params->canvas->fill_image_mask(params->canvas, image, ctm, colorspace, color, alpha);
    }
}

static void
fz_pbook_clip_image_mask(fz_device *dev, fz_image *image, const fz_rect *rect, const fz_matrix *ctm)
{
    pbook_params* params = (pbook_params*)dev->user;
    if ((params->filter & VF_IMAG) && params->canvas->clip_image_mask)
    {
        params->canvas->clip_image_mask(params->canvas, image, rect, ctm);
    }
}

static void
fz_pbook_pop_clip(fz_device *dev)
{
    pbook_params* params = (pbook_params*)dev->user;
    if (params->canvas->pop_clip)
    {
        params->canvas->pop_clip(params->canvas);
    }
}

static void
fz_pbook_begin_mask(fz_device *dev, const fz_rect *bbox, int luminosity, fz_colorspace *colorspace, float *color)
{
    pbook_params* params = (pbook_params*)dev->user;
    if (params->canvas->begin_mask)
    {
        params->canvas->begin_mask(params->canvas, bbox, luminosity, colorspace, color);
    }
}

static void
fz_pbook_end_mask(fz_device *dev)
{
    pbook_params* params = (pbook_params*)dev->user;
    if (params->canvas->end_mask)
    {
        params->canvas->end_mask(params->canvas);
    }
}

static void
fz_pbook_begin_group(fz_device *dev, const fz_rect *bbox, int isolated, int knockout, int blendmode, float alpha)
{
    pbook_params* params = (pbook_params*)dev->user;
    if (params->canvas->begin_group)
    {
        params->canvas->begin_group(params->canvas, bbox, isolated, knockout, blendmode, alpha);
    }
}

static void
fz_pbook_end_group(fz_device *dev)
{
    pbook_params* params = (pbook_params*)dev->user;
    if (params->canvas->end_group)
    {
        params->canvas->end_group(params->canvas);
    }
}

static void
fz_pbook_begin_tile(fz_device *dev, const fz_rect *area, const fz_rect *view, float xstep, float ystep, const fz_matrix *ctm)
{
    pbook_params* params = (pbook_params*)dev->user;
    if (params->canvas->begin_tile)
    {
        params->canvas->begin_tile(params->canvas, area, view, xstep, ystep, ctm);
    }
}

static void
fz_pbook_end_tile(fz_device *dev)
{
    pbook_params* params = (pbook_params*)dev->user;
    if (params->canvas->end_tile)
    {
        params->canvas->end_tile(params->canvas);
    }
}

fz_device* fz_new_pbook_device(fz_context* ctx, pbook_params* params)
{
    fz_device *dev = fz_new_device(ctx, params);

    dev->fill_path = fz_pbook_fill_path;
    dev->stroke_path = fz_pbook_stroke_path;
    dev->clip_path = fz_pbook_clip_path;
    dev->clip_stroke_path = fz_pbook_clip_stroke_path;

    dev->fill_text = fz_pbook_fill_text;
    dev->stroke_text = fz_pbook_stroke_text;
    dev->clip_text = fz_pbook_clip_text;
    dev->clip_stroke_text = fz_pbook_clip_stroke_text;
    dev->ignore_text = fz_pbook_ignore_text;

    dev->fill_shade = fz_pbook_fill_shade;
    dev->fill_image = fz_pbook_fill_image;
    dev->fill_image_mask = fz_pbook_fill_image_mask;
    dev->clip_image_mask = fz_pbook_clip_image_mask;

    dev->pop_clip = fz_pbook_pop_clip;

    dev->begin_mask = fz_pbook_begin_mask;
    dev->end_mask = fz_pbook_end_mask;
    dev->begin_group = fz_pbook_begin_group;
    dev->end_group = fz_pbook_end_group;

    dev->begin_tile = fz_pbook_begin_tile;
    dev->end_tile = fz_pbook_end_tile;

    return dev;
}
