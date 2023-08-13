
#include "zstandard/zstandard.h"
#include "zstandard/zCloud.h"

// access_token -
// sl.BhunON2lZ92eKmw81w3UaeibqGjHk3LgIAbkD5QqpkqQw02a0G6m5R1TM-NayB5rypBzNf7RMP22XBd8rSEeSWOlHUByn-giBC2lYv_Xt4UdKFDJjVSOcyjcfzOKz65RjDYhc2Lx
// app_key - pergipm59i4tn05
// app_secret - keyv41fqyzf3r04

bool zCloudDropbox::auth(cstr id, cstr secret, cstr _token) {
	token = _token;
	req.setEmbeddedParams(zHttpRequest::HTTP_ACCEPT, "application/json");
	req.setEmbeddedParams(zHttpRequest::HTTP_CONTENT_TYPE, "application/x-www-form-urlencoded");
	req.setEmbeddedParams(zHttpRequest::HTTP_AGENT, "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.72 Safari/537.36");
	req.setAuth(id, secret);
	req.setCustomHeader("Authorization", "Bearer " + token);
	return true;
}

bool zCloudDropbox::addFolder(czs& path) {
	req.setEmbeddedParams(zHttpRequest::HTTP_CONTENT_TYPE, "application/json");
	js.clear(); js.addNode("path", path);
	return (req.request(zHttpRequest::HTTP_POST, host + "files/create_folder_v2", {js.save()}) == zHttpRequest::HTTP_OK);
}

bool zCloudDropbox::upload(czs& srcPath, czs& dstPath) {
	zFile f(srcPath, true);
	if(f.getFd() > 0) {
		req.setEmbeddedParams(zHttpRequest::HTTP_CONTENT_TYPE, "application/octet-stream");
		js.clear(); js.addNode("autorename", "true", zJSON::_bool);
		js.addNode("path", dstPath + srcPath.substrAfterLast("/"));
		req.setCustomHeader("Dropbox-API-Arg", js.save());
		auto url("https://content.dropboxapi.com/2/files/upload");
		return req.request(zHttpRequest::HTTP_PUT, url, {"POST", f.getFd(), f.length()}) == zHttpRequest::HTTP_OK;
	}
	return false;
}

bool zCloudDropbox::download(zString8 source, zString8 dest) {
	req.setEmbeddedParams(zHttpRequest::HTTP_CONTENT_TYPE, "text/plain");
	js.clear(); js.addNode("path", source);
	req.setCustomHeader("Dropbox-API-Arg", js.save());
	auto url("https://content.dropboxapi.com/2/files/download");
	if(req.request(zHttpRequest::HTTP_POST, url, {}) == zHttpRequest::HTTP_OK)
		return req.saveResponse(dest.slash() + source.substrAfterLast("/", source));
	return false;
}

bool zCloudDropbox::remove(czs& path) {
	req.setEmbeddedParams(zHttpRequest::HTTP_CONTENT_TYPE, "application/json");
	js.clear(); js.addNode("path", path);
	return req.request(zHttpRequest::HTTP_POST, host + "files/delete_v2", {js.save()}) == zHttpRequest::HTTP_OK;
}

bool zCloudDropbox::rename(czs& path, czs& name) {
	req.setEmbeddedParams(zHttpRequest::HTTP_CONTENT_TYPE, "application/json");
	js.clear(); js.addNode("from_path", path); js.addNode("to_path", path.substrBeforeLast("/", path) + "/" + name);
	return req.request(zHttpRequest::HTTP_POST, host + "files/move_v2", {js.save()}) == zHttpRequest::HTTP_OK;
}

bool zCloudDropbox::moveOrCopy(czs& srcPath, czs& dstPath, bool move) {
	req.setEmbeddedParams(zHttpRequest::HTTP_CONTENT_TYPE, "application/json");
	js.clear(); js.addNode("from_path", srcPath); js.addNode("to_path", dstPath);
	auto url(z_fmt8("%s%s", host.str(), move ? "move_v2" : "copy_v2"));
	return req.request(zHttpRequest::HTTP_POST, url , {js.save()}) == zHttpRequest::HTTP_OK;
}

zString8 zCloudDropbox::publish(czs&, bool) {
	return "";
}

zArray<zFile::zFileInfo> zCloudDropbox::getFiles(czs& _path) {
	zArray<zFile::zFileInfo> ret;
	req.setEmbeddedParams(zHttpRequest::HTTP_CONTENT_TYPE, "application/json");
	js.clear(); js.addNode("path", _path); 
	js.addNode("include_mounted_folders", "false", zJSON::_bool);
	js.addNode("include_non_downloadable_files", "false", zJSON::_bool);
	if(req.request(zHttpRequest::HTTP_POST, host + "files/list_folder", {js.save()}, true) == zHttpRequest::HTTP_OK) {
		auto lst(js["entries"]);
		auto count(lst->count());
		for(int i = 0; i < count; i++) {
			auto o(js.getNode(i, lst));
			fi.usize = js.getNode("size", o)->integer();
			fi.attr  = js.getNode(".tag", o)->string() == "folder" ? S_IFDIR : S_IFREG;
			fi.setPath(js.getNode("path_display", o)->string());
            fi.index = 0;
			ret      += fi;
		}
	}
	return ret;
}
