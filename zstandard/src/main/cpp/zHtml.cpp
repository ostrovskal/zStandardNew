//
// Created by User on 03.05.2022.
//

#include "zstandard/zstandard.h"
#include "zstandard/zHtml.h"

int zHtml::next(int* len) {
    int ch(0), ll(0);
    if(start < end) {
        ch = z_charUTF8(start, &ll);
        if(ch == '\n') line++;
    }
    if(len) *len = ll; else start += ll;
    return ch;
}

zHtml::zHtml(cstr _text, const std::function<bool (cstr, bool, zHtml*)>& parser) {
    text = decode((u8*)_text, z_sizeUTF8(_text), parser);
}

zStringUTF8 zHtml::decode(u8* ptr, i32 size, const std::function<bool (cstr, bool, zHtml*)>& _parser) {
    zStringUTF8 stkHtml((cstr)ptr, z_sizeCountUTF8((cstr)ptr, (cstr)ptr + size));
    line = 1; stkHtml.trim();
    start = stkHtml.buffer(); html = start;
    end = start + stkHtml.size();
    z_skip_spc(&start, line); text.empty();
    if(!parser(text, _parser)) text.empty();
    return text;
}

zStringUTF8 zHtml::getValue(char delim) {
    int len; start += (*start == '\"');
    auto _s(start); int count(0);
    // определить конец значения
    while(start < end) {
        if(next(&len) == delim) break;
        start += len; count++;
    }
    zStringUTF8 v(_s, count);
    start += (*start == '\"');
    return v.replaceAmp(true);
}

zStringUTF8 zHtml::getName() {
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

bool zHtml::parser(const zStringUTF8& _tag, const std::function<bool (cstr, bool, zHtml*)>& _parser) {
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
                    ILOG("line:%i position:%i", line, z_sizeCountUTF8(html, start));
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

zStringUTF8 zHtml::findValueAttribute(cstr name) const {
    zStringUTF8 value;
    for(int i = 0 ; i < attrs.size(); i += 2) {
        if(attrs[i] == name) { value = attrs[i + 1]; break; }
    }
    return value;
}

zStringUTF8 zHtml::getStringAttr(cstr name, cstr def) {
    auto ret(findValueAttribute(name));
    return ret.isNotEmpty() ? ret : zStringUTF8(def);
}

int zHtml::getIntegerAttr(cstr name, int radix, int def) {
    auto ret(findValueAttribute(name));
    return ret.isNotEmpty() ? z_ston<int>(ret.str(), radix) : def;
}

int zHtml::getColorAttr(cstr name, int def) {
    auto ret(findValueAttribute(name));
    ret.remove('#');
    auto a((ret.count() < 7) * 0xff000000);
    return ret.isNotEmpty() ? (int)(a | z_ston<u32>(ret.str(), RADIX_HEX)) : def;
}

float zHtml::getFloatAttr(cstr name, float def) {
    auto ret(findValueAttribute(name));
    return ret.isNotEmpty() ? z_ston<float>(ret.str(), RADIX_FLT) : def;
}
