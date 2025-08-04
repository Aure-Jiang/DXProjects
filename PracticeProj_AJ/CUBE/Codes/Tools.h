#pragma once
#include "Includes.h"
//�¶���ĺ�������ȡ������
#define GRS_UPPER_DIV(A,B) ((UINT)(((A)+((B)-1))/(B)))

//���������ϱ߽�����㷨 �ڴ�����г��� ���ס
#define GRS_UPPER(A,B) ((UINT)(((A)+((B)-1))&~(B - 1)))

#define GRS_ALLOC(sz)		::HeapAlloc(GetProcessHeap(),0,sz)
#define GRS_CALLOC(sz)		::HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz)
#define GRS_REALLOC(p,sz)	::HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,p,sz)

#define GRS_FREE(p)		     ::HeapFree(GetProcessHeap(),0,p)
#define GRS_SAFE_FREE(p)     if(nullptr != (p)){::HeapFree(GetProcessHeap(),0,(p));(p)=nullptr;}

#define GRS_MSIZE(p)		 ::HeapSize(GetProcessHeap(),0,p)
#define GRS_MVALID(p)        ::HeapValidate(GetProcessHeap(),0,p)

#ifndef GRS_OPEN_HEAP_LFH
//������������ڴ򿪶ѵ�LFH����,���������
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

    //wcsrchr �� C/C++ ��׼�⺯���������ڿ��ַ�����wchar_t*���дӺ���ǰ����ָ�����ַ������һ�γ���λ��
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
    //����executablefile
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