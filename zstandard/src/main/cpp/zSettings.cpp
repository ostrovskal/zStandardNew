
#include "zstandard/zstandard.h"
#include "zstandard/zSettings.h"
#include "zstandard/zFile.h"

zSettings::zSettings(cstr root, cstr* opts) {
    pathRoot = root; pathRoot.slash();
    pathFiles = pathRoot + "files/";
    pathCache = pathRoot + "cache/";
    // формируем массив опций со значениями по умолчанию
    int offs(0), plus(1); ZOPTION::TOPTION t(ZOPTION::TOPTION::_unk);
    zString o;
    while((o = *opts++).isNotEmpty()) {
        auto name(o.substrBefore('='));
        auto isname(name.isNotEmpty());
        if(!isname) {
            // поиск значений
            auto cat(o.substrAfterLast(',').substrBeforeLast(']').lower());
            if(cat == "byt")      t = ZOPTION::TOPTION::_byt, plus = 1;
            else if(cat == "bol") t = ZOPTION::TOPTION::_bol, plus = 1;
            else if(cat == "hex") t = ZOPTION::TOPTION::_hex, plus = 4;
            else if(cat == "int") t = ZOPTION::TOPTION::_int, plus = 4;
            else if(cat == "mru") t = ZOPTION::TOPTION::_mru, plus = 0;
            else if(cat == "pth") t = ZOPTION::TOPTION::_pth, plus = 0;
            name = o;
        }
        defs += ZOPTION(name, o.substrAfter('='), t, offs);
        offs += isname * plus;
    }
}

void zSettings::init(u8* ptr, cstr name) {
    zFile fr; auto opts(defs);
    if(fr.open(pathCache + name, true, false)) {
        zString str;
        while((str = fr.readString()).isNotEmpty()) {
            auto opt(str.substrBefore('='));
            if(opt.isEmpty()) continue;
            str.trim();
            auto index(opts.indexOf<cstr>(opt));
            if(index != -1) opts[index].value = str.substrAfter('=');
        }
        fr.close();
    }
    for(auto& o : opts) setOption(ptr, o);
}

void zSettings::setOption(u8* ptr, const ZOPTION& opt) {
    auto v(opt.value.str()); auto o(opt.offs), radix(RADIX_DEC);
    switch(opt.type) {
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
	
zString zSettings::getOption(const u8* ptr, int idx) {
    u32 n; int radix(RADIX_DEC); cstr ret;
    auto opt(defs[idx]); auto o(opt.offs);
    switch(opt.type) {
        case ZOPTION::TOPTION::_hex:
            radix = RADIX_HEX;
        case ZOPTION::TOPTION::_int:
            n = *(u32*)(ptr + o);
            ret = z_ntos(&n, radix, true);
            break;
        case ZOPTION::TOPTION::_bol:
            radix = RADIX_BOL;
        case ZOPTION::TOPTION::_byt:
            n = *(u8*)(ptr + o);
            ret = z_ntos(&n, radix, true);
            break;
        case ZOPTION::TOPTION::_mru:
            ret = mrus[idx - o];
            break;
        case ZOPTION::TOPTION::_pth:
            ret = paths[idx - o].substrAfter(pathRoot);
            break;
        case ZOPTION::TOPTION::_unk:
        default:
            ret = "";
            break;
    }
    return zString(ret);
}

void zSettings::save(u8* ptr, cstr name) {
    zFile file;
    if(file.open(pathCache + name, false, false)) {
        for(int i = 0 ; i < defs.size(); i++) {
            auto opt(&defs[i]);
            auto cat(opt->name.startsWith('['));
            file.writeString(opt->name, cat);
            if(!cat) {
                file.writeString("=", false);
                file.writeString(getOption(ptr, i), true);
            }
        }
        file.close();
    }
}

cstr zSettings::mruOpen(int index, cstr pth, bool error) {
    auto pos(index);
    if(error) {
        for(; pos < 9; pos++) mrus[pos] = mrus[pos + 1];
        mrus[9] = "Empty";
        return "";
    }
    zString title(z_strlen(pth) > 0 ? pth : mrus[index].str());
    for(pos = 0; pos < 9; pos++)
        if(mrus[pos] == title) break;
    for(; pos > 0; pos--) mrus[pos] = mrus[pos - 1];
    return mrus[0] = title;
}

zString zSettings::mruDecorate(int index) const {
    zString mru(mrus[index]);
    mru = mru.substrAfterLast("\\", mru);
    return mru.substrAfterLast("/", mru);
}

zString zSettings::makePath(cstr pth, int type) const {
    switch(type) {
        case FOLDER_CACHE:
            return pathCache + pth;
        case FOLDER_ROOT:
            return pathRoot + pth;
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
