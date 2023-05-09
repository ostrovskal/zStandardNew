
#pragma once

#define FOLDER_CACHE    -1
#define FOLDER_FILES    -2
#define FOLDER_ROOT     -3

class zSettings {
public:
    struct ZOPTION {
        enum class TOPTION { _unk, _hex, _int, _bol, _byt, _mru, _pth };
        ZOPTION() { }
        ZOPTION(zString _name, zString _value, TOPTION _type, int _offs) : name(_name), value(_value), type(_type), offs(_offs) { }
        bool operator == (cstr nm) const { return name == nm; }
        // имя настройки
        zString name;
        // значение по умолчанию
        zString value;
        // тип
        TOPTION type;
        // смещение
        int offs;
    };
    zSettings(cstr root, cstr* opts);
	virtual ~zSettings() { }
    // инициализация
    void init(u8* ptr, cstr name);
    // сохранение установок
    void save(u8* ptr, cstr name);
    // вернуть строку
    cstr getPath(int idx) const { return (idx >= 0 && idx < paths.size()) ? paths[idx].str() : ""; }
    // октрыть файл из MRU
    cstr mruOpen(int index, cstr path, bool error);
    // вернуть значение по умолчанию
    cstr getDefault(cstr name) const { auto idx(defs.indexOf(name)); return (idx >= 0 && idx < defs.size() ? defs[idx].value.str() : nullptr); }
    // установить значение по умолчанию для опции
    void setDefault(u8* ptr, cstr opt);
    // сформировать путь от папки проги
    zString makePath(cstr pth, int type) const;
    // вернуть декоративную MRU строку
    zString mruDecorate(int index) const;
protected:
    // установка опции
	void setOption(u8* ptr, const ZOPTION& opt);
    // получение опции
	zString getOption(const u8* ptr, int idx);
    // корневой путь
    zString pathRoot, pathFiles, pathCache;
    // массив опций со значениями по умолчанию
    zArray<ZOPTION> defs;
    // массив последних файлов
    zArray<zString> mrus;
    // массив путей
    zArray<zString> paths;
};
