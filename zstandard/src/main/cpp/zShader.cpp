//
// Created by User on 18.04.2023.
//

#include "zstandard/zstandard.h"
#include "zstandard/zShader.h"


// лог
static char logInfo[256]{};

zShader::zShader(zShader* vert, zShader* frag) {
    // компилировать шейдеры
    id = glCreateProgram();
    glAttachShader(id, vert->id);
    glAttachShader(id, frag->id);
    glLinkProgram(id);
    glGetProgramiv(id, GL_LINK_STATUS, &status);
    if(status == GL_FALSE) {
        glGetProgramInfoLog(id, sizeof(logInfo), nullptr, logInfo);
        ILOG("Error compile shader: %s", logInfo);
    } else {
        glUseProgram(id);
        DLOG("Compile programm shader is ok!");
    }
    SAFE_DELETE(vert);
    SAFE_DELETE(frag);
}

zShader::zShader(cstr programm, i32 type) {
    id = glCreateShader(type);
    glShaderSource(id, 1, &programm, nullptr);
    glCompileShader(id);
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE) {
        glGetShaderInfoLog(id, sizeof(logInfo), nullptr, logInfo);
        ILOG("Error create %s shader: %s", type == GL_VERTEX_SHADER ? "vertex" : "fragment", logInfo);
    }
}

zShader::~zShader() {
    glDeleteShader(id);
    id = 0;
}

GLuint zShader::linkVariables(i32* buf, czs& vars) const {
    auto arr(vars.split(";")); int pos;
    for(auto& v : arr) {
        auto _v(v.str());
        switch(*_v++) {
            case 'u': pos = glGetUniformLocation(id, _v); break;
            case 'a': pos = glGetAttribLocation(id, _v); break;
            default: pos = -1; break;
        }
        *buf++ = pos;
    }
    return id;
}
