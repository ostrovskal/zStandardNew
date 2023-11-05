
#pragma once

#include <stdint.h>
#include <fcntl.h>

#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <SDKDDKVer.h>
    #include <iostream>
    #include <windows.h>
    //#include <time.h>
    #include <mmsystem.h>
    #include <io.h>
    #include <direct.h>
    #define stricmp _stricmp
    #define lseekf _lseek
    #define unlinkf _unlink
    #define openf _open
    #define closef _close
    #define readf _read
    #define writef _write
    #define filenof _fileno
#else
#define _USE_32BIT_TIME_T
    #include <EGL/egl.h>
    #include <GLES2/gl2.h>
    #include <GLES/gl.h>
    #include <string>
    #include <utime.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <jni.h>
    #include <android/log.h>
    #include <dirent.h>
    #include <mutex>
    #include <pthread.h>
    #define stricmp strcasecmp
    #define lseekf lseek
    #define unlinkf unlink
    #define openf ::open
    #define closef ::close
    #define readf ::read
    #define writef ::write
    #define filenof fileno
    extern JavaVM* jvm;
    extern thread_local JNIEnv* jenv;
#endif

#include <curl2/curl.h>

using u64       = uint64_t;
using i64       = int64_t;
using u32       = uint32_t;
using i32       = int32_t;
using u8        = uint8_t;
using i8        = int8_t;
using u16       = uint16_t;
using i16       = int16_t;
using cstr      = const char*;
using cwstr     = const wchar_t*;
using ulg       = unsigned long;
using hfile     = int;
using lutime_t  = unsigned;

#pragma pack(push, 1)
struct HEADER_TGA {
    u8  bIdLength;		//
    u8  bColorMapType;	// тип цветовой карты ()
    u8  bType;			// тип файла ()
    u16	wCmIndex;		// тип индексов в палитре
    u16	wCmLength;		// длина палитры
    u8  bCmEntrySize;	// число бит на элемент палитры
    u16	wXorg;			//
    u16	wYorg;			//
    u16	wWidth;			// ширина
    u16	wHeight;		// высота
    u8  bBitesPerPixel;	// бит на пиксель
    u8  bFlags;			//
};

struct HEADER_WAV {
    char    RIFF[4];
    u32     ChunkSize;
    char    WAVE[4];
    char    fmt[4];
    u32     Subchunk1Size;
    u16     AudioFormat;
    u16     NumOfChan;          // число каналов
    u32     SampleRate;         // частота
    u32     bytesPerSec;        //
    u16     blockAlign;
    u16     bitsPerSample;      // бит на канал(8/16)
    char    Subchunk2ID[4];
    u32     Subchunk2Size;
};
#pragma pack(pop)

#include "zTypes.h"

// вернуть длину символа
inline i32 z_charLength8(cstr _str) {
    static u8 length[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 4 };
    return length[(((*_str) & 0b11110000) >> 4)];
}

// вернуть символ
inline int z_char8(cstr _str, i32* length = nullptr) {
    static u32 mask[] = { 0, 0xff, 0xffff, 0xffffff, 0xffffffff };
    i32 l(z_charLength8(_str)); if(length) *length = l;
    return (*(int*)_str & mask[l]);
}

// определение пустой строки
inline bool z_is8(cstr _str) {
    return _str && *_str;
}

// вернуть адрес в строке по индексу
inline cstr z_ptr8(cstr _str, i32 idx) {
    while(idx-- > 0 && z_is8(_str)) _str += z_charLength8(_str);
    return _str;
}

// безопасное определение размера строки
inline i32 z_size8(cstr _str, i32 count = INT_MAX) {
    return (i32)(z_ptr8(_str, count) - _str);
}

// безопасное определение количества символов в строке
inline i32 z_count8(cstr _str) {
    i32 count(0);
    while(z_is8(_str)) _str += z_charLength8(_str), count++;
    return count;
}

// сравнение строк
inline int z_strcmp8(cstr _str1, cstr _str2) {
    auto size1(z_size8(_str1)), size2(z_size8(_str2));
    if(size1 == size2) return strncmp(_str1, _str2, size1);
    if(size1 > size2) return 1;
    return -1;
}

// сравнение строк
inline int z_strncmp8(cstr _str1, cstr _str2, i32 _size) {
    i32 l1, l2; int ch;
    if(_size == 0) return 0;
    // берем символы из строк и сравниваем их
    while((ch = z_char8(_str1, &l1)) == z_char8(_str2, &l2)) {
        // проверка на размеры
        if(l1 != l2) break;
        // проверка на конец строки
        if(--_size <= 0 || ch == 0) return 0;
        // приращение строк
        _str1 += l1; _str2 += l2;
    }
    return 1;
}

