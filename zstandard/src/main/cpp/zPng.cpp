
#include "zstandard/zstandard.h"
#include "zstandard/zPng.h"

#define BYTECAST(x)             ((u8) ((x) & 255))
#define PNG_TYPE(a, b, c, d)    (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))

static u8 zdefault_length[288], zdefault_distance[32];
static int zlength_base[31]		= { 3,4,5,6,7,8,9,10,11,13, 15,17,19,23,27,31,35,43,51,59, 67,83,99,115,131,163,195,227,258,0,0 };
static int zlength_extra[31]	= { 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };
static int zdist_base[32]		= { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193, 257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0 };
static int zdist_extra[32]		= { 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13 };

enum { png_none = 0, png_sub, png_up, png_avg, png_paeth, png_avg_first, png_paeth_first };

static u8 depth_scale_table[9] = { 0, 0xff, 0x55, 0, 0x11, 0,0,0, 0x01 };
static u8 first_row_filter[5] = { png_none, png_sub, png_none, png_avg_first, png_paeth_first };

static char* realoc_sized(char* ptr, i32 _old, i32 _new) {
	auto n(new char[_new]);
	memcpy(n, ptr, _old);
	return n;
}

static int bitreverse16(int n) {
	n = ((n & 0xAAAA) >> 1) | ((n & 0x5555) << 1);
	n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2);
	n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
	n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
	return n;
}

static int bit_reverse(int v, int bits) {
	return bitreverse16(v) >> (16 - bits);
}

u32 zPng::zreceive(zbuf* z, int n) {
	if(z->num_bits < n) fill_bits(z);
	u32 k(z->code_buffer & ((1 << n) - 1));
	z->code_buffer >>= n;
	z->num_bits -= n;
	return k;
}

void zPng::fill_bits(zbuf* z) {
	do {
		z->code_buffer |= (u32)zget8(z) << z->num_bits;
		z->num_bits += 8;
	} while(z->num_bits <= 24);
}

static u8 compute_y(int r, int g, int b) {
	return (u8)(((r * 77) + (g * 150) + (29 * b)) >> 8);
}

static u16 compute_y_16(int r, int g, int b) {
	return (u16)(((r * 77) + (g * 150) + (29 * b)) >> 8);
}

static bool addsizes_valid(int a, int b) {
	return (b < 0 ? 0 : (a <= INT_MAX - b));
}

static bool mul2sizes_valid(int a, int b) {
	if(a < 0 || b < 0) return 0;
	if(b == 0) return 1;
	return a <= INT_MAX / b;
}

static bool mad2sizes_valid(int a, int b, int add) {
	return mul2sizes_valid(a, b) && addsizes_valid(a * b, add);
}

static bool mad3sizes_valid(int a, int b, int c, int add) {
	return mul2sizes_valid(a, b) && mul2sizes_valid(a * b, c) && addsizes_valid(a * b * c, add);
}

static void* malloc_mad2(int a, int b, int add) {
	if(!mad2sizes_valid(a, b, add)) return nullptr;
	return new u8[a * b + add];
}

static void* malloc_mad3(int a, int b, int c, int add) {
	if(!mad3sizes_valid(a, b, c, add)) return nullptr;
	return new u8[a * b * c + add];
}

static int paeth(int a, int b, int c) {
	int p(a + b - c), pa(abs(p - a)), pb(abs(p - b)), pc(abs(p - c));
	if(pa <= pb && pa <= pc) return a;
	if(pb <= pc) return b;
	return c;
}

int zPng::getn(u8* buffer, int n) {
	if(img_buffer + n <= img_buffer_end) {
		memcpy(buffer, img_buffer, n);
		img_buffer += n;
		return 1;
	}
	return 0;
}

zPng::chunk zPng::get_chunk_header() {
	chunk c;
	c.length = get32be();
	c.type = get32be();
	return c;
}

static u8* convert_16_to_8(u16* orig, int w, int h, int channels) {
	int img_len(w * h * channels);
	auto reduced(new u8[img_len]);
	if(!reduced) return 0;
	for(int i = 0; i < img_len; ++i) reduced[i] = (u8)((orig[i] >> 8) & 0xFF);
	SAFE_A_DELETE(orig);
	return reduced;
}

