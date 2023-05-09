//
// Created by Sergo on 20.01.2021.
//

#pragma once

class zXml {
public:
	struct zNode {
		friend class zXml;
		struct zAttr {
			zAttr() { }
			zAttr(const zString& _n, const zString& _v) : value(_v) {
				ns = _n.substrBefore(':', "");
				name = _n.substrAfter(':', _n);
			}
			bool operator == (cstr n) const { return name == n; }
			zString ns;
			zString name;
			zString value;
		};
		zNode() { }
		zNode(zNode* p) : parent(p) {
			p->children += this;
		}
		zNode(cstr _name, cstr _value, zNode* p) : name(_name), value(_value) {
			p->children += this;
		}
		// оператор сравнения по имени
		bool operator == (cstr n) const { return name == n; }
		// вернуть количество дочерних тегов
		int getCountNodes() const { return children.size(); }
		// вернуть количество аттрибутов
		int getCountAttrs() const { return attrs.size(); }
		// добавить тэг
		const zNode* addTag(cstr _name, cstr _value) {
			return new zNode(_name, _value, this);
		}
		// добавить атрибут
		const zAttr* addAttr(cstr _name, cstr _value) {
			auto a(new zAttr(_name, _value));
			attrs += a;
			return a;
		}
		// вернуть тег по индексу
		const zNode* getTag(int idx) const {
			return ((idx >= 0 && idx < children.size()) ? children[idx] : nullptr);
		}
		// вернуть тег по имени
		const zNode* getTag(cstr _name) const {
			auto idx(children.indexOf<cstr>(_name));
			return idx == -1 ? nullptr : children[idx];
		}
		// вернуть атрибут по индексу
		const zAttr* getAttr(int idx) const {
			return ((idx >= 0 && idx < attrs.size()) ? attrs[idx] : nullptr);
		}
		// вернуть ноду атрибута по имени
		const zAttr* getAttr(cstr _name) const {
			auto idx(attrs.indexOf<cstr>(_name));
			return idx == -1 ? nullptr : attrs[idx];
		}
		// вернуть значение атрибута по имени
		zString getAttrVal(cstr _name, cstr def) const {
			auto idx(attrs.indexOf<cstr>(_name));
			return idx == -1 ? zString(def) : attrs[idx]->value;
		}
		// вернуть имя тега
		const zString& getName() const { return name; }
		// вернуть значение тега
		const zString& getVal() const { return value; }
	protected:
		// имя
		zString name;
		// значение
		zString value;
		// дочерние ноды
		zArray<zNode*> children;
		// атрибуты
		zArray<zAttr*> attrs;
		// родительская нода
		zNode* parent{nullptr};
	};
	// конструктор из памяти
	zXml(u8* ptr, i32 size);
	// конструктор из файла
	zXml(cstr path);
	// деструктор
	~zXml() { close(); }
	// закрыть
	void close();
	// вставить корневой тэг
	const zNode* insertRoot(cstr _name) {
		auto n(new zNode());
		n->name = _name; n->children += root;
		root->parent = n;
		root = n; return n;
	}
	// открыть
	bool open(cstr path, u8* ptr, i32 size);
	// признак валидности
	bool isValid() const { return root != nullptr; }
	// сохранение
	bool save(cstr path);
	// вернуть корневой
	const zNode* getRoot() const { return root; }
	// вернуть путь 
	const zString& getPath() const { return path; }
	// текущая ошибка/строка
	int err{ERROR_OK}, line{1};
protected:
	enum {
		ERROR_OK, ERROR_EOF, ERROR_INVALID_TAG,
		ERROR_INVALID_STRUCTURED, ERROR_ATTR_VALUE, ERROR_INVALID_TAG_VALUE
	};
	// получить имя
	zString getName();
	// получить значение
	zString getValue(char delim);
	// блок CDATA
	zString cdata();
	// сохранить
	void _save(zNode* n, zFile* f, int tab);
	// распарсить
	bool parser(zNode* p);
	// декодирование
	bool decode();
	// комментарий
	bool skipComment();
	// начало/конец декодирования
	char* xml{nullptr}, *end{nullptr};
	// версия
	float version{1.0f};
	// кодировка
	zString encoding{"utf-8"};
	// путь
	zString path;
	// заголовок
	zNode* capt{nullptr};
	// корневая нода
	zNode* root{nullptr};
};
