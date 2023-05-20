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

bool zFile::open(cstr pth, bool read, bool zipped, bool append) {
    close();
    if(pth) {
        if(strstr(pth, ".zip")) {
            if(zipped) {
                hz = (read ? unzOpen(pth) : zipOpen(pth, APPEND_STATUS_CREATE));
            }
        }
        if(!hz) {
#ifdef WIN32
            u32 access(read ? O_RDWR : O_WRONLY);
            u32 mode(read ? S_IREAD : S_IWRITE);
            access |= O_BINARY;
#else
            i32 access(read ? O_RDWR : O_WRONLY);
            u32 mode(S_IWRITE | S_IREAD);
#endif
            if(!read) { if(!append) access |= O_CREAT | O_TRUNC; else access |= O_APPEND; }
            hf = openf(pth, access, mode);
        }
    }
    bool ret(hf > 0 || hz != nullptr);
    if(ret) path = pth;
    return ret;
}

bool zFile::copy(cstr pth, int index) {
    bool ret(false);
    if(hz) {
        ret = (unzUnzipToFile(hz->unz, index, pth) == ZIP_OK);
    } else if(hf > 0) {
        zFile f;
        if(f.open(pth, false, false)) {
            int bsz(256 * 1024), sz(0);
            auto ptr(new u8[bsz]);
            while(read(&sz, ptr, bsz)) {
                if(sz) { f.write(ptr, sz); ret = true; }
                if(sz != bsz) break;
            }
            delete[] ptr;
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
        if(pos != -1) seek(pos, mode);
        ret = readf(hf, ptr, size) == (int)size;
    }
    if(psize) *psize = size;
    return (ret ? ptr : nullptr);
}

void* zFile::readn(int* psize, int size, int pos, int mode) const {
    u8* ptr(nullptr); bool ret(false);
    if(hz) {
        ret = (unzUnzipToMem(hz->unz, pos, &ptr, (u32*)&size) == ZIP_OK);
    } else if(hf > 0) {
        if(pos != -1) seek(pos, mode);
        if(!size) size = length();
        ptr = new u8[size + 1]; ptr[size] = 0;
        ret = readf(hf, ptr, size) == (int)size;
    }
    if(!ret) {
        SAFE_A_DELETE(ptr);
        size = 0;
    }
    if(psize) *psize = size;
    return ptr;
}

zString zFile::readString(int pos, int mode) const {
    static char stmp[257];
    zString tmp;
    seek(pos, mode);
    tmp.empty();
    if(hf > 0) {
        char ch; int idx(0);
        while(readf(hf, &ch, 1) == 1) {
            if(ch == '\r') continue;
            if(ch == '\n') break;
            stmp[idx++] = ch;
            if(idx > 255) { stmp[idx] = 0; tmp += stmp; idx = 0; }
        }
        if(idx) { stmp[idx] = 0; tmp += stmp; }
    }
    return tmp;
}

zArray<zStringUTF8> zFile::strings() const {
    zArray<zStringUTF8> arr; zStringUTF8 str;
    while((str = readString()).isNotEmpty()) arr += str;
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
    if(hz) {
        if(unzLocateFile(hz->unz, &zinfo, zindex, nullptr, 0)) return false;
        zfi.atime = (time_t)zinfo.mtime; zfi.ctime = (time_t)zinfo.mtime; zfi.mtime = (time_t)zinfo.mtime;
        zfi.csize = zinfo.csize; zfi.usize = zinfo.usize;
        zfi.attr  = zinfo.attr;  zfi.index = zindex;
        strcpy(zfi.name, zinfo.name);
    } else if(hf > 0) {
        struct stat st{};
        if(fstat(hf, &st)) return false;
        zfi.atime = st.st_atime; zfi.mtime = st.st_mtime; zfi.ctime = st.st_ctime;
        zfi.csize = st.st_size;  zfi.usize = st.st_size;
        zfi.attr = st.st_mode;   zfi.index = -1;
        strcpy(zfi.name, path.str());
    }
    zfi.zip = (hz != nullptr);
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
        return (int)st.st_size - tell();
    }
    return (hz ? (int)zinfo.usize : 0);
}

zArray<zFile::zFileInfo> zFile::find(cstr path, cstr _msk) {
    zArray<zFileInfo> fl;
    static char fname[260];
    DIR* dir; struct dirent* ent; zFileInfo info{};
    zString msk(_msk); auto am(msk.split("*"));
    if((dir = opendir(path))) {
        while((ent = readdir(dir))) {
            if((strncmp(ent->d_name, ".", PATH_MAX) == 0) || (strncmp(ent->d_name, "..", PATH_MAX) == 0)) continue;
            // поиск по маске
            bool res(true); auto _s(fname); auto namlen(z_strlen(ent->d_name));
            strcpy(fname, ent->d_name);// z_strlwr(fname);
            for(int i = 0; i < am.size(); i++) {
                auto m(am[i]);
                if(m.isEmpty()) continue;
                auto _m(strstr(_s, m.str()));
                if(i == 0 && _m == fname) { _s = _m + 1; continue; }
                if(i > 0 && _m) { _s = _m + 1; continue; }
                res = false; break;
            }
            if(res) {
                res = false;
                auto l1(z_strlen(path));
                strcpy(info.name, path);
                strcpy(info.name + l1, ent->d_name);
                if(ent->d_type & DT_DIR) {
                    struct stat st{};
                    if(!stat(info.name, &st)) {
                        info.atime = st.st_atime; info.mtime = st.st_mtime; info.ctime = st.st_ctime;
                        info.csize = st.st_size;  info.usize = st.st_size;
                        info.attr = st.st_mode;   info.index = -1;
                        auto ch(info.name[l1 + namlen]);
                        if(ch != '/' && ch != '\\') info.name[l1 + namlen] = '/';
                        info.name[l1 + namlen + 1] = 0;
                        res = true;
                    }
                } else {
                    zFile f(info.name, true, false);
                    res = f.info(info); f.close();
                }
                if(res) fl += info; else *info.name = 0;
            }
        }
        closedir(dir);
    }
    return fl;
}
