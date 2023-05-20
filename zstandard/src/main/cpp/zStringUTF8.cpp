//
// Created by User on 11.05.2023.
//

#include "zstandard/zstandard.h"
#include "zstandard/zStringUTF8.h"

static cstr hex("0123456789ABCDEF-");

char* zStringUTF8::alloc(i32 _count, i32 _size, bool is_copy) {
    auto _old(buffer());
    if(_size) {
        auto size(_size + 4);
        if(size > _str_buffer.size_buf) {
            // выделим память под новый буфер
            auto _new(new char[size]);
            // скопировать старый, если необходимо
            if(is_copy && isNotEmpty()) memcpy(_new, _old, _str_buffer.length);
            // стереть старый
            empty();
            // инициализируем новый
            _str_buffer.size_buf= size;
            _str_buffer.ptr	    = _new;
            _old                = _new;
        }
        _str_buffer.count   = _count;
        _str_buffer.length  = _size;
        _old[_size]         = 0;
    } else { empty(); }
    return _old;
}

zStringUTF8::zStringUTF8(i32 value, u32 offs, bool _hex, u32 radix) {
    init(); *this = z_fmtValue(value, offs, _hex, radix);
}

zStringUTF8::zStringUTF8(cstr _str, i32 _count) {
    init(); auto count(z_countUTF8(_str));
    make(_str, (_count < 0 || _count >= count) ? count : _count);
}

zStringUTF8::zStringUTF8(int ch, i32 _count) {
    auto l(z_charLengthUTF8((cstr)&ch)); init();
    auto _str(alloc(_count, l * _count, false));
    while(_count-- > 0) memcpy(_str, &ch, l), _str += l;
}

const zStringUTF8& zStringUTF8::add(cstr _str, i32 _count) {
    auto _size(z_sizeUTF8(_str, _count)), c(count());
    alloc(c + _count, size() + _size, true);
    memcpy(ptr(c), _str, _size);
    return *this;
}

const zStringUTF8& zStringUTF8::make(cstr _str, i32 _count) {
    auto _size(z_sizeUTF8(_str, _count));
    memcpy(alloc(_count, _size, false), _str, _size);
    return *this;
}

zStringUTF8 zStringUTF8::substr(i32 idx, i32 _count) const {
    auto count1(count());
    if(idx <= count1) {
        if((idx + _count) > count1) _count = (count1 - idx);
    } else idx = 0, _count = 0;
    return { ptr(idx), _count };
}

