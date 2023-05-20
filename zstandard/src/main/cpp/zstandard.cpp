
#include "zstandard/zstandard.h"

u8 zsym[256] = {
        LN, LN, LN, LN, LN, LN, LN, LN, LN, LS, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN,
        LS, LO, LQ, LO, LO, LO, LO, LQ, LO, LO, LO, LO, LO, LO, LO, LO,
        LD, LD, LD, LD, LD, LD, LD, LD, LD, LD,
        LO, LK, LO, LO, LO, LO, LO,
        LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW,
        LO, LO, LO, LO, LW, LO,
        LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW,
        LO, LO, LO, LO, LN, LN,
        LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN, LN,
        LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW,
        LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW,
        LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW, LW
};

static u8   sym[]   = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
static u8   tbl[]   = { 0,  4,  4,  4,  4,  4,  4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 15, 14, 14, 14, 14, 14, 14, 12, 12, 0, 0, 0, 0, 0, 0 };
static u8   valid[] = { 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   1,  2,  3,  4,  5,  6,  7,  8,  9, 0, 0, 0, 0, 0, 0 };

void z_log(bool info, cstr file, cstr func, int line, cstr msg, ...) {
    static char buf[512];
    va_list varArgs;
    va_start(varArgs, msg);
    auto wfile(strrchr(file, '\\'));
    file = wfile ? wfile + 1 : file;
    auto lfile(strrchr(file, '/'));
    file = lfile ? lfile + 1 : file;
#ifdef WIN32
    static char tbuf[128];
    struct timespec tts;
    if(timespec_get(&tts, 1)) {
        auto tm(localtime(&tts.tv_sec));
        sprintf(tbuf, "%02i:%02i:%02i.%03li", tm->tm_hour, tm->tm_min, tm->tm_sec, tts.tv_nsec / 1000000);
    } else *tbuf = 0;
    sprintf(&buf[128], "%s : [<0x%x> %s %s (%s:%i)] - %s\n", tbuf, (u32)::GetCurrentThreadId(), info ? "info" : "debug", func, file, line, msg);
    vsprintf(buf, &buf[128], varArgs);
    OutputDebugString(buf);
#else
    sprintf(buf, "[%s (%s:%i)] - %s\n", func, file, line, msg);
    auto prio(info ? ANDROID_LOG_INFO : ANDROID_LOG_DEBUG);
    __android_log_vprint(prio, "ZX", buf, varArgs);
#endif
    va_end(varArgs);
}

char z_toHex(char** _buf) {
    auto buf(*_buf); auto ch(*buf);
    if(isxdigit(*buf) && isxdigit(buf[1])) {
        auto t1(*buf++), t2(*buf++); *_buf = buf;
        t1 = t1 <= '9' ? (t1 - 48) : (t1 <= 'F' ? t1 - 55 : t1 - 92);
        t2 = t2 <= '9' ? (t2 - 48) : (t2 <= 'F' ? t2 - 55 : t2 - 92);
        ch = (char)(t2 | (t1 << 4));
    }
    return ch;
}

void z_escape(char** _dst, char** _src) {
    static cstr escapes = "a\ab\bf\fn\nr\rt\tv\v\'\'\"\"\\\\?\?x\x01o\x02";
    auto esc((char*)escapes), src(*_src), p(*_dst); auto ch(*src++);
    while(*esc) {
        if(ch == *esc++) {
            ch = *esc;
            switch(ch) {
                case 1: *p++ = z_toHex(&src); ch = z_toHex(&src); break;
                case 2: break;
            }
            *p++ = ch;
            break;
        }
        esc++;
    }
    *_dst = p; *_src = src;
}

u16 z_crc(u8 v, u16 init) {
    u16 c(init ^ (u16)(v << 8U));
    for(int i = 8; i; --i) { if((c <<= 1U) & 0x10000U) c ^= 0x1021U; }
    return c;
}

