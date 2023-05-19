//
// Created by User on 03.05.2022.
//

#pragma once

#include <functional>

class zHtml {
public:
    // конструктор
    zHtml(cstr text, const std::function<bool (cstr tag, bool opened, zHtml* html)>& parser);
    // декодировать
    zStringUTF8 decode(u8* _text, i32 size, const std::function<bool (cstr tag, bool opened, zHtml* html)>& parser);
    // вернуть строковое значение атрибута
    zStringUTF8 getStringAttr(cstr name, cstr def);
    // вернуть целое значение атрибута
    int getIntegerAttr(cstr name, int radix, int def);
    // вернуть цветовое значение атрибута
    int getColorAttr(cstr name, int def);
    // вернуть вещественное значение атрибута
    float getFloatAttr(cstr name, float def);
    // текущая строка
    int line{1};
    // текст
    zStringUTF8 text;
protected:
    // распарсить
    bool parser(const zStringUTF8& _tag, const std::function<bool (cstr tag, bool opened, zHtml* html)>& _parser);
    // комментарий
    bool skipComment();
    // следующий символ
    int next();
    // найти значение атрибута по его имени
    zStringUTF8 findValueAttribute(cstr name) const;
    // получить имя
    zStringUTF8 getName();
    // получить значение
    zStringUTF8 getValue(char delim);
    // начало/конец декодирования
    char* start{nullptr}, *end{nullptr};
    // html текст
    char* html{nullptr};
    // массив атрибутов тега
    zArray<zStringUTF8> attrs{};
};