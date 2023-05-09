//
// Created by User on 28.04.2023.
//

#include "zstandard/zstandard.h"
#include "zstandard/zJniHelper.h"

#define NATIVEACTIVITY_CLASS_NAME "android/app/NativeActivity"

zJniHelper* zJniHelper::instance() {
    static zJniHelper helper;
    return &helper;
}

zJniHelper::~zJniHelper() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto env(attachCurrentThread());
    env->DeleteGlobalRef(jni_helper_java_ref_);
    env->DeleteGlobalRef(jni_helper_java_class_);
    detachCurrentThread();
}

void zJniHelper::init(ANativeActivity* activity, cstr helper_class_name) {
    auto& helper(*instance());
    helper.activity_ = activity;
    // Lock mutex
    std::lock_guard<std::mutex> lock(helper.mutex_);
    auto env(helper.attachCurrentThread());
    jclass android_content_Context(env->GetObjectClass(helper.activity_->clazz));
    jmethodID midGetPackageName(env->GetMethodID(android_content_Context, "getPackageName", "()Ljava/lang/String;"));
    auto packageName((jstring)env->CallObjectMethod(helper.activity_->clazz, midGetPackageName));
    cstr appname(env->GetStringUTFChars(packageName, nullptr));
    helper.app_name_ = appname;
    if(helper_class_name) {
        jclass cls = helper.retrieveClass(env, helper_class_name);
        helper.jni_helper_java_class_ = (jclass) env->NewGlobalRef(cls);
        jmethodID constructor = env->GetMethodID(helper.jni_helper_java_class_, "<init>", "(Landroid/app/NativeActivity;)V");
        helper.jni_helper_java_ref_ = env->NewObject(helper.jni_helper_java_class_, constructor, activity->clazz);
        helper.jni_helper_java_ref_ = env->NewGlobalRef(helper.jni_helper_java_ref_);
        // Get app label
        auto labelName((jstring) helper.callObjectMethod("getApplicationName", "()Ljava/lang/String;"));
        cstr label = env->GetStringUTFChars(labelName, nullptr);
        helper.app_label_ = label;
        env->ReleaseStringUTFChars(labelName, label);
        env->DeleteLocalRef(labelName);
        env->DeleteLocalRef(cls);
    }
    env->ReleaseStringUTFChars(packageName, appname);
    env->DeleteLocalRef(packageName);
}

void zJniHelper::init(ANativeActivity* activity, cstr helper_class_name, cstr native_soname) {
    init(activity, helper_class_name);
    if(native_soname) {
        auto& helper(*instance());
        // Lock mutex
        std::lock_guard<std::mutex> lock(helper.mutex_);
        auto env(helper.attachCurrentThread());
        // Setup soname
        jstring soname = env->NewStringUTF(native_soname);
        jmethodID mid = env->GetMethodID(helper.jni_helper_java_class_, "loadLibrary", "(Ljava/lang/String;)V");
        env->CallVoidMethod(helper.jni_helper_java_ref_, mid, soname);
        env->DeleteLocalRef(soname);
    }
}

zString zJniHelper::getExternalFilesDir() {
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex_);
    // First, try reading from externalFileDir;
    auto env(attachCurrentThread());
    jstring strPath = getExternalFilesDirJString(env);
    cstr path(env->GetStringUTFChars(strPath, nullptr));
    zString s(path);
    env->ReleaseStringUTFChars(strPath, path);
    env->DeleteLocalRef(strPath);
    return s;
}

