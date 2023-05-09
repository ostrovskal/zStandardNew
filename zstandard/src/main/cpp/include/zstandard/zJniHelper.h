//
// Created by User on 28.04.2023.
//

#pragma once

#include <mutex>
#include <pthread.h>
#include "android/native_window_jni.h"
#include "android/native_activity.h"

class zJniHelper {
private:
    zString app_name_{};
    ANativeActivity* activity_{nullptr};
    jobject jni_helper_java_ref_{nullptr};
    jclass jni_helper_java_class_{nullptr};
    jstring getExternalFilesDirJString(JNIEnv* env);
    jclass retrieveClass(JNIEnv* jni, cstr class_name);
    zJniHelper() { }
    ~zJniHelper();
    zJniHelper(const zJniHelper& rhs);
    zJniHelper& operator = (const zJniHelper& rhs);
    zString app_label_;
    mutable std::mutex mutex_{};
    jobject callObjectMethod(cstr strMethodName, cstr strSignature, ...);
    void callVoidMethod(cstr strMethodName, cstr strSignature, ...);
    static void detachCurrentThreadDtor(void* p) { if(p) { auto activity((ANativeActivity*)p); activity->vm->DetachCurrentThread(); } }
public:
    static void init(ANativeActivity* activity, cstr helper_class_name);
    static void init(ANativeActivity* activity, cstr helper_class_name, cstr native_soname);
    static zJniHelper* instance();
    // убрать кнопки панели навигации
    void hideNavigationPanel();
    void runOnUiThread(std::function<void()> callback);
    void detachCurrentThread() { activity_->vm->DetachCurrentThread(); }
    void deleteObject(jobject obj);
    void callVoidMethod(jobject object, cstr strMethodName, cstr strSignature, ...);
    float callFloatMethod(jobject object, cstr strMethodName, cstr strSignature, ...);
    bool callBooleanMethod(jobject object, cstr strMethodName, cstr strSignature, ...);
    cstr getAppName() const { return app_name_.str(); }
    cstr getAppLabel() const { return app_label_.str(); }
    zString convertString(cstr str, cstr encode);
    zString getExternalFilesDir();
    zString getStringResource(const zString& resourceName);
    jobject createObject(cstr class_name);
    jobject callObjectMethod(jobject object, cstr strMethodName, cstr strSignature, ...);
    jobject loadImage(cstr file_name, int32_t* outWidth = nullptr, int32_t* outHeight = nullptr, bool* hasAlpha = nullptr);
    int32_t callIntMethod(jobject object, cstr strMethodName, cstr strSignature, ...);
    int32_t getNativeAudioBufferSize();
    int32_t getNativeAudioSampleRate();
    uint32_t loadTexture(cstr file_name, int32_t* outWidth = nullptr, int32_t* outHeight = nullptr, bool* hasAlpha = nullptr);
    uint32_t loadCubemapTexture(cstr file_name, const int32_t face, const int32_t miplevel, const bool sRGB,
                                int32_t* outWidth = nullptr, int32_t* outHeight = nullptr, bool* hasAlpha = nullptr);
    // создать поверхность
    ANativeWindow* makeSurface(u32 textureId);
    JNIEnv* attachCurrentThread() {
        JNIEnv* env;
        if(activity_->vm->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK) return env;
        activity_->vm->AttachCurrentThread(&env, NULL);
        pthread_key_t key;
        if(pthread_key_create(&key, detachCurrentThreadDtor) == 0) pthread_setspecific(key, (void *)activity_);
        return env;
    }
};
