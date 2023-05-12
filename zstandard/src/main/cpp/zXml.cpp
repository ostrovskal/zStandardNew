//
// Created by Sergo on 20.01.2021.
//

#include "zstandard/zstandard.h"
#include "zstandard/zXml.h"

static cstr msg_err[] = {
		"OK", "EOF", "INVALID_TAG", "INVALID_STRUCTURED", 
		"INVALID_ATTR_VALUE", "INVALID_TAG_VALUE", "INVALID_TAG_VALUE"
};

zXml::zXml(u8* ptr, i32 size) {
	open("mem_ptr", ptr, size);
}

zXml::zXml(cstr _path) {
	zFile f;
	if(f.open(_path, true, false)) {
		int size(0); auto tmp((u8*)f.readn(&size));
		f.close();
		open(_path, tmp, size);
	}
}

bool zXml::open(cstr _path, u8* ptr, i32 size) {
	path = _path; zStringUTF8 stkXml(ptr, size);
	stkXml.trim(); xml = stkXml.buffer(); end = xml + stkXml.count();
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

static bool isDelimiter(char c) {
	if(c == '-' || c == ':' || c == '_') return false;
	if(c < 48 || c > 'z') return true;
	if(c > '9' && c < '@') return true;
	return (c > 'Z' && c < 'a');
}

zStringUTF8 zXml::getValue(char delim) {
	z_skip_spc(&xml, line);
	zStringUTF8 val;
	if(delim == '\"' && *xml != delim) { err = ERROR_ATTR_VALUE; return ""; }
	xml += (*xml == '\"'); err = ERROR_OK; auto _s(xml);
	while(xml < end) {
		if(delim == '<') {
			if((xml + 9) < end && strncmp(xml, "<![CDATA[", 9) == 0) {
				zStringUTF8 v(_s, (int)(xml - _s)), cd(cdata());
				if(err != ERROR_OK) return "";
				val += v.replaceAmp(true) + cd;
				_s = xml;
			}
		}
		if(*xml == delim) break;
		else if(*xml++ == '\n') line++;
	}
	if(xml < end) {
		zStringUTF8 v(_s, (int)(xml - _s));
		xml += (delim == '\"');
		return val + v.replaceAmp(true);
	}
	err = ERROR_EOF;
	return "";
}

zStringUTF8 zXml::getName() {
	z_skip_spc(&xml, line); auto _s(xml);
	err = ERROR_OK;
	while(xml < end && !isDelimiter(*xml)) xml++;
	if(xml < end) return { _s, (int)(xml - _s) };
	err = ERROR_EOF;
	return "";
}

zStringUTF8 zXml::cdata() {
	xml += 9; auto cdata(xml);
	while((xml + 2) < end) {
		if(*xml == '\n') line++;
		if(*xml++ != ']') continue;
		if(*xml++ != ']') continue;
		if(*xml++ == '>') return { cdata, (int)(xml - 3 - cdata) };
	}
	err = ERROR_EOF;
	return "";
}

bool zXml::skipComment() {
	xml += 3;
	while((xml + 3) < end) {
		if(strncmp(xml, "-->", 3) == 0) { xml += 3; return true; }
		if(*xml++ == '\n') line++;
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
		auto ch(*xml++);
		if(ch == '<') {
			// comment?
			auto isComment(*xml == '!' && xml[1] == '-' && xml[2] == '-');
			if(isComment) { if(!skipComment()) return false; continue; }
			// head?
			auto isCapt(*xml == '?' && root == nullptr);
			if(isCapt) xml++;
			// может это конец тега?
			isEndTag = (*xml == '/');
			if(isEndTag) xml++;
			auto tag(getName());
			if(err != ERROR_OK) return false;
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
					if(err != ERROR_OK) return false;
					// если конец тега
					if(isEndTag) {
						// убрать пробелы
						z_skip_spc(&xml, line);
						err = ERROR_INVALID_TAG;
						if(*xml++ != '>') return false;
						// если нет родителя - ошибка
						if(!p) return false;
						auto res(p->name == tag);
						err = (res ? ERROR_OK : ERROR_INVALID_STRUCTURED);
						if(!p->parent && err == ERROR_OK) return true;
						return res;
					}
					// конец тега >,/>
					auto isFinish(xml[0] == '/' || (isCapt && xml[0] == '?'));
					if(isFinish) xml++;
					if(*xml++ != '>') { err = ERROR_INVALID_TAG; return false; }
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
					if(*xml++ != '=') return false;
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
	z_skip_spc(&xml, line);
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
	f->writeStringUTF8(t, n->value.isEmpty());
	if(isCapt) return;
	for(auto _n : n->children) _save(_n, f, tab + 1);
	if(isClose) {
		t = (n->value.isEmpty() ? tb : zStringUTF8("")) + "</" + n->name + ">";
		f->writeStringUTF8(t, true);
	}
}

bool zXml::save(cstr _path) {
	zFile f;
	if(f.open(_path, false, false)) {
		if(capt) _save(capt, &f, 0);
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
