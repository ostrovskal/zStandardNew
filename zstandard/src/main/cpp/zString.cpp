//
// Created by Sergo on 10.12.2020.
//

#include "zstandard/zstandard.h"
#include "zstandard/zString.h"

static cstr hex("0123456789ABCDEF-");

_zStringUTF8::_zStringUTF8(cstr s, size_t _l) {
    len = strlen(s); len = ((_l <= 0 || _l >= len) ? len : _l);
    ptr = new u8[++len]; memcpy(ptr, s, len);
    z_utfToAscii(ptr, len); len = strlen((cstr)ptr);
}

zString::zString(i32 value, u32 offs, bool _hex, u32 radix) {
    init();
    *this = z_fmtValue(value, offs, _hex, radix);
}

zString::zString(cstr cws, i32 len) {
    init();
    auto t(z_strlen(cws));
    make(cws, (len < 0 || len >= t) ? t : len);
}

zString::zString(char ws, i32 rep) {
    init();
    auto buf(alloc(rep, false));
    memset(buf, ws, rep);
}

const zString& zString::add(cstr wcs, i32 len) {
    auto l(length());
    memcpy(alloc(l + len, true) + l, wcs, len);
    return *this;
}

const zString& zString::make(cstr wcs, i32 len) {
    memcpy(alloc(len, false), wcs, len);
    return *this;
}

char* zString::alloc(i32 sz, bool is_copy) {
    if(sz) {
        auto _old(buffer()); auto nsz(sz + 1);
        if(nsz > _str.len_buf) {
            // выделим память под новый буфер
            auto _new(new char[nsz]);
            // скопировать старый, если необходимо
            if(is_copy && isNotEmpty()) memcpy(_new, _old, length());
            // стереть старый
            empty();
            // инициализируем новый
            _str.len_buf	= nsz;
            _str.ptr		= _new;
            _old            = _new;
        }
        _str.len = (int)sz;
        _old[sz] = 0;
        return _old;
    }
    empty();
    return _str.str;
}

zString zString::substr(i32 idx, i32 len) const {
    auto l(length());
    if(idx <= l) {
        if(len < 0) len = l;
        if((idx + len) > l) len = (l - idx);
    } else idx = len = 0;
    return {str() + idx, len};
}

zString zString::_after(cstr str, i32 len, cstr no, bool last) const {
    auto idx(last ? _indexOfLast(str, 0, len) : _indexOf(str, 0, len));
    return (idx != -1 ? substr(idx + len) : zString(no));
}

zString zString::_before(cstr str, i32 len, cstr no, bool last) const {
    auto idx(last ? _indexOfLast(str, 0, len) : _indexOf(str, 0, len));
    return (idx != -1 ? left(idx) : zString(no));
}

int zString::indexOf(cstr wcs[]) const {
    cstr v;
    int idx(-1);
    while((v = wcs[++idx])) {
        if(*this == v) return idx;
    }
    return -1;
}

int zString::_indexOf(cstr wcs, i32 idx, i32 len) const {
    auto buf(buffer());
    char* str(nullptr);
    if(idx < length()) str = (len == 1 ? strchr(buf + idx, *wcs) : strstr(buf + idx, wcs));
    return (str ? (int)(str - buf) : -1);
}

int zString::_indexOfLast(cstr wcs, i32 idx, i32 len) const {
    int last(-1), i;
    while((i = _indexOf(wcs, idx, len)) != -1) {
        last = i; idx = i + len;
    }
    return last;
}

zString zString::add(cstr wcs1, i32 len1, cstr wcs2, i32 len2) {
    zString ret('\0', len1 + len2);
    memcpy((uint8_t*)memcpy(ret.buffer(), wcs1, len1) + len1, wcs2, len2);
    return ret;
}

void zString::_replace(cstr _old, cstr _new) {
    auto l_old(z_strlen(_old)), l_new(z_strlen(_new));
    int nCount(0);
    auto b_old(buffer()), b_new(b_old), buf(b_old);
    // расчитать новый размер
    while((buf = strstr(buf, _old))) nCount++, buf += l_old;
    if(nCount) {
        // выделяем память под новый буфер
        zString ret('\0', length() + (int)(l_new - l_old) * nCount); b_new = ret.buffer();
        // непосредственно замена
        while((buf = strstr(b_old, _old))) {
            auto lll((int)(buf - b_old));
            z_memcpy((u8**)&b_new, b_old, lll);
            z_memcpy((u8**)&b_new, _new, l_new);
            b_old = buf + l_old;
        }
        z_memcpy((u8**)&b_new, b_old, z_strlen(b_old));
        *this = ret;
    }
}

const zString& zString::replace(cstr _old, cstr _new) {
    _replace(_old, _new);
    return *this;
}

const zString& zString::replace(char _old, char _new) const {
    auto ptr(buffer());
    while(*ptr) { if(*ptr == _old) *ptr = _new; ptr++; }
    return *this;
}

