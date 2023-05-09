
#pragma once

class zJSON {
public:
    enum JType { _str, _digit, _bool, _array, _object };
    struct Node {
        Node() { }
        Node(Node* p, cstr k, cstr v, JType t) : key(k), val(v), tp{ t } { if(p) p->child += this; }
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
        const zString& string() const { return val; }
        // ключь
        zString key{ "" };
        // значение
        zString val{ "" };
        // тип
        JType tp{ _str };
        // массив дочерних
        zArray<Node*> child;
    };
    zJSON() { }
    zJSON(u8* ptr, int size) { init(ptr, size); }
    virtual ~zJSON() { delete root; root = nullptr; }
    // признак валидности
    bool isValid() const { return error == 0; }
    // инициализация
    bool init(u8* ptr, int size);
    // сформировать строку
    zString save();
    // вернуть корень
    const Node* getRoot() const { return root; }
    // вернуть узел по индексу
    const Node* getNode(int idx, const Node* node = nullptr) const;
    // вернуть узел по имени
    const Node* getNode(cstr name, const Node* node = nullptr) const;
    // вернуть узел из пути('/Name/_Index/_Index/Name/...')
    const Node* getPath(cstr path, const Node* node = nullptr) const;
    // добавить узел
    Node* addNode(cstr key, cstr val, JType jt, Node* node = nullptr);
protected:
    // получение литерала
    bool jliteral();
    // следующий символ
    char jnext() { return jcur < jend ? *jcur++ : 0; }
    // парсер
    bool parse(Node* p, JType jt, int lev);
    // массив для чисел
    char digBuf[128]{};
    // текущая линия/признак ошибки
    int line{ 1 }, error{ 1 };
    // текущая позиция/конечная позиция/позиция литерала
    char* jcur{ nullptr }, *jend{ nullptr }, *lit{ nullptr };
    // тип литерала
    JType _jt{ _str };
    // корневой элемент
    Node* root{ nullptr };
    // пустышка
    static Node dummy;
};

