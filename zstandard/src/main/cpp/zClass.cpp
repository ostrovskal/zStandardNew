
#include "zstandard/zstandard.h"
#include "zstandard/zClass.h"

JavaVM* jvm(nullptr);
thread_local    JNIEnv* jenv(nullptr);

static int getJType(zString t, bool param, bool& array) {
	static cstr rtp[] = { "void", "bool", "char", "byte", "short", "int", "long", "float", "double", "string" };
	array = (t.substrBefore('_') == "array");
	if(array) t = t.substrAfter('_');
	for(int i = param; i < 10; i++) {
		if(t == rtp[i]) return i;
	}
	return 10;
}

void zClass::add(cstr nm) {
	// [static] rtype name(type1, type2, type3, ...)
	static JRet jret[] = { JRet::jvoid, JRet::jbool, JRet::jchar, JRet::jbyte, JRet::jshort, JRet::jint, JRet::jlong, JRet::jfloat, JRet::jdouble, JRet::jstring, JRet::jobject };
	static cstr stp[] = { "V", "Z", "C", "B", "S", "I", "J", "F", "D", "Ljava/lang/String;" };
	zString sign; bool _array;
	auto tmp(strchr(nm, '('));
	auto _field(tmp == nullptr);
	auto decl(zString(nm, _field ? -1 : tmp - nm).split(" "));
	auto _static(decl[0] == "static");
	auto rt(getJType(decl[_static], false, _array));
	auto name(decl[_static + 1]);
	auto mem(new MEMBER(_field, _static, _array, name, jret[rt]));
	if(tmp) {
		// method
		zString p(tmp + 1); p = p.substrBeforeLast(')');
		sign = '('; bool _arr;
		auto pars(p.lower().split(", "));
		for(int i = 0; i < pars.size(); i++) {
			auto rt(getJType(pars[i], true, _arr));
			auto srt(rt == 10 ? 'L' + pars[i] + ';' : zString(stp[rt]));
			if(_arr) sign += '[';
			sign += srt;
		}
		sign += ')';
	}
	if(_array) sign += '[';
	sign += (rt == 10 ? 'L' + decl[_static] + ';' : zString(stp[rt]));
	if(_field) {
		mem->jid.jfd = (_static ? jenv->GetStaticFieldID(cref, name, sign) : jenv->GetFieldID(cref, name, sign));
	} else {
		mem->jid.jmd = (_static ? jenv->GetStaticMethodID(cref, name, sign) : jenv->GetMethodID(cref, name, sign));
	}
	if(mem->jid.jmd) members += mem;
	else delete mem;
}

zClass::MEMBER* zClass::getMember(cstr name) {
	for(int i = 0; i < members.size(); i++) {
		if(members[i]->name == name)
			return members[i];
	}
	return nullptr;
}

