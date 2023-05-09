
#include "zstandard/zstandard.h"
#include "zstandard/zJpg.h"

static char ZZ[64] = { 0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40,
					   48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29,
					   22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

zJpg::zJpg(cstr path) {
	if(path) {
		zFile fl;
		if(fl.open(path, true)) {
			u8* ptr; i32 _size;
			if((ptr = (u8*)fl.readn(&_size))) {
				decode(ptr, _size);
			}
			SAFE_A_DELETE(ptr);
		}
	}
}

bool zJpg::decode(const u8* ptr, size_t sz) {
	pos = ptr; size = (int)(sz & 0x7FFFFFFF);
	if(size < 2) return false;
	if((pos[0] ^ 0xFF) | (pos[1] ^ 0xD8)) return false;
	if(!skip(2)) return false;
	vlct = new VlcCode[4 * 65536];
	while(true) {
		bool result(false);
		if(size < 2 || pos[0] != 0xFF) break;
		if(!skip(2)) break;
		switch(pos[-1]) {
			case 0xC0: result = sof(); break;
			case 0xC4: result = dht(); break;
			case 0xDB: result = dqt(); break;
			case 0xDD: result = dri(); break;
			case 0xDA: result = decodeScan(); if(!result) break; convert(); SAFE_A_DELETE(vlct); return true;
			case 0xFE: result = skipMarker(); break;
			default: result = (pos[-1] & 0xF0) == 0xE0 ? skipMarker() : true;
		}
		if(!result) break;
	}
	cleanup();
	return false;
}

void zJpg::cleanup() {
	for(auto& c : comp) SAFE_DELETE(c.pixels);
	SAFE_A_DELETE(rgb);
	SAFE_A_DELETE(vlct);
	width = height = 0;
}

bool zJpg::_length() {
	if(size < 2) return false;
	length = decode16(pos);
	if(length > size) return false;
	return skip(2);
}

void zJpg::rowIDCT(int* blk) {
	int x0, x1, x2, x3, x4, x5, x6, x7, x8;
	if(!((x1 = blk[4] << 11) | (x2 = blk[6]) | (x3 = blk[2]) | (x4 = blk[1]) | (x5 = blk[7]) | (x6 = blk[5]) | (x7 = blk[3]))) {
		blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] = blk[7] = blk[0] << 3;
	} else {
		x0 = (blk[0] << 11) + 128; x8 = W7 * (x4 + x5);
		x4 = x8 + (W1 - W7) * x4; x5 = x8 - (W1 + W7) * x5;
		x8 = W3 * (x6 + x7); x6 = x8 - (W3 - W5) * x6;
		x7 = x8 - (W3 + W5) * x7; x8 = x0 + x1; x0 -= x1;
		x1 = W6 * (x3 + x2); x2 = x1 - (W2 + W6) * x2;
		x3 = x1 + (W2 - W6) * x3; x1 = x4 + x6; x4 -= x6;
		x6 = x5 + x7; x5 -= x7; x7 = x8 + x3; x8 -= x3;
		x3 = x0 + x2; x0 -= x2;
		x2 = (181 * (x4 + x5) + 128) >> 8;
		x4 = (181 * (x4 - x5) + 128) >> 8;
		blk[0] = (x7 + x1) >> 8; blk[1] = (x3 + x2) >> 8;
		blk[2] = (x0 + x4) >> 8; blk[3] = (x8 + x6) >> 8;
		blk[4] = (x8 - x6) >> 8; blk[5] = (x0 - x4) >> 8;
		blk[6] = (x3 - x2) >> 8; blk[7] = (x7 - x1) >> 8;
	}
}

