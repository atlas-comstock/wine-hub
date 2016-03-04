/*
 * Copyright 2014 Yifu Wang for ESRI
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <float.h>
#include <limits.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include "wine/test.h"
#include <process.h>

#include <locale.h>

#ifdef __i386__
#include "pshpack1.h"
struct thiscall_thunk
{
    BYTE pop_eax;    /* popl  %eax (ret addr) */
    BYTE pop_edx;    /* popl  %edx (func) */
    BYTE pop_ecx;    /* popl  %ecx (this) */
    BYTE push_eax;   /* pushl %eax */
    WORD jmp_edx;    /* jmp  *%edx */
};
#include "poppack.h"

static ULONG_PTR (WINAPI *call_thiscall_func1)( void *func, void *this );
static ULONG_PTR (WINAPI *call_thiscall_func2)( void *func, void *this, const void *a );

static void init_thiscall_thunk(void)
{
    struct thiscall_thunk *thunk = VirtualAlloc( NULL, sizeof(*thunk),
            MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    thunk->pop_eax  = 0x58;   /* popl  %eax */
    thunk->pop_edx  = 0x5a;   /* popl  %edx */
    thunk->pop_ecx  = 0x59;   /* popl  %ecx */
    thunk->push_eax = 0x50;   /* pushl %eax */
    thunk->jmp_edx  = 0xe2ff; /* jmp  *%edx */
    call_thiscall_func1 = (void *)thunk;
    call_thiscall_func2 = (void *)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)(a))

#else

#define init_thiscall_thunk()
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)

#endif /* __i386__ */

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

typedef unsigned char MSVCRT_bool;

typedef struct cs_queue
{
    struct cs_queue *next;
#if _MSVCR_VER >= 110
    BOOL free;
    int unknown;
#endif
} cs_queue;

typedef struct
{
    ULONG_PTR unk_thread_id;
    cs_queue unk_active;
#if _MSVCR_VER >= 110
    void *unknown[2];
#else
    void *unknown[1];
#endif
    cs_queue *head;
    void *tail;
} critical_section;

typedef struct
{
    critical_section *cs;
    void *unknown[3];
} critical_section_scoped_lock;

typedef struct _waiter_state{
    struct _waiter_state* prev, *next;
    HANDLE semaphore;
    LONG waiter_count, notify_count;
} waiter_state;

typedef struct
{
    critical_section_scoped_lock scoped_lock;
    waiter_state* notify_state;
    waiter_state* wait_state;
    LONG total_waiter_count;
} condition_variable;

static inline float __port_infinity(void)
{
    static const unsigned __inf_bytes = 0x7f800000;
    return *(const float *)&__inf_bytes;
}
#define INFINITY __port_infinity()

static inline float __port_nan(void)
{
    static const unsigned __nan_bytes = 0x7fc00000;
    return *(const float *)&__nan_bytes;
}
#define NAN __port_nan()

struct MSVCRT_lconv
{
    char* decimal_point;
    char* thousands_sep;
    char* grouping;
    char* int_curr_symbol;
    char* currency_symbol;
    char* mon_decimal_point;
    char* mon_thousands_sep;
    char* mon_grouping;
    char* positive_sign;
    char* negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
    wchar_t* _W_decimal_point;
    wchar_t* _W_thousands_sep;
    wchar_t* _W_int_curr_symbol;
    wchar_t* _W_currency_symbol;
    wchar_t* _W_mon_decimal_point;
    wchar_t* _W_mon_thousands_sep;
    wchar_t* _W_positive_sign;
    wchar_t* _W_negative_sign;
};

static char* (CDECL *p_setlocale)(int category, const char* locale);
static struct MSVCRT_lconv* (CDECL *p_localeconv)(void);
static size_t (CDECL *p_wcstombs_s)(size_t *ret, char* dest, size_t sz, const wchar_t* src, size_t max);
static int (CDECL *p__dsign)(double);
static int (CDECL *p__fdsign)(float);
static int (__cdecl *p__dpcomp)(double x, double y);
static wchar_t** (CDECL *p____lc_locale_name_func)(void);
static unsigned int (CDECL *p__GetConcurrency)(void);
static void* (CDECL *p__W_Gettnames)(void);
static void (CDECL *p_free)(void*);
static float (CDECL *p_strtof)(const char *, char **);
static int (CDECL *p__finite)(double);
static float (CDECL *p_wcstof)(const wchar_t*, wchar_t**);
static double (CDECL *p_remainder)(double, double);
static int* (CDECL *p_errno)(void);

