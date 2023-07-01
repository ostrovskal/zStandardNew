//
// Created by User on 05.05.2022.
//

#pragma once

#include "zJSON.h"

class zHttpRequest {
public:
    enum HttpCode { HTTP_CONTINUE = 100, HTTP_OK = 200, HTTP_CREATED = 201, HTTP_ACCEPTER = 202, HTTP_BAD_REQUEST = 400,
        HTTP_FORBIDDEN = 403, HTTP_INTERNAL_ERROR = 500 };
    struct DynBuffer {
        ~DynBuffer() { clear(); }
        void clear() { grow = size = 0; SAFE_DELETE(ptr); }
        void realloc(int size, u8* data);
        // дескриптор файла для аплоада
        int fd{0};
        // текущий размер/размер буфера
        int size{0}, grow{0};
        // указатель на данные
        union {
            u8* ptr{nullptr};
            char* str;
        };
    };
    // конструктор
    zHttpRequest(bool follow_location, bool verbose = false);
    // деструктор
    virtual ~zHttpRequest() { close(); }
    void close();
    void setProxy(czs& url, czs& login_pwd);
    void setContentType(czs& content) { changeHeaders("Content-Type: ", content); }
    void setAccept(czs& accept) { changeHeaders("Accept: ", accept); }
    void setContentLength(int size) { changeHeaders("Content-Length: ", z_ntos(&size, RADIX_DEC, false)); }
    void setAgent(czs& agent) const { if(curl) curl_easy_setopt(curl, CURLOPT_USERAGENT, agent.str()); }
    void setReferer(czs& refer) const { if(curl) curl_easy_setopt(curl, CURLOPT_REFERER, refer.str()); }
    void setCookieList(czs& cookie) const { if(curl) curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.str()); }
    void setCustomHeader(czs& header, czs& value, bool reset = false);
    void setCert(czs& cert) const;
    bool requestPost(czs& url, czs& fields, bool body = false);
    bool requestGet(czs& url, bool body = false);
    bool requestPut(czs& url, int fd, int size);
    bool requestCustom(czs& name, czs& url);
    int getStatus() const;
    zStringUTF8 getHeaders(bool out) const;
    zStringUTF8 getCookieList() const;
    zStringUTF8 getRedirect() const;
    const DynBuffer& response() const { return resp; }
protected:
    // применить заголовок
    void changeHeaders(czs& what, czs& value);
    // выполнить запрос
    bool exec(czs& url, bool body);
    // библиотека
    CURL* curl{nullptr};
    // буфер ошибок
    char errorBuffer[CURL_ERROR_SIZE]{};
    // заголовки
    struct curl_slist* hs{nullptr};
    // буферы для ответа/загрузки/заголовков
    DynBuffer resp, upload, header;
};

/*
class zOAuth {
public:
    enum OAuthMethod { PlainText, HMAC_SHA1, RSA_SHA1 };
    zOAuth() { }
    zOAuth(czs& key, czs& secret, czs& _version = "1.0", OAuthMethod method = PlainText);
    virtual ~zOAuth() { }
    void fetchRequestToken(czs& url);
    void fetchAccessToken(czs& url);
    czs& requestToken() const { return reqToken; }
    czs& requestTokenSecret() const { return reqSecret; }
    czs& accessToken() const { return accToken; }
    czs& accessTokenSecret() const { return accSecret; }
    void setAccessTokenAndSecret(czs& token, czs& secret) { accToken = token; accSecret = secret; }
protected:
    void genNonceTimeStamp(zStringUTF8& nonce, zStringUTF8& timeStamp) const;
    void getTokenAndSecret(zStringUTF8& token, zStringUTF8& secret) const;
    void addHeader(czs& treq, czs& url, czs& token, czs& secret, czs& pin);
    zStringUTF8 reqToken;
    zStringUTF8 reqSecret;
    zStringUTF8 accToken;
    zStringUTF8 accSecret;
    zStringUTF8 consumerKey;
    zStringUTF8 consumerSecret;
    zStringUTF8 version;
    OAuthMethod securityMethod{ PlainText };
    zHttpRequest req{true};
};
*/

