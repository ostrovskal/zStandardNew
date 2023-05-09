
#pragma once

#include "zstandard/zGRef.h"

class zClass {
public:
	enum class JRet { jvoid, jbool, jchar, jbyte, jshort, jint, jlong, jfloat, jdouble, jstring, jobject };
	struct MEMBER {
		union JID {
			jfieldID  jfd;
			jmethodID jmd;
		};
		MEMBER(bool f, bool s, bool a, const zString& n, JRet r) : _field(f), _static(s), _array(a), name(n), jret(r) { }
		bool _field, _static, _array;
		zString name;
		JRet jret;
		JID jid;
	};
	zClass(cstr jc) { cref.ref(jenv->FindClass(jc)); }
	~zClass() { }
	// добавление метода/поля
	void add(cstr name);
	// вызов метода
	template<typename T> T call(cstr name, jobject obj, ...) {
		va_list args; va_start(args, obj);
		auto ret(process(name, obj, false, args));
		va_end(args); return *(T*)ret;
	}
	void call(cstr name, jobject obj, ...) {
		va_list args; va_start(args, obj);
		process(name, obj, false, args); va_end(args);
	}
	// получение поля
	template<typename T> T get(cstr name, jobject obj, ...) {
		va_list args; va_start(args, obj);
		auto ret(process(name, obj, false, args));
		va_end(args); return *(T*)ret;
	}
	// установка поля
	template<typename T> void set(cstr name, jobject obj, ...) {
		va_list args; va_start(args, obj);
		process(name, obj, true, args); va_end(args);
	}
	// установка окружения
	static JNIEnv* setEnv() { jvm->GetEnv((void**)&jenv, JNI_VERSION_1_6); return jenv; }
protected:
	MEMBER* getMember(cstr name);
	// обработка call/get/set
	u8* process(cstr name, jobject obj, bool set, va_list args);
	// глобальная ссылка на класс
	zGRef cref;
	// массив методов/полей
	zArray<MEMBER*> members;
};