zPng::zPng(const u8* ptr, int len, int* width, int* height, int* ncomp, int rcomp) {
	result = nullptr;
	img_buffer = img_buffer_original = (u8*)ptr;
	img_buffer_end = img_buffer_original_end = (u8*)ptr + len;
	result_info ri; memset(&ri, 0, sizeof(result_info));
	ri.bits_per_channel = 8; ri.channel_order = 0; ri.num_channels = 0;
	auto r(check_png_header()); rewind();
	auto _result(r ? do_png(width, height, ncomp, rcomp, &ri) : nullptr);
	if(!_result) return;
	if(ri.bits_per_channel != 8 && ri.bits_per_channel != 16) return;
	if(ri.bits_per_channel != 8) {
		_result = convert_16_to_8((u16*)_result, *width, *height, rcomp == 0 ? *ncomp : rcomp);
		ri.bits_per_channel = 8;
	}
	result = (u8*)_result;
}

int zPng::check_png_header() {
	static u8 png_sig[8] = { 137,80,78,71,13,10,26,10 };
	for(int i = 0; i < 8; ++i) {
		if(get8() != png_sig[i]) return 0;
	}
	return 1;
}

void* zPng::do_png(int* x, int* y, int* n, int rcomp, result_info* ri) {
	void* res(nullptr);
	if(rcomp < 0 || rcomp > 4) return 0;
	if(parse_png_file(SCAN_load, rcomp)) {
		ri->bits_per_channel = depth;
		res = out; out = nullptr;
		if(rcomp && rcomp != img_out_n) {
			if(ri->bits_per_channel == 8)
				res = convert_format((u8*)res, img_out_n, rcomp, img_x, img_y);
			else
				res = convert_format16((u16*)res, img_out_n, rcomp, img_x, img_y);
			img_out_n = rcomp;
			if(!res) return res;
		}
		*x = img_x;
		*y = img_y;
		if(n) *n = img_n;
	}
	SAFE_A_DELETE(out);
	SAFE_A_DELETE(expanded);
	SAFE_A_DELETE(idata);
	return res;
}

int zPng::parse_png_file(int scan, int rcomp) {
	u8 palette[1024], pal_img_n(0), has_trans(0), tc[3];
	u16 tc16[3];
	u32 ioff(0), idata_limit(0), i, pal_len(0);
	int first(1), k, interlace(0), color(0), is_iphone(0);
	expanded = nullptr; idata = nullptr; out = nullptr;
	if(!check_png_header()) return 0;
	if(scan == SCAN_type) return 1;
	while(true) {
		chunk c(get_chunk_header());
		switch(c.type) {
			case PNG_TYPE('C', 'g', 'B', 'I'):
				is_iphone = 1;
				skip(c.length);
				break;
			case PNG_TYPE('I', 'H', 'D', 'R'): {
				int comp, filter;
				if(!first) return 0;
				first = 0;
				if(c.length != 13) return 0;
				img_x = get32be(); if(img_x > (1 << 24)) return 0;
				img_y = get32be(); if(img_y > (1 << 24)) return 0;
				depth = get8(); if(depth != 1 && depth != 2 && depth != 4 && depth != 8 && depth != 16)  return 0;
				color = get8(); if(color > 6) return 0;
				if(color == 3 && depth == 16) return 0;
				if(color == 3) pal_img_n = 3; else if(color & 1) return 0;
				comp = get8();  if(comp) return 0;
				filter = get8();  if(filter) return 0;
				interlace = get8(); if(interlace > 1) return 0;
				if(!img_x || !img_y) return 0;
				if(!pal_img_n) {
					img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
					if((1 << 30) / img_x / img_n < img_y) return 0;
					if(scan == SCAN_header) return 1;
				} else {
					img_n = 1;
					if((1 << 30) / img_x / 4 < img_y) return 0;
				}
				break;
			}
			case PNG_TYPE('P', 'L', 'T', 'E'): {
				if(first) return 0;
				if(c.length > 256 * 3) return 0;
				pal_len = c.length / 3;
				if(pal_len * 3 != c.length) return 0;
				for(i = 0; i < pal_len; ++i) {
					palette[i * 4 + 0] = get8();
					palette[i * 4 + 1] = get8();
					palette[i * 4 + 2] = get8();
					palette[i * 4 + 3] = 255;
				}
				break;
			}
			case PNG_TYPE('t', 'R', 'N', 'S'): {
				if(first) return 0;
				if(idata) return 0;
				if(pal_img_n) {
					if(scan == SCAN_header) { img_n = 4; return 1; }
					if(pal_len == 0) return 0;
					if(c.length > pal_len) return 0;
					pal_img_n = 4;
					for(i = 0; i < c.length; ++i) palette[i * 4 + 3] = get8();
				} else {
					if(!(img_n & 1)) return 0;
					if(c.length != (u32)img_n * 2) return 0;
					has_trans = 1;
					if(depth == 16) {
						for(k = 0; k < img_n; ++k) tc16[k] = (u16)get16be();
					} else {
						for(k = 0; k < img_n; ++k) tc[k] = (u8)(get16be() & 255) * depth_scale_table[depth];
					}
				}
				break;
			}
			case PNG_TYPE('I', 'D', 'A', 'T'): {
				if(first) return 0;
				if(pal_img_n && !pal_len) return 0;
				if(scan == SCAN_header) { img_n = pal_img_n; return 1; }
				if((int)(ioff + c.length) < (int)ioff) return 0;
				if(ioff + c.length > idata_limit) {
					u32 idata_limit_old = idata_limit;
					u8* p;
					if(idata_limit == 0) idata_limit = c.length > 4096 ? c.length : 4096;
					while(ioff + c.length > idata_limit) idata_limit *= 2;
					//STBI_NOTUSED(idata_limit_old);
					p = (u8*)realoc_sized((char*)idata, idata_limit_old, idata_limit);
					if(!p) return 0;
					idata = p;
				}
				if(!getn(idata + ioff, c.length)) return 0;
				ioff += c.length;
				break;
			}
			case PNG_TYPE('I', 'E', 'N', 'D'): {
				u32 raw_len, bpl;
				if(first) return 0;
				if(scan != SCAN_load) return 1;
				if(!idata) return 0;
				bpl = (img_x * depth + 7) / 8;
				raw_len = bpl * img_y * img_n + img_y;
				expanded = (u8*)zlib_decode_malloc_guesssize_headerflag((char*)idata, ioff, raw_len, (int*)&raw_len, !is_iphone);
				if(!expanded) return 0;
				SAFE_A_DELETE(idata);
				img_out_n = (((rcomp == img_n + 1 && rcomp != 3 && !pal_img_n) || has_trans) ? img_n + 1 : img_n);
				if(!create_png_image(expanded, raw_len, img_out_n, depth, color, interlace)) return 0;
				if(has_trans) {
					if(depth == 16) {
						if(!compute_transparency16(tc16, img_out_n)) return 0;
					} else {
						if(!compute_transparency(tc, img_out_n)) return 0;
					}
				}
				if(pal_img_n) {
					img_n = pal_img_n; img_out_n = pal_img_n;
					if(rcomp >= 3) img_out_n = rcomp;
					if(!expand_png_palette(palette, pal_len, img_out_n))
						return 0;
				}
				SAFE_A_DELETE(expanded);
				return 1;
			}
			default:
				if(first) return 0;
				if((c.type & (1 << 29)) == 0) return 0;
				skip(c.length);
				break;
		}
		get32be();
	}
}

