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
    zString decode(u8* _text, i32 size, const std::function<bool (cstr tag, bool opened, zHtml* html)>& parser);
    // вернуть строковое значение атрибута
    zString getStringAttr(cstr name, cstr def);
    // вернуть целое значение атрибута
    int getIntegerAttr(cstr name, int radix, int def);
    // вернуть цветовое значение атрибута
    int getColorAttr(cstr name, int def);
    // вернуть вещественное значение атрибута
    float getFloatAttr(cstr name, float def);
    // текущая строка
    int line{1};
    // текст
    zString text;
protected:
    // распарсить
    bool parser(const zString& _tag, const std::function<bool (cstr tag, bool opened, zHtml* html)>& _parser);
    // комментарий
    bool skipComment();
    // найти значение атрибута по его имени
    zString findValueAttribute(cstr name) const;
    // получить имя
    zString getName();
    // получить значение
    zString getValue(char delim);
    // начало/конец декодирования
    char* start{nullptr}, *end{nullptr};
    // html текст
    char* html{nullptr};
    // массив атрибутов тега
    zArray<zString> attrs{};
};