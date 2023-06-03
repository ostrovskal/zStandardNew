//
// Created by User on 18.04.2023.
//

#pragma once

class zShader {
public:
    zShader(cstr programm, i32 type);
    zShader(zShader* vert, zShader* frag);
    virtual ~zShader();
    // связать переменные
    GLuint linkVariables(i32* buf, czs& vars) const;
private:
    // идентификатор
    GLuint id{0};
    // статус
    GLint status{0};
};