int zPng::create_png_image(u8* image_data, u32 image_data_len, int out_n, int dpth, int color, int interlaced) {
	int bytes(dpth == 16 ? 2 : 1), out_bytes(out_n * bytes), p;
	u8* final;
	if(!interlaced) return create_png_image_raw(image_data, image_data_len, out_n, img_x, img_y, dpth, color);
	// de-interlacing
	final = (u8*)malloc_mad3(img_x, img_y, out_bytes, 0);
	for(p = 0; p < 7; ++p) {
		static int xorig[] = { 0,4,0,2,0,1,0 };
		static int yorig[] = { 0,0,4,0,2,0,1 };
		static int xspc[] = { 8,8,4,4,2,2,1 };
		static int yspc[] = { 8,8,8,4,4,2,2 };
		int i, j, x, y;
		x = (img_x - xorig[p] + xspc[p] - 1) / xspc[p];
		y = (img_y - yorig[p] + yspc[p] - 1) / yspc[p];
		if(x && y) {
			u32 img_len = ((((img_n * x * dpth) + 7) >> 3) + 1) * y;
			if(!create_png_image_raw(image_data, image_data_len, out_n, x, y, dpth, color)) {
				SAFE_A_DELETE(final);
				return 0;
			}
			for(j = 0; j < y; ++j) {
				for(i = 0; i < x; ++i) {
					int out_y = j * yspc[p] + yorig[p];
					int out_x = i * xspc[p] + xorig[p];
					memcpy(final + out_y * img_x * out_bytes + out_x * out_bytes, out + (j * x + i) * out_bytes, out_bytes);
				}
			}
			SAFE_A_DELETE(out);
			image_data += img_len;
			image_data_len -= img_len;
		}
	}
	out = final;
	return 1;
}