const zString& zString::remove(cstr wcs) {
    auto nWcs(z_strlen(wcs)), nLen(length());
    auto f(buffer()), _buf(f);
    while((f = strstr(f, wcs))) { nLen -= nWcs; memcpy(f, f + nWcs, ((nLen - (f - _buf)) + 1)); }
    _str.len = (int)nLen;
    return *this;
}

const zString& zString::remove(char ws) {
    auto _buf(buffer()), rem(_buf);
    while(*_buf) { if(*_buf != ws) *rem++ = *_buf; _buf++; }
    *rem = 0; _str.len -= (int)(_buf - rem);
    return *this;
}

const zString& zString::remove(i32 idx, i32 len) {
    auto l(length());
    if(idx < l) {
        if(len < 0) len = l;
        if((idx + len) > (int)l) len = (l - idx);
        auto ll(idx + len); auto _buf(buffer());
        memcpy(_buf + idx, _buf + ll, ((l - ll) + 1));
        _str.len -= len;
    }
    return *this;
}

const zString& zString::insert(i32 idx, cstr wcs) {
    auto len(length()), nWcs(z_strlen(wcs));
    if(idx > len) idx = len;
    if(idx <= len && alloc(len + nWcs, true)) {
        auto _buf(buffer());
        memmove(_buf + idx + nWcs, _buf + idx, len - idx);
        memcpy(_buf + idx, wcs, nWcs);
    }
    return *this;
}

const zString& zString::insert(i32 idx, char ws) {
    auto len(length());
    if(idx > len) idx = len;
    if(idx <= len && alloc(len + 1, true)) {
        auto _buf(buffer());
        memmove(_buf + idx + 1, _buf + idx, len - idx);
        _buf[idx] = ws;
    }
    return *this;
}

static bool is_chars(const char* ws, cstr wcs, i32 ln) {
    for(int i = 0; i < ln; i++) {
        if(*ws == wcs[i]) return true;
    }
    return (*ws == ' ');
}

const zString& zString::trimLeft(cstr wcs) {
    auto len(length()), ln(z_strlen(wcs));
    auto _ws(buffer()), _buf(_ws);
    while(is_chars(_ws, wcs, ln)) { len--; _ws++; }
    memcpy(_buf, _ws, (len + 1));
    _str.len = (int)len;
    return *this;
}

const zString& zString::trimRight(cstr wcs) {
    auto len(length()), ln(z_strlen(wcs));
    auto _ws(buffer() + len - 1);
    while(is_chars(_ws, wcs, ln)) { len--; _ws--; }
    *(_ws + 1) = 0;
    _str.len = (int)len;
    return *this;
}

zArray<zString> zString::splitString() const {
    zArray<zString> arr;
    int pos(0);
    while(pos < length()) {
        auto npos(indexOf('\n', pos));
        auto str(substr(pos, (npos == -1 ? length() : npos) - pos));
        arr += str.trimRight(" \r\t");
        if(npos == -1) break;
        pos = npos + 1;
    }
    return arr;
}

zArray<zString> zString::split(cstr delim) const {
    zArray<zString> arr;
    int pos(0), ldelim(z_strlen(delim));
    while(pos < length()) {
        auto npos(indexOf(delim, pos));
        arr += substr(pos, (npos == -1 ? length() : npos) - pos);
        if(npos == -1) break;
        pos = npos + ldelim;
    }
    return arr;

}

u8* zString::state(u8 *ptr, bool save) {
    if(save) {
        z_memcpy(&ptr, str(), length() + 1);
    } else {
        *this = ptr;
        ptr += length() + 1;
    }
    return ptr;
}

