
#include "zstandard/zstandard.h"
#include "zstandard/zJSON.h"

zJSON::Node zJSON::dummy;
static char* brakket((char*)"()");

int zJSON::jnext() {
    int ch(0);
    if(jcur < jend) {
        ch = z_charUTF8(jcur);
        jcur += z_charLengthUTF8(jcur);
		if(ch == '\n') line++;
    }
    return ch;
}

bool zJSON::init(u8* ptr, int size) {
	jcur = (char*)ptr; jend = (jcur + size);
	z_skip_spc(&jcur, line);
	error = ((*jcur == '{' && parse(nullptr, _object, 1)) ? 0 : (int)(jcur - (char*)ptr) - 1);
	return error == 0;
}

bool zJSON::jliteral() {
    bool _error;
	auto str(*jcur == '\"');
	jcur += str; lit = jcur;
	auto cur(jcur);
	if(str) {
		_jt = _str;
		while(true) {
			auto ch(jnext());
			if((_error = (ch == 0))) break;
			if(ch == '\\') z_escape(&cur, &jcur);
			else {
				if(ch == '\"') break;
                auto sz(z_charLengthUTF8((cstr)&ch));
                memcpy(cur, &ch, sz); cur += sz;
			}
		}
		*(jcur - 1) = 0;
	} else {
		// bool/integer/real
		auto tmp(jcur); _jt = _bool;
		if(strncmp(tmp, "false", 5) == 0) tmp += 5;
		else if(strncmp(tmp, "true", 4) == 0) tmp += 4;
		else {
			_jt = _digit;
			z_ston<double>(jcur, RADIX_DBL, &tmp);
		}
		auto len(tmp - jcur); memcpy(digBuf, jcur, len);
		_error = (jcur == tmp); jcur = tmp; lit = digBuf;
		digBuf[len] = 0;
	}
	return _error;
}

bool zJSON::parse(zJSON::Node* p, zJSON::JType jt, int lev) {
	char* key(nullptr), * val(key); bool colon(jt == _array || p == nullptr);
	while(true) {
		z_skip_spc(&jcur, line);
		auto ch(jnext());
		if(ch == '\"' || isalnum(ch) || ch == '.') {
			jcur--;
			if(jliteral()) break;
			if(!key && !colon && _jt == _str) key = lit;
			else if(!val) {
				if(!colon) break;
				new Node(p, key, val = lit, _jt);
			} else break;
		} else if(ch == ':' && !colon) {
			if(!(colon = key != nullptr)) break;
		} else if(colon) {
			if(ch == ',') { 
				if(!p || !val) break;
				key = val = nullptr; colon = jt == _array;
			} else if(ch == ']' || ch == '}') {
				return true;
			} else if(!val && (ch == '[' || ch == '{')) {
				val = brakket;
				auto n(new Node(p, key, val, ch == '[' ? _array : _object));
				if(!parse(n, n->tp, lev + 1)) break;
				if(!p) root = n;
			} else {
				return !(key || val != brakket);
			}
		} else {
			colon = ch == '[' || ch == '{';
			if(!colon) break;
			jcur--;
		}
	}
    return false;
}

const zJSON::Node* zJSON::getNode(int idx, const Node* node) const {
	auto n(node ? node : root);
	return idx < n->child.size() ? n->child[idx] : &dummy;
}

const zJSON::Node* zJSON::getNode(cstr name, const Node* node) const {
	auto n(node ? node : root);
	auto idx(n->child.indexOf<cstr>(name));
	return idx == -1 ? &dummy : n->child[idx];
}

const zJSON::Node* zJSON::getPath(cstr path, const Node* node) const {
	static char buffer[256];
	auto n(node ? node : root);
	auto epath(path + z_sizeUTF8(path));
	while(path < epath) {
		auto ch(*path++);
		auto f(strchr(path, '/'));
		if(!f) f = epath;
		auto len((int)(f - path));
		memcpy(buffer, path, len); buffer[len] = 0;
		n = (ch == 'i' ? getNode(z_ston<int>(buffer, RADIX_DEC), n) : getNode(buffer, n));
		path = ++f;
	}
	return n;
}

zJSON::Node* zJSON::addNode(cstr key, cstr val, JType jt, Node* node) {
	auto n(node ? node : root);
	return new Node(n, key, val, jt);
}

zStringUTF8 zJSON::save() {
	zStringUTF8 ret;
	return ret;
}
