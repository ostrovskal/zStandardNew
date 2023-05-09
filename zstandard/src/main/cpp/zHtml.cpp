//
// Created by User on 03.05.2022.
//

#include "zstandard/zstandard.h"
#include "zstandard/zHtml.h"

zHtml::zHtml(cstr _text, const std::function<bool (cstr, bool, zHtml*)>& parser) {
    text = decode((u8*)_text, z_strlen(_text) + 1, parser);
}

zString zHtml::decode(u8* ptr, i32 size, const std::function<bool (cstr, bool, zHtml*)>& _parser) {
    zString stkHtml(z_utfToAscii(ptr, size));
    line = 1;
    stkHtml.trim();
    start = stkHtml.buffer(); html = start;
    end = start + stkHtml.length();
    z_skip_spc(&start, line);
    text.empty();
    if(!parser("", _parser))
        text.empty();
    return text;
}

zString zHtml::getValue(char delim) {
    z_skip_spc(&start, line);
    start += (*start == '\"');
    auto _s(start);
    // определить конец значения
    while(start < end) {
        if(*start == delim) break;
        else if(*start++ == '\n') line++;
    }
    zString v(_s, (int) (start - _s));
    start += (*start == '\"');
    return v.replaceAmp(true);
}

zString zHtml::getName() {
    z_skip_spc(&start, line); auto _s(start);
    while(start < end && !z_delimiter(*start)) start++;
    return (start < end ? zString(_s, (int)(start - _s)) : "");
}

bool zHtml::skipComment() {
    start += 3;
    while((start + 3) < end) {
        if(strncmp(start, "-->", 3) == 0) { start += 3; return true; }
        if(*start++ == '\n') line++;
    }
    return false;
}

bool zHtml::parser(const zString& _tag, const std::function<bool (cstr, bool, zHtml*)>& _parser) {
    while(start < end) {
        z_skip_spc(&start, line);
        // текст до начала тега
        text += getValue('<');
        if(start >= end) break;
        auto ch(*start++);
        if(ch == '<') {
            // комментарий?
            if(((start + 3) < end) && *start == '!' && start[1] == '-' && start[2] == '-') {
                if(skipComment()) continue;
                return false;
            }
            // может это конец тега ?
            auto isEndTag(*start == '/');
            start += isEndTag;
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
                        if(*start++ != '=') return false;
                        attrs += attrName;
                        attrs += getValue('\"');
                    } else {
                        // конец тега >, />
                        isEndTag = (*start == '/');
                        start += isEndTag;
                        isFinish = isEndTag;
                        break;
                    }
                }
            }
            // конец тега
            if(*start++ != '>') {
                // недопустимое завершение тега
                return false;
            }
            if(isEndTag && !isFinish) {
                // проверить - конец тега/начало совпадают?
                if(tag != _tag) {
                    ILOG("begin<%s> - end<%s>!!!", _tag.str(), tag.str());
                    ILOG("line:%i position:%i", line, (int)(start - html));
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

zString zHtml::findValueAttribute(cstr name) const {
    zString value;
    for(int i = 0 ; i < attrs.size(); i += 2) {
        if(attrs[i] == name) {
            value = attrs[i + 1];
            break;
        }
    }
    return value;
}

zString zHtml::getStringAttr(cstr name, cstr def) {
    auto ret(findValueAttribute(name));
    return ret.isNotEmpty() ? ret : zString(def);
}

int zHtml::getIntegerAttr(cstr name, int radix, int def) {
    auto ret(findValueAttribute(name));
    return ret.isNotEmpty() ? z_ston<int>(ret.str(), radix) : def;
}

int zHtml::getColorAttr(cstr name, int def) {
    auto ret(findValueAttribute(name));
    ret.remove('#');
    auto a((ret.length() < 7) * 0xff000000);
    return ret.isNotEmpty() ? (int)(a | z_ston<u32>(ret.str(), RADIX_HEX)) : def;
}

float zHtml::getFloatAttr(cstr name, float def) {
    auto ret(findValueAttribute(name));
    return ret.isNotEmpty() ? z_ston<float>(ret.str(), RADIX_FLT) : def;
}
