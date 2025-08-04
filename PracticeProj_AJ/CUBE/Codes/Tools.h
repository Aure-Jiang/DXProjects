#pragma once
#include "Includes.h"
//新定义的宏用于上取整除法
#define GRS_UPPER_DIV(A,B) ((UINT)(((A)+((B)-1))/(B)))

//更简洁的向上边界对齐算法 内存管理中常用 请记住
#define GRS_UPPER(A,B) ((UINT)(((A)+((B)-1))&~(B - 1)))

#define GRS_ALLOC(sz)		::HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		::HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_REALLOC(p,sz)	::HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)

#define GRS_FREE(p)		     ::HeapFree(GetProcessHeap(),0,p)
#define GRS_SAFE_FREE(p)     if(nullptr != (p)){::HeapFree(GetProcessHeap(),0,(p));(p)=nullptr;}

#define GRS_MSIZE(p)		 ::HeapSize(GetProcessHeap(),0,p)
#define GRS_MVALID(p)        ::HeapValidate(GetProcessHeap(),0,p)

#ifndef GRS_OPEN_HEAP_LFH
//下面这个宏用于打开堆的LFH特性,以提高性能
#define GRS_OPEN_HEAP_LFH(h) \
    ULONG  ulLFH = 2;\
    HeapSetInformation((h),HeapCompatibilityInformation,&ulLFH ,sizeof(ULONG) ) ;
#endif // !GRS_OPEN_HEAP_LFH

inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
    if (path == nullptr)
    {
        throw std::exception();
    }

    DWORD size = GetModuleFileName(nullptr, path, pathSize);
    if (size == 0 || size == pathSize)
    {
        // Method failed or path was truncated.
        throw std::exception();
    }

    //wcsrchr 是 C/C++ 标准库函数，用于在宽字符串（wchar_t*）中从后向前查找指定宽字符的最后一次出现位置
    WCHAR* lastSlash = wcsrchr(path, L'\\');
    for (UINT i = 0; i < 4; i++)
    {
        lastSlash = wcsrchr(path, L'\\');
        if (lastSlash)
        {
            *(lastSlash) = L'\0';
        }
    }
    lastSlash = wcsrchr(path, L'\\');
    //跳出executablefile
    if (lastSlash)
    {
        *(lastSlash + 1) = L'\0';
    }
}

inline std::string HrToString(HRESULT hr) {
    char s_str[64] = {};
    printf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class EHandle : public std::runtime_error
{
public:
    EHandle(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}

private:
    const HRESULT m_hr;
};

inline void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) {
        throw EHandle(hr);
    }
}