inline int z_sizeCount8(cstr first, cstr end) {
    int count(0);
    while(first < end) first += z_charLength8(first), count++;
    return count;
}

// поиск строки в строке
inline int z_strstr8(cstr _str1, cstr _str2) {
    cstr ret(nullptr);
    if(z_is8(_str1)) ret = strstr(_str1, _str2);
    auto idx(z_sizeCount8(_str1, ret));
    return ret ? idx : -1;
}

// минимум
template <typename T> T z_min(const T& v1, const T& v2) {
    return v1 <= v2 ? v1 : v2;
}
// максимум
template <typename T> T z_max(const T& v1, const T& v2) {
    return v1 >= v2 ? v1 : v2;
}
// логгирование
void z_log(bool info, cstr file, cstr func, int line, cstr msg, ...);
// повернуть на угол
void z_rotate(cpti& c, zVertex2D* v, int count, float angle);
// масштабировать
void z_scale(cpti& c, zVertex2D* v, int count, float xzoom, float yzoom);
// определение габаритов вершин
rtf z_vertexMinMax(zVertex2D* v, int count);
// обрезка
rti z_clipRect(crti& base, crti& rect);
// проверка на разделитель
bool z_delimiter(int c);
// сохранение TGA файла
bool z_tgaSaveFile(cstr path, u8* ptr, int w, int h, int comp);
// строка ошибки OpenGL
void z_checkGlError();

#ifndef NDEBUG
	#define DLOG(...)       z_log(false, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
	#define DLOG(...)
#endif

#define ILOG(...)           z_log(true, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

// основные константы/макросы
#define deg2rad(a)          (float)(a * -Z_PI / 180.0f)
#define SAFE_DELETE(p)      if(p) delete p, (p) = nullptr
#define SAFE_A_DELETE(p)    if(p) delete[] p, (p) = nullptr
#define RTI_LOG(m, r)       DLOG("%s (%i - %i) : (%i - %i)", m, (r).x, (r).y, (r).w, (r).h)
#define RTF_LOG(m, r)       DLOG("%s (%f - %f) : (%f - %f)", m, (r).x, (r).y, (r).w, (r).h)
#define SZI_LOG(m, s)       DLOG("%s (%i - %i)", m, (s).w, (s).h)
#define SZF_LOG(m, s)       DLOG("%s (%f - %f)", m, (s).w, (s).h)
#define PTI_LOG(m, p)       DLOG("%s (%i - %i)", m, (p).x, (p).y)
#define PTF_LOG(m, p)       DLOG("%s (%f - %f)", m, (p).x, (p).y)
#define COL_LOG(m, c)       DLOG("%s r:%f g:%f b:%f a:%f", m, (c).r, (c).g, (c).b, (c).a)
#define FRTI_LOG(m, r)      DLOG("%s (%i - %i) : (%i - %i)", m, (r).x, (r).y, (r).x + (r).w, (r).y + (r).h)
#define FRTF_LOG(m, r)      DLOG("%s (%f - %f) : (%f - %f)", m, (r).x, (r).y, (r).x + (r).w, (r).y + (r).h)

enum EDITOR_LEXEMS { LD, LW, LN, LS, LO, LQ, LK, LU, L1, L2, L3, L4, L5 };

// система счистления для преобразования строк/чисел
constexpr int RADIX_DEC = 0;
constexpr int RADIX_HEX = 1;
constexpr int RADIX_BIN = 2;
constexpr int RADIX_OCT = 3;
constexpr int RADIX_DBL = 4;
constexpr int RADIX_FLT = 5;
constexpr int RADIX_BOL = 6;
constexpr int RADIX_STR = 7;

