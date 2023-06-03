//
// Created by Sergo on 20.01.2021.
//

#include "zstandard/zstandard.h"
#include "zstandard/zXml.h"

static cstr msg_err[] = {
		"OK", "EOF", "INVALID_TAG", "INVALID_STRUCTURED", 
		"INVALID_ATTR_VALUE", "INVALID_TAG_VALUE", "INVALID_TAG_VALUE"
};

int zXml::next() {
	int ch(0);
	if(xml < end) {
		ch = z_charUTF8(xml);
		xml += z_charLengthUTF8(xml);
		if(ch == 0x0D) line++;
	} else err = ERROR_EOF;
	return ch;
}

zXml::zXml(u8* ptr, i32 size) {
	open("mem_ptr", ptr, size);
}

zXml::zXml(cstr _path) {
	zFile f; path = _path;
	if(f.open(_path, true, false)) {
		int size(0); auto tmp((u8*)f.readn(&size));
		f.close(); open(_path, tmp, size);
	}
}

bool zXml::open(cstr _path, u8* ptr, i32 size) {
	path = _path; zStringUTF8 stkXml(ptr, z_sizeCountUTF8((cstr)ptr, (cstr)ptr + size));
	stkXml.trim(); xml = stkXml.buffer(); end = xml + stkXml.size();
	auto ret(decode());
	if(!ret) {
		ILOG("Error decode %s(%s:%i)!", _path, msg_err[err], line);
		close();
	} else {
		DLOG("%s version:%f encoding:%s", _path, version, encoding.str());
	}
	stkXml.empty();
	return ret;
}

static bool is_delimiter(int c) {
	if(c == '-' || c == ':' || c == '_') return false;
	if(c < 48 || c > 'z') return true;
	if(c > '9' && c < '@') return true;
	return (c > 'Z' && c < 'a');
}

zStringUTF8 zXml::getValue(char delim) {
	zStringUTF8 val;
	z_skip_spc(&xml, line); auto ch(z_charUTF8(xml));
	if(delim == '\"' && ch != delim) { err = ERROR_ATTR_VALUE; return ""; }
	xml += (ch == '\"'); err = ERROR_OK; auto _s(xml); int count(0);
	while(xml < end) {
		if(delim == '<') {
			if((xml + 9) < end && strncmp(xml, "<![CDATA[", 9) == 0) {
				zStringUTF8 v(_s, count), cd(cdata());
				val += v.replaceAmp(true) + cd;
				_s = xml; count = 0;
				continue;
			}
		}
		if(z_charUTF8(xml) == delim) break;
		next(); count++;
	}
	if(err == ERROR_OK) {
		zStringUTF8 v(_s, count);
		xml += (delim == '\"');
		return val + v.replaceAmp(true);
	}
	return "";
}

zStringUTF8 zXml::getName() {
	z_skip_spc(&xml, line);
	err = ERROR_OK; auto _s(xml); int count(0);
	while(xml < end && !is_delimiter(z_charUTF8(xml))) next(), count++;
	return (err == ERROR_OK ? zStringUTF8(_s, count) : zStringUTF8(""));
}

zStringUTF8 zXml::cdata() {
	xml += 9; auto cdata(xml); int count(0);
	while((xml + 2) < end) {
		count++;
		if(next() != ']') continue;
		if(next() != ']') continue;
		if(next() == '>') return { cdata, count };
	}
	return "";
}

bool zXml::skipComment() {
	// пропустить <--
	xml += 3;
	while((xml + 3) < end) {
		// проверить на конец комментария
		if(strncmp(xml, "-->", 3) == 0) { xml += 3; return true; }
		// следующий символ
		next();
	}
	err = ERROR_EOF;
	return false;
}