int zPng::create_png_image_raw(u8* raw, u32 raw_len, int out_n, u32 x, u32 y, int dpth, int color) {
	int bytes(dpth == 16 ? 2 : 1);
	u32 i, j, stride(x * out_n * bytes), img_len, img_width_bytes;
	int k, _img_n(img_n), output_bytes(out_n * bytes), filter_bytes(_img_n * bytes), width(x);
	out = (u8*)malloc_mad3(x, y, output_bytes, 0);
	if(out) return 0;
	img_width_bytes = (((img_n * x * dpth) + 7) >> 3);
	img_len = (img_width_bytes + 1) * y;
	if(img_x == x && img_y == y) {
		if(raw_len != img_len) return 0;
	} else {
		if(raw_len < img_len) return 0;
	}
	for(j = 0; j < y; ++j) {
		u8* cur(out + stride * j);
		u8* prior(cur - stride);
		int filter = *raw++;
		if(filter > 4) return 0;
		if(dpth < 8) {
			cur += x * out_n - img_width_bytes;
			filter_bytes = 1;
			width = img_width_bytes;
		}
		if(j == 0) filter = first_row_filter[filter];
		for(k = 0; k < filter_bytes; ++k) {
			switch(filter) {
				case png_none:      cur[k] = raw[k]; break;
				case png_sub:       cur[k] = raw[k]; break;
				case png_up:        cur[k] = BYTECAST(raw[k] + prior[k]); break;
				case png_avg:       cur[k] = BYTECAST(raw[k] + (prior[k] >> 1)); break;
				case png_paeth:     cur[k] = BYTECAST(raw[k] + paeth(0, prior[k], 0)); break;
				case png_avg_first: cur[k] = raw[k]; break;
				case png_paeth_first: cur[k] = raw[k]; break;
			}
		}

		if(dpth == 8) {
			if(img_n != out_n) cur[img_n] = 255;
			raw += img_n; cur += out_n; prior += out_n;
		} else if(dpth == 16) {
			if(img_n != out_n) {
				cur[filter_bytes] = 255;
				cur[filter_bytes + 1] = 255;
			}
			raw += filter_bytes;
			cur += output_bytes;
			prior += output_bytes;
		} else {
			raw += 1; cur += 1; prior += 1;
		}
		if(dpth < 8 || img_n == out_n) {
			int nk = (width - 1) * filter_bytes;
#define CASE(f) case f: for (k = 0; k < nk; ++k)
			switch(filter) {
				case png_none:      memcpy(cur, raw, nk); break;
					CASE(png_sub) { cur[k] = BYTECAST(raw[k] + cur[k - filter_bytes]); } break;
					CASE(png_up) { cur[k] = BYTECAST(raw[k] + prior[k]); } break;
					CASE(png_avg) { cur[k] = BYTECAST(raw[k] + ((prior[k] + cur[k - filter_bytes]) >> 1)); } break;
					CASE(png_paeth) { cur[k] = BYTECAST(raw[k] + paeth(cur[k - filter_bytes], prior[k], prior[k - filter_bytes])); } break;
					CASE(png_avg_first) { cur[k] = BYTECAST(raw[k] + (cur[k - filter_bytes] >> 1)); } break;
					CASE(png_paeth_first) { cur[k] = BYTECAST(raw[k] + paeth(cur[k - filter_bytes], 0, 0)); } break;
			}
#undef CASE
			raw += nk;
		} else {
#define CASE(f) case f: for (i = x - 1; i >= 1; --i, cur[filter_bytes] = 255, raw += filter_bytes,cur += output_bytes,prior += output_bytes) for (k = 0; k < filter_bytes; ++k)
			switch(filter) {
				CASE(png_none) { cur[k] = raw[k]; } break;
				CASE(png_sub) { cur[k] = BYTECAST(raw[k] + cur[k - output_bytes]); } break;
				CASE(png_up) { cur[k] = BYTECAST(raw[k] + prior[k]); } break;
				CASE(png_avg) { cur[k] = BYTECAST(raw[k] + ((prior[k] + cur[k - output_bytes]) >> 1)); } break;
				CASE(png_paeth) { cur[k] = BYTECAST(raw[k] + paeth(cur[k - output_bytes], prior[k], prior[k - output_bytes])); } break;
				CASE(png_avg_first) { cur[k] = BYTECAST(raw[k] + (cur[k - output_bytes] >> 1)); } break;
				CASE(png_paeth_first) { cur[k] = BYTECAST(raw[k] + paeth(cur[k - output_bytes], 0, 0)); } break;
			}
#undef CASE
			if(dpth == 16) {
				cur = out + stride * j;
				for(i = 0; i < x; ++i, cur += output_bytes) cur[filter_bytes + 1] = 255;
			}
		}
	}
	if(dpth < 8) {
		for(j = 0; j < y; ++j) {
			u8* cur(out + stride * j);
			u8* in(out + stride * j + x * out_n - img_width_bytes);
			u8 scale(color == 0 ? depth_scale_table[dpth] : 1);
			if(dpth == 4) {
				for(k = x * img_n; k >= 2; k -= 2, ++in) {
					*cur++ = scale * ((*in >> 4));
					*cur++ = scale * ((*in) & 0x0f);
				}
				if(k > 0) *cur++ = scale * ((*in >> 4));
			} else if(dpth == 2) {
				for(k = x * img_n; k >= 4; k -= 4, ++in) {
					*cur++ = scale * ((*in >> 6));
					*cur++ = scale * ((*in >> 4) & 0x03);
					*cur++ = scale * ((*in >> 2) & 0x03);
					*cur++ = scale * ((*in) & 0x03);
				}
				if(k > 0) *cur++ = scale * ((*in >> 6));
				if(k > 1) *cur++ = scale * ((*in >> 4) & 0x03);
				if(k > 2) *cur++ = scale * ((*in >> 2) & 0x03);
			} else if(dpth == 1) {
				for(k = x * img_n; k >= 8; k -= 8, ++in) {
					*cur++ = scale * ((*in >> 7));
					*cur++ = scale * ((*in >> 6) & 0x01);
					*cur++ = scale * ((*in >> 5) & 0x01);
					*cur++ = scale * ((*in >> 4) & 0x01);
					*cur++ = scale * ((*in >> 3) & 0x01);
					*cur++ = scale * ((*in >> 2) & 0x01);
					*cur++ = scale * ((*in >> 1) & 0x01);
					*cur++ = scale * ((*in) & 0x01);
				}
				if(k > 0) *cur++ = scale * ((*in >> 7));
				if(k > 1) *cur++ = scale * ((*in >> 6) & 0x01);
				if(k > 2) *cur++ = scale * ((*in >> 5) & 0x01);
				if(k > 3) *cur++ = scale * ((*in >> 4) & 0x01);
				if(k > 4) *cur++ = scale * ((*in >> 3) & 0x01);
				if(k > 5) *cur++ = scale * ((*in >> 2) & 0x01);
				if(k > 6) *cur++ = scale * ((*in >> 1) & 0x01);
			}
			if(img_n != out_n) {
				int q;
				cur = out + stride * j;
				if(img_n == 1) {
					for(q = x - 1; q >= 0; --q) {
						cur[q * 2 + 1] = 255;
						cur[q * 2 + 0] = cur[q];
					}
				} else {
					for(q = x - 1; q >= 0; --q) {
						cur[q * 4 + 3] = 255;
						cur[q * 4 + 2] = cur[q * 3 + 2];
						cur[q * 4 + 1] = cur[q * 3 + 1];
						cur[q * 4 + 0] = cur[q * 3 + 0];
					}
				}
			}
		}
	} else if(dpth == 16) {
		auto cur(out); auto cur16((u16*)cur);
		for(i = 0; i < x * y * out_n; ++i, cur16++, cur += 2) *cur16 = (cur[0] << 8) | cur[1];
	}
	return 1;
}