/* make sure we use the correct errno */
#undef errno
#define errno (*p_errno())

static critical_section* (__thiscall *p_critical_section_ctor)(critical_section*);
static void (__thiscall *p_critical_section_dtor)(critical_section*);
static void (__thiscall *p_critical_section_lock)(critical_section*);
static void (__thiscall *p_critical_section_unlock)(critical_section*);
static critical_section* (__thiscall *p_critical_section_native_handle)(critical_section*);
static MSVCRT_bool (__thiscall *p_critical_section_try_lock)(critical_section*);
static MSVCRT_bool (__thiscall *p_critical_section_try_lock_for)(critical_section*, unsigned int);
static critical_section_scoped_lock* (__thiscall *p_critical_section_scoped_lock_ctor)
    (critical_section_scoped_lock*, critical_section *);
static void (__thiscall *p_critical_section_scoped_lock_dtor)(critical_section_scoped_lock*);
static condition_variable* (__thiscall *p_condition_variable_ctor)(condition_variable*);
static void (__thiscall *p_condition_variable_notify_one)(condition_variable*);
static void (__thiscall *p_condition_variable_notify_all)(condition_variable*);
static void (__thiscall *p_condition_variable_wait)(condition_variable*, critical_section*);
static BOOL (__thiscall *p_condition_variable_wait_for)(condition_variable*, critical_section*, unsigned int);
static void (__thiscall *p_condition_variable_dtor)(condition_variable*);

#define SETNOFAIL(x,y) x = (void*)GetProcAddress(module,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)

