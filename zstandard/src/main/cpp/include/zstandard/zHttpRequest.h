
#pragma once

#include "zJSON.h"

class zHttpRequest {
public:
    enum HttpCode {
        HTTP_CONTINUE = 100, HTTP_OK = 200, HTTP_CREATED = 201, HTTP_ACCEPTER = 202, HTTP_SUCCESEED = 204, HTTP_FOUND = 302,
        HTTP_BAD_REQUEST = 400, HTTP_FORBIDDEN = 403, HTTP_NOT_FOUND = 404, HTTP_INTERNAL_ERROR = 500
    };
    enum HttpRequest { HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_CUSTOM };
    enum HttpParams { HTTP_AGENT, HTTP_REFER, HTTP_ACCEPT, HTTP_CONTENT_TYPE, HTTP_CONTENT_LENGTH, HTTP_CERT, HTTP_PROXY };
    struct DynBuffer {
        // деструктор
        ~DynBuffer() { clear(); }
        // очистить
        void clear();
        // перераспределить память
        void realloc(int size, u8* data);
        // дескриптор файла для аплоада
        int fd{0};
        // текущий размер/размер буфера
        int size{0}, grow{0};
        // указатель на данные
        union { u8* ptr{nullptr}; char* str; };
    };
    struct REQUEST_DATA {
        zStringUTF8 linkParam;
        int fd{0}, size{0};
    };
    // конструктор
    zHttpRequest(bool follow_location, bool verbose = false);
    // деструктор
    virtual ~zHttpRequest() { close(); }
    // закрыть соединение
    void close();
    // установить свой заголовок
    void setCustomHeader(czs& head, czs& value, bool reset = false) {
        if(reset) curl_slist_free_all(hs), hs = nullptr;
        changeHeaders(head, value);
    }
    // установить список куки
    void setCookieList(czs& cookie) const { if(curl) curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.str()); }
    void setAuth(cstr login, cstr pwd, int type = CURLAUTH_BASIC) const;
    // установить связанные параметры
    bool setEmbeddedParams(HttpParams type, cstr value1, cstr value2 = nullptr, int size = 0);
    // сохранить ответ в файл
    bool saveResponse(czs& path) const { return resp.ptr && resp.size && zFile(path, false, false).write(resp.ptr, resp.size); }
    // выполнить запрос
    int request(HttpRequest req, czs& url, const REQUEST_DATA& reqData, bool response = false, bool nobody = false);
    // вернуть статус последнего запроса
    int getStatus() const { long status(HTTP_INTERNAL_ERROR); if(curl) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status); return status; }
    // сформировать json ответ и вернуть код операции
    int jsonResponse() { return (resp.ptr && js.init(resp.ptr, resp.size) ? getStatus() : HTTP_INTERNAL_ERROR); }
    // вернуть редирект ссылку
    zStringUTF8 getRedirect() const;
    // вернуть заголовки
    zStringUTF8 getHeaders(bool out) const;
    // вернуть список куки
    zStringUTF8 getCookieList() const;
    // вернуть буфер ответа
    const DynBuffer& response() const { return resp; }
    // вернуть json
    zJSON& json() { return js; }
protected:
    // применить заголовок
    void changeHeaders(czs& what, czs& value);
    // выполнить запрос
    bool exec(czs& url, bool nobody);
    // буфер ошибок
    char errorBuffer[CURL_ERROR_SIZE]{};
    // заголовки
    struct curl_slist* hs{nullptr};
    // буферы для ответа/загрузки/заголовков
    DynBuffer resp, upload, header;
    // json
    zJSON js;
    // библиотека
    CURL* curl{nullptr};
};
