
#include "zstandard/zstandard.h"
#include "zstandard/zCloud.h"

// ID: 53613083438649108d41b0c0e4a2b677
// password : 7db06a12cbb54af88067f05694563099
// test token = AQAAAAA0J587AAcR7WubvvcO_EWelDCMUhLPNTA

bool zCloudYandex::auth(czs& client_id, czs& client_secret) {
	req.setAccept("application/json");
	req.setContentType("pplication/x-www-form-urlencoded");
	req.setAgent("Mozilla / 5.0 (Windows NT 6.1; Win64; x64) AppleWebKit / 537.36 (KHTML, like Gecko) Chrome / 90.0.4430.72 Safari / 537.36");
	req.setReferer("https://cloud-api.yandex.net/");
	//https://oauth.yandex.ru/device/code?client_id=53613083438649108d41b0c0e4a2b677"
	req.requestPost("https://oauth.yandex.ru/device/code", "client_id=" + zStringUTF8(client_id));
	if(jsonResponse() != zHttpRequest::HTTP_OK) return false;
	auto code(js.getNode("device_code")->string());
	//auto expires(js.getNode("expires_in")->integer());
	req.requestPost("https://oauth.yandex.ru/token",
					z_fmtUTF8("grant_type=device_code&client_id=%s&client_secret=%s&code=%s", client_id.str(), client_secret.str(), code.str()));
	if(jsonResponse() == zHttpRequest::HTTP_OK) {
		token = js.getNode("access_token")->string();
		rtoken = js.getNode("refresh_token")->string();
		req.setCustomHeader("Authorization: ", "OAuth " + token);
		return true;
	}
	return false;
}

bool zCloudYandex::addFolder(czs& path) {
	req.requestPut(host + "?path=" + z_urlEncodeUTF8(path), 0, 0);
	return req.getStatus() == zHttpRequest::HTTP_CREATED;
}

bool zCloudYandex::upload(czs& dstPath, czs& srcPath) {
	auto fd(openf(srcPath, O_RDONLY, 0));
	if(fd > 0) {
		req.requestGet(host + "upload?path=" + z_urlEncodeUTF8(dstPath));
		if(jsonResponse() == zHttpRequest::HTTP_OK) {
			auto url(js.getNode("href")->string());
			auto size(lseekf(fd, 0, SEEK_END)); lseekf(fd, 0, SEEK_SET);
			req.requestPut(url, fd, size);
		}
		closef(fd);
		return req.getStatus() == zHttpRequest::HTTP_CREATED;
	}
	return false;
}

bool zCloudYandex::download(zStringUTF8 source, zStringUTF8 dest, int delim) {
	req.requestGet(host + "download?path=" + z_urlEncodeUTF8(source));
	if(jsonResponse() != zHttpRequest::HTTP_OK) return false;
	req.requestGet(js.getNode("href")->string());
	if(req.getStatus() != zHttpRequest::HTTP_OK) return false;
	saveResponse(dest.slash((cstr)&delim) + source.substrAfterLast((cstr)&delim, source));
	return true;
}

bool zCloudYandex::remove(czs& path) {
	req.requestCustom("DELETE", host + "?permanently=true&path=" + z_urlEncodeUTF8(path));
	return req.getStatus() == 204;
}

bool zCloudYandex::rename(czs& path, czs& name) {
	zStringUTF8 dest(path.substrBeforeLast("/", path) + name);
	auto urn(z_fmtUTF8("from=%s&path=%s&overwrite=true", z_urlEncodeUTF8(path).str(), z_urlEncodeUTF8(dest).str()));
	req.requestPost(host + "move?" + urn, "");
	return req.getStatus() == zHttpRequest::HTTP_CREATED;
}

bool zCloudYandex::moveOrCopy(czs& srcPath, czs& dstPath, bool move) {
	auto urn(z_fmtUTF8("from=%s&path=%s&overwrite=true", z_urlEncodeUTF8(srcPath).str(), z_urlEncodeUTF8(dstPath).str()));
	req.requestPost(z_fmtUTF8(host + "%s?%s", move ? "move" : "copy", urn.str()), "");
	return req.getStatus() == zHttpRequest::HTTP_CREATED;
}

zStringUTF8 zCloudYandex::publish(czs& path, bool remove) {
	zStringUTF8 link;
	req.requestPut(z_fmtUTF8(host + "%s?path=%s", remove ? "unpublish" : "publish", z_urlEncodeUTF8(path).str()), 0, 0);
	if(jsonResponse() == zHttpRequest::HTTP_OK) {
		link = js.getNode("href")->string();
	}
	return link;
}

zArray<zCloud::FileInfo> zCloudYandex::getFiles(czs& path, czs& meta) {
	zArray<zCloud::FileInfo> fi;
	req.requestGet(host + "files");
//	saveResponse("c:\\list.json");
	if(jsonResponse() == zHttpRequest::HTTP_OK) {

	}
	return fi;
}
