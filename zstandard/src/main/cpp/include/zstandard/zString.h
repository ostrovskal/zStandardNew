//
// Created by Sergo on 10.12.2020.
//

#pragma once

#include <stdio.h>
#include <string.h>

// определение пустой строки
inline bool z_isempty(cstr str) {
    return !str || !*str;
}

// безопасное определение длины строки
inline int z_strlen(cstr str) {
    return (str ? (i32)strlen(str) : 0);
}

// поиск символа в строке
inline char* z_strchr(char* ptr, char ch) {
    char* p(nullptr);
    if(ptr) {
        while(*ptr && *ptr != ch) ptr++;
        if(*ptr == ch) p = ptr;
    }
    return p;
}

// обратный поиск символа в строке
inline char* z_strrchr(char* ptr, char ch) {
    char* p(nullptr);
    if(ptr) {
        while(*ptr) {
            while(*ptr && *ptr != ch) ptr++;
            if(*ptr == ch) p = ptr++;
        }
    }
    return p;
}

// строку в нижний регистр
inline char *z_strlwr(char *str) {
    auto p((u8*)str);
    while(*p) *p = (u8)tolower(*p), p++;
    return str;
}

// строку в верхний регистр
inline char *z_strupr(char *str) {
    auto p((u8*)str);
    while(*p) *p = (u8)toupper(*p), p++;
    return str;
}

// развернуть строку
inline char *z_strrev(char *str) {
    if(!str || !*str) return str;
    auto p1(str), p2(str + z_strlen(str) - 1);
    for(; p2 > p1; ++p1, --p2) { *p1 ^= *p2; *p2 ^= *p1; *p1 ^= *p2; }
    return str;
}

