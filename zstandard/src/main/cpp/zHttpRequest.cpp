//
// Created by User on 05.05.2022.
//

#include "zstandard/zstandard.h"
#include "zstandard/zHttpRequest.h"

void zHttpRequest::DynBuffer::clear() {
    grow = size = 0;
    SAFE_DELETE(ptr);
}

void zHttpRequest::DynBuffer::realloc(int len, u8* data) {
    auto nsize(size + len);
    if(nsize >= grow) {
        grow = nsize * 2;
        auto tmp(new u8[grow]);
        memset(tmp, 0, grow); memcpy(tmp, ptr, size);
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

zString8 zHttpRequest::getRedirect() const {
    char* redir(nullptr); 
    if(curl) curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &redir); 
    return { redir };
}

void zHttpRequest::setAuth(cstr login, cstr pwd, int type) const {
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, type);
        zString8 auth(login); auth += ":"; auth += pwd;
        curl_easy_setopt(curl, CURLOPT_USERPWD, auth.str());
    }
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
}

void zHttpRequest::close() {
    if(curl) curl_easy_cleanup(curl), curl = nullptr;
    if(hs) curl_slist_free_all(hs), hs = nullptr;
}

bool zHttpRequest::setEmbeddedParams(HttpParams type, cstr value1, cstr value2, int size) {
    if(!curl) { ILOG("CURL not initialized!"); return false; }
    switch(type) {
        case HTTP_AGENT:
            curl_easy_setopt(curl, CURLOPT_USERAGENT, value1);
            break;
        case HTTP_REFER:
            curl_easy_setopt(curl, CURLOPT_REFERER, value1);
            break;
        case HTTP_ACCEPT:
            changeHeaders("Accept", value1);
            break;
        case HTTP_CONTENT_TYPE:
            changeHeaders("Content-Type", value1);
            break;
        case HTTP_CONTENT_LENGTH:
            changeHeaders("Content-Length", z_ntos(&size, RADIX_DEC, false));
            break;
        case HTTP_CERT:
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, value1 != nullptr);
            curl_easy_setopt(curl, CURLOPT_CAINFO, value1);
            break;
        case HTTP_PROXY:
            curl_easy_setopt(curl, CURLOPT_PROXY, value1);
            curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, value2);
            break;
        default:
            ILOG("Ошибка. Неизвестный спецификатор %i", type);
            return false;
    }
    return true;
}

int zHttpRequest::request(HttpRequest type, czs& url, const REQUEST_DATA& reqData, bool response, bool nobody) {
    if(!curl) { ILOG("CURL not initialized!"); return 0; }
    bool result;
    switch(type) {
        case HTTP_GET:
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
            result = exec(url, nobody);
            break;
        case HTTP_POST:
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, reqData.linkParam.str());
            result = exec(url, nobody);
            break;
        case HTTP_PUT:
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE, reqData.size);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, reqData.linkParam.str());
            upload.fd = reqData.fd; upload.size = reqData.size;
            result = exec(url, false);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 0);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, nullptr);
            break;
        case HTTP_CUSTOM:
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, reqData.linkParam.str());
            result = exec(url, nobody);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, nullptr);
            break;
        default:
            ILOG("Ошибка. Неизвестный спецификатор %i", type);
            return 0;
    }
    if(result && response) return jsonResponse();
    return getStatus();
}

void zHttpRequest::changeHeaders(czs& what, czs& value) {
    struct curl_slist* tmp(nullptr), *each(hs);
    while(each) {
        auto origin(each->data);
        if(z_strstr8(origin, what) != 0) tmp = curl_slist_append(tmp, origin);
        each = each->next;
    }
    curl_slist_free_all(hs); hs = tmp;
    hs = curl_slist_append(hs, what + ": " + value);
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

zString8 zHttpRequest::getCookieList() const {
    zString8 _cookie;
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

zString8 zHttpRequest::getHeaders(bool out) const {
    zString8 headers;
    if(!out) {
        auto tmp(hs);
        while(tmp) {
            headers += tmp->data; headers += "; ";
            tmp = tmp->next;
        }
    } else headers = header.str;
    return headers;
}