uint32_t zJniHelper::loadTexture(cstr file_name, int32_t* outWidth, int32_t* outHeight, bool* hasAlpha) {
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex_);
    auto env(attachCurrentThread());
    jstring name(env->NewStringUTF(file_name));
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    jmethodID mid = env->GetMethodID(jni_helper_java_class_, "loadTexture", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject out = env->CallObjectMethod(jni_helper_java_ref_, mid, name);
    jclass javaCls = retrieveClass(env, "com/sample/helper/NDKHelper$TextureInformation");
    jfieldID fidRet = env->GetFieldID(javaCls, "ret", "Z");
    jfieldID fidHasAlpha = env->GetFieldID(javaCls, "alphaChannel", "Z");
    jfieldID fidWidth = env->GetFieldID(javaCls, "originalWidth", "I");
    jfieldID fidHeight = env->GetFieldID(javaCls, "originalHeight", "I");
    bool ret = env->GetBooleanField(out, fidRet);
    bool alpha = env->GetBooleanField(out, fidHasAlpha);
    int32_t width = env->GetIntField(out, fidWidth);
    int32_t height = env->GetIntField(out, fidHeight);
    if(!ret) {
        glDeleteTextures(1, &tex);
        tex = -1;
        DLOG("Texture load failed %s", file_name);
    }
    DLOG("Loaded texture original size:%dx%d alpha:%d", width, height, (int32_t)alpha);
    if(outWidth) *outWidth = width;
    if(outHeight) *outHeight = height;
    if(hasAlpha) *hasAlpha = alpha;
    // Generate mipmap
    glGenerateMipmap(GL_TEXTURE_2D);
    env->DeleteLocalRef(name);
    detachCurrentThread();
    return tex;
}

uint32_t zJniHelper::loadCubemapTexture(cstr file_name, const int32_t face, const int32_t miplevel, const bool,
                                       int32_t* outWidth, int32_t* outHeight, bool* hasAlpha) {
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex_);
    auto env(attachCurrentThread());
    jstring name = env->NewStringUTF(file_name);
    jmethodID mid = env->GetMethodID(jni_helper_java_class_, "loadCubemapTexture", "(Ljava/lang/String;IIZ)Ljava/lang/Object;");
    jobject out = env->CallObjectMethod(jni_helper_java_ref_, mid, name, face, miplevel);
    jclass javaCls = retrieveClass(env, "com/sample/helper/NDKHelper$TextureInformation");
    jfieldID fidRet = env->GetFieldID(javaCls, "ret", "Z");
    jfieldID fidHasAlpha = env->GetFieldID(javaCls, "alphaChannel", "Z");
    jfieldID fidWidth = env->GetFieldID(javaCls, "originalWidth", "I");
    jfieldID fidHeight = env->GetFieldID(javaCls, "originalHeight", "I");
    bool ret = env->GetBooleanField(out, fidRet);
    bool alpha = env->GetBooleanField(out, fidHasAlpha);
    int32_t width = env->GetIntField(out, fidWidth);
    int32_t height = env->GetIntField(out, fidHeight);
    if(!ret) DLOG("Texture load failed %s", file_name);
    DLOG("Loaded texture original size:%dx%d alpha:%d", width, height, (int32_t)alpha);
    if(outWidth) *outWidth = width;
    if(outHeight) *outHeight = height;
    if(hasAlpha) *hasAlpha = alpha;
    env->DeleteLocalRef(name);
    env->DeleteLocalRef(javaCls);
    return 0;
}