// Варианты форматирования чисел
constexpr int ZFV_CODE_LAST = 0; // "3X", "2X"
constexpr int ZFV_CODE      = 2; // "3X ", "2X "
constexpr int ZFV_PADDR16   = 4; // "5(X)", "4(#X)"
constexpr int ZFV_PADDR8    = 6; // "3(X)", "2(#X)"
constexpr int ZFV_OFFS      = 8; // "3+-X)", "2+-#X)"
constexpr int ZFV_NUM16     = 10;// "5X", "4X"
constexpr int ZFV_OPS16     = 12;// "5X", "4#X"
constexpr int ZFV_OPS8      = 14;// "3X", "2#X"
constexpr int ZFV_CVAL      = 16;// "5[X]", "4[#X]"
constexpr int ZFV_PVAL8     = 18;// "3{X}", "2{#X}"
constexpr int ZFV_PVAL16    = 20;// "5{X}", "4{#X}"
constexpr int ZFV_NUMBER    = 22;// "0X", "0#X"
constexpr int ZFV_SIMPLE    = 24;// "0X", "0X"
constexpr int ZFV_BINARY    = 26;// "%0X", "%0X"
constexpr int ZFV_DECIMAL   = 28;// "0X", "0X"

// запись/чтение различных данных
inline u16 wordLE(u8** ptr) {
    auto p(*ptr); (*ptr) += 2;
    return *(uint16_t*)(p);
}

inline void wordLE(u8** buf, u16 val) {
    *(u16*)(*buf) = val; (*buf) += 2;
}

inline u16 wordBE(u8** ptr) {
    auto p(*ptr); (*ptr) += 2;
    return ((p[0] << 8) | p[1]);
}

inline void wordBE(u8** buf, u16 val) {
    auto ptr(*buf); (*buf) += 2;
    ptr[0] = (u8)((val >> 8) & 0xFF);
    ptr[1] = (u8)((val >> 0) & 0xFF);
}

