//
// Created by User on 05.05.2022.
//

#include "zstandard/zstandard.h"
#include "zstandard/zCloud.h"
#include <zstandard/zHMAC_SHA1.h>

/*
zOAuth::zOAuth(czs& key, czs& secret, czs& _version, OAuthMethod method) :
            consumerKey(key), consumerSecret(secret), version(_version), securityMethod(method) {
}

void zOAuth::genNonceTimeStamp(zStringUTF8& nonce, zStringUTF8& timeStamp) const {
    auto tm(time(nullptr)); srand(tm);
    timeStamp = z_ntos(&tm, RADIX_DEC, false);
    nonce = z_fmtUTF8("%x%s", rand(), timeStamp.str());
}

void zOAuth::addHeader(czs& treq, czs& url, czs& token, czs& secret, czs& pin) {
    static u8 digest[1024];
    zHMAC_SHA1 hmac;
    zStringUTF8 nonce, timeStamp;
    genNonceTimeStamp(nonce, timeStamp);
    auto header(z_fmtUTF8(R"(oauth_consumer_key="%s", oauth_nonce="%s", oauth_signature_method="HMAC-SHA1", oauth_timestamp="%s", oauth_version="1.0")",
                      consumerKey.str(), nonce.str(), timeStamp.str()));
    zStringUTF8 sigBase(treq); sigBase += "&";
    sigBase += url; sigBase += "&";
    zStringUTF8 params(header); params.replace("\"", ""); params.replace(", ", "&");
    sigBase += params;
    zStringUTF8 sigKey(z_fmtUTF8("%s&%s", consumerSecret.str(), secret.str()));
    hmac.make((u8*)sigBase.str(), sigBase.count(), (u8*)sigKey.str(), sigKey.count(), digest);
    sigKey = z_urlEncode(z_base64Encode(digest, 20));
    zStringUTF8 otoken(z_isempty(token) ? "" : z_fmtUTF8(R"(, oauth_token="%s")", token.str()).str());
    header = z_fmtUTF8("OAuth %s, oauth_signature=\"%s\"%s", header.str(), sigKey.str(), otoken.str());
    req.setCustomHeader("Authorization: ", header);
}

void zOAuth::fetchRequestToken(czs& url) {
    addHeader("POST", url, "", "", "");
    req.requestPost(url, "", true);
    printf("%s", req.getHeaders(true).str());
    printf("%s", req.response().str);
    if(req.getStatus() == zHttpRequest::HTTP_OK) {
        getTokenAndSecret(reqToken, reqSecret);
    }
}

void zOAuth::fetchAccessToken(czs& url) {
    addHeader("POST", url, reqToken, reqSecret, "");
    req.requestPost(url, "");
    if(req.getStatus() == zHttpRequest::HTTP_OK) {
        getTokenAndSecret(accToken, accSecret);
    }
}

void zOAuth::getTokenAndSecret(zStringUTF8& token, zStringUTF8& secret) const {
    //auto i = params.find("oauth_token");
    //token = i->second;
    //i = params.find("oauth_token_secret");
    //secret = i->second;
}
*/