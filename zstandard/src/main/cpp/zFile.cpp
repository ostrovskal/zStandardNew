//
// Created by Sergo on 10.12.2020.
//

#include "zstandard/zstandard.h"
#include "zstandard/zFile.h"
#include "zstandard/zUnzip.h"
#include "zstandard/zZip.h"

#ifdef WIN32
    #include <wdirent.h>
#endif

static unz_file_info zinfo;

bool zFile::open(czs& pth, bool read, bool zipped, bool append) {
    close();
    if(pth) {
        if(zipped && pth.right(4).compare(".zip")) {
            hz = (read ? unzOpen(pth) : zipOpen(pth, APPEND_STATUS_CREATE));
        }
        if(!hz) {
            i32 access(read ? O_RDWR : O_WRONLY);
#ifdef WIN32
            access |= O_BINARY;
#endif
            if(!read) { if(!append) access |= O_CREAT | O_TRUNC; else access |= O_APPEND; }
            hf = openf(pth, access, DEFFILEMODE);
        }
    }
    bool ret(hf > 0 || hz != nullptr);
    if(ret) path = pth;
    return ret;
}

bool zFile::copy(czs& pth, int index) {
    bool ret(false);
    if(hz) {
        ret = (unzUnzipToFile(hz->unz, index, pth) == ZIP_OK);
    } else if(hf > 0) {
        zFile f;
        if(f.open(pth, false, false)) {
            int bsz(256 * 1024), sz(0);
            auto tmp(std::unique_ptr<u8>(new u8[bsz]));
            while(true) {
                read(&sz, tmp.get(), bsz);
                if(sz) ret = f.write(tmp.get(), sz);
                if(!ret || sz != bsz) break;
            }
        }
        f.close();
    }
    return ret;
}

void* zFile::read(int* psize, void* ptr, int size, int pos, int mode) const {
    bool ret(false);
    if(hz) {
        ret = (unzUnzipToMem(hz->unz, pos, (u8**)&ptr, (u32*)&size) == ZIP_OK);
    } else if(hf > 0) {
        seek(pos, mode); if(!size) size = length();
        ret = ((pos = readf(hf, ptr, size)) == size);
        size = pos;
    }
    if(psize) *psize = size;
    return (ret ? ptr : nullptr);
}

void* zFile::readn(int* psize, int size, int pos, int mode) const {
    u8* ptr(nullptr); bool ret(false);
    if(hz) {
        ret = (unzUnzipToMem(hz->unz, pos, &ptr, (u32*)&size) == ZIP_OK);
    } else if(hf > 0) {
        if(!size) size = length(); ptr = new u8[size + 1]; ptr[size] = 0;
        ret = read(psize, ptr, size, pos, mode) != nullptr;
    }
    if(!ret) {
        SAFE_A_DELETE(ptr);
        size = 0;
    }
    if(psize) *psize = size;
    return ptr;
}

zString8 zFile::readString8(int pos, int mode) const {
    return z_cp1251ToUtf8(readString(pos, mode));
}

zString zFile::readString(int pos, int mode) const {
    static char stmp[257];
    zString tmp; seek(pos, mode);
    if(hf > 0) {
        char ch; int idx(0);
        while(read(nullptr, &ch, 1)) {
            if(ch == '\r') continue;
            if(ch == '\n') break;
            stmp[idx++] = ch;
            if(idx > 255) { stmp[idx] = 0; tmp += stmp; idx = 0; }
        }
        stmp[idx] = 0; tmp += stmp;
    }
    return tmp;
}

zArray<zString8> zFile::strings() const {
    zArray<zString8> arr; zString8 str;
    while((str = readString8()).isNotEmpty()) arr += str;
    return arr;
}

bool zFile::write(void* ptr, int size, cstr name) const {
    bool ret(false);
    if(hz) {
        if(!name) name = path.str();
        ret = (zipAddMem(hz->zip, name, ptr, size) == ZIP_OK);
    } else if(hf > 0) {
        ret = (writef(hf, ptr, size) == size);
    }
    return ret;
}

