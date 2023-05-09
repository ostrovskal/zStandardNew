
#include "zstandard/zstandard.h"
#include "zstandard/zIOfile.h"

#ifndef SEEK_CUR
#define SEEK_CUR    1
#endif

#ifndef SEEK_END
#define SEEK_END    2
#endif

#ifndef SEEK_SET
#define SEEK_SET    0
#endif

voidpf ZCALLBACK fopen_file_func(voidpf, cstr filename, int mode) {
	FILE* file(nullptr);
	const char* mode_fopen(nullptr);
	if((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_READ) mode_fopen = "rb";
	else if(mode & ZLIB_FILEFUNC_MODE_EXISTING) mode_fopen = "r+b";
	else if(mode & ZLIB_FILEFUNC_MODE_CREATE) mode_fopen = "wb";
	if(filename && mode_fopen) file = fopen(filename, mode_fopen);
	return file;
}

uLong ZCALLBACK fread_file_func(voidpf, voidpf stream, void* buf, uLong size) {
	return (uLong)fread(buf, 1, (size_t)size, (FILE*)stream);
}

uLong ZCALLBACK fwrite_file_func(voidpf, voidpf stream, const void* buf, uLong size) {
	return (uLong)fwrite(buf, 1, (size_t)size, (FILE*)stream);
}

long ZCALLBACK ftell_file_func(voidpf, voidpf stream) {
	return ftell((FILE*)stream);
}

long ZCALLBACK fseek_file_func(voidpf, voidpf stream, uLong offset, int origin) {
	int fseek_origin = 0;
	long ret;
	switch(origin)     {
		case ZLIB_FILEFUNC_SEEK_CUR: fseek_origin = SEEK_CUR; break;
		case ZLIB_FILEFUNC_SEEK_END: fseek_origin = SEEK_END; break;
		case ZLIB_FILEFUNC_SEEK_SET: fseek_origin = SEEK_SET; break;
		default: return -1;
	}
	ret = 0;
	fseek((FILE*)stream, offset, fseek_origin);
	return ret;
}

int ZCALLBACK fclose_file_func(voidpf, voidpf stream) {
	return fclose((FILE*)stream);
}

int ZCALLBACK ferror_file_func(voidpf, voidpf stream) {
	return ferror((FILE*)stream);
}

void fill_fopen_filefunc(zlib_filefunc_def* pzlib_filefunc_def) {
	pzlib_filefunc_def->zopen_file = fopen_file_func;
	pzlib_filefunc_def->zread_file = fread_file_func;
	pzlib_filefunc_def->zwrite_file = fwrite_file_func;
	pzlib_filefunc_def->ztell_file = ftell_file_func;
	pzlib_filefunc_def->zseek_file = fseek_file_func;
	pzlib_filefunc_def->zclose_file = fclose_file_func;
	pzlib_filefunc_def->zerror_file = ferror_file_func;
	pzlib_filefunc_def->opaque = nullptr;
}
