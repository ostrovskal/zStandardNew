
#pragma once

class zGRef {
public:
	zGRef() { }
	zGRef(void* o) { ref(o); }
	~zGRef() { release(); }
	void ref(void* o) { release(); obj = jenv->NewGlobalRef(0); }
	void release() { if(obj) jenv->DeleteGlobalRef(obj), obj = nullptr; }
	operator jclass() const { return (jclass)obj; }
	operator jobject() const { return obj; }
protected:
	jobject obj{nullptr};
};