bool zFile::writeString(cstr s, bool clr) const {
    bool ret(false);
    if(hf > 0) {
        auto l(z_strlen(s));
        if((ret = (writef(hf, s, l) == l))) {
            l--;
            if(clr && s[l] != '\r' && s[l] != '\n')
                ret = (writef(hf, "\n", 1) == 1);
        }
    }
    return ret;
}

int zFile::tell() const {
    return (hf > 0 ? lseekf(hf, 0, SEEK_CUR) : -1);
}

bool zFile::seek(i32 pos, i32 mode) const {
    return ((pos >= 0 && hf > 0) && lseekf(hf, (long)pos, mode) == pos);
}

int zFile::countFiles() const {
    // определить количество файлов в архиве
    int idx(0);
    if(hz) {
        idx = unzCountFiles(hz->unz);
    } else if(hf > 0) {
        idx = 1;
    }
    return idx;
}

bool zFile::info(zFileInfo& zfi, int zindex) const {
    zString8 p;
    if(hz) {
        if(unzLocateFile(hz->unz, &zinfo, zindex, nullptr, 0)) return false;
        zfi.atime = (time_t)zinfo.mtime; zfi.ctime = (time_t)zinfo.mtime; zfi.mtime = (time_t)zinfo.mtime;
        zfi.csize = zinfo.csize; zfi.usize = zinfo.usize;
        zfi.attr  = zinfo.attr;  zfi.index = zindex;
        p = zinfo.name;
    } else if(hf > 0) {
        struct stat st{};
        if(fstat(hf, &st)) return false;
        zfi.atime = st.st_atime; zfi.mtime = st.st_mtime; zfi.ctime = st.st_ctime;
        zfi.csize = st.st_size;  zfi.usize = st.st_size;
        zfi.attr = st.st_mode;   zfi.index = -1;
        p = path;
    }
    zfi.setPath(p);
    return (hf > 0 || hz != nullptr);
}

void zFile::close() {
    if(hz) _zipClose(hz)
    else if(hf > 0) closef(hf);
    hz = nullptr; hf = 0;
}

int zFile::length() const {
    if(hf > 0) {
        struct stat st{};
        fstat(hf, &st);
        return (int)st.st_size;
    }
    return (hz ? (int)zinfo.usize : 0);
}

zArray<zFile::zFileInfo> zFile::find(czs& pth, czs& _msk) {
    static char fname[260];
    zArray<zFileInfo> fl; zFileInfo info{}; auto am(_msk.split("*"));
    if(auto dir = opendir(pth)) {
        while(auto ent = readdir(dir)) {
            if((strncmp(ent->d_name, ".", PATH_MAX) == 0) || (strncmp(ent->d_name, "..", PATH_MAX) == 0)) continue;
            // поиск по маске
            bool res(true); auto _s(fname); strcpy(_s, ent->d_name);
            for(auto i = 0; i < am.size(); i++) {
                auto m(am[i]); if(m.isEmpty()) continue;
                auto _m(_s ? strstr(_s, m.str()) : _s);
                if(i == 0 && _m == fname) { _s = _m + 1; continue; }
                if(i > 0 && _m) { _s = _m + 1; continue; }
                res = false; break;
            }
            if(res) {
                info.setPath(pth + ent->d_name);
                struct stat st{};
                if(!stat(info.path, &st)) {
                    info.atime = st.st_atime; info.mtime = st.st_mtime; info.ctime = st.st_ctime;
                    info.csize = st.st_size;  info.usize = st.st_size;
                    info.attr  = st.st_mode;  info.index = -1;
                    if(info.attr & S_IFDIR) info.path.slash();
                    fl += info;
                }
            }
        }
        closedir(dir);
    }
    return fl;
}