class zString {
    static const i32 Z_BUFFER_LENGTH = 24;
public:
    // конструкторы
    zString() { init(); }
    zString(zString&& str) noexcept { _str = str._str; str.init(); }
    zString(const zString& str) { init(); *this = str; }
    zString(cstr cws, int len = -1);
    zString(char* ws, int len = -1) : zString((cstr)ws, len) {}
    zString(u8* b, int len = -1) : zString((cstr)b, len) {}
    zString(char ws, i32 rep);
    zString(i32 value, u32 offs, bool hex, u32 radix = 0);
    // деструктор
    ~zString() { empty(); }
    // привидение типа
    operator cstr() const { return str(); }
    // вернуть по индексу
    char operator[](i32 idx) const { return at(idx); }
    // операторы сравнения
    friend bool operator == (const zString& str1, const zString& str2) { return (strcmp(str1.str(), str2.str()) == 0); }
    friend bool operator < (const zString& str1, const zString& str2) { return (strcmp(str1.str(), str2.str()) < 0); }
    friend bool operator > (const zString& str1, const zString& str2) { return (strcmp(str1, str2) > 0); }
    friend bool operator == (const zString& str, cstr wcs) { return strcmp(str.str(), wcs) == 0; }
    friend bool operator == (cstr wcs, const zString& str) { return strcmp(wcs, str.str()) == 0; }
    friend bool operator != (const zString& str1, const zString& str2) { return !(operator == (str1, str2)); }
    friend bool operator != (const zString& str, cstr wcs) { return !(operator == (str, wcs)); }
    friend bool operator != (cstr wcs, const zString& str) { return !(operator == (wcs, str)); }
    // операторы присваивания
    const zString& operator = (const zString& str) { return make(str, str.length()); }
    const zString& operator = (zString&& str) noexcept { empty(); _str = str._str; str.init(); return *this; }
    const zString& operator = (char ws) { return make((cstr)&ws, 1); }
    const zString& operator = (cstr wcs) { return make(wcs, z_strlen(wcs)); }
    // операторы контакенции
    const zString& operator += (const zString& str) { return add(str, str.length()); }
    const zString& operator += (char ws) { return add((cstr)&ws, 1); }
    const zString& operator += (cstr wcs) { return add(wcs, z_strlen(wcs)); }
    // дружественные операторы
    friend zString operator + (char ws, const zString& str) { return zString::add((cstr)&ws, 1, str, str.length()); }
    friend zString operator + (const char* wcs, const zString& str) { return zString::add(wcs, z_strlen(wcs), str, str.length()); }
    friend zString operator + (const zString& str1, const zString& str2) { return zString::add(str1, str1.length(), str2, str2.length()); }
    friend zString operator + (const zString& str, char ws) { return zString::add(str, str.length(), (cstr)&ws, 1); }
    friend zString operator + (const zString& str, cstr wcs) { return zString::add(str, str.length(), wcs, z_strlen(wcs)); }
    // методы
    char* buffer() const { return (char*)str(); }
    char* reserved(i32 count) { alloc(count, false); return buffer(); }
    void releaseReserved() { _str.len = (i32)z_strlen(buffer()); }
    int length() const { return _str.len; }
    char at(i32 idx) const { return (idx < length() ? str()[idx] : '\0'); }
    void set(i32 idx, char ws) { if(idx < length()) buffer()[idx] = ws; }
    void empty() { if(_str.len_buf > Z_BUFFER_LENGTH) delete _str.ptr; init(); }
    bool isEmpty() const { return length() == 0; }
    bool isNotEmpty() const { return length() != 0; }
    bool compare(cstr wcs) const { return (stricmp(str(), wcs) == 0); }
    // модификация
    const zString& slash(char ch = '/') { if(!endsWith(ch)) *this += ch; return *this; }
    const zString& lower() { z_strlwr(buffer()); return *this; }
    const zString& upper() { z_strupr(buffer()); return *this; }
    const zString& reverse() { z_strrev(buffer()); return *this; }
    const zString& replaceAmp(bool dir);
    const zString& replace(cstr _old, cstr _new);
    const zString& replace(char _old, char _new) const;
    const zString& remove(cstr wcs);
    const zString& remove(char ws);
    const zString& remove(i32 idx, i32 len = -1);
    const zString& insert(i32 idx, cstr wcs);
    const zString& insert(i32 idx, char ws);
    const zString& trim() { return trim(" \t\r\n"); }
    const zString& trim(const char* wcs) { trimLeft(wcs); return trimRight(wcs); }
    const zString& trimLeft(cstr wcs);
    const zString& trimRight(cstr wcs);
    const zString& crop(i32 len) { if(length() > len) remove(0, len); return *this; }
    zString substr(i32 idx, i32 len = -1) const;
    zString substrAfter(cstr str, cstr no = nullptr) const { return _after(str, z_strlen(str), no, false); }
    zString substrAfter(char ch, cstr no = nullptr) const { auto s(str()), f(strchr(s, ch)); return zString(f ? f + 1 : no, f ? (int)((s + length()) - f) : -1); }
    zString substrAfterLast(cstr str, cstr no = nullptr) const { return _after(str, z_strlen(str), no, true); }
    zString substrAfterLast(char ch, cstr no = nullptr) const { auto s(str()), f(strrchr(s, ch)); return zString(f ? f + 1 : no, f ? (int)((s + length()) - f) : -1); }
    zString substrBefore(cstr str, cstr no = nullptr) const { return _before(str, z_strlen(str), no, false); }
    zString substrBefore(char ch, cstr no = nullptr) const { auto s(str()), f(strchr(s, ch)); return zString(f ? s : no, f ? (int)(f - s) : -1); }
    zString substrBeforeLast(cstr str, cstr no = nullptr) const { return _before(str, z_strlen(str), no, true); }
    zString substrBeforeLast(char ch, cstr no = nullptr) const { auto s(str()), f(strrchr(s, ch)); return zString(f ? s : no, f ? (int)(f - s) : -1); }
    zString left(int idx) const { return substr(0, idx); }
    zString right(int idx) const { return substr(length() - idx); }
    zString translate(char space) const;
    zArray<zString> split(const char* delim) const;
    zArray<zString> splitString() const;
    // поиск
    bool startsWith(cstr wcs) const { return indexOf(wcs) == 0; }
    bool startsWith(char ch) const { return str()[0] == ch; }
    bool endsWith(cstr wcs) const { return indexOfLast(wcs) == (length() - z_strlen(wcs)); }
    bool endsWith(char ch) const { return str()[length() - 1] == ch; }
    int indexOf(cstr wcs[]) const;
    int indexOf(cstr wcs, int idx = 0) const { return _indexOf(wcs, idx, z_strlen(wcs)); }
    int indexOf(char ws, int idx = 0) const { return _indexOf((cstr)&ws, idx, 1); }
    int indexOfLast(cstr cws, int idx = 0) const { return _indexOfLast(cws, idx, z_strlen(cws)); }
    int indexOfLast(char ws, int idx = 0) const { return _indexOfLast((cstr)&ws, idx, 1); }
    cstr str() const { return (_str.len_buf > Z_BUFFER_LENGTH ? _str.ptr : _str.str); }
    u8* state(u8* ptr, bool save);
protected:
#pragma pack(push, 1)
    struct STRING_BUFFER {
        union {
            char* ptr{nullptr};
            char str[Z_BUFFER_LENGTH];
        };
        // длина данных
        i32 len{0};
        // длина буфера
        i32 len_buf{0};
    };
#pragma pack(pop)
    static zString add(cstr wcs1, int len1, cstr wcs2, int len2);
    zString _after(cstr str, i32 len, cstr no, bool last) const;
    zString _before(cstr str, i32 len, cstr no, bool last) const;
    int _indexOf(cstr str, i32 idx, i32 len) const;
    int _indexOfLast(cstr str, i32 idx, i32 len) const;
    void _replace(cstr _old, cstr _new);
    void init() { memset(&_str, 0, sizeof(STRING_BUFFER)); _str.len_buf = Z_BUFFER_LENGTH; }
    char* alloc(i32 size, bool is_copy);
    const zString& add(cstr wcs, i32 len);
    const zString& make(cstr wcs, i32 len);
private:
    STRING_BUFFER _str{};
};

// генерация GUID
zString z_guid();
// двоичный буфер в base64
zString z_base64Encode(const u8* ptr, u32 size);
// base64 в двоичный буфер
zString z_base64Decode(cstr ptr, i32 size);
// кодировать строку
zString z_urlEncode(cstr ptr);
// декодировать строку
zString z_urlDecode(cstr ptr);
// форматирование
zString z_fmt(cstr fmt, ...);
// преобразование из utf8 -> ascii
zString z_stringUtfToAscii(cstr _text);

