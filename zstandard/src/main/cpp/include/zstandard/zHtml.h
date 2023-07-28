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
    zString8 decode(u8* _text, i32 size, const std::function<bool (cstr tag, bool opened, zHtml* html)>& parser);
    // вернуть строковое значение атрибута
    zString8 getStringAttr(cstr name, cstr def);
    // вернуть целое значение атрибута
    int getIntegerAttr(cstr name, int radix, int def);
    // вернуть цветовое значение атрибута
    u32 getColorAttr(cstr name, u32 def);
    // вернуть вещественное значение атрибута
    float getFloatAttr(cstr name, float def);
    // текущая строка
    int line{1};
    // текст
    zString8 text;
protected:
    // распарсить
    bool parser(const zString8& _tag, const std::function<bool (cstr tag, bool opened, zHtml* html)>& _parser);
    // комментарий
    bool skipComment();
    // следующий символ
    int next(int* len);
    // найти значение атрибута по его имени
    zString8 findValueAttribute(cstr name) const;
    // получить имя
    zString8 getName();
    // получить значение
    zString8 getValue(char delim);
    // начало/конец декодирования
    char* start{nullptr}, *end{nullptr};
    // html текст
    char* html{nullptr};
    // массив атрибутов тега
    zArray<zString8> attrs{};
};