
#pragma once

class zJpg {
public:
	zJpg(const u8* ptr, size_t size) { decode(ptr, size); }
	zJpg(cstr path);
	~zJpg() { cleanup(); }
	// ширина
	int getWidth() const { return width; }
	// высота
	int getHeight() const { return height; }
	// кол-во компонент
	int getComponent() const { return ncomp; }
	// признак цветности RGB или Ч/Б
	bool isColor() const { return ncomp != 1; }
	// признак успеха декомпрессии
	bool isValid() const { return getImage() != nullptr; }
	// буфер изображения
	u8* getImage() const { return (ncomp == 1) ? comp[0].pixels : rgb; }
	// размер буфера
	int sizeImage() const { return width * height * ncomp; }
private:
	enum { W1 = 2841, W2 = 2676, W3 = 2408, W5 = 1609, W6 = 1108, W7 = 565, };
	enum {
		CF4A = (-9), CF4B = (111), CF4C = (29), CF4D = (-3), CF3A = (28),
		CF3B = (109), CF3C = (-9), CF3X = (104), CF3Y = (27), CF3Z = (-3),
		CF2A = (139), CF2B = (-11),
	};
	struct VlcCode { u8 bits, code; };
	struct Component {
		int cid;
		int ssx, ssy;
		int width, height;
		int stride;
		int qtsel;
		int actabsel, dctabsel;
		int dcpred;
		u8* pixels;
	};
	const u8* pos{nullptr};
	int size{0};
	int length{0};
	int width{0}, height{0};
	int mbwidth{0}, mbheight{0};
	int mbsizex{0}, mbsizey{0};
	int ncomp{0};
	Component comp[3]{};
	int qtused{0}, qtavail{0};
	u8 qtab[4][64]{};
	VlcCode* vlct{nullptr};
	int buf{0}, bufbits{0};
	int block[64]{};
	int rstinterval{0};
	u8* rgb{nullptr};
	void rowIDCT(int* blk);
	void colIDCT(const int* blk, u8* out, int stride);
	void cleanup();
	void upsampleH(Component* c);
	void upsampleV(Component* c);
	void convert();
	void skipBits(int bits) { (bufbits < bits) ? showBits(bits) : bufbits -= bits; }
	void byteAlign(void) { bufbits &= 0xF8; }
	bool decode(const u8* ptr, size_t sz);
	bool decodeBlock(Component* c, u8* out);
	bool dqt();
	bool dht();
	bool sof();
	bool dri();
	bool decodeScan();
	bool skip(int count) { pos += count; size -= count; length -= count; return (size >= 0); }
	bool _length();
	bool skipMarker() { if(!_length()) return false; return skip(length); }
	int getVLC(VlcCode* vlc, u8* code);
	int showBits(int bits);
	int getBits(int bits) { int res(showBits(bits)); skipBits(bits); return res; }
	u16 decode16(const u8* _pos) { return (_pos[0] << 8) | _pos[1]; }
	u8 clip(const int x) { return (x < 0) ? 0 : ((x > 0xFF) ? 0xFF : (u8)x); }
	u8 CF(const int x) { return clip((x + 64) >> 7); }
};