void zJpg::colIDCT(const int* blk, u8* out, int stride) {
	int x0, x1, x2, x3, x4, x5, x6, x7, x8;
	if(!((x1 = blk[8 * 4] << 8) | (x2 = blk[8 * 6]) | (x3 = blk[8 * 2]) | (x4 = blk[8 * 1]) | (x5 = blk[8 * 7]) | (x6 = blk[8 * 5]) | (x7 = blk[8 * 3]))) {
		x1 = clip(((blk[0] + 32) >> 6) + 128);
		for(x0 = 8; x0; --x0) { *out = (unsigned char)x1; out += stride; }
		return;
	}
	x0 = (blk[0] << 8) + 8192;
	x8 = W7 * (x4 + x5) + 4;
	x4 = (x8 + (W1 - W7) * x4) >> 3;
	x5 = (x8 - (W1 + W7) * x5) >> 3;
	x8 = W3 * (x6 + x7) + 4;
	x6 = (x8 - (W3 - W5) * x6) >> 3;
	x7 = (x8 - (W3 + W5) * x7) >> 3;
	x8 = x0 + x1; x0 -= x1;
	x1 = W6 * (x3 + x2) + 4;
	x2 = (x1 - (W2 + W6) * x2) >> 3;
	x3 = (x1 + (W2 - W6) * x3) >> 3;
	x1 = x4 + x6; x4 -= x6;
	x6 = x5 + x7; x5 -= x7;
	x7 = x8 + x3; x8 -= x3;
	x3 = x0 + x2; x0 -= x2;
	x2 = (181 * (x4 + x5) + 128) >> 8;
	x4 = (181 * (x4 - x5) + 128) >> 8;
	*out = clip(((x7 + x1) >> 14) + 128);  out += stride;
	*out = clip(((x3 + x2) >> 14) + 128);  out += stride;
	*out = clip(((x0 + x4) >> 14) + 128);  out += stride;
	*out = clip(((x8 + x6) >> 14) + 128);  out += stride;
	*out = clip(((x8 - x6) >> 14) + 128);  out += stride;
	*out = clip(((x0 - x4) >> 14) + 128);  out += stride;
	*out = clip(((x3 - x2) >> 14) + 128);  out += stride;
	*out = clip(((x7 - x1) >> 14) + 128);
}

int zJpg::showBits(int bits) {
	if(!bits) return 0;
	while(bufbits < bits) {
		if(size <= 0) { buf = (buf << 8) | 0xFF; bufbits += 8; continue; }
		auto newbyte(*pos++);
		size--; bufbits += 8; buf = (buf << 8) | newbyte;
		if(newbyte == 0xFF) {
			if(size) {
				u8 marker(*pos++);
				size--;
				switch(marker) {
					case 0:    break;
					case 0xD9: size = 0; break;
					default: if((marker & 0xF8) != 0xD0) return 0; else { buf = (buf << 8) | marker; bufbits += 8; }
				}
			} else return 0;
		}
	}
	return (buf >> (bufbits - bits)) & ((1 << bits) - 1);
}

bool zJpg::dri() {
	if(!_length()) return false;
	if(length < 2) return false;
	rstinterval = decode16(pos);
	return skip(length);
}

bool zJpg::sof() {
	int i, ssxmax(0), ssymax(0);
	if(!_length()) return false;
	if(length < 9) return false;
	if(pos[0] != 8) return false;
	height = decode16(pos + 1);
	width = decode16(pos + 3);
	ncomp = pos[5];
	if(!skip(6)) return false;
	if(ncomp != 1 && ncomp != 3) return false;
	if(length < ncomp * 3) return false;
	Component* c;
	for(i = 0, c = comp; i < ncomp; ++i, ++c) {
		c->cid = pos[0];
		if(!(c->ssx = pos[1] >> 4)) return false;
		if(c->ssx & (c->ssx - 1)) return false;
		if(!(c->ssy = pos[1] & 15)) return false;
		if(c->ssy & (c->ssy - 1)) return false;
		if((c->qtsel = pos[2]) & 0xFC) return false;
		if(!skip(3)) return false;
		qtused |= 1 << c->qtsel;
		if(c->ssx > ssxmax) ssxmax = c->ssx;
		if(c->ssy > ssymax) ssymax = c->ssy;
	}
	mbsizex = ssxmax << 3; mbsizey = ssymax << 3;
	mbwidth = (width + mbsizex - 1) / mbsizex;
	mbheight = (height + mbsizey - 1) / mbsizey;
	for(i = 0, c = comp; i < ncomp; ++i, ++c) {
		c->width = (width * c->ssx + ssxmax - 1) / ssxmax;
		c->stride = (c->width + 7) & 0x7FFFFFF8;
		c->height = (height * c->ssy + ssymax - 1) / ssymax;
		c->stride = mbwidth * mbsizex * c->ssx / ssxmax;
		if(((c->width < 3) && (c->ssx != ssxmax)) || ((c->height < 3) && (c->ssy != ssymax))) return false;
		c->pixels = new u8[c->stride * (mbheight * mbsizey * c->ssy / ssymax)];
	}
	if(ncomp == 3) rgb = new u8[width * height * ncomp];
	return skip(length);
}