u16 z_crc(u8* src, u32 size) {
    u16 c((u16)0xcdb4);
    while(size--) c = z_crc(*src++, c);
    return c;
}

u64 z_crc64(cstr str) {
    u64 _val(14695981039346656037ULL);
    while(*str) {
        _val ^= (u64)*str++;
        _val *= (u64)1099511628211ULL;
    }
    _val ^= _val >> 32;
    return _val;
}

int z_remap(i32 value, i32* place) {
    i32 v;
    while((v = *place++) != -1) {
        if(v == value) return *place;
        place++;
    }
    return -1;
}

static void itos(u32 n, char** buffer, bool sign) {
    auto buf((u8*)*buffer);
    u8 s(0);
    if(sign && (int)n < 0) { s = '-'; int* nn((int*)&n); *nn = -*nn; }
    do { *--buf = (u8)((n % 10) + (u8)'0'); n /= 10; } while(n);
    if(s) *--buf = s;
    *buffer = (char*)buf;
}

// преобразовать число в строку, в зависимости от системы счисления
char* z_ntos(void* v, i32 r, bool sign, int size, char** end) {
    static char buffer[128];
    static u8 tbl1[] = { 0, 10, 1, 15, 4, 2, 1, 1, 8, 7, 3, 4, 8, 10, 1, 4, 10, 1, 0, 0, 1 };
    auto buf(&buffer[16]); *buf = 0;
    if(end) *end = buf;
    char* st;
    u8 c(tbl1[r * 3 + 0]), s(tbl1[r * 3 + 1]);
    size *= tbl1[r * 3 + 2];
    u32 n; double d(0);
    u8 sgn(0);
    switch(r) {
        case RADIX_DEC:
            itos(*(u32*)v, &buf, sign);
            break;
        case RADIX_HEX: case RADIX_OCT:
        case RADIX_BIN:
            n = *(u32*)v;
            if(sign && (i32)n < 0) { sgn = '-'; i32* nn((i32*)&n); *nn = -*nn; }
            while(size--) { *--buf = sym[(u8)(n & c)]; n >>= s; }
            if(sgn) *--buf = sgn;
            break;
        case RADIX_FLT: d = (double)(*(float*)v);
        case RADIX_DBL: if(r == RADIX_DBL) d = *(double*)v;
            // отбросим дробную часть
            n = (i32)d;
            itos(n, &buf, sign);
            st = buf; buf = &buffer[16]; *buf++ = '.';
            for(int i = 0 ; i < c; i++) {
                d -= n; d *= 10.0; n = (u32)d;
                *buf++ = (u8)(n + '0');
            }
            if(end) *end = buf;
            *buf = 0; buf = st;
            break;
        case RADIX_BOL:
            memcpy(buf, (*(bool*)v) ? "true\0\0" : "false\0", 6);
            break;
        default: break;
    }
    return buf;
}

static i32 stoi(cstr* s, u8 order, u8 msk) {
    u8* str((u8*)*s), *end(str - 1);
    i32 n(0), nn(1);
    // поиск конца
    u8 ch;
    while((ch = *++end)) {
        if(ch >= 'a') ch -= 32U;
        if(ch < '0' || ch > 'F') break;
        if(!(tbl[ch & 31U] & msk)) break;
    }
    *s = (char*)end;
    while(end-- != str) {
        n += (valid[(*end) & -97] * nn);
        nn *= order;
    }
    return n;
}