zStringUTF8 zStringUTF8::_after(cstr str, i32 _count, cstr no, bool last) const {
    auto idx(last ? _indexOfLast(str, 0, _count) : _indexOf(str, 0));
    return (idx != -1 ? substr(idx + _count) : zStringUTF8(no));
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

const zStringUTF8& zStringUTF8::reverse() const {
    auto size(z_sizeUTF8(str())), _count(count());
    auto buf(std::unique_ptr<char>(new char[size + 1])); auto _buf(buf.get());
    while(_count-- > 0) {
        auto _ptr(ptr(_count)); auto sz(charSize(_ptr));
        memcpy(_buf, _ptr, sz); _buf += sz;
    }
    memcpy(buffer(), buf.get(), _str_buffer.length);
    return *this;
}

void zStringUTF8::set(i32 idx, cstr _str) {
    auto _ptr(ptr(idx)); auto _size1(charSize(_ptr)), _size2(z_sizeUTF8(_str));
    memmove(_ptr + _size2, _ptr + _size1, sizeCopy(_ptr + _size1));
    memcpy(_ptr, _str, _size2);
    _str_buffer.length += _size2 - _size1;
    _str_buffer.count  = z_countUTF8(str());
}

zStringUTF8 zStringUTF8::add(cstr str1, i32 _count1, cstr str2, i32 _count2) {
    auto size1(z_sizeUTF8(str1, _count1)), size2(z_sizeUTF8(str2, _count2));
    auto ptr(std::unique_ptr<char>(new char[size1 + size2 + 1])); ptr.get()[size1 + size2] = 0;
    memcpy((char*)memcpy(ptr.get(), str1, size1) + size1, str2, size2);
    return { ptr.get() };
}

const zStringUTF8& zStringUTF8::replace(cstr _old, cstr _new) {
    auto sizeNew(z_sizeUTF8(_new)), sizeOld(z_sizeUTF8(_old)), count(0);
    auto _str(buffer()), _buf(_str);
    // подсчитать новый размер
    while((_buf = strstr(_buf, _old))) count++, _buf += sizeOld;
    if(count) {
        auto size(z_sizeUTF8(str()) - (sizeOld - sizeNew) * count);
        auto tmp(std::unique_ptr<char>(new char[size + 1]));
        auto _tmp(tmp.get()); _tmp[size] = 0;
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
        memcpy(_tmp, _str, size - (_tmp - tmp.get()));
        memcpy(alloc(z_countUTF8(tmp.get()), size, false), tmp.get(), size);
    }
    return *this;
}

const zStringUTF8& zStringUTF8::remove(cstr _str) {
    auto _count(z_countUTF8(_str)), _size(z_sizeUTF8(_str)); auto f(buffer());
    while((f = strstr(f, _str))) {
        _str_buffer.count -= _count;
        _str_buffer.length -= _size;
        memcpy(f, f + _size, sizeCopy(f + _size));
    }
    return *this;
}

const zStringUTF8& zStringUTF8::remove(i32 idx, i32 _count) {
    auto count1(count());
    if(idx < count1) {
        if((idx + _count) > count1) _count = (count1 - idx);
        auto _ptr(ptr(idx)); auto _size(z_sizeUTF8(_ptr, _count));
        memcpy(_ptr, _ptr + _size, sizeCopy(_ptr + _size));
        _str_buffer.count -= _count;
        _str_buffer.length -= _size;
    }
    return *this;
}

const zStringUTF8& zStringUTF8::insert(i32 idx, cstr _str) {
    auto _count(count()), _size(z_sizeUTF8(_str));
    if(idx > _count) idx = _count;
    if(idx <= _count && alloc(_count + z_countUTF8(_str), size() + _size, true)) {
        auto _buf(ptr(idx));
        memmove(_buf + _size, _buf, sizeCopy(_buf + _size));
        memcpy(_buf, _str, _size);
    }
    return *this;
}

static cstr is_chars(cstr self, cstr _str, i32 _count) {
    i32 l; auto ch1(z_charUTF8(self));
    for(int i = 0; i < _count; i++) {
        if(ch1 == z_charUTF8(_str, &l)) return _str;
        _str += l;
    }
    return nullptr;
}

const zStringUTF8& zStringUTF8::trimLeft(cstr _str) {
    auto _count(z_countUTF8(_str)); auto _tmp(buffer());
    while(is_chars(_tmp, _str, _count)) {
        auto l(z_charLengthUTF8(_tmp));
        _str_buffer.count--; _str_buffer.length -= l; _tmp += l;
    }
    memcpy(buffer(), _tmp, sizeCopy(_tmp));
    return *this;
}

const zStringUTF8& zStringUTF8::trimRight(cstr _str) {
    auto _count(z_countUTF8(_str)); char* _ptr;
    while(is_chars(_ptr = ptr(count() - 1), _str, _count)) {
        _str_buffer.length -= z_charLengthUTF8(_ptr); _str_buffer.count--;
    }
    *ptr(count()) = 0; return *this;
}

zArray<zStringUTF8> zStringUTF8::_split(cstr _delim, bool _trim) const {
    zArray<zStringUTF8> arr;
    int pos(0), ldelim(z_countUTF8(_delim));
    while(pos < count()) {
        auto npos(indexOf(_delim, pos));
        auto str(substr(pos, (npos == -1 ? count() : npos) - pos));
        if(_trim) str.trimRight(" \r\t");
        arr += str;
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
        replace("&amp;", "&");
        replace("&lt;", "<");
        replace("&gt;", ">");
        replace("&quot;", "\"");
        replace("&nbsp;", " ");
    } else {
        replace("&", "&amp;");
        replace("<", "&lt;");
        replace(">", "&gt;");
        replace("\"", "&quot;");
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
            memmove(_str + sizeEng, _str + sizeRus, sizeCopy(_str + sizeRus));
            memcpy(_str, _eng, sizeEng);
        }
        _str += z_charLengthUTF8(_str);
    }
    if(*str() < '9') insert(0, space);
    _str_buffer.count = z_countUTF8(str());
    _str_buffer.length = z_sizeUTF8(str());
    return *this;
}

const zStringUTF8& zStringUTF8::changeRegister(cstr _search, cstr _replace) const {
    auto _str(buffer());
    while(*_str) {
        auto ret(is_chars(_str, _search, 59));
        if(ret) {
            auto rpl(&_replace[ret - _search]);
            memcpy(_str, rpl, charSize(ret));
        }
        _str += charSize(_str);
    }
    return *this;
}

static cstr big   = "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯABCDEFGHIJKLMNOPQRSTUVWXYZ";
static cstr small = "абвгдеёжзийклмнопрстуфхцчшщъыьэюяabcdefghijklmnopqrstuvwxyz";

const zStringUTF8& zStringUTF8::lower() {
    return changeRegister(big, small);
}

const zStringUTF8& zStringUTF8::upper() {
    return changeRegister(small, big);
}

bool zStringUTF8::compare(cstr str) const {
    if(z_sizeUTF8(str) != size()) return false;
    auto _str(buffer()); int l1, l2;
    while(*_str) {
        auto ch1(z_charUTF8(_str, &l1));
        auto ch2(z_charUTF8(str, &l2));
        if(l1 != l2) return false;
        if(ch1 != ch2) {
            auto ret1(is_chars(_str, big, 33 + 26));
            auto ok(ret1 && z_charUTF8(&small[ret1 - big]) == ch2);
            if(!ok) {
                auto ret2(is_chars(_str, small, 33 + 26));
                ok = (ret2 && z_charUTF8(&big[ret2 - small]) == ch2);
            }
            if(!ok) return false;
        }
        _str += charSize(_str);
        str  += charSize(str);
    }
    return true;
}