bool zJpg::dht() {
	int codelen, currcnt, remain, spread, i, j;
	if(!_length()) return false;
	u8 counts[16];
	while(length >= 17) {
		i = pos[0];
		if(i & 0xEC) return false;
		if(i & 0x02) return false;
		i = (i | (i >> 3)) & 3;
		for(codelen = 1; codelen <= 16; ++codelen) counts[codelen - 1] = pos[codelen];
		if(!skip(17)) return false;
		auto vlc(&vlct[i * 65536]);
		remain = spread = 65536;
		for(codelen = 1; codelen <= 16; ++codelen) {
			spread >>= 1;
			currcnt = counts[codelen - 1];
			if(!currcnt) continue;
			if(length < currcnt) return false;
			remain -= currcnt << (16 - codelen);
			if(remain < 0) return false;
			for(i = 0; i < currcnt; ++i) {
				auto code(pos[i]);
				for(j = spread; j; --j) {
					vlc->bits = (u8)codelen;
					vlc->code = code;
					++vlc;
				}
			}
			if(!skip(currcnt)) return false;
		}
		while(remain--) { vlc->bits = 0; ++vlc; }
	}
	return length == 0;
}

bool zJpg::dqt() {
	if(!_length()) return false;
	while(length >= 65) {
		auto i(pos[0]);
		if(i & 0xFC) return false;
		qtavail |= 1 << i;
		auto t(&qtab[i][0]);
		for(i = 0; i < 64; ++i) t[i] = pos[i + 1];
		if(!skip(65)) return false;
	}
	return length == 0;
}

int zJpg::getVLC(VlcCode* vlc, u8* code) {
	int value(showBits(16)), bits(vlc[value].bits);
	if(!bits) return 0;
	skipBits(bits);
	value = vlc[value].code;
	if(code) *code = (u8)value;
	bits = value & 15; if(!bits) return 0;
	value = getBits(bits);
	if(value < (1 << (bits - 1))) value += ((-1) << bits) + 1;
	return value;
}

bool zJpg::decodeBlock(Component* c, u8* out) {
	u8 code;
	int value, coef(0);
	memset(block, 0, sizeof(block));
	c->dcpred += getVLC(&vlct[c->dctabsel * 65536], nullptr);
	block[0] = (c->dcpred) * qtab[c->qtsel][0];
	do {
		value = getVLC(&vlct[c->actabsel * 65536], &code);
		if(!code) break;
		if(!(code & 0x0F) && (code != 0xF0)) return false;
		coef += (code >> 4) + 1;
		if(coef > 63) return false;
		block[(int)ZZ[coef]] = value * qtab[c->qtsel][coef];
	} while(coef < 63);
	for(coef = 0; coef < 64; coef += 8) rowIDCT(&block[coef]);
	for(coef = 0; coef < 8; ++coef) colIDCT(&block[coef], &out[coef], c->stride);
	return true;
}

bool zJpg::decodeScan() {
	int i, mbx, mby, sbx, sby;
	int rstcount(rstinterval), nextrst(0);
	Component* c;
	if(!_length()) return false;
	if(length < (4 + 2 * ncomp)) return false;
	if(pos[0] != ncomp) return false;
	if(!skip(1)) return false;
	for(i = 0, c = comp; i < ncomp; ++i, ++c) {
		if(pos[0] != c->cid) return false;
		if(pos[1] & 0xEE) return false;
		c->dctabsel = pos[1] >> 4;
		c->actabsel = (pos[1] & 1) | 2;
		if(!skip(2)) return false;
	}
	if(pos[0] || (pos[1] != 63) || pos[2]) return false;
	if(!skip(length)) return false;
	for(mby = 0; mby < mbheight; ++mby) {
		for(mbx = 0; mbx < mbwidth; ++mbx) {
			for(i = 0, c = comp; i < ncomp; ++i, ++c) {
				for(sby = 0; sby < c->ssy; ++sby) {
					for(sbx = 0; sbx < c->ssx; ++sbx) {
						if(!decodeBlock(c, &c->pixels[((mby * c->ssy + sby) * c->stride + mbx * c->ssx + sbx) << 3]))
							return false;
					}
				}
			}
			if(rstinterval && !(--rstcount)) {
				byteAlign();
				nextrst = (nextrst + 1) & 7;
				rstcount = rstinterval;
				for(i = 0; i < 3; ++i) comp[i].dcpred = 0;
			}
		}
	}
	return true;
}