static BOOL init(void)
{
    HMODULE module;

    module = LoadLibraryA("msvcr120.dll");
    if (!module)
    {
        win_skip("msvcr120.dll not installed\n");
        return FALSE;
    }

    p_setlocale = (void*)GetProcAddress(module, "setlocale");
    p_localeconv = (void*)GetProcAddress(module, "localeconv");
    p_wcstombs_s = (void*)GetProcAddress(module, "wcstombs_s");
    p__dsign = (void*)GetProcAddress(module, "_dsign");
    p__fdsign = (void*)GetProcAddress(module, "_fdsign");
    p__dpcomp = (void*)GetProcAddress(module, "_dpcomp");
    p____lc_locale_name_func = (void*)GetProcAddress(module, "___lc_locale_name_func");
    p__GetConcurrency = (void*)GetProcAddress(module,"?_GetConcurrency@details@Concurrency@@YAIXZ");
    p__W_Gettnames = (void*)GetProcAddress(module, "_W_Gettnames");
    p_free = (void*)GetProcAddress(module, "free");
    p_strtof = (void*)GetProcAddress(module, "strtof");
    p__finite = (void*)GetProcAddress(module, "_finite");
    p_wcstof = (void*)GetProcAddress(module, "wcstof");
    p_remainder = (void*)GetProcAddress(module, "remainder");
    p_errno = (void*)GetProcAddress(module, "_errno");
    if(sizeof(void*) == 8) { /* 64-bit initialization */
        SET(p_critical_section_ctor,
                "??0critical_section@Concurrency@@QEAA@XZ");
        SET(p_critical_section_dtor,
                "??1critical_section@Concurrency@@QEAA@XZ");
        SET(p_critical_section_lock,
                "?lock@critical_section@Concurrency@@QEAAXXZ");
        SET(p_critical_section_unlock,
                "?unlock@critical_section@Concurrency@@QEAAXXZ");
        SET(p_critical_section_native_handle,
                "?native_handle@critical_section@Concurrency@@QEAAAEAV12@XZ");
        SET(p_critical_section_try_lock,
                "?try_lock@critical_section@Concurrency@@QEAA_NXZ");
        SET(p_critical_section_try_lock_for,
                "?try_lock_for@critical_section@Concurrency@@QEAA_NI@Z");
        SET(p_critical_section_scoped_lock_ctor,
                "??0scoped_lock@critical_section@Concurrency@@QEAA@AEAV12@@Z");
        SET(p_critical_section_scoped_lock_dtor,
                "??1scoped_lock@critical_section@Concurrency@@QEAA@XZ");
        SET(p_condition_variable_ctor,
                "??0_Condition_variable@details@Concurrency@@QEAA@XZ");
        SET(p_condition_variable_notify_one,
                "?notify_one@_Condition_variable@details@Concurrency@@QEAAXXZ");
        SET(p_condition_variable_notify_all,
                "?notify_all@_Condition_variable@details@Concurrency@@QEAAXXZ");
        SET(p_condition_variable_wait,
                "?wait@_Condition_variable@details@Concurrency@@QEAAXAEAVcritical_section@3@@Z");
        SET(p_condition_variable_wait_for,
                "?wait_for@_Condition_variable@details@Concurrency@@QEAA_NAEAVcritical_section@3@I@Z");
        SET(p_condition_variable_dtor,
                "??1_Condition_variable@details@Concurrency@@QEAA@XZ");
    } else {
#ifdef __arm__
        SET(p_critical_section_ctor,
                "??0critical_section@Concurrency@@QAA@XZ");
        SET(p_critical_section_dtor,
                "??1critical_section@Concurrency@@QAA@XZ");
        SET(p_critical_section_lock,
                "?lock@critical_section@Concurrency@@QAAXXZ");
        SET(p_critical_section_unlock,
                "?unlock@critical_section@Concurrency@@QAAXXZ");
        SET(p_critical_section_native_handle,
                "?native_handle@critical_section@Concurrency@@QAAAAV12@XZ");
        SET(p_critical_section_try_lock,
                "?try_lock@critical_section@Concurrency@@QAA_NXZ");
        SET(p_critical_section_try_lock_for,
                "?try_lock_for@critical_section@Concurrency@@QAA_NI@Z");
        SET(p_critical_section_scoped_lock_ctor,
                "??0scoped_lock@critical_section@Concurrency@@QAA@AAV12@@Z");
        SET(p_critical_section_scoped_lock_dtor,
                "??1scoped_lock@critical_section@Concurrency@@QAA@XZ");
        SET(p_condition_variable_ctor,
                "??0_Condition_variable@details@Concurrency@@QAA@XZ");
        SET(p_condition_variable_notify_one,
                "?notify_one@_Condition_variable@details@Concurrency@@QAAXXZ");
        SET(p_condition_variable_notify_all,
                "?notify_all@_Condition_variable@details@Concurrency@@QAAXXZ");
        SET(p_condition_variable_wait,
                "?wait@_Condition_variable@details@Concurrency@@QAAXAAVcritical_section@3@@Z");
        SET(p_condition_variable_wait_for,
                "?wait_for@_Condition_variable@details@Concurrency@@QAA_NAAVcritical_section@3@I@Z");
        SET(p_condition_variable_dtor,
                "??1_Condition_variable@details@Concurrency@@QAA@XZ");
#else
        SET(p_critical_section_ctor,
                "??0critical_section@Concurrency@@QAE@XZ");
        SET(p_critical_section_dtor,
                "??1critical_section@Concurrency@@QAE@XZ");
        SET(p_critical_section_lock,
                "?lock@critical_section@Concurrency@@QAEXXZ");
        SET(p_critical_section_unlock,
                "?unlock@critical_section@Concurrency@@QAEXXZ");
        SET(p_critical_section_native_handle,
                "?native_handle@critical_section@Concurrency@@QAEAAV12@XZ");
        SET(p_critical_section_try_lock,
                "?try_lock@critical_section@Concurrency@@QAE_NXZ");
        SET(p_critical_section_try_lock_for,
                "?try_lock_for@critical_section@Concurrency@@QAE_NI@Z");
        SET(p_critical_section_scoped_lock_ctor,
                "??0scoped_lock@critical_section@Concurrency@@QAE@AAV12@@Z");
        SET(p_critical_section_scoped_lock_dtor,
                "??1scoped_lock@critical_section@Concurrency@@QAE@XZ");
        SET(p_condition_variable_ctor,
                "??0_Condition_variable@details@Concurrency@@QAE@XZ");
        SET(p_condition_variable_notify_one,
                "?notify_one@_Condition_variable@details@Concurrency@@QAEXXZ");
        SET(p_condition_variable_notify_all,
                "?notify_all@_Condition_variable@details@Concurrency@@QAEXXZ");
        SET(p_condition_variable_wait,
                "?wait@_Condition_variable@details@Concurrency@@QAEXAAVcritical_section@3@@Z");
        SET(p_condition_variable_wait_for,
                "?wait_for@_Condition_variable@details@Concurrency@@QAE_NAAVcritical_section@3@I@Z");
        SET(p_condition_variable_dtor,
                "??1_Condition_variable@details@Concurrency@@QAE@XZ");
#endif
    }

    init_thiscall_thunk();
    return TRUE;
}

