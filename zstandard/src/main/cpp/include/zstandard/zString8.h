//
// Created by User on 11.05.2023.
//

#pragma once

class zString8 {
    static const i32 Z_BUFFER_LENGTH = 52;
public:
    // конструкторы
    zString8() { init(); }
    zString8(zString8&& _str) noexcept { _str_buffer = _str._str_buffer; _str.init(); }
    zString8(const zString8& _str) { init(); *this = _str; }
    zString8(cstr _str, i32 _count = INT_MAX);
    zString8(char* _str, i32 _count = INT_MAX) : zString8((cstr)_str, _count) {}
    zString8(u8* _str, i32 _count = INT_MAX) : zString8((cstr)_str, _count) {}
    zString8(int ch, i32 _repeat);
    zString8(i32 value, u32 offs, bool hex, u32 radix = 0);
    // деструктор
    ~zString8() { empty(); }
    // привидение типа
    operator cstr() const { return str(); }
    // вернуть по индексу
    int operator[](i32 idx) const { return at(idx); }
    // операторы сравнения
    friend bool operator == (const zString8& str1, const zString8& str2) { return z_strcmp8(str1, str2) == 0; }
    friend bool operator < (const zString8& str1, const zString8& str2) { return z_strcmp8(str1, str2) < 0; }
    friend bool operator > (const zString8& str1, const zString8& str2) { return z_strcmp8(str1, str2) > 0; }
    friend bool operator == (const zString8& str1, cstr str2) { return z_strcmp8(str1, str2) == 0; }
    friend bool operator == (cstr str1, const zString8& str2) { return z_strcmp8(str1, str2) == 0; }
    friend bool operator != (const zString8& str1, const zString8& str2) { return !(operator == (str1, str2)); }
    friend bool operator != (const zString8& str1, cstr str2) { return !(operator == (str1, str2)); }
    friend bool operator != (cstr str1, const zString8& str2) { return !(operator == (str1, str2)); }
    // операторы присваивания
    const zString8& operator = (const zString8& _str) { return make(_str, _str.count()); }
    const zString8& operator = (zString8&& _str) noexcept { empty(); _str_buffer = _str._str_buffer; _str.init(); return *this; }
    const zString8& operator = (cstr _str) { return make(_str, z_count8(_str)); }
    // операторы контакенции
    const zString8& operator += (const zString8& str) { return add(str, str.count()); }
    const zString8& operator += (cstr str) { return add(str, z_count8(str)); }
    // дружественные операторы
    friend zString8 operator + (cstr str1, const zString8& str2) { return zString8::add(str1, z_count8(str1), str2, str2.count()); }
    friend zString8 operator + (const zString8& str1, const zString8& str2) { return zString8::add(str1, str1.count(), str2, str2.count()); }
    friend zString8 operator + (const zString8& str1, cstr str2) { return zString8::add(str1, str1.count(), str2, z_count8(str2)); }
    // методы
    char* buffer() const { return (char*)str(); }
    // зарезервировать определенный размер
    char* reserved(i32 _size) { alloc(0, _size, false); return buffer(); }
    // зафиксировать зарезервированное место
    void releaseReserved() { _str_buffer.count = z_count8(buffer()); _str_buffer.length = z_size8(buffer()); }
    // длина символа
    static i32 charSize(int ch) { return z_charLength8((cstr)&ch); }
    static i32 charSize(cstr _str) { return z_charLength8(_str); }
    // следующий символ
    static cstr next(cstr _str) { return _str + z_charLength8(_str); }
    // вернуть количество символов
    i32 count() const { return _str_buffer.count; }
    // вернуть размер строки
    i32 size() const { return _str_buffer.length; }
    int at(i32 idx) const { return z_char8(z_ptr8(str(), idx)); }
    // преобразовать из 8 в ASCII
    static int decode(cstr _str, int* length = nullptr) { return z_decode8(z_char8(_str, length)); }
    int decode(i32 idx, int* length = nullptr) const { return decode(z_ptr8(str(), idx), length); }
    // преобразовать из ASCII в 8
    static int encode(int ch) { return z_encode8(ch); }
    void set(i32 idx, cstr _str);
    void empty() { if(_str_buffer.size_buf > Z_BUFFER_LENGTH) delete _str_buffer.ptr; init(); }
    bool isEmpty() const { return count() == 0; }
    bool isNotEmpty() const { return count() != 0; }
    bool compare(cstr str) const;
    // модификация
    const zString8& appendNotEmpty(const zString8& str, cstr sep = "|") { if(isNotEmpty()) *this += sep; *this += str; return *this; }
    const zString8& slash(cstr sep = "/") { if(!endsWith(sep)) *this += sep; return *this; }
    const zString8& lower();
    const zString8& upper();
    const zString8& reverse() const;
    const zString8& replaceAmp(bool dir);
    const zString8& replace(cstr _old, cstr _new);
    const zString8& remove(cstr str);
    const zString8& remove(i32 idx, i32 _count = INT_MAX);
    const zString8& insert(i32 idx, cstr str);
    const zString8& trim() { return trim(" \t\r\n"); }
    const zString8& trim(cstr str) { trimLeft(str); return trimRight(str); }
    const zString8& trimLeft(cstr str);
    const zString8& trimRight(cstr str);
    const zString8& crop(i32 _count) { remove(0, _count); return *this; }
    const zString8& translate(cstr space);
    zString8 substr(i32 idx, i32 _count = INT_MAX) const;
    zString8 substrAfter(cstr str, cstr no = nullptr) const { return _after(str, z_count8(str), no, false); }
    zString8 substrAfterLast(cstr str, cstr no = nullptr) const { return _after(str, z_count8(str), no, true); }
    zString8 substrBefore(cstr str, cstr no = nullptr) const { return _before(str, z_count8(str), no, false); }
    zString8 substrBeforeLast(cstr str, cstr no = nullptr) const { return _before(str, z_count8(str), no, true); }
    zString8 left(i32 _count) const { return substr(0, _count); }
    zString8 right(i32 _count) const { return substr(count() - _count); }
    zArray<zString8> split(cstr delim) const { return _split(delim, false); }
    zArray<zString8> splitString() const { return _split("\n", true); }
    // поиск
    bool startsWith(cstr str) const { return indexOf(str) == 0; }
    bool endsWith(cstr str) const { return indexOfLast(str) == (count() - z_count8(str)); }
    template<typename R> int indexOf(R* str, int count) const {
        R v; i32 idx(-1);
        while(count-- > 0 && (v = str[++idx])) { if(*this == v) return idx; } return -1;
    }
    int indexOf(cstr str, i32 idx = 0) const { return _indexOf(str, idx); }
    int indexOfLast(cstr str, i32 idx = 0) const { return _indexOfLast(str, idx, z_count8(str)); }
    cstr str() const { return (_str_buffer.size_buf > Z_BUFFER_LENGTH ? _str_buffer.ptr : _str_buffer.str); }
    char* ptr(i32 idx = INT_MAX) const { return (char*)z_ptr8(buffer(), idx); }
    u8* state(u8* ptr, bool save);
protected:
#pragma pack(push, 1)
    struct STRING_BUFFER {
        union {
            char* ptr{nullptr};
            char str[Z_BUFFER_LENGTH];
        };
        // количество символов
        i32 count{0};
        // длина символов
        i32 length{0};
        // размер буфера
        i32 size_buf{Z_BUFFER_LENGTH};
    };
#pragma pack(pop)
    static zString8 add(cstr str1, i32 _count1, cstr str2, i32 _count2);
    zString8 _after(cstr str, i32 _count, cstr no, bool last) const;
    zString8 _before(cstr str, i32 _count, cstr no, bool last) const;
    int _indexOf(cstr _str, i32 idx) const { auto _idx(z_strstr8(ptr(idx), _str)); return (_idx == -1 ? -1 : _idx + idx); }
    int _indexOfLast(cstr str, i32 idx, i32 _count) const;
    void init() { memset(&_str_buffer, 0, sizeof(STRING_BUFFER)); _str_buffer.size_buf = Z_BUFFER_LENGTH; }
    zArray<zString8> _split(cstr _delim, bool _trim) const;
    char* alloc(i32 count, i32 _size, bool is_copy);
    const zString8& add(cstr str, i32 _count);
    const zString8& make(cstr str, i32 _count);
    const zString8& changeRegister(cstr _search, cstr _replace) const;
    // вернуть размер при копировании строки
    i32 sizeCopy(cstr ptr) const { return _str_buffer.size_buf - (i32)(ptr - str()); }
private:
    STRING_BUFFER _str_buffer{};
};

using czs = const zString8;

// генерация GUID
inline zString8 z_guid8() { return { z_guid().str() }; }
// двоичный буфер в base64
inline zString8 z_base64Encode8(const u8* ptr, u32 size) { return { z_base64Encode(ptr, size).str() }; }
// base64 в двоичный буфер
inline zString8 z_base64Decode8(cstr ptr, i32 size) { return { z_base64Decode(ptr, size).str() }; }
// кодировать строку
inline zString8 z_urlEncode8(cstr _buf) { return { z_urlEncode(_buf).str() }; }
// декодировать строку
inline zString8 z_urlDecode8(cstr buf) { return {z_urlDecode(buf).str() }; }
// форматирование
zString8 z_fmt8(cstr fmt, ...);
