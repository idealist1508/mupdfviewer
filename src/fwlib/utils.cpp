#include <cassert>
#include <stdarg.h>
#include <stdio.h>
#include <ctime>
#include "inkview.h"
#include "utils.hpp"

using namespace std;

static string LogFileName;
static FILE* LogFile;

void Halt(const string& aMsg)
{
    Message(ICON_ERROR, "Fatal error", aMsg.c_str(), 5000);
    WriteLog(aMsg);
    exit(1);
}

void InitUtils(const string& aLogFileName)
{
    LogFileName = aLogFileName;
}

void FinitUtils()
{
    if (LogFile != NULL)
    {
        fclose(LogFile);
        LogFile = NULL;
    }
}

void WriteLog(const string& aMsg)
{   
    string aLogMsg = Format("%s: %s\n", FormatDateTime("%F %T", time(NULL)).c_str(), aMsg.c_str());
    write(stderr->_fileno, aLogMsg.c_str(), aLogMsg.size());

    if (LogFileName.empty()) return;
    if (LogFile == NULL)
    {
        LogFile = fopen(LogFileName.c_str(), "a");
        if (LogFile == NULL) return;
    }
    write(LogFile->_fileno, aLogMsg.c_str(), aLogMsg.size());
}

string FormatDateTime(const string &aFmt, time_t aTime)
{
    struct tm* aLocalTime = localtime(&aTime);

    int aDataLen = 0;
    size_t aBufLen = aFmt.length();
    char* aBuf = NULL;
    string aResult;
    while (aDataLen == 0)
    {
        aBuf = (char*)malloc(aBufLen + 1);
        aDataLen = strftime(aBuf, aBufLen, aFmt.c_str(), aLocalTime);
        if (aDataLen > 0)
            aResult.assign(aBuf, aDataLen);
        else
            aBufLen *= 2;
        free(aBuf);
    }

    return aResult;
}

string Format(const string& aFmt, ...) {
    int aSize = 100;
    string aResult;
    va_list p;
    while (1) {
        aResult.resize(aSize);
        va_start(p, aFmt);
        int aLen = vsnprintf((char *)aResult.c_str(), aSize, aFmt.c_str(), p);
        va_end(p);
        if (aLen > -1 && aLen < aSize) {
            aResult.resize(aLen);
            return aResult;
        }
        if (aLen > -1)
            aSize = aLen + 1;
        else
            aSize *= 2;
    }
    return aResult;
}

irect MakeRect(int aLeft, int aTop, int aWidth, int aHeight)
{
    irect aResult;
    aResult.x = aLeft;
    aResult.y = aTop;
    aResult.w = aWidth;
    aResult.h = aHeight;
    aResult.flags = 0;
    return aResult;
}

void InflateRect(irect& aRect, int dx, int dy)
{
    aRect.x -= dx;
    aRect.y -= dy;
    aRect.w += 2 * dx;
    aRect.h += 2 * dy;
}

irect UnionRect(const irect& r1, const irect& r2)
{
    irect r;
    r.x = Min(r1.x, r2.x);
    r.y = Min(r1.y, r2.y);
    r.w = Max(r1.x + r1.w, r2.x + r2.w) - r.x;
    r.h = Max(r1.y + r1.h, r2.y + r2.h) - r.y;
    return r;
}

void OffsetRect(irect& aRect, int dx, int dy)
{
    aRect.x += dx;
    aRect.y += dy;
}

bool PtInRect(const irect& aRect, int x, int y)
{
    return (x >= aRect.x) && (x <= aRect.x + aRect.w) &&
           (y >= aRect.y) && (y <= aRect.y + aRect.h);
}

char* MakeStr(const string& aStr)
{
    char* aResult = (char*)malloc(aStr.size() + 1);
    aResult[aStr.size()] = 0;
    memmove(aResult, aStr.c_str(), aStr.size());
    return aResult;
}

string ToUpper(string aStr)
{
    string aResult = aStr;
    for (unsigned i = 0; i < aResult.size(); i ++)
        aResult[i] = toupper(aResult[i]);
    return aResult;
}

string ToLower(string aStr)
{
    string aResult = aStr;
    for (unsigned i = 0; i < aResult.size(); i ++)
        aResult[i] = tolower(aResult[i]);
    return aResult;
}

int Rnd(int aMin, int aMax)
{
    return (int)(aMin + (aMax - aMin) * (rand() + 0.0) / RAND_MAX);
}

struct mallinfo MemUsage(struct mallinfo* aRefInfo, bool aDetails)
{
    struct mallinfo aInfo = mallinfo();
    if (aDetails)
    {
        if (aRefInfo != NULL)
            fprintf(stderr, "delta mmap#: %d, delta mmap size: %d, delta heap size: %d\n", aInfo.hblks - aRefInfo->hblks, aInfo.hblkhd - aRefInfo->hblkhd, aInfo.uordblks - aRefInfo->uordblks);
        else
            fprintf(stderr, "mmap#: %d, mmap size: %d, heap size: %d\n", aInfo.hblks, aInfo.hblkhd, aInfo.uordblks);
    }
    else
    {
        if (aRefInfo != NULL)
            fprintf(stderr, "delta mem size: %d\n", aInfo.hblkhd + aInfo.uordblks - (aRefInfo->hblkhd + aRefInfo->uordblks));
        else
            fprintf(stderr, "mem size: %d\n", aInfo.hblkhd + aInfo.uordblks);
    }

    return aInfo;
}