static void test_lconv_helper(const char *locstr)
{
    struct MSVCRT_lconv *lconv;
    char mbs[256];
    size_t i;

    if(!p_setlocale(LC_ALL, locstr))
    {
        win_skip("locale %s not available\n", locstr);
        return;
    }

    lconv = p_localeconv();

    /* If multi-byte version available, asserts that wide char version also available.
     * If wide char version can be converted to a multi-byte string , asserts that the
     * conversion result is the same as the multi-byte version.
     */
    if(strlen(lconv->decimal_point) > 0)
        ok(wcslen(lconv->_W_decimal_point) > 0, "%s: decimal_point\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_decimal_point, 256) == 0)
        ok(strcmp(mbs, lconv->decimal_point) == 0, "%s: decimal_point\n", locstr);

    if(strlen(lconv->thousands_sep) > 0)
        ok(wcslen(lconv->_W_thousands_sep) > 0, "%s: thousands_sep\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_thousands_sep, 256) == 0)
        ok(strcmp(mbs, lconv->thousands_sep) == 0, "%s: thousands_sep\n", locstr);

    if(strlen(lconv->int_curr_symbol) > 0)
        ok(wcslen(lconv->_W_int_curr_symbol) > 0, "%s: int_curr_symbol\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_int_curr_symbol, 256) == 0)
        ok(strcmp(mbs, lconv->int_curr_symbol) == 0, "%s: int_curr_symbol\n", locstr);

    if(strlen(lconv->currency_symbol) > 0)
        ok(wcslen(lconv->_W_currency_symbol) > 0, "%s: currency_symbol\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_currency_symbol, 256) == 0)
        ok(strcmp(mbs, lconv->currency_symbol) == 0, "%s: currency_symbol\n", locstr);

    if(strlen(lconv->mon_decimal_point) > 0)
        ok(wcslen(lconv->_W_mon_decimal_point) > 0, "%s: decimal_point\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_mon_decimal_point, 256) == 0)
        ok(strcmp(mbs, lconv->mon_decimal_point) == 0, "%s: decimal_point\n", locstr);

    if(strlen(lconv->positive_sign) > 0)
        ok(wcslen(lconv->_W_positive_sign) > 0, "%s: positive_sign\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_positive_sign, 256) == 0)
        ok(strcmp(mbs, lconv->positive_sign) == 0, "%s: positive_sign\n", locstr);

    if(strlen(lconv->negative_sign) > 0)
        ok(wcslen(lconv->_W_negative_sign) > 0, "%s: negative_sign\n", locstr);
    if(p_wcstombs_s(&i, mbs, 256, lconv->_W_negative_sign, 256) == 0)
        ok(strcmp(mbs, lconv->negative_sign) == 0, "%s: negative_sign\n", locstr);
}

static void test_lconv(void)
{
    int i;
    const char *locstrs[] =
    {
        "American", "Belgian", "Chinese",
        "Dutch", "English", "French",
        "German", "Hungarian", "Icelandic",
        "Japanese", "Korean", "Spanish"
    };

    for(i = 0; i < sizeof(locstrs) / sizeof(char *); i ++)
        test_lconv_helper(locstrs[i]);
}

static void test__dsign(void)
{
    int ret;

    ret = p__dsign(1);
    ok(ret == 0, "p_dsign(1) = %x\n", ret);
    ret = p__dsign(0);
    ok(ret == 0, "p_dsign(0) = %x\n", ret);
    ret = p__dsign(-1);
    ok(ret == 0x8000, "p_dsign(-1) = %x\n", ret);

    ret = p__fdsign(1);
    ok(ret == 0, "p_fdsign(1) = %x\n", ret);
    ret = p__fdsign(0);
    ok(ret == 0, "p_fdsign(0) = %x\n", ret);
    ret = p__fdsign(-1);
    ok(ret == 0x8000, "p_fdsign(-1) = %x\n", ret);
}

static void test__dpcomp(void)
{
    struct {
        double x, y;
        int ret;
    } tests[] = {
        {0, 0, 2}, {1, 1, 2}, {-1, -1, 2},
        {-2, -1, 1}, {-1, 1, 1}, {1, 2, 1},
        {1, -1, 4}, {2, 1, 4}, {-1, -2, 4},
        {NAN, NAN, 0}, {NAN, 1, 0}, {1, NAN, 0},
        {INFINITY, INFINITY, 2}, {-1, INFINITY, 1}, {1, INFINITY, 1},
    };
    int i, ret;

    for(i=0; i<sizeof(tests)/sizeof(*tests); i++) {
        ret = p__dpcomp(tests[i].x, tests[i].y);
        ok(ret == tests[i].ret, "%d) dpcomp(%f, %f) = %x\n", i, tests[i].x, tests[i].y, ret);
    }
}

static void test____lc_locale_name_func(void)
{
    struct {
        const char *locale;
        const WCHAR name[10];
        const WCHAR broken_name[10];
        BOOL todo;
    } tests[] = {
        { "American",  {'e','n',0}, {'e','n','-','U','S',0} },
        { "Belgian",   {'n','l','-','B','E',0} },
        { "Chinese",   {'z','h',0}, {'z','h','-','C','N',0}, TRUE },
        { "Dutch",     {'n','l',0}, {'n','l','-','N','L',0} },
        { "English",   {'e','n',0}, {'e','n','-','U','S',0} },
        { "French",    {'f','r',0}, {'f','r','-','F','R',0} },
        { "German",    {'d','e',0}, {'d','e','-','D','E',0} },
        { "Hungarian", {'h','u',0}, {'h','u','-','H','U',0} },
        { "Icelandic", {'i','s',0}, {'i','s','-','I','S',0} },
        { "Japanese",  {'j','a',0}, {'j','a','-','J','P',0} },
        { "Korean",    {'k','o',0}, {'k','o','-','K','R',0} }
    };
    int i, j;
    wchar_t **lc_names;

    for(i=0; i<sizeof(tests)/sizeof(*tests); i++) {
        if(!p_setlocale(LC_ALL, tests[i].locale))
            continue;

        lc_names = p____lc_locale_name_func();
        ok(lc_names[0] == NULL, "%d - lc_names[0] = %s\n", i, wine_dbgstr_w(lc_names[0]));
        todo_wine_if(tests[i].todo)
            ok(!lstrcmpW(lc_names[1], tests[i].name) || broken(!lstrcmpW(lc_names[1], tests[i].broken_name)),
                    "%d - lc_names[1] = %s\n", i, wine_dbgstr_w(lc_names[1]));

        for(j=LC_MIN+2; j<=LC_MAX; j++) {
            ok(!lstrcmpW(lc_names[1], lc_names[j]), "%d - lc_names[%d] = %s, expected %s\n",
                    i, j, wine_dbgstr_w(lc_names[j]), wine_dbgstr_w(lc_names[1]));
        }
    }

    p_setlocale(LC_ALL, "C");
    lc_names = p____lc_locale_name_func();
    ok(!lc_names[1], "___lc_locale_name_func()[1] = %s\n", wine_dbgstr_w(lc_names[1]));
}

static void test__GetConcurrency(void)
{
    SYSTEM_INFO si;
    unsigned int c;

    GetSystemInfo(&si);
    c = (*p__GetConcurrency)();
    ok(c == si.dwNumberOfProcessors, "expected %u, got %u\n", si.dwNumberOfProcessors, c);
}

static void test__W_Gettnames(void)
{
    static const char *str[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
        "Sunday", "Monday", "Tuesday", "Wednesday",
        "Thursday", "Friday", "Saturday",
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
        "January", "February", "March", "April", "May", "June", "July",
        "August", "September", "October", "November", "December",
        "AM", "PM", "M/d/yyyy"
    };

    struct {
        char *str[43];
        int  unk[2];
        wchar_t *wstr[43];
        wchar_t *locname;
        char data[1];
    } *ret;
    int i, size;
    WCHAR buf[64];

    if(!p_setlocale(LC_ALL, "english"))
        return;

    ret = p__W_Gettnames();
    size = ret->str[0]-(char*)ret;
    if(sizeof(void*) == 8)
        ok(size==0x2c0, "structure size: %x\n", size);
    else
        ok(size==0x164, "structure size: %x\n", size);

    for(i=0; i<sizeof(str)/sizeof(*str); i++) {
        ok(!strcmp(ret->str[i], str[i]), "ret->str[%d] = %s, expected %s\n",
                i, ret->str[i], str[i]);

        MultiByteToWideChar(CP_ACP, 0, str[i], strlen(str[i])+1,
                buf, sizeof(buf)/sizeof(*buf));
        ok(!lstrcmpW(ret->wstr[i], buf), "ret->wstr[%d] = %s, expected %s\n",
                i, wine_dbgstr_w(ret->wstr[i]), wine_dbgstr_w(buf));
    }
    p_free(ret);

    p_setlocale(LC_ALL, "C");
}

static void test__strtof(void)
{
    const char float1[] = "12.0";
    const char float2[] = "3.402823466e+38";          /* FLT_MAX */
    const char float3[] = "-3.402823466e+38";
    const char float4[] = "1.7976931348623158e+308";  /* DBL_MAX */

    const WCHAR twelve[] = {'1','2','.','0',0};
    const WCHAR arabic23[] = { 0x662, 0x663, 0};

    char *end;
    float f;

    f = p_strtof(float1, &end);
    ok(f == 12.0, "f = %lf\n", f);
    ok(end == float1+4, "incorrect end (%d)\n", (int)(end-float1));

    f = p_strtof(float2, &end);
    ok(f == FLT_MAX, "f = %lf\n", f);
    ok(end == float2+15, "incorrect end (%d)\n", (int)(end-float2));

    f = p_strtof(float3, &end);
    ok(f == -FLT_MAX, "f = %lf\n", f);
    ok(end == float3+16, "incorrect end (%d)\n", (int)(end-float3));

    f = p_strtof(float4, &end);
    ok(!p__finite(f), "f = %lf\n", f);
    ok(end == float4+23, "incorrect end (%d)\n", (int)(end-float4));

    f = p_strtof("inf", NULL);
    ok(f == 0, "f = %lf\n", f);

    f = p_strtof("INF", NULL);
    ok(f == 0, "f = %lf\n", f);

    f = p_strtof("1.#inf", NULL);
    ok(f == 1, "f = %lf\n", f);

    f = p_strtof("INFINITY", NULL);
    ok(f == 0, "f = %lf\n", f);

    f = p_strtof("0x12", NULL);
    ok(f == 0, "f = %lf\n", f);

    f = p_wcstof(twelve, NULL);
    ok(f == 12.0, "f = %lf\n", f);

    f = p_wcstof(arabic23, NULL);
    ok(f == 0, "f = %lf\n", f);

    if(!p_setlocale(LC_ALL, "Arabic")) {
        win_skip("Arabic locale not available\n");
        return;
    }

    f = p_wcstof(twelve, NULL);
    ok(f == 12.0, "f = %lf\n", f);

    f = p_wcstof(arabic23, NULL);
    ok(f == 0, "f = %lf\n", f);

    p_setlocale(LC_ALL, "C");
}

static void test_remainder(void)
{
    struct {
        double  x, y, r;
        errno_t e;
    } tests[] = {
        { 3.0,      2.0,       -1.0,    -1   },
        { 1.0,      1.0,        0.0,    -1   },
        { INFINITY, 0.0,        NAN,    EDOM },
        { INFINITY, 42.0,       NAN,    EDOM },
        { NAN,      0.0,        NAN,    EDOM },
        { NAN,      42.0,       NAN,    EDOM },
        { 0.0,      INFINITY,   0.0,    -1   },
        { 42.0,     INFINITY,   42.0,   -1   },
        { 0.0,      NAN,        NAN,    EDOM },
        { 42.0,     NAN,        NAN,    EDOM },
        { 1.0,      0.0,        NAN,    EDOM },
        { INFINITY, INFINITY,   NAN,    EDOM },
    };
    errno_t e;
    double r;
    int i;

    if(sizeof(void*) != 8) /* errno handling slightly different on 32-bit */
        return;

    for(i=0; i<sizeof(tests)/sizeof(*tests); i++) {
        errno = -1;
        r = p_remainder(tests[i].x, tests[i].y);
        e = errno;

        ok(tests[i].e == e, "expected errno %i, but got %i\n", tests[i].e, e);
        if(_isnan(tests[i].r))
            ok(_isnan(r), "expected NAN, but got %f\n", r);
        else
            ok(tests[i].r == r, "expected result %f, but got %f\n", tests[i].r, r);
    }
}

int enter_flag = 1;
critical_section mutex;
static unsigned __stdcall test_critical_section_lock(void *arg)
{
    critical_section *native_handle;
    native_handle = (critical_section*)call_func1(p_critical_section_native_handle, &mutex);
    ok(native_handle == &mutex, "native_handle = %p\n", native_handle);
    call_func1(p_critical_section_lock, &mutex);
    Sleep(50);
    call_func1(p_critical_section_unlock, &mutex);
    enter_flag = 0;
    return 0;
}

static unsigned __stdcall test_critical_section_try_lock(void *arg)
{
    if(call_func1(p_critical_section_try_lock, &mutex)) {
        ++enter_flag;
        Sleep(50);
        call_func1(p_critical_section_unlock, &mutex);
    }
    return 0;
}

static unsigned __stdcall test_critical_section_try_lock_for(void *arg)
{
    if(call_func2(p_critical_section_try_lock_for, &mutex, 50)) {
        Sleep(20);
        call_func1(p_critical_section_unlock, &mutex);
    }else {
        enter_flag = 0;
    }
    return 0;
}

static unsigned __stdcall test_critical_section_scoped_lock(void *arg)
{
    critical_section_scoped_lock counter_scope_lock;

    call_func2(p_critical_section_scoped_lock_ctor, &counter_scope_lock, &mutex);
    Sleep(50);
    call_func1(p_critical_section_scoped_lock_dtor, &counter_scope_lock);

    enter_flag = 0;
    return 0;
}

static void test_critical_section(void)
{
    HANDLE thread, second_thread;
    DWORD ret;

    call_func1(p_critical_section_ctor, &mutex);
    thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_lock, NULL, 0, NULL);
    second_thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_lock, NULL, 0, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    ok(second_thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    ok(enter_flag, "enter_flag expect 1, got %d\n", enter_flag);
    ret = WaitForSingleObject(thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "ret = %d\n", ret);
    ret = CloseHandle(thread);
    ok(ret, "ret = %d\n", ret);
    ret = WaitForSingleObject(second_thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "ret = %d\n", ret);
    ret = CloseHandle(second_thread);
    ok(ret, "ret = %d\n", ret);
    ok(!enter_flag, "enter_flag expect 0, got %d\n", enter_flag);
    call_func1(p_critical_section_dtor, &mutex);

    call_func1(p_critical_section_ctor, &mutex);
    thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_try_lock, NULL, 0, NULL);
    second_thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_try_lock, NULL, 0, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    ok(second_thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    ret = WaitForSingleObject(thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "ret = %d\n", ret);
    ret = CloseHandle(thread);
    ok(ret, "ret = %d\n", ret);
    ret = WaitForSingleObject(second_thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "ret = %d\n", ret);
    ret = CloseHandle(second_thread);
    ok(ret, "ret = %d\n", ret);
    ok(enter_flag, "enter_flag expect 1, got %d\n", enter_flag);
    call_func1(p_critical_section_dtor, &mutex);

    call_func1(p_critical_section_ctor, &mutex);
    thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_try_lock_for, NULL, 0, NULL);
    second_thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_try_lock_for, NULL, 0, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    ok(second_thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    ret = WaitForSingleObject(thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "ret = %d\n", ret);
    ret = CloseHandle(thread);
    ok(ret, "ret = %d\n", ret);
    ret = WaitForSingleObject(second_thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "ret = %d\n", ret);
    ret = CloseHandle(second_thread);
    ok(ret, "ret = %d\n", ret);
    ok(enter_flag, "enter_flag expect 1, got %d\n", enter_flag);
    call_func1(p_critical_section_dtor, &mutex);

    call_func1(p_critical_section_ctor, &mutex);
    thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_scoped_lock, NULL, 0, NULL);
    second_thread = (HANDLE)_beginthreadex(NULL, 0, test_critical_section_scoped_lock, NULL, 0, NULL);
    ok(thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    ok(second_thread != INVALID_HANDLE_VALUE, "_beginthread failed (%d)\n", errno);
    ret = WaitForSingleObject(thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "ret = %d\n", ret);
    ret = CloseHandle(thread);
    ok(ret, "ret = %d\n", ret);
    ret = WaitForSingleObject(second_thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "ret = %d\n", ret);
    ret = CloseHandle(second_thread);
    ok(ret, "ret = %d\n", ret);
    ok(!enter_flag, "enter_flag expect 0, got %d\n", enter_flag);
    call_func1(p_critical_section_dtor, &mutex);
}

BOOL ready = FALSE;
condition_variable cv;
static void start_test_condition_variable_notify_all(void)
{
    printf("start_test_condition_variable_notify_all start\n");
    //call_func1(p_critical_section_lock, &mutex);
    ready = TRUE;
    printf("ready is TRUE \n");
    //char c; scanf("%c", &c);
    call_func1(p_condition_variable_notify_all, &cv);
    printf("notify all  \n");
    //call_func1(p_critical_section_unlock, &mutex);
    printf("start_test_condition_variable_notify_all end\n\n\n");
}

static unsigned __stdcall test_condition_variable_notify_all(int id)
{
    //call_func1(p_critical_section_ctor, &mutex);
    printf("number %d thread start lock\n", id);
    call_func1(p_critical_section_lock, &mutex);
    printf("number %d thread \n", id);
    if(ready != TRUE) {
        printf("wait for id %d\n", id);
        call_func2(p_condition_variable_wait, &cv, &mutex);
    }
    printf("id is %d\n", id);
    printf("destroy!\n");
    call_func1(p_critical_section_unlock, &mutex);
    //call_func1(p_critical_section_dtor, &mutex);
    return 1;
}

static void test_condition_variable(void)
{
    HANDLE threads[10];
    DWORD ret;
    int i;
    printf("condition_variable_ctor!\n");
    call_func1(p_condition_variable_ctor, &cv);
    call_func1(p_critical_section_ctor, &mutex);
    char c;
    printf("condition_variable_ctor end !\n\n");
    for(i=0; i<10; ++i) {
        //scanf("%c", &c);
        printf("boot number %d \n", i);
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, test_condition_variable_notify_all, i, 0, NULL);
        //ok(threads[i] != NULL, "_beginthreadex failed (%d)\n", errno);
    }
    printf("threads end\n\n");
    start_test_condition_variable_notify_all();
    ready = TRUE;
    printf("start_test_condition_variable_notify_all end\n\n\n");
    //Sleep(9000);
    //scanf("%c", &c);
    //printf("Sleep end\n\n");
    printf("start wait object\n\n");
    for(i=0; i<10; ++i) {
        printf("WaitForSingleObject  :  %d\n", i);
        //Sleep(100);
        ret = WaitForSingleObject(threads[i], INFINITE);
        ok(ret == WAIT_OBJECT_0, "ret = %d\n", ret);
        printf("CloseHandle i :  %d\n", i);
        ret = CloseHandle(threads[i]);
        ok(ret, "ret = %d\n", ret);
        printf("wait end\n");
    }
    printf("end wait object\n\n");
    call_func1(p_condition_variable_dtor, &cv);
    call_func1(p_critical_section_dtor, &mutex);
    //Sleep(1000);
    //Sleep(1000);
}

START_TEST(msvcr120)
{
    if (!init()) return;
    test__strtof();
    test_lconv();
    test__dsign();
    test__dpcomp();
    test____lc_locale_name_func();
    test__GetConcurrency();
    test__W_Gettnames();
    test__strtof();
    test_remainder();
    test_critical_section();
    test_condition_variable();
}
