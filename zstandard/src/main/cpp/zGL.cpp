//
// Created by User on 26.04.2023.
//

#include "zstandard/zstandard.h"
#include "zstandard/zGL.h"
#include "zstandard/gl3stub.h"

void zGL::initGLES() {
    if(!gles_initialized) {
        cstr versionStr((cstr) glGetString(GL_VERSION));
        if(strstr(versionStr, "OpenGL ES 3.") && gl3stubInit()) {
            es3_supported = true;
            gl_version = 3.0f;
        } else {
            gl_version = 2.0f;
        }
        gles_initialized = true;
    }
}

bool zGL::init(ANativeWindow* _window) {
    if(!egl_context_initialized) {
        sizeScreen.empty();
        window = _window;
        initEGLSurface();
        initEGLContext();
        initGLES();
        egl_context_initialized = true;
    }
    return true;
}

bool zGL::initEGLSurface() {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, 0, 0);
    // параметры поверхности
    EGLint attribs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,  // Request opengl ES2.0
                        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 24,
                        EGL_NONE };
    EGLint num_configs;
    eglChooseConfig(display, attribs, &config, 1, &num_configs);
    if(!num_configs) {
        attribs[11] = 16;
        eglChooseConfig(display, attribs, &config, 1, &num_configs);
    }
    if(!num_configs) {
        DLOG("Unable to retrieve EGL config");
        return false;
    }
    surface = eglCreateWindowSurface(display, config, window, nullptr);
    eglQuerySurface(display, surface, EGL_WIDTH, &sizeScreen.w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &sizeScreen.h);
    eglSwapInterval(display, 1);
    return true;
}

bool zGL::initEGLContext() {
    // запросить контекст OpenGL ES 2.
    const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    context = eglCreateContext(display, config, nullptr, context_attribs);
    if(eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        DLOG("Unable to eglMakeCurrent");
        return false;
    }
    return true;
}

EGLSurface zGL::makeSurface(int width, int height) const {
    EGLint numConfigs; EGLConfig config;
    // параметры поверхности
    const EGLint PBUF_ATTRIBS[] = { EGL_HEIGHT, width, EGL_WIDTH, height,
                                    EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGB,
                                    EGL_TEXTURE_TARGET, EGL_TEXTURE_2D, EGL_NONE };
    const EGLint DISPLAY_ATTRIBS[] = {  EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
                                        EGL_BIND_TO_TEXTURE_RGB, EGL_TRUE, EGL_NONE };
    eglChooseConfig(display, DISPLAY_ATTRIBS, &config, 1, &numConfigs);
    if(numConfigs != 1) return EGL_NO_SURFACE;
    auto _surface(eglCreatePbufferSurface(display, config, PBUF_ATTRIBS));
    if(_surface == EGL_NO_SURFACE) {
        DLOG("Error: EGL surface creation failed (error code: 0x%x)\n", eglGetError());
        return EGL_NO_SURFACE;
    }
    return _surface;
}

EGLint zGL::swap() {
    bool b(eglSwapBuffers(display, surface));
    EGLint err(EGL_SUCCESS);
    if(!b) {
        err = eglGetError();
        if(err == EGL_BAD_SURFACE) {
            // пересоздать поверхность
            initEGLSurface();
            err = EGL_SUCCESS;
        } else if (err == EGL_CONTEXT_LOST || err == EGL_BAD_CONTEXT) {
            // контекст может быть потерян
            terminate();
            initEGLContext();
        }
    }
    return err;
}

void zGL::terminate() {
    if(display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if(context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
        }
        if(surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
        }
        eglTerminate(display);
    }
    display = EGL_NO_DISPLAY;
    context = EGL_NO_CONTEXT;
    surface = EGL_NO_SURFACE;
    window = nullptr;
}

EGLint zGL::resume(ANativeWindow* _window) {
    if(!egl_context_initialized) {
        init(_window);
        return EGL_SUCCESS;
    }
    auto original(sizeScreen);
    // Create surface
    window = _window;
    surface = eglCreateWindowSurface(display, config, window, nullptr);
    eglQuerySurface(display, surface, EGL_WIDTH, &sizeScreen.w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &sizeScreen.h);
    if(original.w != sizeScreen.w || original.h != sizeScreen.h) {
        // Screen resized
        DLOG("Screen resized");
    }
    if(eglMakeCurrent(display, surface, surface, context) == EGL_TRUE)
        return EGL_SUCCESS;
    EGLint err(eglGetError());
    DLOG("Unable to eglMakeCurrent %d", err);
    if(err == EGL_CONTEXT_LOST) {
        // Recreate context
        DLOG("Re-creating egl context");
        initEGLContext();
    } else {
        // Recreate surface
        terminate();
        initEGLSurface();
        initEGLContext();
    }
    return err;
}

void zGL::suspend() {
    if(surface != EGL_NO_SURFACE) {
        eglDestroySurface(display, surface);
        surface = EGL_NO_SURFACE;
    }
}

void zGL::invalidate() {
    terminate();
    egl_context_initialized = false;
}

bool zGL::checkExtension(cstr extension) {
    if(!extension) return false;
    zString extensions((char*)glGetString(GL_EXTENSIONS));
    zString str(extension); str += " ";
    return extensions.indexOf(extension) != -1;
}
