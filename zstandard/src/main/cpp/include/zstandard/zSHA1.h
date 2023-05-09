
#pragma once

#if !defined(SHA1_LITTLE_ENDIAN) && !defined(SHA1_BIG_ENDIAN)
#define SHA1_LITTLE_ENDIAN
#endif

#if !defined(SHA1_WIPE_VARIABLES) && !defined(SHA1_NO_WIPE_VARIABLES)
#define SHA1_WIPE_VARIABLES
#endif

typedef union {
	u8  c[64];
	u32 l[16];
} SHA1_WORKSPACE_BLOCK;

class zSHA1 {
public:
	zSHA1();
	~zSHA1() { reset(); }
	u32 m_state[5], m_count[2], __reserved1[1], __reserved2[3];
	u8  m_digest[20], m_buffer[64];
	void reset();
	void update(u8 *data, u32 len);
	void final();
	void getHash(u8 *puDest) { memcpy(puDest, m_digest, 20); }
private:
	void transform(u32 *state, u8 *buffer);
	u8 m_workspace[64];
	SHA1_WORKSPACE_BLOCK *m_block;
};
