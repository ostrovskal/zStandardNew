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
    zArray<int> linkVariables(czs& str) const;
    // вернуть идентификатор
    GLuint getID() const { return id; }
private:
    // идентификатор
    GLuint id{0};
    // статус
    GLint status{0};
};