// преобразовать строку в число в зависимости от системы счисления
void* z_ston_(cstr s, i32 r, char** end) {
    static u8 rdx[] = { 10, 8, 16, 4, 2, 1, 8, 2, 10, 8, 10, 8, 0, 0 };
    static u64 res; int ln;

    i32 sign = 1;
    if(*s == '+' || *s == '-') {
        sign = (*s++ == '-') ? -1 : 1;
        z_skip_spc((char**)&s, ln);
    }
    if(*s == '#') { r = RADIX_HEX; s++; }
    else if(*s == '%') { r = RADIX_BIN; s++; }
    auto msk(rdx[r * 2 + 1]);
    i32 n; double d(0.0);
    switch(r) {
        case RADIX_DEC: case RADIX_HEX: case RADIX_BIN: case RADIX_OCT:
            n = stoi(&s, rdx[r * 2 + 0], msk) * sign;
            *(i32*)&res = n;
            break;
        case RADIX_DBL: case RADIX_FLT:
            n = stoi(&s, 10, 8);
            if(*s == '.') {
                auto e((u8*)s);
                // поиск конца
                while(*++e) {
                    auto ch(*e & -33);
                    if(ch > 'F') break;
                    if(!(tbl[ch & 31U] & msk)) break;
                }
                if(end) *end = (char*)e;
                end = nullptr;
                while(--e != (u8*)s) {
                    auto ch(*e & -33);
                    if(ch > 'F') break;
                    ch &= 31U;
                    if(!(tbl[ch] & msk)) break;
                    d += valid[ch]; d /= 10.0;
                }
            }
            d += n; d *= sign;
            if(r == RADIX_FLT) *(float*)&res = (float)d; else *(double*)&res = d;
            break;
        case RADIX_BOL:
            if(!stricmp(s, "true")) { s += 4; n = 1; } else { s += strlen(s); n = 0; }
            *(u8*)&res = (u8)n;
            break;
        default: break;
    }
    if(end) *end = (char*)s;
    return &res;
}

u8* z_rle_compress(u8* in, int size, int& nsize) {
	auto _out(new u8[size * 2]), out(_out), _in(in + size);
	// найти самый часто повторяющийся символ в последовательности
	int count[256], idx(0), mn(0); memset(count, 0, 256 * 4);
	for(int i = 0; i < size; i++) count[in[i]]++;
	for(int i = 0; i < 256; i++) { if(count[i] > mn) mn = count[i], idx = i; }
	// bit 7 - упаковано idx 1-128 раз
	// иначе 1-128 других данных
	while(in < _in) {
		auto c1(*in++), c2(*in);
		auto is1(c1 == c2 && c1 == idx);
		auto m(out++); *out++ = c1; int ll(0);
		while(in < _in) {
			c1 = *in++; c2 = *in; ll++;
			auto is2(c1 == c2 && c1 == idx);
			if(is1 != is2 || ll > 127) {
				ll--;
				if(is1) { ll |= 0x80; out = m + 1; } else in--;
				*m = (u8)ll; ll = 0; break;
			}
			*out++ = c1;
		}
		if(ll) { if(is1) ll |= 0x80, out = m + 1; *m = (u8)ll; }
	}
	auto l((int)(out - _out) + 1); nsize = l;
	// маркер
	*out = (u8)idx;
	return _out;
}

u8* z_rle_decompress(u8* in, int size, int nsize) {
	auto _out(new uint8_t[nsize]), out(_out), _in(in + size - 1);
	auto idx(in[size - 1]);
	while(in < _in) {
		auto l(*in++);
		if(l & 0x80) {
			l = (l & 0x7f) + 2; z_memset(&out, idx, l);
		} else { l++; z_memcpy(&out, in, l); in += l; }
	}
	return _out;
}

