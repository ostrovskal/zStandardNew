
#pragma once

#define ZFAST_BITS  9
#define ZFAST_MASK  ((1 << ZFAST_BITS) - 1)

class zPng {
public:
	enum { ORDER_RGB, ORDER_BGR };
	enum { SCAN_load = 0, SCAN_type, SCAN_header };
	zPng(const u8* ptr, int len, int* width, int* height, int* ncomp, int rcomp);
protected:
	struct result_info {
		int bits_per_channel;
		int num_channels;
		int channel_order;
	};
	struct chunk {
		u32 length;
		u32 type;
	};
	u8 get8() { return (img_buffer < img_buffer_end ? *img_buffer++ : 0); }
	int get16be() { int z(get8()); return (z << 8) + get8(); }
	u32 get32be() { u32 z(get16be()); return (z << 16) + get16be(); }
	void skip(int n) { if(n < 0) { img_buffer = img_buffer_end; return; } img_buffer += n; }
	void rewind() { img_buffer = img_buffer_original; img_buffer_end = img_buffer_original_end; }
	u16* convert_format16(u16* data, int img_n, int req_comp, u32 x, u32 y);
	u8* convert_format(u8* data, int img_n, int req_comp, u32 x, u32 y);
	int expand_png_palette(u8* palette, int len, int pal_img_n);
	int compute_transparency16(u16 tc[3], int out_n);
	int compute_transparency(u8 tc[3], int out_n);
	int create_png_image_raw(u8* raw, u32 raw_len, int out_n, u32 x, u32 y, int depth, int color);
	int create_png_image(u8* image_data, u32 image_data_len, int out_n, int depth, int color, int interlaced);
	int getn(u8* buffer, int n);
	chunk get_chunk_header();
	int parse_png_file(int scan, int rcomp);
	void* do_png(int* x, int* y, int* n, int rcomp, result_info* ri);
	int check_png_header();
	// context
	int depth, buflen, img_n, img_out_n;
	u32 img_x, img_y;
	u8 buffer_start[128];
	u8* img_buffer, *img_buffer_end;
	u8* img_buffer_original, *img_buffer_original_end;
	u8* idata, *expanded, *out;
	u8* result;
	// zlib
	struct zhuffman {
		u16 fast[1 << ZFAST_BITS];
		u16 firstcode[16];
		int maxcode[17];
		u16 firstsymbol[16];
		u8  size[288];
		u16 value[288];
	};
	struct zbuf {
		u8* zbuffer, *zbuffer_end;
		int num_bits;
		u32 code_buffer;
		char* zout;
		char* zout_start;
		char* zout_end;
		int   z_expandable;
		zhuffman z_length, z_distance;
	} stbi__zbuf;
	int parse_zlib_header(zbuf* a);
	char* zlib_decode_malloc_guesssize_headerflag(cstr buffer, int len, int initial_size, int* outlen, int parse_header);
	u8 zget8(zbuf* z) { return (z->zbuffer < z->zbuffer_end ? *z->zbuffer++ : 0); }
	void fill_bits(zbuf* z);
	int parse_zlib(zbuf* a, int parse_header);
	u32 zreceive(zbuf* z, int n);
	int do_zlib(zbuf* a, char* obuf, int olen, int exp, int parse_header);
	int parse_huffman_block(zbuf* a);
	int zexpand(zbuf* z, char* zout, int n);
	int zhuffman_decode(zbuf* a, zhuffman* z);
	int zhuffman_decode_slowpath(zbuf* a, zhuffman* z);
	int parse_uncompressed_block(zbuf* a);
	int zbuild_huffman(zhuffman* z, u8* sizelist, int num);
	int compute_huffman_codes(zbuf* a);
	void init_zdefaults(void);
};

