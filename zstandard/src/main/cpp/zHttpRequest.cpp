//
// Created by User on 05.05.2022.
//

#include "zstandard/zstandard.h"
#include "zstandard/zCloud.h"

void zHttpRequest::DynBuffer::clear() {
    grow = size = 0;
    SAFE_DELETE(ptr);
}

void zHttpRequest::DynBuffer::realloc(int len, u8* data) {
    auto nsize(size + len);
    if(nsize >= grow) {
        grow = nsize * 2;
        auto tmp(new u8[grow]);
        memset(tmp, 0, grow);
        memcpy(tmp, ptr, size);
        delete ptr; ptr = tmp;
    }
    memcpy(ptr + size, data, len);
    size = nsize;
}

static size_t reader(char* bufptr, size_t size, size_t nitems, zHttpRequest::DynBuffer* upload) {
    int result(0);
    if(bufptr) {
        result = (int)(size * nitems);
        if(upload->size < result) result = upload->size;
        result = readf(upload->fd, bufptr, result);
        upload->size -= result;
    }
    return result;
}

static int writer(char* data, size_t size, size_t nmemb, zHttpRequest::DynBuffer* resp) {
    int result(0);
    if(resp) {
        result = (int)(size * nmemb);
        resp->realloc(result, (u8*)data);
    }
    return result;
}

zHttpRequest::zHttpRequest(bool follow_location, bool verbose) {
    if(!(curl = curl_easy_init())) return;
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, reader);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, writer);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header);
    curl_easy_setopt(curl, CURLOPT_HEADER, false);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, follow_location);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, verbose);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(curl, CURLOPT_COOKIELIST, "");
//    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1);
}

void zHttpRequest::close() {
    if(curl) curl_easy_cleanup(curl), curl = nullptr;
    if(hs) curl_slist_free_all(hs), hs = nullptr;
}

void zHttpRequest::setProxy(czs& url, czs& login_pwd) {
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_PROXY, url.str());
        curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, login_pwd.str());
    }
}

void zHttpRequest::changeHeaders(czs& what, czs& value) {
    struct curl_slist* tmp(nullptr), *each(hs);
    while(each) {
        auto origin(each->data);
        if(z_strstrUTF8(origin, what) != 0) tmp = curl_slist_append(tmp, origin);
        each = each->next;
    }
    curl_slist_free_all(hs); hs = tmp;
    hs = curl_slist_append(hs, what + value);
}

void zHttpRequest::setCustomHeader(czs &_header, czs &_value, bool _reset) {
    if(curl) {
        if(_reset) curl_slist_free_all(hs), hs = nullptr;
        changeHeaders(_header, _value);
    }
}

void zHttpRequest::setCert(czs& cert) const {
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, cert.isNotEmpty());
        curl_easy_setopt(curl, CURLOPT_CAINFO, cert.str());
    }
}

bool zHttpRequest::exec(czs& url, bool nobody) {
    CURLcode code(CURL_LAST);
    resp.clear(); header.clear();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_NOBODY, nobody);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
        curl_easy_setopt(curl, CURLOPT_URL, url.str());
        code = curl_easy_perform(curl);
    }
    return code == CURLE_OK;
}

bool zHttpRequest::requestPost(czs& url, czs& fields, bool nobody) {
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields.str());
    return exec(url, nobody);
}

bool zHttpRequest::requestGet(czs& url, bool nobody) {
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    return exec(url, nobody);
}

bool zHttpRequest::requestPut(czs& url, int fd, int size) {
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE, size);
    upload.fd = fd; upload.size = size;
    auto ret(exec(url, false));
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 0);
    return ret;
}

bool zHttpRequest::requestCustom(czs& name, czs& url) {
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, name.str());
    auto ret(exec(url, false));
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, nullptr);
    return ret;
}

int zHttpRequest::getStatus() const {
    long status(0);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    return status;
}

zStringUTF8 zHttpRequest::getCookieList() const {
    zStringUTF8 _cookie;
    if(curl) {
        struct curl_slist* cookies(nullptr);
        auto result(curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies));
        if(!result && cookies) {
            auto each(cookies);
            while(each) {
                _cookie += each->data; _cookie += "; ";
                each = each->next;
            }
            _cookie.replace("#HttpOnly_", "");
            curl_slist_free_all(cookies);
        }
        auto cookieList(_cookie.split("; ")); _cookie.empty();
        for(auto& cookie : cookieList) {
            auto cook(cookie.split("\t"));
            _cookie += cook[5] + "=" + cook[6] + "; ";
        }
    }
    return _cookie;
}

zStringUTF8 zHttpRequest::getHeaders(bool out) const {
    zStringUTF8 headers;
    if(!out) {
        auto tmp(hs);
        while(tmp) {
            headers += tmp->data; headers += "; ";
            tmp = tmp->next;
        }
    } else headers = header.str;
    return headers;
}

zStringUTF8 zHttpRequest::getRedirect() const {
    char* red(nullptr);
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &red);
    return { red };
}