int zPng::compute_transparency(u8 tc[3], int out_n) {
	u32 i, pixel_count(img_x * img_y);
	u8* p(out);
	if(out_n == 2) {
		for(i = 0; i < pixel_count; ++i) {
			p[1] = (p[0] == tc[0] ? 0 : 255);
			p += 2;
		}
	} else {
		for(i = 0; i < pixel_count; ++i) {
			if(p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
				p[3] = 0;
			p += 4;
		}
	}
	return 1;
}

int zPng::compute_transparency16(u16 tc[3], int out_n) {
	u32 i, pixel_count(img_x * img_y);
	u16* p((u16*)out);
	if(out_n == 2) {
		for(i = 0; i < pixel_count; ++i) {
			p[1] = (p[0] == tc[0] ? 0 : 65535);
			p += 2;
		}
	} else {
		for(i = 0; i < pixel_count; ++i) {
			if(p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
				p[3] = 0;
			p += 4;
		}
	}
	return 1;
}

int zPng::expand_png_palette(u8* palette, int/* len*/, int pal_img_n) {
	u32 i, pixel_count(img_x * img_y);
	u8* temp_out, * orig(out);

	auto p((u8*)malloc_mad2(pixel_count, pal_img_n, 0));
	if(!p) return 0;
	temp_out = p;
	if(pal_img_n == 3) {
		for(i = 0; i < pixel_count; ++i) {
			int n = orig[i] * 4;
			p[0] = palette[n];
			p[1] = palette[n + 1];
			p[2] = palette[n + 2];
			p += 3;
		}
	} else {
		for(i = 0; i < pixel_count; ++i) {
			int n = orig[i] * 4;
			p[0] = palette[n];
			p[1] = palette[n + 1];
			p[2] = palette[n + 2];
			p[3] = palette[n + 3];
			p += 4;
		}
	}
	SAFE_A_DELETE(out);
	out = temp_out;
	return 1;
}

u8* zPng::convert_format(u8* data, int imgn, int req_comp, u32 x, u32 y) {
	int i, j;
	if(req_comp == imgn) return data;
	auto good((u8*)malloc_mad3(req_comp, x, y, 0));
	if(!good) { SAFE_A_DELETE(data); return nullptr; }
	for(j = 0; j < (int)y; ++j) {
		u8* src(data + j * x * imgn);
		u8* dest(good + j * x * req_comp);
#define COMBO(a,b)  ((a) * 8 + (b))
#define CASE(a,b)   case COMBO(a, b): for(i = x - 1; i >= 0; --i, src += a, dest += b)
		switch(COMBO(imgn, req_comp)) {
			CASE(1, 2) { dest[0] = src[0], dest[1] = 255; } break;
			CASE(1, 3) { dest[0] = dest[1] = dest[2] = src[0]; } break;
			CASE(1, 4) { dest[0] = dest[1] = dest[2] = src[0], dest[3] = 255; } break;
			CASE(2, 1) { dest[0] = src[0]; } break;
			CASE(2, 3) { dest[0] = dest[1] = dest[2] = src[0]; } break;
			CASE(2, 4) { dest[0] = dest[1] = dest[2] = src[0], dest[3] = src[1]; } break;
			CASE(3, 4) { dest[0] = src[0], dest[1] = src[1], dest[2] = src[2], dest[3] = 255; } break;
			CASE(3, 1) { dest[0] = compute_y(src[0], src[1], src[2]); } break;
			CASE(3, 2) { dest[0] = compute_y(src[0], src[1], src[2]), dest[1] = 255; } break;
			CASE(4, 1) { dest[0] = compute_y(src[0], src[1], src[2]); } break;
			CASE(4, 2) { dest[0] = compute_y(src[0], src[1], src[2]), dest[1] = src[3]; } break;
			CASE(4, 3) { dest[0] = src[0], dest[1] = src[1], dest[2] = src[2]; } break;
			default: return nullptr;
		}
#undef CASE
#undef COMBO
	}
	SAFE_A_DELETE(data);
	return good;
}

u16* zPng::convert_format16(u16* data, int imgn, int req_comp, u32 x, u32 y) {
	int i, j;
	if(req_comp == imgn) return data;
	auto good((u16*)new u8[req_comp * x * y * 2]);
	if(!good) { SAFE_A_DELETE(data); return nullptr; }
	for(j = 0; j < (int)y; ++j) {
		u16* src(data + j * x * imgn);
		u16* dest(good + j * x * req_comp);
#define COMBO(a, b)  ((a) * 8 + (b))
#define CASE(a, b)   case COMBO(a, b): for(i = x - 1; i >= 0; --i, src += a, dest += b)
		switch(COMBO(imgn, req_comp)) {
			CASE(1, 2) { dest[0] = src[0], dest[1] = 0xffff; } break;
			CASE(1, 3) { dest[0] = dest[1] = dest[2] = src[0]; } break;
			CASE(1, 4) { dest[0] = dest[1] = dest[2] = src[0], dest[3] = 0xffff; } break;
			CASE(2, 1) { dest[0] = src[0]; } break;
			CASE(2, 3) { dest[0] = dest[1] = dest[2] = src[0]; } break;
			CASE(2, 4) { dest[0] = dest[1] = dest[2] = src[0], dest[3] = src[1]; } break;
			CASE(3, 4) { dest[0] = src[0], dest[1] = src[1], dest[2] = src[2], dest[3] = 0xffff; } break;
			CASE(3, 1) { dest[0] = compute_y_16(src[0], src[1], src[2]); } break;
			CASE(3, 2) { dest[0] = compute_y_16(src[0], src[1], src[2]), dest[1] = 0xffff; } break;
			CASE(4, 1) { dest[0] = compute_y_16(src[0], src[1], src[2]); } break;
			CASE(4, 2) { dest[0] = compute_y_16(src[0], src[1], src[2]), dest[1] = src[3]; } break;
			CASE(4, 3) { dest[0] = src[0], dest[1] = src[1], dest[2] = src[2]; } break;
			default: return nullptr;
		}
#undef CASE
#undef COMBO
	}
	SAFE_A_DELETE(data);
	return good;
}

char* zPng::zlib_decode_malloc_guesssize_headerflag(cstr buffer, int len, int initial_size, int* outlen, int parse_header) {
	zbuf a;
	auto p(new char[initial_size]);
	if(!p) return nullptr;
	a.zbuffer = (u8*)buffer;
	a.zbuffer_end = (u8*)buffer + len;
	if(do_zlib(&a, p, initial_size, 1, parse_header)) {
		if(outlen) *outlen = (int)(a.zout - a.zout_start);
		return a.zout_start;
	}
	SAFE_A_DELETE(a.zout_start);
	return nullptr;
}

int zPng::parse_zlib_header(zbuf* a) {
	int cmf(zget8(a)), cm(cmf & 15), flg(zget8(a));
	if((cmf * 256 + flg) % 31 != 0) return 0;
	if(flg & 32) return 0;
	if(cm != 8) return 0;
	return 1;
}

void zPng::init_zdefaults() {
	int i;
	for(i = 0; i <= 143; ++i)zdefault_length[i] = 8;
	for(; i <= 255; ++i)     zdefault_length[i] = 9;
	for(; i <= 279; ++i)     zdefault_length[i] = 7;
	for(; i <= 287; ++i)     zdefault_length[i] = 8;
	for(i = 0; i <= 31; ++i) zdefault_distance[i] = 5;
}

int zPng::parse_zlib(zbuf* a, int parse_header) {
	int final, type;
	if(parse_header && !parse_zlib_header(a)) return 0;
	a->num_bits = 0;
	a->code_buffer = 0;
	do {
		final = zreceive(a, 1);
		type = zreceive(a, 2);
		if(type == 0) {
			if(!parse_uncompressed_block(a)) return 0;
		} else if(type == 3) {
			return 0;
		} else {
			if(type == 1) {
				if(!zdefault_distance[31]) init_zdefaults();
				if(!zbuild_huffman(&a->z_length, zdefault_length, 288)) return 0;
				if(!zbuild_huffman(&a->z_distance, zdefault_distance, 32)) return 0;
			} else {
				if(!compute_huffman_codes(a)) return 0;
			}
			if(!parse_huffman_block(a)) return 0;
		}
	} while(!final);
	return 1;
}

int zPng::do_zlib(zbuf* a, char* obuf, int olen, int exp, int parse_header) {
	a->zout_start = obuf;
	a->zout = obuf;
	a->zout_end = obuf + olen;
	a->z_expandable = exp;
	return parse_zlib(a, parse_header);
}

int zPng::zhuffman_decode(zbuf* a, zhuffman* z) {
	int b, s;
	if(a->num_bits < 16) fill_bits(a);
	b = z->fast[a->code_buffer & ZFAST_MASK];
	if(b) {
		s = b >> 9;
		a->code_buffer >>= s;
		a->num_bits -= s;
		return b & 511;
	}
	return zhuffman_decode_slowpath(a, z);
}

int zPng::zexpand(zbuf* z, char* zout, int n) {
	char* q;
	int cur, limit, old_limit;
	z->zout = zout;
	if(!z->z_expandable) return 0;
	cur = (int)(z->zout - z->zout_start);
	limit = old_limit = (int)(z->zout_end - z->zout_start);
	while(cur + n > limit) limit *= 2;
	q = (char*)realoc_sized(z->zout_start, old_limit, limit);
	if(!q) return 0;
	z->zout_start = q;
	z->zout = q + cur;
	z->zout_end = q + limit;
	return 1;
}

int zPng::parse_huffman_block(zbuf* a) {
	char* zout(a->zout);
	while(true) {
		int z(zhuffman_decode(a, &a->z_length));
		if(z < 256) {
			if(z < 0) return 0;
			if(zout >= a->zout_end) {
				if(!zexpand(a, zout, 1)) return 0;
				zout = a->zout;
			}
			*zout++ = (char)z;
		} else {
			u8* p;
			int len, dist;
			if(z == 256) {
				a->zout = zout;
				return 1;
			}
			z -= 257;
			len = zlength_base[z];
			if(zlength_extra[z]) len += zreceive(a, zlength_extra[z]);
			z = zhuffman_decode(a, &a->z_distance);
			if(z < 0) return 0;;
			dist = zdist_base[z];
			if(zdist_extra[z]) dist += zreceive(a, zdist_extra[z]);
			if(zout - a->zout_start < dist) return 0;
			if(zout + len > a->zout_end) {
				if(!zexpand(a, zout, len)) return 0;
				zout = a->zout;
			}
			p = (u8*)(zout - dist);
			if(dist == 1) {
				u8 v(*p);
				if(len) { do *zout++ = v; while(--len); }
			} else {
				if(len) { do *zout++ = *p++; while(--len); }
			}
		}
	}
}

int zPng::zhuffman_decode_slowpath(zbuf* a, zhuffman* z) {
	int b, s, k;
	k = bit_reverse(a->code_buffer, 16);
	for(s = ZFAST_BITS + 1; ; ++s)
		if(k < z->maxcode[s]) break;
	if(s == 16) return -1;
	b = (k >> (16 - s)) - z->firstcode[s] + z->firstsymbol[s];
	a->code_buffer >>= s;
	a->num_bits -= s;
	return z->value[b];
}

int zPng::parse_uncompressed_block(zbuf* a) {
	u8 header[4];
	int len, nlen, k;
	if(a->num_bits & 7) zreceive(a, a->num_bits & 7);
	k = 0;
	while(a->num_bits > 0) {
		header[k++] = (u8)(a->code_buffer & 255);
		a->code_buffer >>= 8;
		a->num_bits -= 8;
	}
	while(k < 4) header[k++] = zget8(a);
	len = header[1] * 256 + header[0];
	nlen = header[3] * 256 + header[2];
	if(nlen != (len ^ 0xffff)) return 0;
	if(a->zbuffer + len > a->zbuffer_end) return 0;
	if(a->zout + len > a->zout_end) {
		if(!zexpand(a, a->zout, len)) return 0;
	}
	memcpy(a->zout, a->zbuffer, len);
	a->zbuffer += len;
	a->zout += len;
	return 1;
}

int zPng::zbuild_huffman(zhuffman* z, u8* sizelist, int num) {
	int i, k = 0;
	int code, next_code[16], sizes[17];
	memset(sizes, 0, sizeof(sizes));
	memset(z->fast, 0, sizeof(z->fast));
	for(i = 0; i < num; ++i) ++sizes[sizelist[i]];
	sizes[0] = 0;
	for(i = 1; i < 16; ++i) {
		if(sizes[i] > (1 << i)) return 0;
	}
	code = 0;
	for(i = 1; i < 16; ++i) {
		next_code[i] = code;
		z->firstcode[i] = (u16)code;
		z->firstsymbol[i] = (u16)k;
		code = (code + sizes[i]);
		if(sizes[i] && (code - 1 >= (1 << i))) return 0;
		z->maxcode[i] = code << (16 - i);
		code <<= 1;
		k += sizes[i];
	}
	z->maxcode[16] = 0x10000;
	for(i = 0; i < num; ++i) {
		int s = sizelist[i];
		if(s) {
			int c(next_code[s] - z->firstcode[s] + z->firstsymbol[s]);
			auto fastv((u16)(s << 9) | i);
			z->size[c] = (u8)s;
			z->value[c] = (u16)i;
			if(s <= ZFAST_BITS) {
				int j(bit_reverse(next_code[s], s));
				while(j < (1 << ZFAST_BITS)) {
					z->fast[j] = (u16)fastv;
					j += (1 << s);
				}
			}
			++next_code[s];
		}
	}
	return 1;
}

int zPng::compute_huffman_codes(zbuf* a) {
	static u8 length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
	zhuffman z_codelength;
	u8 lencodes[286 + 32 + 137], codelength_sizes[19];
	int i, n;
	int hlit(zreceive(a, 5) + 257), hdist(zreceive(a, 5) + 1), hclen(zreceive(a, 4) + 4), ntot(hlit + hdist);
	memset(codelength_sizes, 0, sizeof(codelength_sizes));
	for(i = 0; i < hclen; ++i) {
		int s = zreceive(a, 3);
		codelength_sizes[length_dezigzag[i]] = (u8)s;
	}
	if(!zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;
	n = 0;
	while(n < ntot) {
		int c(zhuffman_decode(a, &z_codelength));
		if(c < 0 || c >= 19) return 0;
		if(c < 16)
			lencodes[n++] = (u8)c;
		else {
			u8 fill(0);
			if(c == 16) {
				c = zreceive(a, 2) + 3;
				if(n == 0) return 0;
				fill = lencodes[n - 1];
			} else if(c == 17)
				c = zreceive(a, 3) + 3;
			else {
				c = zreceive(a, 7) + 11;
			}
			if(ntot - n < c) return 0;
			memset(lencodes + n, fill, c);
			n += c;
		}
	}
	if(n != ntot) return 0;
	if(!zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
	if(!zbuild_huffman(&a->z_distance, lencodes + hlit, hdist)) return 0;
	return 1;
}