inline u32 dwordLE(u8** ptr) {
    auto p(*ptr); (*ptr) += 4;
    return (p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

inline void dwordLE(u8** buf, u32 val) {
    auto ptr(*buf); (*buf) += 4;
    *(u16*)(ptr + 0) = (u16)((val >> 0)  & 0xFFFF);
    *(u16*)(ptr + 2) = (u16)((val >> 16) & 0xFFFF);
}

inline u32 dwordBE(u8** ptr) {
    auto p(*ptr); (*ptr) += 4;
    return (p[3] | (p[2] << 8) | (p[1] << 16) | (p[0] << 24));
}

inline void dwordBE(u8** buf, u32 val) {
    auto ptr(*buf); (*buf) += 4;
    *(u16*)(ptr + 0) = (u16)((val >> 16) & 0xFFFF);
    *(u16*)(ptr + 2) = (u16)((val >> 0)  & 0xFFFF);
}

inline u64 qwordLE(u8** ptr) {
    auto p(*ptr); (*ptr) += 8;
    u64 ret(0); auto d((u8*)&ret);
    d[0] = p[0]; d[1] = p[1]; d[2] = p[2]; d[3] = p[3];
    d[4] = p[4]; d[5] = p[5]; d[6] = p[6]; d[7] = p[7];
    return ret;
}

inline void qwordLE(u8** buf, u64 val) {
    auto p(*buf); (*buf) += 8; auto d((u8*)&val);
    p[0] = d[0]; p[1] = d[1]; p[2] = d[2]; p[3] = d[3];
    p[4] = d[4]; p[5] = d[5]; p[6] = d[6]; p[7] = d[7];
}

inline u64 qwordBE(u8** ptr) {
    auto p(*ptr); (*ptr) += 8;
    u64 ret(0); auto d((u8*)&ret);
    d[0] = p[7]; d[1] = p[6]; d[2] = p[5]; d[3] = p[4];
    d[4] = p[3]; d[5] = p[2]; d[6] = p[1]; d[7] = p[0];
    return ret;
}

inline void qwordBE(u8** buf, u64 val) {
    auto p(*buf); (*buf) += 8; auto d((u8*)&val);
    p[0] = d[7]; p[1] = d[6]; p[2] = d[5]; p[3] = d[4];
    p[4] = d[3]; p[5] = d[2]; p[6] = d[1]; p[7] = d[0];
}

// из строки в hex
cstr z_toHex(int ch);
int z_fromHex(char ch);
// преобразование escape
void z_escape(char** dst, char** src);
// вычисление CRC
u16 z_crc(u8 v, u16 init = 0xcdb4);
u16 z_crc(u8* src, u32 size);
u64 z_crc64(cstr str);
u32 z_hash(cstr str, i32 len);
// ремаппинг
i32 z_remap(i32 value, i32* place);
// число в строку с формированием
char* z_fmtValue(i32 value, u32 type, bool hex = false, i32 radix = RADIX_DEC);
// число в строку
char* z_ntos(void* v, i32 r, bool sign, int size = 4, char** end = nullptr);
// строку в число
void* z_ston_(cstr s, i32 r, char** end = nullptr);
template<typename T = int> T z_ston(cstr s, i32 r, char** end = nullptr) {
    return *(T*)z_ston_(s, r, end);
}
// найти после вхождения
inline char* z_strAfter(cstr s, int f) {
    s = strchr(s, f); return const_cast<char*>(s ? s + 1 : nullptr);
}
// RLE упаковщик
u8* z_rle_compress(u8* in, int size, int& nsize);
// RLE распаковщик
u8* z_rle_decompress(u8* in, int size, int nsize);
// создание директории
inline void z_mkdir(cstr dir) {
#ifdef WIN32
    _mkdir(dir);
#else
    mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}
// дату в строку с форматированием
inline cstr z_date(cstr fmt, time_t rawtime) {
    static char buf[260];
    if(!rawtime) time(&rawtime);
    strftime(buf, 260, fmt, localtime(&rawtime));
    return buf;
}
// возвращает количество миллисекунд
inline i64 z_timeMillis() {
#ifdef WIN32
    return timeGetTime();
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (u_long)(((tv.tv_sec * 1000) + (tv.tv_usec / 1000)));
#endif
}

inline bool z_isspace(char ch) {
    return ((ch >= 0x09 && ch <= 0x0D) || ch == 0x20);
}

// пропуск пробелов
inline void z_skip_spc(char** s, int& ln) {
    auto t(*s); t--;
    while(z_isspace(*++t)) { if(*t == '\n') ln++; }
    *s = t;
}

// вычисление процентов
inline int z_percent(int value, int percent) {
    return (value * percent) / 100;
}

// заполнение памяти
inline void z_memset(u8** ptr, i32 set, u32 count) {
    if(ptr) *ptr = (u8*)(memset(*ptr, set, count)) + count;
}

inline void z_memset2(u8** ptr, u16 set, u32 count) {
    if(ptr) while(count-- > 0) wordLE(ptr, set);
}

inline void z_memset4(u8** ptr, u32 set, u32 count) {
    if(ptr) while(count-- > 0) dwordLE(ptr, set);
}

inline void z_memzero(u8* ptr, u32 count) {
    z_memset(&ptr, 0, count);
}

inline void z_memcpy(u8** dst, const void* src, u32 count) {
    if(dst && src) *dst = (u8*)(memcpy(*dst, src, count)) + count;
}

inline void z_char(char** dst, char ch, i32 count) {
    if(dst && count) {
        auto tmp(*dst);
        while(count-- > 0) *tmp++ = ch;
        *dst = tmp;
    }
}

inline void z_strcpy(char** dst, cstr src) {
    if(dst && src) {
        strcpy(*dst, src);
        *dst += strlen(src);
    }
}

// преобразовать значение в ближайшую степень двойки
inline i32 z_pow2(i32 val, bool nearest) {
    auto v(val - 1);
    v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v |= v >> 16;
    v++;
    return ((v == val || nearest) ? v >> 1 : v);
}

struct HZIP {
    HZIP(void* _unz, void* _zip) : unz(_unz), zip(_zip) { }
    void* unz{ nullptr };
    void* zip{ nullptr };
};

#define _zipClose(hz)    { hz->zip ? zipClose(hz->zip) : unzClose(hz->unz); }

int z_decode8(u32 ch);
int z_encode8(u32 ch);

#include "zZip.h"
#include "zUnzip.h"
#include "zRand.h"
#include "zArray.h"
#include "zString.h"
#include "zString8.h"
#include "zFile.h"
#include "zJSON.h"
#include "zXml.h"
#include "zEnum.h"

using zString8Array = zArray<zString8>;

#ifndef WIN32
    #include "zGRef.h"
#endif

zString8 z_cp1251ToUtf8(const zString& src);
zString z_utf8ToCp1251(const zString8& src);

template<typename T> void z_logBuffer(const zString8& tips, const T& elem, int size = 128, bool hex = false, bool show = false) {
    static int count(0);
    static T* buf(nullptr);
    if(!buf) { buf = new T[size]; memset(buf, 0, sizeof(T) * size); }
    if(count < size) buf[count++] = elem;
    if(count >= size || show) {
        zString8 tmp(tips);
        for(int i = 0 ; i < count; i++) tmp.appendNotEmpty(z_ntos(&buf[i], hex, !hex, sizeof(T)), ", ");
        DLOG(tmp.str());
        count = 0;
    }
}

zString8 z_kilo(int val);