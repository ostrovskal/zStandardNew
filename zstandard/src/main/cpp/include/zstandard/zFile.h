//
// Created by Sergo on 10.12.2020.
//

#pragma once

class zFile {
public:
    struct zFileInfo {
        // признак архива
        bool zip;
        // индекс элемента
        int index;
        // имя
        char name[260];
        // атрибуты
        u32 attr;
        // время
        time_t atime, ctime, mtime;
        // размер
        long long csize, usize;
    };
    // конструктор по умолчанию
    zFile() { }
    // конструктор по значению
    zFile(cstr path, bool read, bool zipped = true, bool append = false) { open(path, read, zipped, append); }
    // деструктор
    ~zFile() { close(); }
    // открыть/создать файл
    virtual bool open(cstr path, bool read, bool zipped = true, bool append = false);
    // закрыть
    virtual void close();
    // копировать/распаковать в папку
    bool copy(cstr path, i32 index);
    // прочитать в буфер
    virtual void* read(i32* psize, void* ptr, i32 size = 0, i32 pos = -1, i32 mode = 0) const;
    // прочитать в автобуфер
    virtual void* readn(i32* psize = nullptr, i32 size = 0, i32 pos = -1, i32 mode = 0) const;
    // прочитать строку
    virtual zString readString(i32 pos = -1, i32 mode = 0) const;
    // вернуть массив строк из файла
    zArray<zString> strings() const;
    // записать буфер
    virtual bool write(void* ptr, int size, cstr name = nullptr) const;
    // записать строку
    virtual bool writeString(const zString& s, bool clr) const;
    // определить количество файлов
    virtual int	countFiles() const;
    // получить информацию
    bool info(zFileInfo& zfi, int zindex = 0) const;
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
    cstr pth() const { return path; }
    // найти
    static zArray<zFileInfo> find(const zString& path, cstr _msk);
    // переименование/перемещение
    static bool	move(cstr _old, cstr _new) { return rename(_old, _new) == 0; }
    // удаление
    static bool remove(cstr path) { return unlinkf(path) == 0; }
protected:
    int		hf{0};
    HZIP* 	hz{nullptr};
    zString path{};
};

