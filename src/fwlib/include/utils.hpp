#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <sys/mman.h>
#include <time.h>
#include <malloc.h>
#include "inkview.h"

#define tic(t) t = clock()
#define toc(t, msg) fprintf(stderr, "%s: %f ms\n", msg, (float)(clock() - t) * 1000 / CLOCKS_PER_SEC)
#define FreeAndNil(aInst) if (aInst != NULL) { delete aInst; aInst = NULL; }
#define ClrObjList(aList) \
{ \
    for (unsigned i = 0; i < aList.size(); i++) FreeAndNil(aList[i]); aList.clear(); \
}
#define Min(aVal1, aVal2) aVal1 < aVal2 ? aVal1 : aVal2
#define Max(aVal1, aVal2) aVal1 > aVal2 ? aVal1 : aVal2

using namespace std;

class TMemoryGC
{
private:
    vector<void*> FList;
public:
    void Add(void* aMemory) { FList.push_back(aMemory); }
    TMemoryGC(void* aMemory) { FList.push_back(aMemory);}
    ~TMemoryGC()
    {
        for (unsigned i = 0; i < FList.size(); i++)
        {
            free(FList[i]);
            FList[i] = NULL;
        }
        FList.clear();
    }
};

class TMemAlloc
{
public:
    virtual void* Alloc(size_t aSize) = 0;
    virtual void Free(void* p) = 0;
    virtual ~TMemAlloc() { }
};

class THeapAlloc : public TMemAlloc
{
public:
    virtual void* Alloc(size_t aSize) { return malloc(aSize); }
    virtual void Free(void* p) { free(p); }
};

class Exception
{
private:
    string FMsg;
public:
    const string& GetMessage() { return FMsg; }

    Exception(const string& aMsg) { FMsg = aMsg; }
    virtual ~Exception() { }
};

class EUIError : public Exception
{
public:
    EUIError(const string& aMsg) : Exception(aMsg) { }
};

class EAbort : public Exception
{

};

class EFileError : public Exception
{
public:
    EFileError(int aErrNo) : Exception(strerror(aErrNo)) { }
    EFileError(const string& aMsg) : Exception(aMsg) { }
};

class TMmapAlloc : public TMemAlloc
{
    struct TMmapRec
    {
        void* Ptr;
        size_t Size;
        FILE* File;

        TMmapRec(size_t aSize)
        {
            Size = aSize;
            File = tmpfile();
            if (File == NULL)
                throw new EFileError(errno);
            if (lseek(File->_fileno, Size - 1, SEEK_SET) == -1)
                throw new EFileError(errno);
            if (write(File->_fileno, "", 1) == -1)
                throw new EFileError(errno);
            Ptr = (char*)mmap(0, Size, PROT_READ | PROT_WRITE, MAP_SHARED, File->_fileno, 0);
            if (Ptr == MAP_FAILED)
                throw new EFileError(errno);
        }
        ~TMmapRec()
        {
            munmap(Ptr, Size);
            fclose(File);
        }
    };

private:
    list<TMmapRec*> FRecs;
public:
    virtual void* Alloc(size_t aSize)
    {
        FRecs.push_back(new TMmapRec(aSize));
        return FRecs.back()->Ptr;
    }
    virtual void Free(void* p)
    {
        for (list<TMmapRec*>::iterator aRec = FRecs.begin(); aRec != FRecs.end(); ++aRec)
            if ((*aRec)->Ptr == p)
            {
                delete *aRec;
                FRecs.erase(aRec);
                break;
            }
    }
    virtual ~TMmapAlloc()
    {
        for (list<TMmapRec*>::iterator aRec = FRecs.begin(); aRec != FRecs.end(); ++aRec)
            delete *aRec;
        FRecs.clear();
    }
};

void Halt(const string& aMsg);
void InitUtils(const string& aLogFileName);
void FinitUtils();
void WriteLog(const string& aMsg);
string FormatDateTime(const string& aFmt, time_t aTime);
string Format(const string& aFmt, ...);
irect MakeRect(int aLeft, int aTop, int aWidth, int aHeight);
void InflateRect(irect& aRect, int dx, int dy);
void OffsetRect(irect& aRect, int dx, int dy);
irect UnionRect(const irect& r1, const irect& r2);
bool PtInRect(const irect& aRect, int x, int y);
char* MakeStr(const string& aStr);
string ToUpper(string aStr);
string ToLower(string aStr);
int Rnd(int aMin, int aMax);
struct mallinfo MemUsage(struct mallinfo* aRefInfo = NULL, bool aDetails = false);

#endif // UTILS_H
