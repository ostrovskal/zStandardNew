//
// Created by User on 05.05.2022.
//

#pragma once

#include "zHttpRequest.h"

class zCloud {
public:
    // конструктор
    zCloud(cstr _host, cstr _login, cstr _password) : login(_login), password(_password), host(_host), js(req.json()) { }
    // деструктор
    virtual ~zCloud() { req.close(); }
    // закрыть соединение
    virtual void close() { req.close(); }
    // авторизация
    virtual bool auth(cstr client_id, cstr client_secret, cstr token) = 0;
    // создание папки
    virtual bool addFolder(czs& path) = 0;
    // загрузка файла
    virtual bool upload(czs& dstPath, czs& srcPath) = 0;
    // скачивание файла/папки
    virtual bool download(zString8 source, zString8 dest) = 0;
    // удаление файла
    virtual bool remove(czs& path) = 0;
    // переименование файла/папки
    virtual bool rename(czs& path, czs& name) = 0;
    // копирование/перемещение файла
    virtual bool moveOrCopy(czs& srcPath, czs& dstPath, bool move) = 0;
    // публикация
    virtual zString8 publish(czs& path, bool remove) = 0;
    // получение списка файлов/папок
    virtual zArray<zFile::zFileInfo> getFiles(czs& path) = 0;
    // вернуть код ответа от сервера
    int getStatus() const { return req.getStatus(); }
    // сохранить заголовки в файл
    void saveHeaders(czs& path, bool out = true) const { zFile(path, false, false).writeString(req.getHeaders(out), true); }
    // вернуть логин
    zString8 getLogin() const { return login; }
    // вернуть пароль
    zString8 getPassword() const { return password; }
    // вернуть почту
    zString8 getEmail() const { return email; }
protected:
    // запрос
    zHttpRequest req{true};
    // авторизатор
    //zOAuth* oauth{nullptr};
    // логин/пароль/почта
    zString8 login, password, email;
    // токен доступа/токен обновления
    zString8 token, rtoken;
    // базовый хост
    zString8 host;
    // JSON
    zJSON& js;
    // структура информации о файле
    static zFile::zFileInfo fi;

};

class zCloudMail : public zCloud {
public:
    // конструктор
    zCloudMail(cstr login, cstr password) : zCloud("https://cloud.mail.ru/api/v2/", login, password) { }
    // деструктор
    virtual ~zCloudMail() { }
    // авторизация
    virtual bool auth(cstr client_id, cstr client_secret, cstr token) override;
    // создание папки: path => полный путь к папке
    virtual bool addFolder(czs& path) override;
    // загрузка файла: srcPath => полный путь к файлу(с именем файла), dstPath => полный путь(имя берется из srcPath)
    virtual bool upload(czs& srcPath, czs& dstPath) override;
    // скачивание файла/папки: source => полный путь на сервере, dest => путь без имени
    virtual bool download(zString8 source, zString8 dest) override;
    // удаление файла: path => полный путь на сервере
    virtual bool remove(czs& path) override;
    // переименование файла/папки: path => полный путь на сервере, name => новое имя
    virtual bool rename(czs& path, czs& name) override;
    // копирование/перемещение файла: srcPath => полный путь к файлу на сервере, dstPath => путь к новой папке на сервер, 
    // move => признак переносить/копировать
    virtual bool moveOrCopy(czs& srcPath, czs& dstPath, bool move) override;
    // публикация: path => полный путь на сервере, remove => признак создавать ссылку/отозвать ее
    virtual zString8 publish(czs& path, bool remove) override;
    // получение списка файлов/папок: path => полный путь к папке на сервере из которой берутся файлы
    virtual zArray<zFile::zFileInfo> getFiles(czs& path) override;
protected:
    // добавление файла/папки
    bool add(czs& path, czs& hash, int size);
    // получение линка
    zString8 getUrl(cstr type);
};

class zCloudYandex : public zCloud {
public:
    // конструктор -
    zCloudYandex(cstr login, cstr password) : zCloud("https://cloud-api.yandex.net/v1/disk/resources/", login, password) {}
    // авторизация
    virtual bool auth(cstr client_id, cstr client_secret, cstr token) override;
    // создание папки
    virtual bool addFolder(czs& path) override;
    // загрузка файла
    virtual bool upload(czs& dstPath, czs& srcPath) override;
    // скачивание файла/папки
    virtual bool download(zString8 source, zString8 dest) override;
    // удаление файла
    virtual bool remove(czs& path) override;
    // переименование файла/папки
    virtual bool rename(czs& path, czs& name) override;
    // копирование/перемещение файла
    virtual bool moveOrCopy(czs& srcPath, czs& dstPath, bool move) override;
    // публикация
    virtual zString8 publish(czs& path, bool remove) override;
    // получение списка файлов/папок
    virtual zArray<zFile::zFileInfo> getFiles(czs& path) override;
};

class zCloudDropbox : public zCloud {
public:
    // конструктор -
    zCloudDropbox(cstr login, cstr password) : zCloud("https://api.dropboxapi.com/2/", login, password) {}
    // авторизация
    virtual bool auth(cstr client_id, cstr client_secret, cstr token) override;
    // создание папки
    virtual bool addFolder(czs& path) override;
    // загрузка файла
    virtual bool upload(czs& dstPath, czs& srcPath) override;
    // скачивание файла/папки
    virtual bool download(zString8 source, zString8 dest) override;
    // удаление файла
    virtual bool remove(czs& path) override;
    // переименование файла/папки
    virtual bool rename(czs& path, czs& name) override;
    // копирование/перемещение файла
    virtual bool moveOrCopy(czs& srcPath, czs& dstPath, bool move) override;
    // публикация
    virtual zString8 publish(czs& path, bool remove) override;
    // получение списка файлов/папок
    virtual zArray<zFile::zFileInfo> getFiles(czs& path) override;
};

