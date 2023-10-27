
#pragma once

#define FOLDER_CACHE    -1
#define FOLDER_FILES    -2
#define FOLDER_ROOT     -3

class zSettings {
public:
    struct ZOPTION {
        enum class TOPTION { _unk, _flt, _hex, _int, _bol, _byt, _mru, _pth };
        ZOPTION() { }
        ZOPTION(zString8 _name, zString8 _value, TOPTION _type, int _offs) : name(_name), value(_value), type(_type), offs(_offs) { }
        bool operator == (cstr nm) const { return name == nm; }
        // имя настройки
        zString8 name;
        // значение по умолчанию
        zString8 value;
        // тип
        TOPTION type;
        // смещение
        int offs;
    };
    // конструктор
    zSettings(cstr root, cstr* opts);
    // инициализация
    void init(u8* ptr, cstr name);
    // сохранение установок
    void save(u8* ptr, cstr name);
    // установить значение по умолчанию для опции
    void setDefault(u8* ptr, cstr opt);
    // вернуть путь по индексу
    zString8 getPath(int idx) const { return (idx >= 0 && idx < paths.size()) ? paths[idx] : ""; }
    // установить путь по индексу
    void setPath(int idx, czs& pth) const { if(idx >= 0 && idx < paths.size()) paths[idx] = pth; }
    // октрыть/добавить файл из MRU
    zString8 mruOpen(int index, czs& path, bool error);
    // вернуть значение по умолчанию(по имени)
    zString8 getDefault(cstr name) const;
    // вернуть значение по умолчанию(по смещению)
    template<typename T> bool getDefault(int offs, T* v) const {
        for(auto& o : defs) {
            if(o.offs == offs) {
                int radix(RADIX_DEC);
                if(o.type == ZOPTION::TOPTION::_bol) radix = RADIX_BOL;
                else if(o.type == ZOPTION::TOPTION::_flt) radix = RADIX_FLT;
                else if(o.type == ZOPTION::TOPTION::_hex) radix = RADIX_HEX;
                if(v) *v = z_ston<T>(o.value, radix);
                return true;
            }
        }
        return false;
    }
    // сформировать путь от папки проги
    zString8 makePath(cstr pth, int type) const;
    // вернуть декоративную MRU строку
    zString8 mruDecorate(int index) const { auto s(mrus[index]); return s.substrAfterLast("/", s); }
protected:
    // установка опции
	void setOption(u8* ptr, const ZOPTION& opt);
    // получение опции
    zString8 getOption(const u8* ptr, int idx);
    // корневой путь
    zString8 pathRoot, pathFiles, pathCache;
    // массив опций со значениями по умолчанию
    zArray<ZOPTION> defs;
    // массив последних файлов
    zArray<zString8> mrus;
    // массив путей
    zArray<zString8> paths;
};
