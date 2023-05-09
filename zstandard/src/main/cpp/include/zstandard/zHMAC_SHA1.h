
#pragma once

#include "zstandard/zSHA1.h"

class zHMAC_SHA1 : public zSHA1 {
public:
    enum {
        SHA1_DIGEST_LENGTH	= 20,
        SHA1_BLOCK_SIZE		= 64
    };
    zHMAC_SHA1() { }
    void make(u8* text, i32 text_len, u8* key, i32 key_len, u8* digest);
private:
    u8 m_ipad[SHA1_BLOCK_SIZE];
    u8 m_opad[SHA1_BLOCK_SIZE];
    char SHA1_Key[SHA1_BLOCK_SIZE];
};
