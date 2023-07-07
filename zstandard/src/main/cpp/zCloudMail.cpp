//
// Created by User on 05.05.2022.
//

#include "zstandard/zstandard.h"
#include "zstandard/zCloud.h"

zStringUTF8 zCloudMail::getUrl(cstr type) {
    zStringUTF8 link;
    req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "application/json");
    req.setEmbeddedParams(zHttpRequest::HTTP_CONTENT_TYPE, "application/x-www-form-urlencoded");
    if(req.request(zHttpRequest::HTTP_GET, host + "dispatcher", {}, true) == zHttpRequest::HTTP_OK ) {
        auto n(js.getNode(0, js.getNode(type, js["body"])));
        link = n->child[1]->string();
    }
    return link;
}

bool zCloudMail::auth(cstr, cstr, cstr) {
    req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "text/html,application/xhtml + xml,application/xml; q=0.9,*/*;q=0.8");
    req.setEmbeddedParams(zHttpRequest::HTTP_AGENT, "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.72 Safari/537.36");
    req.setEmbeddedParams(zHttpRequest::HTTP_REFER, "https://cloud.mail.ru/home/");
    // запускаем авторизацию
    auto urn(z_fmtUTF8("Login=%s&Domain=mail.ru&Password=%s&new_auth_form=1&FailPage=", login.str(), password.str()));
    auto url("https://auth.mail.ru/cgi-bin/auth?from=splash");
    if(req.request(zHttpRequest::HTTP_POST, url, { urn }) == zHttpRequest::HTTP_OK) {
        // получаем cookie sdc
        url = "https://auth.mail.ru/sdc?from=https://cloud.mail.ru/home";
        if(req.request(zHttpRequest::HTTP_GET, url, {}, false, true) == zHttpRequest::HTTP_OK) {
            // получаем токен
            req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "application/json");
            if(req.request(zHttpRequest::HTTP_GET, host + "tokens/csrf", {}, true) == zHttpRequest::HTTP_OK) {
                token = js.getPath("nbody/ntoken")->string();
                email = js["email"]->string();
                req.setCustomHeader("X-CSRF-Token", token);
                return true;
            }
        }
    }
    return false;
}

bool zCloudMail::add(czs& path, czs& hash, int size) {
    auto hasFile(hash.isNotEmpty() && size);
    auto part(hasFile ? z_fmtUTF8("&hash=%s&size=%i", hash.str(), size) : zStringUTF8(""));
    auto urn(z_fmtUTF8("api=2&conflict=strict&home=%s&email=%s&x-email=%s%s", path.str(), getEmail().str(), getEmail().str(), part.str()));
    auto url(z_fmtUTF8("%s%s/add", host.str(), hasFile ? "file" : "folder"));
    req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "*/*");
    req.setEmbeddedParams(zHttpRequest::HTTP_CONTENT_TYPE, "application/x-www-form-urlencoded");
    return req.request(zHttpRequest::HTTP_POST, url, { urn }) == zHttpRequest::HTTP_OK;
}

bool zCloudMail::addFolder(czs& path) {
    return add(path, "", 0);
}

bool zCloudMail::upload(czs& srcPath, czs& dstPath) {
    zFile f(srcPath, true);
    if(f.getFd() > 0) {
        auto url(z_fmtUTF8("%s?cloud_domain=2&x-email=%s", getUrl("upload").str(), getEmail().str()));
        req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "*/*");
        if(req.request(zHttpRequest::HTTP_PUT, url, { "", f.getFd(), f.length() }) == zHttpRequest::HTTP_CREATED)
            return add(dstPath + srcPath.substrAfterLast("/"), req.response().str, f.length());
    }
    return false;
}

bool zCloudMail::download(zStringUTF8 source, zStringUTF8 dest) {
    auto url(z_fmtUTF8("%s%s", getUrl("get").str(), source.trimLeft("/").str()));
    req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "text/html, application/xhtml + xml, application/xml; q=0.9, */*;q=0.8");
    if(req.request(zHttpRequest::HTTP_GET, url, {}) == zHttpRequest::HTTP_OK) {
        req.saveResponse(dest.slash() + source.substrAfterLast("/", source));
        return true;
    }
    return false;
}

bool zCloudMail::remove(czs& path) {
    auto urn(z_fmtUTF8("home=%s&api=2&email=%s&x-email=%s", path.str(), getEmail().str(), getEmail().str()));
    req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "*/*");
    return req.request(zHttpRequest::HTTP_POST, host + "file/remove", { urn }) == zHttpRequest::HTTP_OK;
}

bool zCloudMail::rename(czs& path, czs& name) {
    auto urn(z_fmtUTF8("home=%s&api=2&email=%s&x-email=%s&conflict=rename&name=%s", path.str(), getEmail().str(), getEmail().str(), name.str()));
    req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "*/*");
    return req.request(zHttpRequest::HTTP_POST, host + "file/rename", { urn }) == zHttpRequest::HTTP_OK;
}

bool zCloudMail::moveOrCopy(czs& srcPath, czs& dstPath, bool move) {
    auto url(z_fmtUTF8(host + "file/%s", move ? "move" : "copy"));
    auto urn(z_fmtUTF8("home=%s&api=2&email=%s&x-email=%s&conflict=rename&folder=%s", srcPath.str(), getEmail().str(), getEmail().str(), dstPath.str()));
    req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "*/*");
    return req.request(zHttpRequest::HTTP_POST, url, { urn }) == zHttpRequest::HTTP_OK;
}

zArray<zCloudMail::FileInfo> zCloudMail::getFiles(czs& _path) {
    zArray<FileInfo> ret;
    auto uri(z_fmtUTF8(host + "folder?home=%s", _path.str()));
    req.setEmbeddedParams(zHttpRequest::HTTP_CONTENT_TYPE, "application/x-www-form-urlencoded");
    req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "application/json");
    if(req.request(zHttpRequest::HTTP_GET, uri, {}, true) == zHttpRequest::HTTP_OK) {
        auto lst(js.getPath("nbody/nlist"));
        for(int i = 0; i < lst->count(); i++) {
            // count, tree, name, grev, size, kind, rev, type, home
            auto o(js.getNode(i, lst));
            auto size(js.getNode("size", o)->integer());
            auto type(js.getNode("type", o)->string());
            auto path(js.getNode("home", o)->string());
            ret += FileInfo(path, path.substrAfterLast("."), size, type == "folder");
        }
    }
    return ret;
}

zStringUTF8 zCloudMail::publish(czs& path, bool remove) {
    zStringUTF8 link;
    auto url(z_fmtUTF8("%sfile/%s", host.str(), remove ? "unpublish" : "publish"));
    auto urn(z_fmtUTF8("conflict=&home=%s&email=%s&x-email=%s&api=2", path.str(), getEmail().str(), getEmail().str()));
    req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "*/*");
    if(req.request(zHttpRequest::HTTP_POST, url, { urn }, true) == zHttpRequest::HTTP_OK)
        link = "https://cloud.mail.ru/public/" + js["body"]->string();
    return link;
}