const zString &zString::replaceAmp(bool dir) {
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

zString zString::translate(char space) const {
    static cstr russion     = "А Б В Г Д Е Ж З И Й К Л М Н О П Р С Т У Ф Х Ц Ч Ш Щ Ъ Ы Ь Э Ю Я "
                              "а б в г д е ж з и й к л м н о п р с т у ф х ц ч ш щ ъ ы ь э ю я ";
    static cstr translate   = "A B V G D E GZZ I Y K L M N O P R S T U F H C CHSHSCQ IYW X JUJA"
                              "a b v g d e gzz i y k l m n o p r s t u f h c chshscq iyw x juja";
    zString n; u8 ch;
    for(int i = 0; i < length(); i++) {
        ch = at(i);
        if(ch < '0') {
            ch = (ch == ' ' ? space : '_');
        } else if(ch >= 192) {
            ch -= 192;
            u8 _1(translate[ch * 2]), _2(translate[ch * 2 + 1]);
            if(_2 != ' ') { n += (char)_1; ch = _2; } else ch = (char)_1;
        } else if(ch > '0' && ch < '9') {
            if(i == 0) n += '_';
        } else if(ch == 168) {
            n += 'O'; ch = 'E';
        } else if(ch == 184) {
            n += 'o'; ch = 'e';
        } else if(ch > 'z') ch = '_';
        n += (char)ch;
    }
    return n;
}

zString z_guid() {
    static cstr templ("xxxxxxxx-xxxx-zxxx-yxxx-xxxxxxxxxxxx");
    static char GUID[40];
    zRand rand;
    for(int t = 0; t < 37; t++) {
        auto r(zRand::nextInt(16)); char c(' ');
        switch(templ[t]) {
            case 'x': c = hex[r]; break;
            case 'y': c = hex[(r & 0x03) | 0x08]; break;
            case '-': c = '-'; break;
            case 'z': c = '4'; break;
        }

        GUID[t] = (t < 36) ? c : '\0';
    }
    return {GUID};
}

static cstr base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline bool is_base64(u8 c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

zString z_base64Encode(const u8* ptr, u32 size) {
    zString ret;
    int i(0), j;
    u8 char_array_3[3], char_array_4[4];
    while(size--) {
        char_array_3[i++] = *ptr++;
        if(i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc)  >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for(i = 0; (i < 4); i++) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if(i) {
        for(j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc)  >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        for(j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];
        while((i++ < 3)) ret += '=';
    }
    return ret;

}

zString z_base64Decode(cstr ptr, i32 size) {
    int i(0), j, in_(0);
    u8 char_array_4[4], char_array_3[3];
    zString ret;
    while(size-- && (ptr[in_] != '=') && is_base64(ptr[in_])) {
        char_array_4[i++] = ptr[in_]; in_++;
        if(i == 4) {
            for(i = 0; i < 4; i++) char_array_4[i] = base64_chars[char_array_4[i]];
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for(i = 0; (i < 3); i++) ret += (char)char_array_3[i];
            i = 0;
        }
    }
    if(i) {
        for(j = i; j < 4; j++) char_array_4[j] = 0;
        for(j = 0; j < 4; j++) char_array_4[j] = base64_chars[char_array_4[j]];
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
        for(j = 0; (j < i - 1); j++) ret += (char)char_array_3[j];
    }
    return ret;
}

zString z_urlEncode(cstr _buf) {
    zString ptr('\0', z_strlen(_buf) * 3);
    auto _ptr(ptr.buffer()); char ch;
    while((ch = *_buf++)) {
        if(!isalnum(ch) && ch != '.' && ch != '-' && ch != '_' && ch != '~') {
            *_ptr++ = '%'; *_ptr++ = hex[(ch & 240) >> 4];
            ch = hex[ch & 15];
        }
        *_ptr++ = ch;
    }
    *_ptr = 0;
    return {ptr};
}

zString z_urlDecode(cstr buf) {
    auto _buf((char*)buf); char ch;
    while((ch = *_buf++)) {
        if(ch == '%') ch = z_toHex(&_buf);
        *_buf++ = ch;
    }
    *_buf = 0;
    return {buf};
}

zString z_fmt(cstr fmt, ...) {
    va_list args;
    va_start(args, fmt);
    zString tmp('\0', vsnprintf(nullptr, 0, fmt, args));
    va_end(args); va_start(args, fmt);
    vsnprintf(tmp.buffer(), tmp.length() + 1, fmt, args);
    va_end(args);
    return tmp;
}

zString z_stringUtfToAscii(cstr _text) {
    zString ret(_text);
    if(_text) {
        ret = (cstr)z_utfToAscii((u8 *)ret.buffer(), ret.length() + 1);
    }
    return ret;
}

/*
std::wstring utf8_to_utf16(const std::string& utf8)
{
    std::vector<unsigned long> unicode;
    size_t i = 0;
    while (i < utf8.size())
    {
        unsigned long uni;
        size_t todo;
        bool error = false;
        unsigned char ch = utf8[i++];
        if (ch <= 0x7F)
        {
            uni = ch;
            todo = 0;
        }
        else if (ch <= 0xBF)
        {
            throw std::logic_error("not a UTF-8 string");
        }
        else if (ch <= 0xDF)
        {
            uni = ch&0x1F;
            todo = 1;
        }
        else if (ch <= 0xEF)
        {
            uni = ch&0x0F;
            todo = 2;
        }
        else if (ch <= 0xF7)
        {
            uni = ch&0x07;
            todo = 3;
        }
        else
        {
            throw std::logic_error("not a UTF-8 string");
        }
        for (size_t j = 0; j < todo; ++j)
        {
            if (i == utf8.size())
                throw std::logic_error("not a UTF-8 string");
            unsigned char ch = utf8[i++];
            if (ch < 0x80 || ch > 0xBF)
                throw std::logic_error("not a UTF-8 string");
            uni <<= 6;
            uni += ch & 0x3F;
        }
        if (uni >= 0xD800 && uni <= 0xDFFF)
            throw std::logic_error("not a UTF-8 string");
        if (uni > 0x10FFFF)
            throw std::logic_error("not a UTF-8 string");
        unicode.push_back(uni);
    }
    std::wstring utf16;
    for (size_t i = 0; i < unicode.size(); ++i)
    {
        unsigned long uni = unicode[i];
        if (uni <= 0xFFFF)
        {
            utf16 += (wchar_t)uni;
        }
        else
        {
            uni -= 0x10000;
            utf16 += (wchar_t)((uni >> 10) + 0xD800);
            utf16 += (wchar_t)((uni & 0x3FF) + 0xDC00);
        }
    }
    return utf16;
}*/

