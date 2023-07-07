
#pragma once

class zJSON {
public:
    enum JType { _str, _digit, _bool, _array, _object };
    struct zNode {
        zNode() { }
        zNode(zNode* p, czs& k, czs& v, JType t) : key(k), val(v), tp(t) { if(p) p->child += this; }
        bool operator == (cstr n) const { return key == n; }
        // проверить на соответствие типа
        bool isType(JType t) const { return tp == t; }
        // вернуть количество дочерних
        int count() const { return child.size(); }
        // вернуть вещественное
        double real() const { return z_ston<double>(val, RADIX_DBL); }
        // вернуть целое
        int integer() const { return z_ston<int>(val, RADIX_DEC); }
        // вернуть булево
        bool boolean() const { return val == "true"; }
        // вернуть строку
        zStringUTF8 string() const { return val; }
        // ключь
        zStringUTF8 key{""};
        // значение
        zStringUTF8 val{""};
        // тип
        JType tp{_str};
        // массив дочерних
        zArray<zNode*> child;
    };
    zJSON() { }
    zJSON(u8* ptr, int size) { init(ptr, size); }
    virtual ~zJSON() { clear(); }
    // признак валидности
    bool isValid() const { return error == 0; }
    // инициализация
    bool init(u8* ptr, int size);
    // очистка
    void clear() { SAFE_DELETE(root); }
    // сформировать строку
    zStringUTF8 save();
    // вернуть корень
    const zNode* getRoot() const { return root; }
    // вернуть узел по индексу
    const zNode* operator[](int idx) const { return getNode(idx); }
    const zNode* getNode(int idx, const zNode* node = nullptr) const;
    // вернуть узел по имени
    const zNode* operator[](cstr name) const { return getNode(name); }
    const zNode* getNode(cstr name, const zNode* node = nullptr) const;
    // вернуть узел из пути('/Name/_Index/_Index/Name/...')
    const zNode* getPath(cstr path, const zNode* node = nullptr) const;
    // добавить узел
    zNode* addNode(cstr key, cstr val, JType jt = _str, zNode* node = nullptr);
protected:
    // получение литерала
    bool jliteral();
    // следующий символ
    int jnext();
    // парсер
    bool parse(zNode* p, JType jt, int lev);
    // массив для чисел
    char digBuf[512]{};
    // текущая линия/признак ошибки
    int line{1}, error{1};
    // текущая позиция/конечная позиция/позиция литерала
    char* jcur{nullptr}, *jend{nullptr}, *lit{nullptr};
    // тип литерала
    JType _jt{_str};
    // корневой элемент
    zNode* root{nullptr};
    // пустышка
    static zNode dummy;
};