jobject zJniHelper::loadImage(cstr file_name, int32_t* outWidth, int32_t* outHeight, bool* hasAlpha) {
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex_);
    auto env(attachCurrentThread());
    jstring name = env->NewStringUTF(file_name);
    jmethodID mid = env->GetMethodID(jni_helper_java_class_, "loadImage", "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject out = env->CallObjectMethod(jni_helper_java_ref_, mid, name);
    jclass javaCls = retrieveClass(env, "com/sample/helper/NDKHelper$TextureInformation");
    jfieldID fidRet = env->GetFieldID(javaCls, "ret", "Z");
    jfieldID fidHasAlpha = env->GetFieldID(javaCls, "alphaChannel", "Z");
    jfieldID fidWidth = env->GetFieldID(javaCls, "originalWidth", "I");
    jfieldID fidHeight = env->GetFieldID(javaCls, "originalHeight", "I");
    bool ret = env->GetBooleanField(out, fidRet);
    bool alpha = env->GetBooleanField(out, fidHasAlpha);
    int32_t width = env->GetIntField(out, fidWidth);
    int32_t height = env->GetIntField(out, fidHeight);
    if(!ret) DLOG("Texture load failed %s", file_name);
    DLOG("Loaded texture original size:%dx%d alpha:%d", width, height, (int32_t)alpha);
    if(outWidth) *outWidth = width;
    if(outHeight) *outHeight = height;
    if(hasAlpha) *hasAlpha = alpha;
    jfieldID fidImage = env->GetFieldID(javaCls, "image", "Ljava/lang/Object;");
    jobject array = env->GetObjectField(out, fidImage);
    jobject objGlobal = env->NewGlobalRef(array);
    env->DeleteLocalRef(name);
    env->DeleteLocalRef(javaCls);
    return objGlobal;
}

zString zJniHelper::convertString(cstr str, cstr encode) {
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex_);
    auto env(attachCurrentThread());
    env->PushLocalFrame(16);
    auto iLength((int32_t)strlen((cstr)str));
    jbyteArray array = env->NewByteArray(iLength);
    env->SetByteArrayRegion(array, 0, iLength, (const signed char*)str);
    jstring strEncode = env->NewStringUTF(encode);
    jclass cls = env->FindClass("java/lang/String");
    jmethodID ctor = env->GetMethodID(cls, "<init>", "([BLjava/lang/String;)V");
    auto object((jstring)env->NewObject(cls, ctor, array, strEncode));
    cstr cparam = env->GetStringUTFChars(object, nullptr);
    zString s(cparam);
    env->ReleaseStringUTFChars(object, cparam);
    env->DeleteLocalRef(array);
    env->DeleteLocalRef(strEncode);
    env->DeleteLocalRef(object);
    env->DeleteLocalRef(cls);
    env->PopLocalFrame(nullptr);
    return s;
}

zString zJniHelper::getStringResource(const zString& resourceName) {
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex_);
    auto env(attachCurrentThread());
    jstring name = env->NewStringUTF(resourceName.str());
    auto ret((jstring)callObjectMethod("getStringResource", "(Ljava/lang/String;)Ljava/lang/String;", name));
    cstr resource = env->GetStringUTFChars(ret, nullptr);
    zString s(resource);
    env->ReleaseStringUTFChars(ret, resource);
    env->DeleteLocalRef(ret);
    env->DeleteLocalRef(name);
    return s;
}

int32_t zJniHelper::getNativeAudioBufferSize() {
    auto env(attachCurrentThread());
    jmethodID mid = env->GetMethodID(jni_helper_java_class_, "getNativeAudioBufferSize", "()I");
    return env->CallIntMethod(jni_helper_java_ref_, mid);
}

int32_t zJniHelper::getNativeAudioSampleRate() {
    auto env(attachCurrentThread());
    jmethodID mid = env->GetMethodID(jni_helper_java_class_, "getNativeAudioSampleRate", "()I");
    return env->CallIntMethod(jni_helper_java_ref_, mid);
}