class zCloud {
public:
    struct FileInfo {
        FileInfo() { }
        FileInfo(const zStringUTF8& p, int s, int r, bool f) : size(s), rev(r), folder(f), path(p) { }
        // размер/ревизия
        int size{0}, rev{0};
        // признак папки
        bool folder{false};
        // полный путь
        zStringUTF8 path;
    };
    // конструктор
    zCloud(czs& _host, czs& _login, czs& _password) : login(_login), password(_password), host(_host) { }
    // деструктор
    virtual ~zCloud() { req.close(); /*SAFE_DELETE(oauth);*/ }
    // авторизация
    virtual bool auth(czs& client_id, czs& client_secret) = 0;
    // создание папки
    virtual bool addFolder(czs& path) = 0;
    // загрузка файла
    virtual bool upload(czs& dstPath, czs& srcPath) = 0;
    // скачивание файла/папки
    virtual bool download(zStringUTF8 source, zStringUTF8 dest, int delim) = 0;
    // удаление файла
    virtual bool remove(czs& path) = 0;
    // переименование файла/папки
    virtual bool rename(czs& path, czs& name) = 0;
    // копирование/перемещение файла
    virtual bool moveOrCopy(czs& srcPath, czs& dstPath, bool move) = 0;
    // публикация
    virtual zStringUTF8 publish(czs& path, bool remove) = 0;
    // получение списка файлов/папок
    virtual zArray<FileInfo> getFiles(czs& path, czs& meta) = 0;
    // вернуть логин
    zStringUTF8 getLogin() const { return login; }
    // вернуть пароль
    zStringUTF8 getPassword() const { return password; }
    // вернуть почту
    zStringUTF8 getEmail() const { return email; }
protected:
    // сохранить ответ в файл
    void saveResponse(czs& path) const {
        zStringUTF8 resp(req.response().str);
        zFile(path, false, false).writeString(resp, false);
    }
    // сохранить заголовки в файл
    void saveHeaders(czs& path, bool out = true) const {
        zFile(path, false, false).writeString(req.getHeaders(out), true);
    }
    // сформировать json ответ и вернуть код операции
    int jsonResponse() {
        auto r(req.response());
        if(!js.init(r.ptr, r.size)) return 0;
        return req.getStatus();
    }
    // запрос
    zHttpRequest req{true};
    // авторизатор
    //zOAuth* oauth{nullptr};
    // логин/пароль/почта
    zStringUTF8 login, password, email;
    // токен доступа/токен обновления
    zStringUTF8 token, rtoken;
    // базовый хост
    zStringUTF8 host;
    // json
    zJSON js;
};

class zCloudMail : public zCloud {
public:
    // конструктор
    zCloudMail(czs& login, czs& password) : zCloud("https://cloud.mail.ru/api/v2/", login, password) { }
    // деструктор
    virtual ~zCloudMail() { }
    // авторизация
    virtual bool auth(czs& client_id, czs& client_secret) override;
    // создание папки
    virtual bool addFolder(czs& path) override;
    // загрузка файла
    virtual bool upload(czs& dstPath, czs& srcPath) override;
    // скачивание файла/папки
    virtual bool download(zStringUTF8 source, zStringUTF8 dest, int delim) override;
    // удаление файла
    virtual bool remove(czs& path) override;
    // переименование файла/папки
    virtual bool rename(czs& path, czs& name) override;
    // копирование/перемещение файла
    virtual bool moveOrCopy(czs& srcPath, czs& dstPath, bool move) override;
    // публикация
    virtual zStringUTF8 publish(czs& path, bool remove) override;
    // получение списка файлов/папок
    virtual zArray<FileInfo> getFiles(czs& path, czs& meta) override;
protected:
    // добавление файла/папки
    bool add(czs& path, czs& hash, int size);
    // получение линка
    zStringUTF8 getUrl(czs& type);
};

class zCloudYandex : public zCloud {
public:
    // конструктор -
    zCloudYandex(czs& login, czs& password) : zCloud("https://cloud-api.yandex.net/v1/disk/resources/", login, password) { }
    // авторизация
    virtual bool auth(czs& client_id, czs& client_secret) override;
    // создание папки
    virtual bool addFolder(czs& path) override;
    // загрузка файла
    virtual bool upload(czs& dstPath, czs& srcPath) override;
    // скачивание файла/папки
    virtual bool download(zStringUTF8 source, zStringUTF8 dest, int delim) override;
    // удаление файла
    virtual bool remove(czs& path) override;
    // переименование файла/папки
    virtual bool rename(czs& path, czs& name) override;
    // копирование/перемещение файла
    virtual bool moveOrCopy(czs& srcPath, czs& dstPath, bool move) override;
    // публикация
    virtual zStringUTF8 publish(czs& path, bool remove) override;
    // получение списка файлов/папок
    virtual zArray<FileInfo> getFiles(czs& path, czs& meta) override;
};