bool zXml::parser(zNode* p) {
	zNode* n(nullptr);
	bool isEndTag;
	while(xml < end) {
		z_skip_spc(&xml, line);
		if(p) {
			// еще значение(а может и нет?)
			p->value += getValue('<');
			if(p->value.isEmpty() && err != ERROR_OK) return false;
		}
		auto ch(next());
		if(ch == '<') {
			// comment?
			auto isComment(z_charUTF8(xml) == '!' && xml[1] == '-' && xml[2] == '-');
			if(isComment) { if(!skipComment()) return false; continue; }
			// head?
			auto isCapt(z_charUTF8(xml) == '?' && root == nullptr); xml += isCapt;
			// может это конец тега?
			isEndTag = (z_charUTF8(xml) == '/'); xml += isEndTag;
			auto tag(getName());
			if(tag.isEmpty() || tag[0] <= '9') {
				err = ERROR_INVALID_TAG;
				return false;
			}
			// сформировать ноду
			if(!isEndTag) {
				if(p == nullptr) {
					if(root) capt = root;
					n = root = new zNode();
				} else n = new zNode(p);
				if(isCapt) tag = "?" + tag;
				n->name = tag;
			}
			while(xml < end) {
				// либо атрибуты, либо конец тега > />
				auto attrName(getName());
				if(attrName.isEmpty()) {
					if(err != ERROR_OK) break;
					// если конец тега
					if(isEndTag) {
						// убрать пробелы
						z_skip_spc(&xml, line);
						err = ERROR_INVALID_TAG;
						if(next() != '>') return false;
						// если нет родителя - ошибка
						if(!p) return false;
						auto res(p->name == tag);
						err = (res ? ERROR_OK : ERROR_INVALID_STRUCTURED);
						if(!p->parent && err == ERROR_OK) return true;
						return res;
					}
					// конец тега >,/>
					auto isFinish(z_charUTF8(xml) == '/' || (isCapt && z_charUTF8(xml) == '?'));
					xml += isFinish;
					if(next() != '>') { err = ERROR_INVALID_TAG; return false; }
					if(isFinish) break;
					// value tag
					if(n) {
						n->value = getValue('<');
						if(n->value.isEmpty() && err != ERROR_OK) return false;
						if(!parser(n)) return false;
						if(xml < end) break;
						return true;
					}
				} else {
					if(isEndTag) { err = ERROR_INVALID_STRUCTURED; return false; }
					// attributes
					z_skip_spc(&xml, line);
					if(next() != '=') return false;
					auto attrVal(getValue('\"'));
					if(err != ERROR_OK) return false;
					auto a(new zNode::zAttr(attrName, attrVal));
					if(n) n->attrs += a;
				}
			}
		} else {
			err = ERROR_INVALID_STRUCTURED;
			return false;
		}
	}
	err = ERROR_EOF;
	return false;
}

bool zXml::decode() {
	if(!parser(nullptr)) return false;
	if(capt) {
		auto enc(capt->getAttr("encoding"));
		encoding = enc ? enc->value.str() : "";
		version = z_ston<float>(capt->getAttrVal("version", "1.0"), RADIX_FLT);
	}
	return root != nullptr;
}

void zXml::_save(zNode* n, zFile* f, int tab) {
	zStringUTF8 tb('\t', tab);
	zStringUTF8 t = tb + "<";
	t += n->name;
	// attrs
	for(auto a : n->attrs) {
		zStringUTF8 an(a->ns);
		if(an.isNotEmpty()) an += ":";
		an += a->name;
		t += " " + an + "=\"" + a->value.replaceAmp(false) + "\"";
	}
	bool isClose(n->children.isNotEmpty() || n->value.isNotEmpty());
	bool isCapt(n->name[0] == '?');
	if(isCapt) {
		t += "?>";
	} else {
		if(isClose) t += ">"; else t += "/>\r\n";
		t += n->value.replaceAmp(false);
	}
	f->writeString(t, n->value.isEmpty());
	if(isCapt) return;
	for(auto _n : n->children) _save(_n, f, tab + 1);
	if(isClose) {
		t = (n->value.isEmpty() ? tb : zStringUTF8("")) + "</" + n->name + ">";
		f->writeString(t, true);
	}
}

bool zXml::save(cstr _path) {
	zFile f;
	if(f.open(_path, false, false)) {
		f.writeString("<?xml version = \"1.0\" encoding=\"utf-8\" ?>", true);
		_save(root, &f, 0);
		f.close();
		return true;
	}
	return false;
}

void zXml::close() {
	SAFE_DELETE(capt);
	SAFE_DELETE(root);
}