jclass zJniHelper::retrieveClass(JNIEnv* jni, cstr class_name) {
    jclass activity_class = jni->FindClass(NATIVEACTIVITY_CLASS_NAME);
    jmethodID get_class_loader = jni->GetMethodID(activity_class, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject cls = jni->CallObjectMethod(activity_->clazz, get_class_loader);
    jclass class_loader = jni->FindClass("java/lang/ClassLoader");
    jmethodID find_class = jni->GetMethodID(class_loader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    jstring str_class_name = jni->NewStringUTF(class_name);
    auto class_retrieved((jclass)jni->CallObjectMethod(cls, find_class, str_class_name));
    jni->DeleteLocalRef(str_class_name);
    jni->DeleteLocalRef(activity_class);
    jni->DeleteLocalRef(class_loader);
    return class_retrieved;
}

jstring zJniHelper::getExternalFilesDirJString(JNIEnv* env) {
    jstring obj_Path = nullptr;
    jclass cls_Env = env->FindClass(NATIVEACTIVITY_CLASS_NAME);
    jmethodID mid = env->GetMethodID(cls_Env, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
    jobject obj_File = env->CallObjectMethod(activity_->clazz, mid, obj_Path);
    if(obj_File) {
        jclass cls_File = env->FindClass("java/io/File");
        jmethodID mid_getPath = env->GetMethodID(cls_File, "getPath", "()Ljava/lang/String;");
        obj_Path = (jstring)env->CallObjectMethod(obj_File, mid_getPath);
    }
    return obj_Path;
}

void zJniHelper::deleteObject(jobject obj) {
    if(!obj) { DLOG("obj can not be NULL"); return; }
    auto env(attachCurrentThread());
    env->DeleteGlobalRef(obj);
}

jobject zJniHelper::callObjectMethod(cstr strMethodName, cstr strSignature, ...) {
    auto env(attachCurrentThread());
    jmethodID mid = env->GetMethodID(jni_helper_java_class_, strMethodName, strSignature);
    if(!mid) { DLOG("method ID %s, '%s' not found", strMethodName, strSignature); return nullptr; }
    va_list args;
    va_start(args, strSignature);
    jobject obj = env->CallObjectMethodV(jni_helper_java_ref_, mid, args);
    va_end(args);
    return obj;
}

void zJniHelper::callVoidMethod(cstr strMethodName, cstr strSignature, ...) {
    auto env(attachCurrentThread());
    jmethodID mid = env->GetMethodID(jni_helper_java_class_, strMethodName, strSignature);
    if(!mid) { DLOG("method ID %s, '%s' not found", strMethodName, strSignature); return; }
    va_list args;
    va_start(args, strSignature);
    env->CallVoidMethodV(jni_helper_java_ref_, mid, args);
    va_end(args);
}

jobject zJniHelper::callObjectMethod(jobject object, cstr strMethodName, cstr strSignature, ...) {
    auto env(attachCurrentThread());
    jclass cls = env->GetObjectClass(object);
    jmethodID mid = env->GetMethodID(cls, strMethodName, strSignature);
    if(!mid) { DLOG("method ID %s, '%s' not found", strMethodName, strSignature); return nullptr; }
    va_list args;
    va_start(args, strSignature);
    jobject obj = env->CallObjectMethodV(object, mid, args);
    va_end(args);
    env->DeleteLocalRef(cls);
    return obj;
}

void zJniHelper::callVoidMethod(jobject object, cstr strMethodName, cstr strSignature, ...) {
    auto env(attachCurrentThread());
    jclass cls = env->GetObjectClass(object);
    jmethodID mid = env->GetMethodID(cls, strMethodName, strSignature);
    if(!mid) { DLOG("method ID %s, '%s' not found", strMethodName, strSignature); return;
    }
    va_list args;
    va_start(args, strSignature);
    env->CallVoidMethodV(object, mid, args);
    va_end(args);
    env->DeleteLocalRef(cls);
}

float zJniHelper::callFloatMethod(jobject object, cstr strMethodName, cstr strSignature, ...) {
    float f = 0.f;
    auto env(attachCurrentThread());
    jclass cls = env->GetObjectClass(object);
    jmethodID mid = env->GetMethodID(cls, strMethodName, strSignature);
    if(!mid) { DLOG("method ID %s, '%s' not found", strMethodName, strSignature); return f; }
    va_list args;
    va_start(args, strSignature);
    f = env->CallFloatMethodV(object, mid, args);
    va_end(args);
    env->DeleteLocalRef(cls);
    return f;
}

int32_t zJniHelper::callIntMethod(jobject object, cstr strMethodName, cstr strSignature, ...) {
    int32_t i = 0;
    auto env(attachCurrentThread());
    jclass cls = env->GetObjectClass(object);
    jmethodID mid = env->GetMethodID(cls, strMethodName, strSignature);
    if(!mid) { DLOG("method ID %s, '%s' not found", strMethodName, strSignature); return i; }
    va_list args;
    va_start(args, strSignature);
    i = env->CallIntMethodV(object, mid, args);
    va_end(args);
    env->DeleteLocalRef(cls);
    return i;
}

bool zJniHelper::callBooleanMethod(jobject object, cstr strMethodName, cstr strSignature, ...) {
    bool b;
    auto env(attachCurrentThread());
    jclass cls = env->GetObjectClass(object);
    jmethodID mid = env->GetMethodID(cls, strMethodName, strSignature);
    if(!mid) { DLOG("method ID %s, '%s' not found", strMethodName, strSignature); return false; }
    va_list args;
    va_start(args, strSignature);
    b = env->CallBooleanMethodV(object, mid, args);
    va_end(args);
    env->DeleteLocalRef(cls);
    return b;
}

jobject zJniHelper::createObject(cstr class_name) {
    auto env(attachCurrentThread());
    jclass cls = env->FindClass(class_name);
    jmethodID constructor = env->GetMethodID(cls, "<init>", "()V");
    jobject obj = env->NewObject(cls, constructor);
    jobject objGlobal = env->NewGlobalRef(obj);
    env->DeleteLocalRef(obj);
    env->DeleteLocalRef(cls);
    return objGlobal;
}

void zJniHelper::runOnUiThread(std::function<void()> callback) {
    // Lock mutex
    std::lock_guard<std::mutex> lock(mutex_);
    auto env(attachCurrentThread());
    static jmethodID mid(nullptr);
    if(!mid) mid = env->GetMethodID(jni_helper_java_class_, "runOnUIThread", "(J)V");
    // Allocate temporary function object to be passed around
    auto pCallback(new std::function<void()>(std::move(callback)));
    env->CallVoidMethod(jni_helper_java_ref_, mid, (int64_t)pCallback);
}

void zJniHelper::hideNavigationPanel() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto env(attachCurrentThread());
    auto activityClass = env->FindClass(NATIVEACTIVITY_CLASS_NAME);
    auto windowClass   = env->FindClass("android/view/Window");
    auto viewClass     = env->FindClass("android/view/View");
    auto getWindow     = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");
    auto getDecorView  = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");
    auto setSystemUiVisibility = env->GetMethodID(viewClass, "setSystemUiVisibility", "(I)V");
    auto window        = env->CallObjectMethod(activity_->clazz, getWindow);
    auto decorView     = env->CallObjectMethod(window, getDecorView);
    env->CallVoidMethod(decorView, setSystemUiVisibility, 0x00001006);
    detachCurrentThread();
}

ANativeWindow* zJniHelper::makeSurface(u32 textureId) {
    auto env(attachCurrentThread());
    auto activityClass = env->FindClass(NATIVEACTIVITY_CLASS_NAME);
    auto makeSurface   = env->GetStaticMethodID(activityClass, "makeSurface", "(I)Landroid/view/Surface;");
    if(makeSurface) {
        auto _surface(env->CallStaticObjectMethod(activityClass, makeSurface, textureId));
        auto wnd(ANativeWindow_fromSurface(env, _surface));
        DLOG("class:%llx make:%llx surf:%llx wnd:%llx", activityClass, makeSurface, _surface, wnd);
        return wnd;
    }
    return nullptr;
}

extern "C" {
    JNIEXPORT void Java_com_sample_helper_NDKHelper_RunOnUiThreadHandler(JNIEnv*, jobject, int64_t pointer) {
        auto pCallback((std::function<void()>*)pointer);
        (*pCallback)();
        // Deleting temporary object
        delete pCallback;
    }
}
