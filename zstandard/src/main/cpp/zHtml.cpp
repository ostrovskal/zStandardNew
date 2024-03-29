﻿//
// Created by User on 03.05.2022.
//

#include "zstandard/zstandard.h"
#include "zstandard/zHtml.h"

int zHtml::next(int* len) {
    int ch(0), ll(0);
    if(start < end) {
        ch = z_char8(start, &ll);
        if(ch == '\n') line++;
    }
    if(len) *len = ll; else start += ll;
    return ch;
}

zHtml::zHtml(cstr _text, const std::function<bool (cstr, bool, zHtml*)>& parser) {
    text = decode((u8*)_text, z_size8(_text), parser);
}

zString8 zHtml::decode(u8* ptr, i32 size, const std::function<bool (cstr, bool, zHtml*)>& _parser) {
    zString8 stkHtml((cstr)ptr, z_sizeCount8((cstr)ptr, (cstr)ptr + size));
    line = 1; stkHtml.trim();
    start = stkHtml.buffer(); html = start;
    end = start + stkHtml.size();
    z_skip_spc(&start, line); text.empty();
    if(!parser(text, _parser)) text.empty();
    return text;
}

zString8 zHtml::getValue(char delim) {
    int len; start += (*start == '\"');
    auto _s(start); int count(0);
    // определить конец значения
    while(start < end) {
        if(next(&len) == delim) break;
        start += len; count++;
    }
    zString8 v(_s, count);
    start += (*start == '\"');
    return v.replaceAmp(true);
}

zString8 zHtml::getName() {
    z_skip_spc(&start, line); auto _s(start); int count(0);
    while(start < end && !z_delimiter(*start)) next(nullptr), count++;
    return { _s, count };
}

bool zHtml::skipComment() {
    start += 3;
    while((start + 3) < end) {
        if(strncmp(start, "-->", 3) == 0) { start += 3; return true; }
        next(nullptr);
    }
    return false;
}

bool zHtml::parser(const zString8& _tag, const std::function<bool (cstr, bool, zHtml*)>& _parser) {
    while(start < end) {
        // текст до начала тега
        text += getValue('<');
        if(start >= end) break;
        auto ch(next(nullptr));
        if(ch == '<') {
            // комментарий?
            if(((start + 3) < end) && *start == '!' && start[1] == '-' && start[2] == '-') {
                if(skipComment()) continue;
                return false;
            }
            // может это конец тега ?
            auto isEndTag(*start == '/'); start += isEndTag;
            // берем имя тега
            auto tag(getName());
            if(tag.isEmpty() || tag[0] <= '9') {
                // проверить на заголовок - пропустить его
                if(tag[0] != '!' && strncmp(start, "DOCTYPE ", 8) != 0) {
                    // недопустимое имя
                    return false;
                }
                tag = "!DOCTYPE";
                start += 7;
            }
            attrs.clear();
            bool isFinish(false);
            // если не конец
            if(!isEndTag) {
                // атрибуты тега
                while(start < end) {
                    // имя атрибута
                    auto attrName(getName());
                    attrName.lower();
                    if(attrName.isNotEmpty()) {
                        // берем значение атрибута
                        z_skip_spc(&start, line);
                        if(next(nullptr) != '=') return false;
                        attrs += attrName; z_skip_spc(&start, line);
                        attrs += getValue('\"');
                    } else {
                        // конец тега >, />
                        isEndTag = (*start == '/');
                        start += isEndTag; isFinish = isEndTag;
                        break;
                    }
                }
            }
            // конец тега
            if(next(nullptr) != '>') return false;
            if(isEndTag && !isFinish) {
                // проверить - конец тега/начало совпадают?
                if(tag != _tag) {
                    ILOG("begin<%s> - end<%s>!!!", _tag.str(), tag.str());
                    ILOG("line:%i position:%i", line, z_sizeCount8(html, start));
                    return false;
                }
            }
            // вызвать клиента
            _parser(tag, isEndTag, this);
            if(isEndTag) {
                if(!isFinish) {
                    // если конец тега - выйти/продолжить
                    if(_tag.isNotEmpty()) return true;
                }
                continue;
            }
            if(parser(tag, _parser))
                continue;
        }
        return false;
    }
    return true;
}

zString8 zHtml::findValueAttribute(cstr name) const {
    zString8 value;
    for(int i = 0 ; i < attrs.size(); i += 2) {
        if(attrs[i] == name) { value = attrs[i + 1]; break; }
    }
    return value;
}

zString8 zHtml::getStringAttr(cstr name, cstr def) {
    auto ret(findValueAttribute(name));
    return ret.isNotEmpty() ? ret : zString8(def);
}

int zHtml::getIntegerAttr(cstr name, int radix, int def) {
    auto ret(findValueAttribute(name));
    return ret.isNotEmpty() ? z_ston<int>(ret.str(), radix) : def;
}

u32 zHtml::getColorAttr(cstr name, u32 def) {
    auto ret(findValueAttribute(name));
    return (ret.isNotEmpty() ? zColor(ret).toARGB() : def);
}

float zHtml::getFloatAttr(cstr name, float def) {
    auto ret(findValueAttribute(name));
    return ret.isNotEmpty() ? z_ston<float>(ret.str(), RADIX_FLT) : def;
}