char* z_fmtValue(i32 value, u32 offs, bool hex, u32 radix, bool _showHex) {
    static const char* fmtTypes[] = {
            "3X", "2X", "3X ", "2X ",
            "5(X)", "4(#X)", "3(X)", "2(#X)",
            "3X)", "2#X)",
            "5X", "4X",
            "5X", "4#X", "3X", "2#X",
            "5[X]", "4[#X]",
            "3{X}", "2{#X}",
            "5{X}", "4{#X}",
            "0X", "0#X",
            "0X", "0X",
            "8X", "8X",
            "0X", "0X"
    };
    static char _buffer[128];
    char ch, *end(nullptr);
    if(offs == ZFV_BINARY) radix = RADIX_BIN;
    else if(offs == ZFV_DECIMAL) hex = false;
    auto buffer(_buffer), tmp(buffer);
    auto rdx(offs + (hex ? _showHex : 0));
    auto res(z_ntos(&value, radix == 0 ? rdx & 1U : radix, true, 4, &end));
    auto spec(fmtTypes[rdx]); auto lnum((spec[0] - '0') - (end - res));
    bool znak(false);
    while((ch = *++spec)) {
        if(ch == 'X' || ch == '#') {
            if(!znak) {
                if(*res == '-') {
                    *tmp++ = *res++; lnum++;
                } else if(offs == ZFV_OFFS) {
                    *tmp++ = '+';
                }
                znak = true;
            }
            if(ch == '#') *tmp++ = ch;
            else {
                while(lnum-- > 0) *tmp++ = '0';
                while(*res) *tmp++ = *res++;
            }
        } else *tmp++ = ch;
    }
    *tmp = 0;
    return buffer;
}

rtf z_vertexMinMax(zVertex2D* v, int count) {
    auto mnx(100000.0f), mny(100000.0f), mxx(0.0f), mxy(0.0f);
    while(count-- > 0) {
        if(mnx > v->x) mnx = v->x;
        if(mxx < v->x) mxx = v->x;
        if(mny > v->y) mny = v->y;
        if(mxy < v->y) mxy = v->y;
        v++;
    }
    return rtf(mnx, mny, mxx - mnx, mxy - mny);
}

void z_rotate(cpti& c, zVertex2D* v, int count, float angle) {
    auto theta(deg2rad(angle));
    float cs(cosf(theta)), sn(sinf(theta));
    for(int i = 0 ; i < count; i++) {
        auto xx(v[i].x - c.x), yy(v[i].y - c.y);
        v[i].x = (xx * cs - yy * sn) + c.x;
        v[i].y = (xx * sn + yy * cs) + c.y;
    }
}

void z_scale(cpti& c, zVertex2D* v, int count, float xzoom, float yzoom) {
    for(int i = 0 ; i < count; i++) {
        auto x((v[i].x - c.x) * xzoom), y((v[i].y - c.y) * yzoom);
        v[i].x = x + c.x; v[i].y = y + c.y;
    }
}

rti z_clipRect(crti& p, crti& r) {
    static rti out;
    out.empty();
    auto _x(r.x - p.x), t(_x + r.w), _w(t - p.w);
    if(t > 0 && _w < r.w) {
        auto _y(r.y - p.y); t = (_y + r.h); auto _h(t - p.h);
        if(t > 0 && _h < r.h) {
            _x = (_x > 0 ? 0 : -_x); _y = (_y > 0 ? 0 : -_y);
            if(_w < 0) _w = 0; if(_h < 0) _h = 0;
            // скорректировать
            out.offset(r.x + _x, r.y + _y);
            out.w = r.w - (_x + _w);
            out.h = r.h - (_y + _h);
        }
    }
    return out;
}

bool z_delimiter(int c) {
    if(c == '_' || c >= 0xC0) return false;
    if(c < '0') return true;
    if(c > '9' && c < '@') return true;
    return (c > 'Z' && c < 'a');
}

static const u16 cp1251_2uni[128] = {
        /* 0x80 */
        0x0402, 0x0403, 0x201a, 0x0453, 0x201e, 0x2026, 0x2020, 0x2021,
        0x20ac, 0x2030, 0x0409, 0x2039, 0x040a, 0x040c, 0x040b, 0x040f,
        /* 0x90 */
        0x0452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
        0xfffd, 0x2122, 0x0459, 0x203a, 0x045a, 0x045c, 0x045b, 0x045f,
        /* 0xa0 */
        0x00a0, 0x040e, 0x045e, 0x0408, 0x00a4, 0x0490, 0x00a6, 0x00a7,
        0x0401, 0x00a9, 0x0404, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x0407,
        /* 0xb0 */
        0x00b0, 0x00b1, 0x0406, 0x0456, 0x0491, 0x00b5, 0x00b6, 0x00b7,
        0x0451, 0x2116, 0x0454, 0x00bb, 0x0458, 0x0405, 0x0455, 0x0457,
        /* 0xc0 */
        0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
        0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f,
        /* 0xd0 */
        0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
        0x0428, 0x0429, 0x042a, 0x042b, 0x042c, 0x042d, 0x042e, 0x042f,
        /* 0xe0 */
        0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
        0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f,
        /* 0xf0 */
        0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
        0x0448, 0x0449, 0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x044f,
};

