
#include "zstandard/zstandard.h"
#include "zstandard/zHMAC_SHA1.h"

void zHMAC_SHA1::make(u8 *text, i32 text_len, u8 *key, i32 key_len, u8 *digest) {
	memset(SHA1_Key, 0, SHA1_BLOCK_SIZE);
	memset(m_ipad, 0x36, sizeof(m_ipad));
	memset(m_opad, 0x5c, sizeof(m_opad));
	zSHA1 sha1;
	if(key_len > SHA1_BLOCK_SIZE) {
		sha1.reset(); sha1.update(key, key_len); sha1.final();
		sha1.getHash((u8*)SHA1_Key);
	} else memcpy(SHA1_Key, key, key_len);
	for(int i = 0; i < (int)sizeof(m_ipad); i++) m_ipad[i] ^= SHA1_Key[i];
	sha1.reset(); sha1.update(m_ipad, sizeof(m_ipad)); sha1.update(text, text_len); sha1.final();
	char szReport[SHA1_DIGEST_LENGTH];
	sha1.getHash((u8*)szReport);
	for(int j = 0; j < (int)sizeof(m_opad); j++) m_opad[j] ^= SHA1_Key[j];
	sha1.reset(); sha1.update(m_opad, sizeof(m_opad)); sha1.update((u8*)szReport, SHA1_DIGEST_LENGTH); sha1.final();
	sha1.getHash((u8*)digest);
}
