#include <stdio.h>
#include <png.h>
#include "inkview.h"
#include "utils.hpp"

using namespace std;

class EPNGError : public EUIError
{
public:
    EPNGError(const string& aMsg) : EUIError(aMsg) { }
};

bool ReadPNG(FILE* aFile, ibitmap** aBMP, TMemAlloc* aBigMem)
{	
    *aBMP = NULL;
    size_t aSize = 0;
    char header[8];
    fread(header, 1, 8, aFile);
    if (png_sig_cmp((png_bytep) header, 0, 8))
        throw new EPNGError("Invalid PNG signature");

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        throw new EPNGError("Failed to allocate PNG structures");

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        throw new EPNGError("Failed to allocate PNG structures");

    if (setjmp(png_jmpbuf(png_ptr)))
        throw new EPNGError("Failed to allocate PNG structures");

    png_init_io(png_ptr, aFile);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);

    int W = png_get_image_width(png_ptr, info_ptr);
    int H = png_get_image_height(png_ptr, info_ptr);
    int B = png_get_bit_depth(png_ptr, info_ptr);
    int S = png_get_rowbytes(png_ptr, info_ptr);

    png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);
    if (setjmp(png_jmpbuf(png_ptr)))
        throw new EPNGError("Failed to allocate PNG structures");

    aSize = sizeof(ibitmap) + H * S;
    char* p = (char*)aBigMem->Alloc(aSize);
    *aBMP = (ibitmap*)p;
    try
    {
        p += sizeof(ibitmap);
        png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * H);

        for (int y = 0; y < H; y++)
            row_pointers[y] = (png_byte*)(p + y * S);
        png_read_image(png_ptr, row_pointers);

        free(row_pointers);

        (*aBMP)->width = W;
        (*aBMP)->height = H;
        (*aBMP)->scanline = S;
        (*aBMP)->depth = B;
        memmove(&((*aBMP)->data), &p, sizeof(p));

        return true;
    }
    catch (...)
    {
        aBigMem->Free(*aBMP);
        throw;
    }
}

