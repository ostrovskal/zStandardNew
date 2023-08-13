//
// Created by Sergo on 10.12.2020.
//

#pragma once

class zFile {
public:
    struct zFileInfo {
        void setPath(zString8 p) {
            path = p;
            zip = p.right(4).compare(".zip");
        }
        // признак архива
        bool zip;
        // индекс элемента
        int index;
        // имя
        zString8 path;
        // атрибуты
        u32 attr;
        // время
        time_t atime, ctime, mtime;
        // размер(compress/uncompress)
        long long csize, usize;
    };
    // конструктор по умолчанию
    zFile() { }
    // конструктор по значению
    zFile(czs& pth, bool read, bool zipped = true, bool append = false) { _open(pth, read, zipped, append); }
    // деструктор
    virtual ~zFile() { close(); }
    // открыть/создать файл
    virtual bool open(czs& pth, bool read, bool zipped = true, bool append = false);
    // закрыть
    virtual void close();
    // копировать/распаковать в папку
    bool copy(czs& pth, i32 index);
    // прочитать в буфер
    virtual void* read(i32* psize, void* ptr, i32 size = 0, i32 pos = -1, i32 mode = 0) const;
    // прочитать в автобуфер
    virtual void* readn(i32* psize = nullptr, i32 size = 0, i32 pos = -1, i32 mode = 0) const;
    // прочитать строку
    virtual zString readString(i32 pos = -1, i32 mode = 0) const;
    virtual zString8 readString8(int pos = -1, int mode = 0) const;
    // вернуть массив строк из файла
    zArray<zString8> strings() const;
    // записать буфер
    virtual bool write(void* ptr, int size, cstr name = nullptr) const;
    // записать строку
    virtual bool writeString(cstr s, bool clr) const;
    // определить количество файлов
    virtual int	countFiles() const;
    // получить информацию
    virtual bool info(zFileInfo& zfi, int zindex = 0) const;
    // вернуть признак архива
    bool isZip() const { return hz != nullptr; }
    // установить позицию
    virtual bool seek(i32 pos, i32 mode) const;
    // вернуть длину
    virtual int length() const;
    // вернуть позицию
    virtual int tell() const;
    // вернуть дескриптор
    int getFd() const { return hf; }
    // вернуть путь
    zString8 pth() const { return path; }
    // найти
    static zArray<zFileInfo> find(czs& pth, czs& _msk);
    // проверка на существование файла/папки
    static bool isFile(cstr pth) { struct stat sb{}; return (stat(pth, &sb) ? 0 : (sb.st_mode & S_IFREG) != 0); }
    static bool isFolder(cstr pth) { struct stat sb{}; return (stat(pth, &sb) ? 0 : (sb.st_mode & S_IFDIR) != 0); }
    // переименование/перемещение
    static bool	move(cstr _old, cstr _new) { return rename(_old, _new) == 0; }
    // удаление
    static bool remove(cstr pth) { return unlinkf(pth) == 0; }
protected:
    bool    _open(czs& pth, bool read, bool zipped, bool append) { return open(pth, read, zipped, append); }
    int		hf{0};
    HZIP* 	hz{nullptr};
    zString8 path{};
};

