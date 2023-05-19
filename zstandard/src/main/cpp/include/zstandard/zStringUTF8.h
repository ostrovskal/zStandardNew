//
// Created by User on 11.05.2023.
//

#pragma once

class zStringUTF8 {
    static const i32 Z_BUFFER_LENGTH = 52;
public:
    // конструкторы
    zStringUTF8() { init(); }
    zStringUTF8(zStringUTF8&& _str) noexcept { _str_buffer = _str._str_buffer; _str.init(); }
    zStringUTF8(const zStringUTF8& _str) { init(); *this = _str; }
    zStringUTF8(cstr _str, i32 _count = INT_MAX);
    zStringUTF8(char* _str, i32 _count = INT_MAX) : zStringUTF8((cstr)_str, _count) {}
    zStringUTF8(u8* _str, i32 _count = INT_MAX) : zStringUTF8((cstr)_str, _count) {}
    zStringUTF8(int ch, i32 _repeat);
    zStringUTF8(i32 value, u32 offs, bool hex, u32 radix = 0);
    // деструктор
    ~zStringUTF8() { empty(); }
    // привидение типа
    operator cstr() const { return str(); }
    // вернуть по индексу
    int operator[](i32 idx) const { return at(idx); }
    // операторы сравнения
    friend bool operator == (const zStringUTF8& str1, const zStringUTF8& str2) { return z_strcmpUTF8(str1, str2) == 0; }
    friend bool operator < (const zStringUTF8& str1, const zStringUTF8& str2) { return z_strcmpUTF8(str1, str2) < 0; }
    friend bool operator > (const zStringUTF8& str1, const zStringUTF8& str2) { return z_strcmpUTF8(str1, str2) > 0; }
    friend bool operator == (const zStringUTF8& str1, cstr str2) { return z_strcmpUTF8(str1, str2) == 0; }
    friend bool operator == (cstr str1, const zStringUTF8& str2) { return z_strcmpUTF8(str1, str2) == 0; }
    friend bool operator != (const zStringUTF8& str1, const zStringUTF8& str2) { return !(operator == (str1, str2)); }
    friend bool operator != (const zStringUTF8& str1, cstr str2) { return !(operator == (str1, str2)); }
    friend bool operator != (cstr str1, const zStringUTF8& str2) { return !(operator == (str1, str2)); }
    // операторы присваивания
    const zStringUTF8& operator = (const zStringUTF8& _str) { return make(_str, _str.count()); }
    const zStringUTF8& operator = (zStringUTF8&& _str) noexcept { empty(); _str_buffer = _str._str_buffer; _str.init(); return *this; }
    const zStringUTF8& operator = (cstr _str) { return make(_str, z_countUTF8(_str)); }
    // операторы контакенции
    const zStringUTF8& operator += (const zStringUTF8& str) { return add(str, str.count()); }
    const zStringUTF8& operator += (cstr str) { return add(str, z_countUTF8(str)); }
    // дружественные операторы
    friend zStringUTF8 operator + (cstr str1, const zStringUTF8& str2) { return zStringUTF8::add(str1, z_countUTF8(str1), str2, str2.count()); }
    friend zStringUTF8 operator + (const zStringUTF8& str1, const zStringUTF8& str2) { return zStringUTF8::add(str1, str1.count(), str2, str2.count()); }
    friend zStringUTF8 operator + (const zStringUTF8& str1, cstr str2) { return zStringUTF8::add(str1, str1.count(), str2, z_countUTF8(str2)); }
    // методы
    char* buffer() const { return (char*)str(); }
    // зарезервировать определенный размер
    char* reserved(i32 _size) { alloc(0, _size, false); return buffer(); }
    // зафиксировать зарезервированное место
    void releaseReserved() { _str_buffer.count = z_countUTF8(buffer()); _str_buffer.length = z_sizeUTF8(buffer()); }
    // длина символа
    i32 charSize(int ch) const { return z_charLengthUTF8((cstr)&ch); }
    i32 charSize(cstr _str) const { return z_charLengthUTF8(_str); }
    // следующий символ
    cstr next(cstr _str) const { return _str + z_charLengthUTF8(_str); }
    // вернуть количество символов
    i32 count() const { return _str_buffer.count; }
    // вернуть размер строки
    i32 size() const { return _str_buffer.length; }
    int at(i32 idx) const { return z_charUTF8(z_ptrUTF8(str(), idx)); }
    // преобразовать из UTF8 в ASCII
    int decode(cstr _str) const { return z_decodeUTF8(z_charUTF8(_str)); }
    int decode(i32 idx) const { return decode(z_ptrUTF8(str(), idx)); }
    // преобразовать из ASCII в UTF8
    int encode(int ch) const { return z_encodeUTF8(ch); }
    void set(i32 idx, cstr _str);
    void empty() { if(_str_buffer.size_buf > Z_BUFFER_LENGTH) delete _str_buffer.ptr; init(); }
    bool isEmpty() const { return count() == 0; }
    bool isNotEmpty() const { return count() != 0; }
    bool compare(cstr str) const;
    // модификация
    const zStringUTF8& slash(cstr ch = "/") { if(!endsWith(ch)) *this += ch; return *this; }
    const zStringUTF8& lower();
    const zStringUTF8& upper();
    const zStringUTF8& reverse() const;
    const zStringUTF8& replaceAmp(bool dir);
    const zStringUTF8& replace(cstr _old, cstr _new);
    const zStringUTF8& remove(cstr str);
    const zStringUTF8& remove(i32 idx, i32 _count = INT_MAX);
    const zStringUTF8& insert(i32 idx, cstr str);
    const zStringUTF8& trim() { return trim(" \t\r\n"); }
    const zStringUTF8& trim(cstr str) { trimLeft(str); return trimRight(str); }
    const zStringUTF8& trimLeft(cstr str);
    const zStringUTF8& trimRight(cstr str);
    const zStringUTF8& crop(i32 _count) { remove(0, _count); return *this; }
    const zStringUTF8& translate(cstr space);
    zStringUTF8 substr(i32 idx, i32 _count = INT_MAX) const;
    zStringUTF8 substrAfter(cstr str, cstr no = nullptr) const { return _after(str, z_countUTF8(str), no, false); }
    zStringUTF8 substrAfterLast(cstr str, cstr no = nullptr) const { return _after(str, z_countUTF8(str), no, true); }
    zStringUTF8 substrBefore(cstr str, cstr no = nullptr) const { return _before(str, z_countUTF8(str), no, false); }
    zStringUTF8 substrBeforeLast(cstr str, cstr no = nullptr) const { return _before(str, z_countUTF8(str), no, true); }
    zStringUTF8 left(i32 _count) const { return substr(0, _count); }
    zStringUTF8 right(i32 _count) const { return substr(count() - _count); }
    zArray<zStringUTF8> split(cstr delim) const { return _split(delim, false); }
    zArray<zStringUTF8> splitString() const { return _split("\n", true); }
    // поиск
    bool startsWith(cstr str) const { return indexOf(str) == 0; }
    bool endsWith(cstr str) const { return indexOfLast(str) == (count() - z_countUTF8(str)); }
    int indexOf(cstr str[]) const;
    int indexOf(cstr str, i32 idx = 0) const { return _indexOf(str, idx); }
    int indexOfLast(cstr str, i32 idx = 0) const { return _indexOfLast(str, idx, z_countUTF8(str)); }
    cstr str() const { return (_str_buffer.size_buf > Z_BUFFER_LENGTH ? _str_buffer.ptr : _str_buffer.str); }
    char* ptr(i32 idx = INT_MAX) const { return (char*)z_ptrUTF8(buffer(), idx); }
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
        i32 size_buf{0};
    };
#pragma pack(pop)
    static zStringUTF8 add(cstr str1, i32 _count1, cstr str2, i32 _count2);
    zStringUTF8 _after(cstr str, i32 _count, cstr no, bool last) const;
    zStringUTF8 _before(cstr str, i32 _count, cstr no, bool last) const;
    int _indexOf(cstr _str, i32 idx) const { auto _idx(z_strstrUTF8(ptr(idx), _str)); return (_idx == -1 ? -1 : _idx + idx); }
    int _indexOfLast(cstr str, i32 idx, i32 _count) const;
    void init() { memset(&_str_buffer, 0, sizeof(STRING_BUFFER)); _str_buffer.size_buf = Z_BUFFER_LENGTH; }
    zArray<zStringUTF8> _split(cstr _delim, bool _trim) const;
    char* alloc(i32 count, i32 _size, bool is_copy);
    const zStringUTF8& add(cstr str, i32 _count);
    const zStringUTF8& make(cstr str, i32 _count);
    const zStringUTF8& changeRegister(cstr _search, cstr _replace) const;
    // вернуть размер при копировании строки
    i32 sizeCopy(cstr ptr) const { return _str_buffer.size_buf - (i32)(ptr - str()); }
private:
    STRING_BUFFER _str_buffer{};
};
