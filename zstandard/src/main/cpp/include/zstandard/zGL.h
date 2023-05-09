//
// Created by User on 26.04.2023.
//

#pragma once

class zGL {
private:
    // EGL параметры
    ANativeWindow* window{nullptr};
    EGLDisplay display{EGL_NO_DISPLAY};
    EGLSurface surface{EGL_NO_SURFACE};
    EGLContext context{EGL_NO_CONTEXT};
    EGLConfig config{nullptr};
    // размер экрана
    szi sizeScreen{};
    // Flags
    bool gles_initialized{false}, egl_context_initialized{false}, es3_supported{false};
    float gl_version{0};
    void initGLES();
    void terminate();
    bool initEGLSurface();
    bool initEGLContext();
    zGL(zGL const&);
    void operator = (zGL const&);
    // конструктор
    zGL() { }
    // деструктор
    virtual ~zGL() { terminate(); }
public:
    // вернуть синглтон
    static zGL* instance() { static zGL inst; return &inst; }
    // инициализация
    bool init(ANativeWindow* window);
    // отобазить буфер
    EGLint swap();
    // уничтожение
    void invalidate();
    // создание произвольной поверхности
    EGLSurface makeSurface(int width, int height) const;
    // приостановить
    void suspend();
    // возобновить
    EGLint resume(ANativeWindow* window);
    // окно
    ANativeWindow* getWindow(void) const { return window; };
    // вернуть габариты экрана
    i32 getSizeScreen(bool _height) const { return _height ? sizeScreen.h : sizeScreen.w; }
    cszi& getSizeScreen() const { return sizeScreen; }
    // версия
    float getGLVersion() const { return gl_version; }
    // проверить на расширение
    bool checkExtension(cstr extension);
    // дисплей
    EGLDisplay getDisplay() const { return display; }
    // поверхность
    EGLSurface getSurface() const { return surface; }
};