u8* zClass::process(cstr name, jobject obj, bool set, va_list args) {
	static u8 ret[16];
	auto mem(getMember(name));
	if(mem) {
		jclass cls(cref); auto s(mem->_static);
		if(!obj) obj = jenv->GetObjectClass(cref);
		if(mem->_field) {
			auto fid(mem->jid.jfd);
			if(set) {
				switch(mem->jret) {
					case JRet::jbool:  (s ? jenv->SetStaticBooleanField(cls, fid, (jboolean)va_arg(args, int)) : jenv->SetBooleanField(obj, fid, (jboolean)va_arg(args, int))); break;
					case JRet::jbyte:  (s ? jenv->SetStaticByteField(cls, fid, (jbyte)va_arg(args, int)) : jenv->SetByteField(obj, fid, (jbyte)va_arg(args, int))); break;
					case JRet::jchar:  (s ? jenv->SetStaticCharField(cls, fid, (jchar)va_arg(args, int)) : jenv->SetCharField(obj, fid, (jchar)va_arg(args, int))); break;
					case JRet::jshort: (s ? jenv->SetStaticShortField(cls, fid, (jshort)va_arg(args, int)) : jenv->SetShortField(obj, fid, (jshort)va_arg(args, int))); break;
					case JRet::jint:   (s ? jenv->SetStaticIntField(cls, fid, va_arg(args, int)) : jenv->SetIntField(obj, fid, va_arg(args, int))); break;
					case JRet::jlong:  (s ? jenv->SetStaticLongField(cls, fid, va_arg(args, jlong)) : jenv->SetLongField(obj, fid, va_arg(args, jlong))); break;
					case JRet::jfloat: (s ? jenv->SetStaticFloatField(cls, fid, (jfloat)va_arg(args, double)) : jenv->SetFloatField(obj, fid, (jfloat)va_arg(args, double))); break;
					case JRet::jdouble:(s ? jenv->SetStaticDoubleField(cls, fid, va_arg(args, double)) : jenv->SetDoubleField(obj, fid, va_arg(args, double))); break;
					case JRet::jstring:
					case JRet::jobject:(s ? jenv->SetStaticObjectField(cls, fid, va_arg(args, jobject)) : jenv->SetObjectField(obj, fid, va_arg(args, jobject))); break;
					case JRet::jvoid: break;
				}
			} else {
				switch(mem->jret) {
					case JRet::jbool:  *(jboolean*)ret	= (s ? jenv->GetStaticBooleanField(cls, fid) : jenv->GetBooleanField(obj, fid)); break;
					case JRet::jbyte:  *(jbyte*)ret		= (s ? jenv->GetStaticByteField(cls, fid) : jenv->GetByteField(obj, fid)); break;
					case JRet::jchar:  *(jchar*)ret		= (s ? jenv->GetStaticCharField(cls, fid) : jenv->GetCharField(obj, fid)); break;
					case JRet::jshort: *(jshort*)ret	= (s ? jenv->GetStaticShortField(cls, fid) : jenv->GetShortField(obj, fid)); break;
					case JRet::jint:   *(jint*)ret		= (s ? jenv->GetStaticIntField(cls, fid) : jenv->GetIntField(obj, fid)); break;
					case JRet::jlong:  *(jlong*)ret		= (s ? jenv->GetStaticLongField(cls, fid) : jenv->GetLongField(obj, fid)); break;
					case JRet::jfloat: *(jfloat*)ret	= (s ? jenv->GetStaticFloatField(cls, fid) : jenv->GetFloatField(obj, fid)); break;
					case JRet::jdouble:*(jdouble*)ret	= (s ? jenv->GetStaticDoubleField(cls, fid) : jenv->GetDoubleField(obj, fid)); break;
					case JRet::jstring:
					case JRet::jobject:*(jobject*)ret	= (s ? jenv->GetStaticObjectField(cls, fid) : jenv->GetObjectField(obj, fid)); break;
					case JRet::jvoid: break;
				}
			}
		} else {
			auto mid(mem->jid.jmd);
			switch(mem->jret) {
				case JRet::jvoid:					  (s ? jenv->CallStaticVoidMethod(cls, mid, args) : jenv->CallVoidMethod(obj, mid, args)); break;
				case JRet::jbool:  *(jboolean*)ret	= (s ? jenv->CallStaticBooleanMethod(cls, mid, args) : jenv->CallBooleanMethod(obj, mid, args)); break;
				case JRet::jbyte:  *(jbyte*)ret		= (s ? jenv->CallStaticByteMethod(cls, mid, args) : jenv->CallByteMethod(obj, mid, args)); break;
				case JRet::jchar:  *(jchar*)ret		= (s ? jenv->CallStaticCharMethod(cls, mid, args) : jenv->CallCharMethod(obj, mid, args)); break;
				case JRet::jshort: *(jshort*)ret	= (s ? jenv->CallStaticShortMethod(cls, mid, args) : jenv->CallShortMethod(obj, mid, args)); break;
				case JRet::jint:   *(jint*)ret		= (s ? jenv->CallStaticIntMethod(cls, mid, args) : jenv->CallIntMethod(obj, mid, args)); break;
				case JRet::jlong:  *(jlong*)ret		= (s ? jenv->CallStaticLongMethod(cls, mid, args) : jenv->CallLongMethod(obj, mid, args)); break;
				case JRet::jfloat: *(jfloat*)ret	= (s ? jenv->CallStaticFloatMethod(cls, mid, args) : jenv->CallFloatMethod(obj, mid, args)); break;
				case JRet::jdouble:*(jdouble*)ret	= (s ? jenv->CallStaticDoubleMethod(cls, mid, args) : jenv->CallDoubleMethod(obj, mid, args)); break;
				case JRet::jstring:
				case JRet::jobject:*(jobject*)ret	= (s ? jenv->CallStaticObjectMethod(cls, mid, args) : jenv->CallObjectMethod(obj, mid, args)); break;
			}
		}
	}
	return ret;
}