void zJpg::upsampleH(Component* c) {
	const int xmax(c->width - 3);
	int x, y;
	auto out((u8*)malloc((c->width * c->height) << 1)), lin(c->pixels), lout(out);
	for(y = c->height; y; --y) {
		lout[0] = CF(CF2A * lin[0] + CF2B * lin[1]);
		lout[1] = CF(CF3X * lin[0] + CF3Y * lin[1] + CF3Z * lin[2]);
		lout[2] = CF(CF3A * lin[0] + CF3B * lin[1] + CF3C * lin[2]);
		for(x = 0; x < xmax; ++x) {
			lout[(x << 1) + 3] = CF(CF4A * lin[x] + CF4B * lin[x + 1] + CF4C * lin[x + 2] + CF4D * lin[x + 3]);
			lout[(x << 1) + 4] = CF(CF4D * lin[x] + CF4C * lin[x + 1] + CF4B * lin[x + 2] + CF4A * lin[x + 3]);
		}
		lin += c->stride;
		lout += c->width << 1;
		lout[-3] = CF(CF3A * lin[-1] + CF3B * lin[-2] + CF3C * lin[-3]);
		lout[-2] = CF(CF3X * lin[-1] + CF3Y * lin[-2] + CF3Z * lin[-3]);
		lout[-1] = CF(CF2A * lin[-1] + CF2B * lin[-2]);
	}
	c->width <<= 1;
	c->stride = c->width;
	free(c->pixels);
	c->pixels = out;
}

void zJpg::upsampleV(Component* c) {
	const int w(c->width), s1(c->stride), s2(s1 + s1);
	int x, y;
	auto out((u8*)malloc((c->width * c->height) << 1));
	for(x = 0; x < w; ++x) {
		auto cin(&c->pixels[x]), cout(&out[x]);
		*cout = CF(CF2A * cin[0] + CF2B * cin[s1]);  cout += w;
		*cout = CF(CF3X * cin[0] + CF3Y * cin[s1] + CF3Z * cin[s2]);  cout += w;
		*cout = CF(CF3A * cin[0] + CF3B * cin[s1] + CF3C * cin[s2]);  cout += w;
		cin += s1;
		for(y = c->height - 3; y; --y) {
			*cout = CF(CF4A * cin[-s1] + CF4B * cin[0] + CF4C * cin[s1] + CF4D * cin[s2]);  cout += w;
			*cout = CF(CF4D * cin[-s1] + CF4C * cin[0] + CF4B * cin[s1] + CF4A * cin[s2]);  cout += w;
			cin += s1;
		}
		cin += s1;
		*cout = CF(CF3A * cin[0] + CF3B * cin[-s1] + CF3C * cin[-s2]);  cout += w;
		*cout = CF(CF3X * cin[0] + CF3Y * cin[-s1] + CF3Z * cin[-s2]);  cout += w;
		*cout = CF(CF2A * cin[0] + CF2B * cin[-s1]);
	}
	c->height <<= 1;
	c->stride = c->width;
	free(c->pixels);
	c->pixels = out;
}

void zJpg::convert() {
	int i;
	Component* c;
	for(i = 0, c = comp; i < ncomp; ++i, ++c) {
		while((c->width < width) || (c->height < height)) {
			if(c->width < width) upsampleH(c);
			if(c->height < height) upsampleV(c);
		}
		if((c->width < width) || (c->height < height)) return;// TODO
	}
	if(ncomp == 3) {
		// convert to RGB
		int x, yy;
		auto prgb(rgb), py(comp[0].pixels), pcb(comp[1].pixels), pcr(comp[2].pixels);
		for(yy = height; yy; --yy) {
			for(x = 0; x < width; ++x) {
				int y(py[x] << 8), cb(pcb[x] - 128), cr(pcr[x] - 128);
				*prgb++ = clip((y + 359 * cr + 128) >> 8);
				*prgb++ = clip((y - 88 * cb - 183 * cr + 128) >> 8);
				*prgb++ = clip((y + 454 * cb + 128) >> 8);
			}
			py += comp[0].stride; pcb += comp[1].stride; pcr += comp[2].stride;
		}
	} else if(comp[0].width != comp[0].stride) {
		// grayscale -> only remove stride
		auto pin(&comp[0].pixels[comp[0].stride]), pout(&comp[0].pixels[comp[0].width]);
		int y;
		for(y = comp[0].height - 1; y; --y) {
			memcpy(pout, pin, comp[0].width);
			pin += comp[0].stride;
			pout += comp[0].width;
		}
		comp[0].stride = comp[0].width;
	}
}