int z_decodeUTF8(u32 ch) {
    int r(0);
    switch(z_charLengthUTF8((cstr)&ch)) {
        case 1: return ch;
        case 2: r = ((ch & 0b0000000000011111) << 6) | ((ch & 0b00111111'00000000) >> 8); break;
        case 3: r = ((ch & 0b00000000'00000000'00001111) << 12) | ((ch & 0b00000000'00111111'00000000) >> 2) |
                    ((ch & 0b00111111'00000000'00000000) >> 16); break;
        case 4: r = ((ch & 0b00000000'00000000'00000000'00000111) << 18) | ((ch & 0b00000000'00000000'00111111'00000000) << 4) |
                    ((ch & 0b00000000'00111111'00000000'00000000) >> 10) | ((ch & 0b00111111'00000000'00000000'00000000) >> 24);
                r -= 0x10000; r = (((r / 0x400) + 0xD800) >> 16) | ((r % 0x400) + 0xDC00); break;
    }
    return ((r >= 0x410 && r <= 0x44f) ? (0xC0 + (r - 0x410)) : '?');
}

int z_encodeUTF8(u32 ch) {
    if(ch < 0x80) return ch;
    auto wc(cp1251_2uni[ch - 0x80]);
    return (wc == 0xfffd ? '?' : ((wc & 0b00000000'00111111) << 8) | (((wc & 0b00000111'11000000) >> 6) | 0b10000000'11000000));
}

zStringUTF8 z_cp1251ToUtf8(const zString& src) {
    auto len(src.length()); auto _src((u8*)src.str());
    auto tmp(std::unique_ptr<char>(new char[(len + 1) * 4])); auto _tmp(tmp.get());
    for(int i = 0; i < len; i++) {
        *(int*)_tmp = z_encodeUTF8(*_src++);
        _tmp += z_charLengthUTF8(_tmp);
    }
    *_tmp = 0;
    return {tmp.get()};
}

zString z_utf8ToCp1251(const zStringUTF8& src) {
    auto len(src.count()); auto _src(src.str());
    auto tmp(std::unique_ptr<char>(new char[(len + 1)])); auto _tmp(tmp.get());
    for(int i = 0 ; i < len; i++) {
        *_tmp++ = (char)z_decodeUTF8(z_charUTF8(_src));
        _src += z_charLengthUTF8(_src);
    }
    *_tmp = 0;
    return { tmp.get() };
}

static const u8 sTable[256] = {
        0xa3,0xd7,0x09,0x83,0xf8,0x48,0xf6,0xf4,0xb3,0x21,0x15,0x78,0x99,0xb1,0xaf,0xf9,
        0xe7,0x2d,0x4d,0x8a,0xce,0x4c,0xca,0x2e,0x52,0x95,0xd9,0x1e,0x4e,0x38,0x44,0x28,
        0x0a,0xdf,0x02,0xa0,0x17,0xf1,0x60,0x68,0x12,0xb7,0x7a,0xc3,0xe9,0xfa,0x3d,0x53,
        0x96,0x84,0x6b,0xba,0xf2,0x63,0x9a,0x19,0x7c,0xae,0xe5,0xf5,0xf7,0x16,0x6a,0xa2,
        0x39,0xb6,0x7b,0x0f,0xc1,0x93,0x81,0x1b,0xee,0xb4,0x1a,0xea,0xd0,0x91,0x2f,0xb8,
        0x55,0xb9,0xda,0x85,0x3f,0x41,0xbf,0xe0,0x5a,0x58,0x80,0x5f,0x66,0x0b,0xd8,0x90,
        0x35,0xd5,0xc0,0xa7,0x33,0x06,0x65,0x69,0x45,0x00,0x94,0x56,0x6d,0x98,0x9b,0x76,
        0x97,0xfc,0xb2,0xc2,0xb0,0xfe,0xdb,0x20,0xe1,0xeb,0xd6,0xe4,0xdd,0x47,0x4a,0x1d,
        0x42,0xed,0x9e,0x6e,0x49,0x3c,0xcd,0x43,0x27,0xd2,0x07,0xd4,0xde,0xc7,0x67,0x18,
        0x89,0xcb,0x30,0x1f,0x8d,0xc6,0x8f,0xaa,0xc8,0x74,0xdc,0xc9,0x5d,0x5c,0x31,0xa4,
        0x70,0x88,0x61,0x2c,0x9f,0x0d,0x2b,0x87,0x50,0x82,0x54,0x64,0x26,0x7d,0x03,0x40,
        0x34,0x4b,0x1c,0x73,0xd1,0xc4,0xfd,0x3b,0xcc,0xfb,0x7f,0xab,0xe6,0x3e,0x5b,0xa5,
        0xad,0x04,0x23,0x9c,0x14,0x51,0x22,0xf0,0x29,0x79,0x71,0x7e,0xff,0x8c,0x0e,0xe2,
        0x0c,0xef,0xbc,0x72,0x75,0x6f,0x37,0xa1,0xec,0xd3,0x8e,0x62,0x8b,0x86,0x10,0xe8,
        0x08,0x77,0x11,0xbe,0x92,0x4f,0x24,0xc5,0x32,0x36,0x9d,0xcf,0xf3,0xa6,0xbb,0xac,
        0x5e,0x6c,0xa9,0x13,0x57,0x25,0xb5,0xe3,0xbd,0xa8,0x3a,0x01,0x05,0x59,0x2a,0x46
};

u32 z_hash(cstr str, i32 len) {
    i32 hash(len), i;
    for(i = 0; i != len; i++, str++) {
        hash ^= sTable[(*str + i) & 255];
        hash = hash * 1717;
    }
    return hash;
}

long zRand::seed(0);

void z_checkGlError() {
    auto err(glGetError());
//    auto str((cstr)glGetString(err));
    if(err == GL_NO_ERROR) return;
    cstr str;
    switch (err) {
        case GL_INVALID_ENUM:
            str = "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE:
            str = "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:
            str = "GL_INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:
            str = "GL_STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:
            str = "GL_STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:
            str = "GL_OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            str = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
        default:
            str = "unknown error";
            break;
    }
    ILOG("OpenGL ES error %X(%s)", err, str);
}

void z_tgaSaveFile(cstr path, u8* ptr, int w, int h, int comp) {
    HEADER_TGA head{ 0, 0, 2, 0, 0, 0, 0, 0, (u16)w, (u16)h, 32, 40 };
    zFile f;
    if(f.open(path, false)) {
        f.write(&head, sizeof(head));
        int* dst(new int[w * h]); auto src(ptr);
        int r, g, b, a;
        for(i32 y = 0; y < h; y++) {
            auto row(dst + y * w);
            for(i32 x = 0; x < w; x++) {
                switch(comp) {
                    case 1: r = g = b = a = *src++; break;
                    case 3: r = *src++; g = *src++; b = *src++; a = 255; break;
                    case 4: r = *src++; g = *src++; b = *src++; a = *src++; break;
                    default: r = g = b = a = 0; break;
                }
                *row++ = (i32)(b | (g << 8) | (r << 16) | (a << 24));
            }
        }
        f.write(dst, w * h * 4);
        delete [] dst;
    }
}
