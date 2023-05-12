//
// Created by User on 11.05.2023.
//

#include "zstandard/zstandard.h"
#include "zstandard/zStringUTF8.h"

static cstr hex("0123456789ABCDEF-");

char* zStringUTF8::alloc(i32 _count, bool is_copy) {
    if(_count) {
        auto _old(buffer()); auto size((_count + 1) * 4);
        if(size > _str_buffer.size) {
            // выделим память под новый буфер
            auto _new(new char[size]); memset(_new, 0, size);
            // скопировать старый, если необходимо
            if(is_copy && isNotEmpty()) memcpy(_new, _old, _str_buffer.size);
            // стереть старый
            empty();
            // инициализируем новый
            _str_buffer.size= size;
            _str_buffer.ptr	= _new;
            _old            = _new;
        }
        _str_buffer.count   = _count;
        return _old;
    }
    empty();
    return _str_buffer.str;
}

zStringUTF8::zStringUTF8(i32 value, u32 offs, bool _hex, u32 radix) {
    init(); *this = z_fmtValue(value, offs, _hex, radix);
}

zStringUTF8::zStringUTF8(cstr _str, i32 _count) {
    init(); auto count(z_countUTF8(_str));
    make(_str, (_count < 0 || _count >= count) ? count : _count);
}

zStringUTF8::zStringUTF8(int ch, i32 _count) {
    init(); auto _str(alloc(_count, false));
    auto l(z_charLengthUTF8((cstr)&ch));
    while(_count-- > 0) *(int*)_str = ch, _str += l;
}

const zStringUTF8& zStringUTF8::add(cstr _str, i32 _count) {
    auto size(z_sizeUTF8(_str, (i32)_count));
    alloc(length() + _count, true);
    memcpy((void*)z_ptrUTF8(str(), (i32)_count), _str, size);
    buffer()[size] = 0; return *this;
}

const zStringUTF8& zStringUTF8::make(cstr _str, i32 _count) {
    auto size(z_sizeUTF8(_str, _count)); memcpy(alloc(_count, false), _str, size);
    buffer()[size] = 0; return *this;
}

zStringUTF8 zStringUTF8::substr(i32 idx, i32 _count) const {
    auto count(length());
    if(idx <= count) {
        if(_count < 0) _count = count;
        if((idx + _count) > count) _count = (count - idx);
    } else idx = 0, _count = 0;
    return { z_ptrUTF8(str(), idx), _count };
}

zStringUTF8 zStringUTF8::_after(cstr str, i32 _count, cstr no, bool last) const {
    auto idx(last ? _indexOfLast(str, 0, _count) : _indexOf(str, 0));
    return (idx != -1 ? substr(idx + (i32)_count) : zStringUTF8(no));
}

zStringUTF8 zStringUTF8::_before(cstr str, i32 _count, cstr no, bool last) const {
    auto idx(last ? _indexOfLast(str, 0, _count) : _indexOf(str, 0));
    return (idx != -1 ? left(idx) : zStringUTF8(no));
}

i32 zStringUTF8::indexOf(cstr str[]) const {
    cstr v; i32 idx(-1);
    while((v = str[++idx])) {
        if(*this == v) return idx;
    }
    return -1;
}

int zStringUTF8::_indexOfLast(cstr str, i32 idx, i32 _count) const {
    int last(-1), i;
    while((i = _indexOf(str, idx)) != -1) last = i, idx = (i32)(i + _count);
    return last;
}

const zStringUTF8& zStringUTF8::reverse() {
    auto size(z_sizeUTF8(str())), count(length());
    auto buf(new char[size + 1]), _buf(buf), _str(buffer()); buf[size] = 0;
    while(count-- > 0) {
        auto ptr(z_ptrUTF8(_str, count));
        auto sz(z_charLengthUTF8(ptr));
        memcpy(_buf, ptr, sz); _buf += sz;
    }
    *this = buf; delete [] buf;
    return *this;
}

