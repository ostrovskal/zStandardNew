//
// Created by User on 05.05.2022.
//

#include "zstandard/zstandard.h"
#include "zstandard/zCloud.h"

zStringUTF8 zCloudMail::getUrl(czs& type) {
    zStringUTF8 link;
    req.setAccept("application/json");
    req.setContentType("application/x-www-form-urlencoded");
    if(req.requestGet(host + "dispatcher")) {
        if(jsonResponse() == zHttpRequest::HTTP_OK) {
            auto n(js.getNode(0, js.getNode(type, js.getNode("body"))));
            link = n->child[1]->string();
        }
    }
    return link;
}

bool zCloudMail::auth(cstr client_id, cstr client_secret) {
    req.setAccept("text/html,application/xhtml + xml,application/xml; q=0.9,*/*;q=0.8");
    req.setAgent("Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.72 Safari/537.36");
    req.setReferer("https://cloud.mail.ru/home/");
    // запускаем авторизацию
    auto urn(z_fmtUTF8("Login=%s&Domain=mail.ru&Password=%s&new_auth_form=1&FailPage=", login.str(), password.str()));
    if(req.requestPost("https://auth.mail.ru/cgi-bin/auth?from=splash", urn)) {
        // получаем cookie sdc
        if(req.requestGet("https://auth.mail.ru/sdc?from=https://cloud.mail.ru/home", true)) {
            // получаем токен
            req.setAccept("application/json");
            req.requestGet(host + "tokens/csrf");
            if(jsonResponse() == zHttpRequest::HTTP_OK) {
                token = js.getPath("nbody/ntoken")->string();
                email = js.getNode("email")->string();
                req.setCustomHeader("X-CSRF-Token: ", token);
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
    req.setAccept("*/*");
    req.setContentType("application/x-www-form-urlencoded");
    req.requestPost(url, urn);
    return req.getStatus() == zHttpRequest::HTTP_OK;
}

bool zCloudMail::addFolder(czs& path) {
    return add(path, "", 0);
}

bool zCloudMail::upload(czs& dstPath, czs& srcPath) {
    auto fd(openf(srcPath, O_RDONLY, 0));
    if(fd > 0) {
        auto size(lseekf(fd, 0, SEEK_END)); lseekf(fd, 0, SEEK_SET);
        auto url(z_fmtUTF8("%s?cloud_domain=2&x-email=%s", getUrl("upload").str(), getEmail().str()));
        req.setAccept("*/*");
        req.requestPut(url, fd, size);
        closef(fd);
        if(req.getStatus() == zHttpRequest::HTTP_CREATED)
            return add(dstPath, req.response()->str, size);
    }
    return false;
}

bool zCloudMail::download(zStringUTF8 source, zStringUTF8 dest, int delim) {
    auto url(z_fmtUTF8("%s%s", getUrl("get").str(), source.trimLeft((cstr)&delim).str()));
    req.setAccept("text/html, application/xhtml + xml, application/xml; q=0.9, */*;q=0.8");
    req.requestGet(url);
    if(req.getStatus() == zHttpRequest::HTTP_OK) {
        saveResponse(dest.slash((cstr)&delim) + source.substrAfterLast((cstr)&delim, source));
        return true;
    }
    return false;
}

bool zCloudMail::remove(czs& path) {
    auto urn(z_fmtUTF8("home=%s&api=2&email=%s&x-email=%s", path.str(), getEmail().str(), getEmail().str()));
    req.setAccept("*/*");
    req.requestPost(host + "file/remove", urn);
    return req.getStatus() == zHttpRequest::HTTP_OK;
}

bool zCloudMail::rename(czs& path, czs& name) {
    auto urn(z_fmtUTF8("home=%s&api=2&email=%s&x-email=%s&conflict=rename&name=%s", path.str(), getEmail().str(), getEmail().str(), name.str()));
    req.setAccept("*/*");
    req.requestPost(host + "file/rename", urn, true);
    return req.getStatus() == zHttpRequest::HTTP_OK;
}

bool zCloudMail::moveOrCopy(czs& srcPath, czs& dstPath, bool move) {
    auto urn(z_fmtUTF8("home=%s&api=2&email=%s&x-email=%s&conflict=rename&folder=%s", srcPath.str(), getEmail().str(), getEmail().str(), dstPath.str()));
    auto url(z_fmtUTF8(host + "file/%s", move ? "move" : "copy"));
    req.setAccept("*/*");
    req.requestPost(url, urn);
    return req.getStatus() == zHttpRequest::HTTP_OK;
}

zArray<zCloudMail::FileInfo> zCloudMail::getFiles(czs& _path, czs& _meta) {
    zArray<FileInfo> ret;
    auto uri(z_fmtUTF8(host + "folder?home=%s", _path.str()));
    req.setContentType("application/x-www-form-urlencoded");
    req.setAccept("application/json");
    if(req.requestGet(uri)) {
        if(jsonResponse() == zHttpRequest::HTTP_OK) {
            auto lst(js.getPath("nbody/nlist"));
            for(int i = 0; i < lst->count(); i++) {
                // count, tree, name, grev, size, kind, rev, type, home
                auto o(js.getNode(i, lst));
                auto size(js.getNode("size", o)->integer());
                auto rev(js.getNode("rev", o)->integer());
                auto type(js.getNode("type", o)->string());
                auto path(js.getNode("home", o)->string());
                ret += FileInfo(path, size, rev, type == "folder");
            }
        }
    }
    return ret;
}

zStringUTF8 zCloudMail::publish(czs& path, bool remove) {
    zStringUTF8 link;
    auto urn(z_fmtUTF8("conflict=&home=%s&email=%s&x-email=%s&api=2", path.str(), getEmail().str(), getEmail().str()));
    req.setAccept("*/*");
    req.requestPost(z_fmtUTF8("%sfile/%s", host.str(), remove ? "unpublish" : "publish"), urn);
    if(jsonResponse() == zHttpRequest::HTTP_OK) {
        link = "https://cloud.mail.ru/public/";
        link += js.getNode("body")->string();
    }
    return link;
}
