
#include "zstandard/zstandard.h"
#include "zstandard/zCloud.h"

// clientID: 53613083438649108d41b0c0e4a2b677
// clientSecret: 7db06a12cbb54af88067f05694563099
// 
// password : 7db06a12cbb54af88067f05694563099
// test token = y0_AgAAAAA0J587AAcR7QAAAADm1At90JOM0CmvTN25iK9EtbufR_5dfLQ
// redirect: https://oauth.yandex.ru/verification_code

bool zCloudYandex::auth(cstr id, cstr secret, cstr _token) {
	token = _token;
	req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "application/json");
	req.setEmbeddedParams(zHttpRequest::HTTP_CONTENT_TYPE, "application/x-www-form-urlencoded");
	req.setEmbeddedParams(zHttpRequest::HTTP_AGENT, "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.72 Safari/537.36");
	req.setEmbeddedParams(zHttpRequest::HTTP_REFER, "https://cloud-api.yandex.net/");
	req.setAuth(id, secret);
	req.setCustomHeader("Authorization", "OAuth " + token);
	return true;
}

bool zCloudYandex::addFolder(czs& path) {
	auto url(host.substrBeforeLast("/") + "?path=" + z_urlEncode8(path));
	return req.request(zHttpRequest::HTTP_CUSTOM, url, { "PUT" }) == zHttpRequest::HTTP_CREATED;
}

bool zCloudYandex::upload(czs& srcPath, czs& dstPath) {
	zFile f(srcPath, true);
	if(f.getFd() > 0) {
		auto url(host + "upload?overwrite=true&path=" + z_urlEncode8(dstPath + srcPath.substrAfterLast("/", srcPath)));
		if(req.request(zHttpRequest::HTTP_GET, url, {}, true) == zHttpRequest::HTTP_OK) {
			url = req.json().getNode("href")->string();
			return req.request(zHttpRequest::HTTP_PUT, url, { "", f.getFd(), f.length() }) == zHttpRequest::HTTP_CREATED;
		}
	}
	return false;
}

bool zCloudYandex::download(zString8 source, zString8 dest) {
	if(req.request(zHttpRequest::HTTP_GET, host + "download?path=" + z_urlEncode8(source), {}, true) == zHttpRequest::HTTP_OK) {
		if(req.request(zHttpRequest::HTTP_GET, req.json().getNode("href")->string(), {}) == zHttpRequest::HTTP_OK) {
			return req.saveResponse(dest.slash() + source.substrAfterLast("/", source));
		}
	}
	return false;
}

bool zCloudYandex::remove(czs& path) {
	auto url(host.substrBeforeLast("/") + "?permanently=true&path=" + z_urlEncode8(path));
	return req.request(zHttpRequest::HTTP_CUSTOM, url, { "DELETE" }) == zHttpRequest::HTTP_SUCCESEED;
}

bool zCloudYandex::rename(czs& path, czs& name) {
	zString8 dest(path.substrBeforeLast("/", path) + "/" + name);
	auto urn(z_fmt8("move?from=%s&path=%s&overwrite=true", z_urlEncode8(path).str(), z_urlEncode8(dest).str()));
	return req.request(zHttpRequest::HTTP_POST, host + urn, {}) == zHttpRequest::HTTP_CREATED;
}

bool zCloudYandex::moveOrCopy(czs& srcPath, czs& dstPath, bool move) {
	auto urn(z_fmt8("%s?from=%s&path=%s&overwrite=true", move ? "move" : "copy", z_urlEncode8(srcPath).str(), z_urlEncode8(dstPath).str()));
	return req.request(zHttpRequest::HTTP_POST, host + urn, {}) == zHttpRequest::HTTP_CREATED;
}

zString8 zCloudYandex::publish(czs& path, bool remove) {
	zString8 link;
	auto urn(z_fmt8("%s?path=%s", remove ? "unpublish" : "publish", z_urlEncode8(path).str()));
	if(req.request(zHttpRequest::HTTP_CUSTOM, host + urn, { "PUT" }, true) == zHttpRequest::HTTP_OK)
		link = js["href"]->string();
	return link;
}

zArray<zFile::zFileInfo> zCloudYandex::getFiles(czs& _path) {
	zArray<zFile::zFileInfo> ret; int offset(0);
	auto url(host.substrBeforeLast("/") + "?path=" + z_urlEncode8(_path) +
			 "&limit=40&sort=name&fields=_embedded.items.path,_embedded.items.type,_embedded.items.size");
	while(req.request(zHttpRequest::HTTP_GET, z_fmt8("%s&offset=%i", url.str(), offset), {}, true) == zHttpRequest::HTTP_OK) {
		auto lst(js.getPath("n_embedded/nitems"));
		auto count(lst->count());
		for(int i = 0; i < count; i++) {
			auto o(js.getNode(i, lst));
			fi.usize = js.getNode("size", o)->integer();
			fi.attr  = js.getNode("type", o)->string() == "dir" ? S_IFDIR : S_IFREG;
			fi.setPath(js.getNode("path", o)->string());
			fi.index = 0;
			ret      += fi;
		}
		if(count < 40) break;
		offset += count;
	}
	return ret;
}