void zStringUTF8::set(i32 idx, cstr _str) const {
    auto ptr(z_ptrUTF8(str(), idx));
    auto _size1(z_charLengthUTF8(ptr)), _size2(z_sizeUTF8(_str));
    memmove((char*)ptr + _size2, ptr + _size1, _str_buffer.size - _size1);
    memcpy((char*)ptr, _str, _size2);
}

zStringUTF8 zStringUTF8::add(cstr str1, i32 _count1, cstr str2, i32 _count2) {
    auto size1(z_sizeUTF8(str1, _count1)), size2(z_sizeUTF8(str2, _count2));
    auto ptr(new char[size1 + size2 + 1]); ptr[size1 + size2] = 0;
    memcpy((char*)memcpy(ptr, str1, size1) + size1, str2, size2);
    zStringUTF8 ret(ptr); delete [] ptr;
    return ret;
}

const zStringUTF8& zStringUTF8::replace(cstr _old, cstr _new) {
    auto sizeNew(z_sizeUTF8(_new)), sizeOld(z_sizeUTF8(_old)), count(0);
    auto _str(buffer()), _buf(_str);
    // подсчитать новый размер
    while((_buf = strstr(_buf, _old))) count++, _buf += sizeOld;
    auto size(z_sizeUTF8(str()) - (sizeOld - sizeNew) * count);
    auto _tmp(new char[size + 1]), tmp(_tmp); tmp[size] = 0;
    // непосредственно замена
    while((_buf = strstr(_str, _old))) {
        // копируем разницу
        auto sz(_buf - _str);
        memcpy(_tmp, _str, sz); _tmp += sz;
        // копируем новый
        memcpy(_tmp, _new, sizeNew); _tmp += sizeNew;
        // сдвигаем указатель
        _str = _buf + sizeOld;
    }
    memcpy(_tmp, _str, size - (_tmp - tmp));
    *this = tmp; delete [] tmp;
    return *this;
}

const zStringUTF8& zStringUTF8::remove(cstr _str) {
    auto _count(z_countUTF8(_str)), _size(z_sizeUTF8(_str)); auto f(buffer()), _f(f);
    while((f = strstr(f, _str))) {
        _str_buffer.count -= _count;
        memcpy(f, f + _size, _str_buffer.size - (i32)((f + _size) - _f));
    }
    return *this;
}

const zStringUTF8& zStringUTF8::remove(i32 idx, i32 _count) {
    auto count(length());
    if(idx < count) {
        if(_count < 0) _count = count;
        if((idx + _count) > count) _count = (count - idx);
        auto ptr((char*)z_ptrUTF8(str(), idx + _count));
        memcpy((char*)z_ptrUTF8(str(), idx), ptr, _str_buffer.size - (i32)(ptr - str()));
        _str_buffer.count -= _count;
    }
    return *this;
}

const zStringUTF8& zStringUTF8::insert(i32 idx, cstr _str) {
    auto count(length()), _size(z_sizeUTF8(_str));
    if(idx > count) idx = count;
    if(idx <= count && alloc(count + z_countUTF8(_str), true)) {
        auto _buf((char*)z_ptrUTF8(str(), idx));
        memmove(_buf + _size, _buf, _str_buffer.size - (i32)(_buf - str()));
        memcpy(_buf, _str, _size);
    }
    return *this;
}

static cstr is_chars(cstr str, cstr wcs, i32 count) {
    i32 l;
    auto ch1(z_charUTF8(str));
    for(int i = 0; i < count; i++) {
        auto ch2(z_charUTF8(wcs, &l));
        if(ch1 == ch2) return wcs;
        wcs += l;
    }
    return nullptr;
}

const zStringUTF8& zStringUTF8::trimLeft(cstr _str) {
    auto _count(z_countUTF8(_str)); auto _buf(buffer()), _tmp(_buf);
    while(is_chars(_tmp, _str, _count)) _str_buffer.count--, _tmp += z_charLengthUTF8(_tmp);
    memcpy(_buf, _tmp, _str_buffer.size - (_tmp - _buf));
    return *this;
}

