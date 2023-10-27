
#include "zstandard/zstandard.h"
#include "zstandard/zSettings.h"
#include "zstandard/zFile.h"

zSettings::zSettings(cstr root, cstr* opts) {
    pathRoot = root; pathRoot.slash();
    pathFiles = pathRoot + "files/";
    pathCache = pathRoot + "cache/";
    // формируем массив опций со значениями по умолчанию
    if(opts) {
        int offs(0), plus(1); ZOPTION::TOPTION t(ZOPTION::TOPTION::_unk);
        zString8 o;
        while((o = *opts++).isNotEmpty()) {
            auto name(o.substrBefore("="));
            auto isname(name.isNotEmpty());
            if(!isname) {
                // поиск значений
                auto cat(o.substrAfterLast("[").substrBeforeLast("]").lower());
                if(cat == "bytes")          t = ZOPTION::TOPTION::_byt, plus = 1;
                else if(cat == "boolean")   t = ZOPTION::TOPTION::_bol, plus = 1;
                else if(cat == "hex")       t = ZOPTION::TOPTION::_hex, plus = 4;
                else if(cat == "float")     t = ZOPTION::TOPTION::_flt, plus = 4;
                else if(cat == "integer")   t = ZOPTION::TOPTION::_int, plus = 4;
                else if(cat == "mru")       t = ZOPTION::TOPTION::_mru, plus = 1, offs = 0;
                else if(cat == "path")      t = ZOPTION::TOPTION::_pth, plus = 1, offs = 0;
                name = o;
            }
            defs += ZOPTION(name, o.substrAfter("="), t, isname ? offs : -1);
            offs += isname * plus;
        }
    }
}

void zSettings::init(u8* ptr, cstr name) {
    if(ptr && name) {
        zFile fr;
        auto opts(defs);
        if(fr.open(pathCache + name, true, false)) {
            zString8 str;
            while((str = fr.readString()).isNotEmpty()) {
                auto opt(str.substrBefore("="));
                if(opt.isEmpty()) continue;
                str.trim();
                auto index(opts.indexOf<cstr>(opt));
                if(index != -1) opts[index].value = str.substrAfter("=");
            }
            fr.close();
        }
        for(auto &o: opts) {
            if(o.offs >= 0) setOption(ptr, o);
        }
    }
}

void zSettings::setOption(u8* ptr, const ZOPTION& opt) {
    auto v(opt.value.str()); auto o(opt.offs), radix(RADIX_DEC);
    switch(opt.type) {
        case ZOPTION::TOPTION::_flt:
            *(float*)(ptr + o) = z_ston<float>(v, RADIX_FLT);
            break;
        case ZOPTION::TOPTION::_hex:
            radix = RADIX_HEX;
        case ZOPTION::TOPTION::_int:
            *(u32*)(ptr + o) = z_ston<u32>(v, radix);
            break;
        case ZOPTION::TOPTION::_bol:
            radix = RADIX_BOL;
        case ZOPTION::TOPTION::_byt:
            *(u8*)(ptr + o) = z_ston<u8>(v, radix);
            break;
        case ZOPTION::TOPTION::_mru:
            mrus += opt.value;
            break;
        case ZOPTION::TOPTION::_pth:
            paths += pathRoot + opt.value;
            break;
        case ZOPTION::TOPTION::_unk:
            break;
    }
}
	
zString8 zSettings::getOption(const u8* ptr, int idx) {
    u32 n; int radix(RADIX_DEC); zString8 ret;
    auto opt(defs[idx]); auto o(opt.offs);
    switch(opt.type) {
        case ZOPTION::TOPTION::_flt: {
            auto f(*(float*)(ptr + o));
            ret = z_ntos(&f, RADIX_FLT, true);
            break;
        }
        case ZOPTION::TOPTION::_hex:
            radix = RADIX_HEX;
        case ZOPTION::TOPTION::_int:
            n = *(u32*)(ptr + o);
            ret = z_ntos(&n, radix, opt.type == ZOPTION::TOPTION::_int);
            break;
        case ZOPTION::TOPTION::_bol:
            radix = RADIX_BOL;
        case ZOPTION::TOPTION::_byt:
            n = *(u8*)(ptr + o);
            ret = z_ntos(&n, radix, false);
            break;
        case ZOPTION::TOPTION::_mru:
            ret = mrus[o];
            break;
        case ZOPTION::TOPTION::_pth:
            ret = paths[o]; ret = ret.substrAfter(pathRoot, ret);
            break;
        case ZOPTION::TOPTION::_unk:
        default: ret = ""; break;
    }
    return { ret };
}

void zSettings::save(u8* ptr, cstr name) {
    zFile file;
    if(ptr && file.open(pathCache + name, false, false)) {
        for(int i = 0 ; i < defs.size(); i++) {
            auto opt(&defs[i]);
            auto cat(opt->offs == -1);
            file.writeString(opt->name, cat);
            if(!cat) {
                file.writeString("=", false);
                file.writeString(getOption(ptr, i), true);
            }
        }
        file.close();
    }
}

zString8 zSettings::getDefault(cstr name) const {
    auto idx(defs.indexOf(name));
    return (idx >= 0 && idx < defs.size() ? defs[idx].value : "");
}

zString8 zSettings::mruOpen(int index, czs& pth, bool error) {
    auto pos(index);
    if(error) {
        for(; pos < 9; pos++) mrus[pos] = mrus[pos + 1];
        mrus[9] = "Empty";
        return "";
    }
    // либо переставить на нулевую позицию, либо поставить новую
    auto title(pth.isNotEmpty() ? pth : mrus[index]);
    for(pos = 0; pos < 9; pos++)
        if(mrus[pos] == title) break;
    for(; pos > 0; pos--) mrus[pos] = mrus[pos - 1];
    return mrus[0] = title;
}

zString8 zSettings::makePath(cstr pth, int type) const {
    switch(type) {
        case FOLDER_CACHE:
            return pathCache + pth;
        case FOLDER_ROOT:
            return pathRoot  + pth;
        case FOLDER_FILES:
            return pathFiles + pth;
        default:
            return getPath(type);
    }
}

void zSettings::setDefault(u8* ptr, cstr opt) {
    auto idx(defs.indexOf(opt));
    if(idx >= 0 && idx < defs.size()) {
        setOption(ptr, defs[idx]);
    }
}