const zStringUTF8& zStringUTF8::trimRight(cstr _str) {
    auto _count(z_countUTF8(_str));
    while(is_chars(z_ptrUTF8(str(), length() - 1), _str, _count)) _str_buffer.count--;
    *(char*)z_ptrUTF8(str(), length()) = 0; return *this;
}

zArray<zStringUTF8> zStringUTF8::splitString() const {
    zArray<zStringUTF8> arr;
    int pos(0);
    while(pos < length()) {
        auto npos(indexOf("\n", pos));
        auto str(substr(pos, (npos == -1 ? length() : npos) - pos));
        arr += str.trimRight(" \r\t");
        if(npos == -1) break;
        pos = npos + 1;
    }
    return arr;
}

zArray<zStringUTF8> zStringUTF8::split(cstr delim) const {
    zArray<zStringUTF8> arr;
    int pos(0), ldelim(z_countUTF8(delim));
    while(pos < length()) {
        auto npos(indexOf(delim, pos));
        arr += substr(pos, (npos == -1 ? length() : npos) - pos);
        if(npos == -1) break;
        pos = npos + ldelim;
    }
    return arr;

}

u8* zStringUTF8::state(u8 *ptr, bool save) {
    if(save) {
        z_memcpy(&ptr, str(), z_sizeUTF8(str()) + 1);
    } else {
        *this = ptr;
        ptr += z_sizeUTF8(str()) + 1;
    }
    return ptr;
}

const zStringUTF8 &zStringUTF8::replaceAmp(bool dir) {
    if(dir) {
        replace("&lt;", "<");
        replace("&gt;", ">");
        replace("&quot;", "\"");
        replace("&amp;", "&");
        replace("&nbsp;", " ");
    } else {
        replace("<", "&lt;");
        replace(">", "&gt;");
        replace("\"", "&quot;");
        replace("&", "&amp;");
    }
    return *this;
}

const zStringUTF8& zStringUTF8::translate(cstr space) {
    static cstr russian     = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя !@#$%^&*():;[]{}/\\\"\'?~`+-<>=,.№|";
    static cstr translate   = "A B V G D E OEGZZ I Y K L M N O P R S T U F H C CHSHSCQ IYW X JUJA"
                              "a b v g d e oegzz i y k l m n o p r s t u f h c chshscq iyw x juja";
    auto _str(buffer()); cstr _eng; int sizeEng, sizeSpace(z_sizeUTF8(space));
    while(*_str) {
        auto _rus(is_chars(_str, russian, 98));
        if(_rus) {
            auto sizeRus(z_charLengthUTF8(_rus));
            auto idx((_rus - russian) / sizeRus);
            if(idx > 131) { _eng = space; sizeEng = sizeSpace; }
            else { _eng = &translate[idx * 2]; sizeEng = (1 + (_eng[1] != ' ')); }
            memmove(_str + sizeEng, _str + sizeRus, _str_buffer.size - (_str + sizeRus - buffer()));
            memcpy(_str, _eng, sizeEng);
        }
        _str += z_charLengthUTF8(_str);
    }
    if(*str() < '9') insert(0, space);
    _str_buffer.count = z_countUTF8(str());
    return *this;
}

static cstr big   = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯABCDEFGHIJKLMNOPQRSTUVWXYZ";
static cstr small = "абвгдеёжзийклмнопрстуфхцчшщъыьэюяabcdefghijklmnopqrstuvwxyz";

void zStringUTF8::changeRegister(cstr _search, cstr _replace) const {
    auto _str(buffer());
    while(*_str) {
        auto _ret(is_chars(_str, _search, 33 + 26));
        if(_ret) memcpy(_str, &_replace[_ret - _search], z_charLengthUTF8(_ret));
        _str += z_charLengthUTF8(_str);
    }
}

const zStringUTF8& zStringUTF8::lower() {
    changeRegister(big, small);
    return *this;
}

const zStringUTF8& zStringUTF8::upper() {
    changeRegister(small, big);
    return *this;
}

bool zStringUTF8::compare(cstr str) const {

    return false;
}

