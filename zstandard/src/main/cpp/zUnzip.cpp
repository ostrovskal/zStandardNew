
#include "zstandard/zstandard.h"

#ifdef NO_ERRNO_H
extern int errno;
#else
#include <errno.h>
#endif

#ifndef local
#define local static
#endif

#ifndef CASESENSITIVITYDEFAULT_NO
#if !defined(unix) && !defined(CASESENSITIVITYDEFAULT_YES)
#define CASESENSITIVITYDEFAULT_NO
#endif
#endif

#ifndef UNZ_BUFSIZE
#define UNZ_BUFSIZE (16384)
#endif

#ifndef UNZ_MAXFILENAMEINZIP
#define UNZ_MAXFILENAMEINZIP (256)
#endif

#ifndef ALLOC
#define ALLOC(size) (malloc(size))
#endif
#ifndef TRYFREE
#define TRYFREE(p) {if (p) free(p);}
#endif

#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)

const char unz_copyright[] = " unzip 1.01 Copyright 1998-2004 Gilles Vollant - http://www.winimage.com/zLibDll";
typedef struct unz_file_info_internal_s {
	uLong offset_curfile;
} unz_file_info_internal;


typedef struct {
	char* read_buffer;         /* internal buffer for compressed data */
	z_stream stream;            /* zLib stream structure for inflate */

	uLong pos_in_zipfile;       /* position in byte on the zipfile, for fseek*/
	uLong stream_initialised;   /* flag set if stream structure is initialised*/

	uLong offset_local_extrafield;/* offset of the local extra field */
	uInt  size_local_extrafield;/* size of the local extra field */
	uLong pos_local_extrafield;   /* position in the local extra field in read*/

	uLong crc32;                /* crc32 of all data uncompressed */
	uLong crc32_wait;           /* crc32 we must obtain after decompress all */
	uLong rest_read_compressed; /* number of byte to be decompressed */
	uLong rest_read_uncompressed;/*number of byte to be obtained after decomp*/
	zlib_filefunc_def z_filefunc;
	voidpf filestream;        /* io structore of the zipfile */
	uLong compression_method;   /* compression method (0==store) */
	uLong byte_before_the_zipfile;/* byte before the zipfile, (>0 for sfx)*/
	int   raw;
} file_in_zip_read_info_s;

typedef struct {
	zlib_filefunc_def z_filefunc;
	voidpf filestream;
	unz_global_info gi;
	uLong byte_before_the_zipfile;
	uLong num_file;
	uLong pos_in_central_dir;
	uLong current_file_ok;
	uLong central_pos;
	uLong size_central_dir;
	uLong offset_central_dir;
	unz_file_info cur_file_info;
	unz_file_info_internal cur_file_info_internal;
	file_in_zip_read_info_s* pfile_in_zip_read;
	int encrypted;
#ifndef NOUNCRYPT
	unsigned long keys[3];
	const unsigned long* pcrc_32_tab;
#endif
} unz_s;

#ifndef NOUNCRYPT
#include <zstandard/zCrypt.h>
#endif

local int unzlocal_getByte(const zlib_filefunc_def* pzlib_filefunc_def, voidpf filestream, int* pi) {
	unsigned char c;
	int err = (int)ZREAD(*pzlib_filefunc_def, filestream, &c, 1);
	if(err == 1) {
		*pi = (int)c;
		return UNZ_OK;
	}
	return (ZERROR(*pzlib_filefunc_def, filestream) ? UNZ_ERRNO : UNZ_EOF);
}

local int unzlocal_getShort(const zlib_filefunc_def* pzlib_filefunc_def, voidpf filestream, uLong* pX) {
	uLong x;
	int i, err;
	err = unzlocal_getByte(pzlib_filefunc_def, filestream, &i);
	x = (uLong)i;
	if(err == UNZ_OK) err = unzlocal_getByte(pzlib_filefunc_def, filestream, &i);
	x += ((uLong)i) << 8;
	*pX = (err == UNZ_OK ? x : 0);
	return err;
}

local int unzlocal_getLong(const zlib_filefunc_def* pzlib_filefunc_def, voidpf filestream, uLong* pX) {
	uLong x;
	int i, err;
	err = unzlocal_getByte(pzlib_filefunc_def, filestream, &i);
	x = (uLong)i;
	if(err == UNZ_OK) err = unzlocal_getByte(pzlib_filefunc_def, filestream, &i);
	x += ((uLong)i) << 8;
	if(err == UNZ_OK) err = unzlocal_getByte(pzlib_filefunc_def, filestream, &i);
	x += ((uLong)i) << 16;
	if(err == UNZ_OK) err = unzlocal_getByte(pzlib_filefunc_def, filestream, &i);
	x += ((uLong)i) << 24;
	*pX = (err == UNZ_OK ? x : 0);
	return err;
}

#ifdef  CASESENSITIVITYDEFAULT_NO
#define CASESENSITIVITYDEFAULTVALUE 2
#else
#define CASESENSITIVITYDEFAULTVALUE 1
#endif

extern int ZEXPORT unzStringFileNameCompare(cstr fileName1, cstr fileName2, int iCaseSensitivity) {
	if(iCaseSensitivity == 0) iCaseSensitivity = CASESENSITIVITYDEFAULTVALUE;
	if(iCaseSensitivity == 1) return strcmp(fileName1, fileName2);
	return stricmp(fileName1, fileName2);
}

#ifndef BUFREADCOMMENT
	#define BUFREADCOMMENT (0x400)
#endif

local uLong unzlocal_SearchCentralDir(const zlib_filefunc_def* pzlib_filefunc_def, voidpf filestream) {
	unsigned char* buf;
	uLong uSizeFile, uBackRead, uMaxBack = 0xffff, uPosFound = 0;
	if(ZSEEK(*pzlib_filefunc_def, filestream, 0, ZLIB_FILEFUNC_SEEK_END) != 0) return 0;
	uSizeFile = ZTELL(*pzlib_filefunc_def, filestream);
	if(uMaxBack > uSizeFile) uMaxBack = uSizeFile;
	buf = (unsigned char*)ALLOC(BUFREADCOMMENT + 4);
	if(!buf) return 0;
	uBackRead = 4;
	while(uBackRead < uMaxBack) {
		uLong uReadSize, uReadPos; int i;
		uBackRead = (uBackRead + BUFREADCOMMENT > uMaxBack ? uMaxBack : uBackRead + BUFREADCOMMENT);
		uReadPos = uSizeFile - uBackRead;
		uReadSize = ((BUFREADCOMMENT + 4) < (uSizeFile - uReadPos)) ? (BUFREADCOMMENT + 4) : (uSizeFile - uReadPos);
		if(ZSEEK(*pzlib_filefunc_def, filestream, uReadPos, ZLIB_FILEFUNC_SEEK_SET) != 0) break;
		if(ZREAD(*pzlib_filefunc_def, filestream, buf, uReadSize) != uReadSize) break;
		for(i = (int)uReadSize - 3; (i--) > 0;)
			if(((*(buf + i)) == 0x50) && ((*(buf + i + 1)) == 0x4b) && ((*(buf + i + 2)) == 0x05) && ((*(buf + i + 3)) == 0x06)) {
				uPosFound = uReadPos + i;
				break;
			}
		if(uPosFound) break;
	}
	TRYFREE(buf);
	return uPosFound;
}

extern HZIP* ZEXPORT unzOpen(cstr path, zlib_filefunc_def* pzlib_filefunc_def) {
	unz_s us; unz_s* s;
	uLong central_pos, uL, number_disk, number_disk_with_CD, number_entry_CD;
	int err = UNZ_OK;
	if(unz_copyright[0] != ' ') return nullptr;
	if(!pzlib_filefunc_def) fill_fopen_filefunc(&us.z_filefunc); else us.z_filefunc = *pzlib_filefunc_def;
	us.filestream = (*(us.z_filefunc.zopen_file))(us.z_filefunc.opaque, path, ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_EXISTING);
	if(!us.filestream) return nullptr;
	central_pos = unzlocal_SearchCentralDir(&us.z_filefunc, us.filestream);
	if(central_pos == 0) err = UNZ_ERRNO;
	if(ZSEEK(us.z_filefunc, us.filestream, central_pos, ZLIB_FILEFUNC_SEEK_SET) != 0) err = UNZ_ERRNO;
	if(unzlocal_getLong(&us.z_filefunc, us.filestream, &uL) != UNZ_OK) err = UNZ_ERRNO;
	if(unzlocal_getShort(&us.z_filefunc, us.filestream, &number_disk) != UNZ_OK) err = UNZ_ERRNO;
	if(unzlocal_getShort(&us.z_filefunc, us.filestream, &number_disk_with_CD) != UNZ_OK) err = UNZ_ERRNO;
	if(unzlocal_getShort(&us.z_filefunc, us.filestream, &us.gi.number_entry) != UNZ_OK) err = UNZ_ERRNO;
	if(unzlocal_getShort(&us.z_filefunc, us.filestream, &number_entry_CD) != UNZ_OK) err = UNZ_ERRNO;
	if((number_entry_CD != us.gi.number_entry) || (number_disk_with_CD != 0) || (number_disk != 0)) err = UNZ_BADZIPFILE;
	if(unzlocal_getLong(&us.z_filefunc, us.filestream, &us.size_central_dir) != UNZ_OK) err = UNZ_ERRNO;
	if(unzlocal_getLong(&us.z_filefunc, us.filestream, &us.offset_central_dir) != UNZ_OK) err = UNZ_ERRNO;
	if(unzlocal_getShort(&us.z_filefunc, us.filestream, &us.gi.size_comment) != UNZ_OK) err = UNZ_ERRNO;
	if((central_pos < us.offset_central_dir + us.size_central_dir) && (err == UNZ_OK)) err = UNZ_BADZIPFILE;
	if(err != UNZ_OK) { ZCLOSE(us.z_filefunc, us.filestream); return nullptr; }
	us.byte_before_the_zipfile = central_pos - (us.offset_central_dir + us.size_central_dir);
	us.central_pos = central_pos;
	us.pfile_in_zip_read = nullptr;
	us.encrypted = 0;
	s = (unz_s*)ALLOC(sizeof(unz_s));
	*s = us;
	unzGoToFirstFile((unzFile)s, nullptr);
	return new HZIP(s, nullptr);
}

local lutime_t unzlocal_DosDateToTmuDate(uLong dosTm) {
	struct tm st;
	u16 dosdate((dosTm >> 16) & 0xFFFF), dostime(dosTm & 0xFFFF);
	st.tm_sec	= (dostime & 0x1f) >> 1;
	st.tm_min	= ((dostime >> 5) & 0x3f);
	st.tm_hour	= ((dostime >> 11) & 0x1f);
	st.tm_mday	= (dosdate & 0x1f);
	st.tm_mon	= ((dosdate >> 5) & 0xf) - 1;
	st.tm_year	= ((dosdate >> 9) & 0x7f) + 80;
	st.tm_isdst = 0;
#ifdef WIN32
	return _mktime32(&st);
#else
	return mktime(&st);
#endif

}

local int unzlocal_FileInfoInternal(unzFile file, unz_file_info* info, unz_file_info_internal* iinfo) {
	if(!file) return UNZ_PARAMERROR;
	auto s((unz_s*)file);
	if(ZSEEK(s->z_filefunc, s->filestream, s->pos_in_central_dir + s->byte_before_the_zipfile, ZLIB_FILEFUNC_SEEK_SET) != 0) return UNZ_ERRNO;
	uLong uMagic;
	if(unzlocal_getLong(&s->z_filefunc, s->filestream, &uMagic) != UNZ_OK) return UNZ_ERRNO;
	if(uMagic != 0x02014b50) return UNZ_BADZIPFILE;
	unz_file_info finfo; unz_file_info_internal fiinfo;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &finfo.version) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &finfo.version_needed) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &finfo.flag) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &finfo.cmethod) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getLong(&s->z_filefunc, s->filestream, &finfo.dosDate) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getLong(&s->z_filefunc, s->filestream, &finfo.crc) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getLong(&s->z_filefunc, s->filestream, &finfo.csize) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getLong(&s->z_filefunc, s->filestream, &finfo.usize) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &finfo.sfilename) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &finfo.sextra) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &finfo.scomment) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &finfo.disk_num_start) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &finfo.attr) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getLong(&s->z_filefunc, s->filestream, &finfo.attr) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getLong(&s->z_filefunc, s->filestream, &fiinfo.offset_curfile) != UNZ_OK) return UNZ_ERRNO;
	finfo.mtime = unzlocal_DosDateToTmuDate(finfo.dosDate);
	uLong uSizeRead(finfo.sfilename);
	if(ZREAD(s->z_filefunc, s->filestream, finfo.name, uSizeRead) != uSizeRead) return UNZ_ERRNO;
	finfo.name[uSizeRead] = '\0';
	if(info) *info = finfo;
	if(iinfo) *iinfo = fiinfo;
	return UNZ_OK;
}

extern int ZEXPORT unzFileInfo(unzFile file, unz_file_info* pfile_info) {
	return unzlocal_FileInfoInternal(file, pfile_info, nullptr);
}

extern int ZEXPORT unzGoToFirstFile(unzFile file, unz_file_info* info) {
	if(!file) return UNZ_PARAMERROR;
	auto s((unz_s*)file);
	s->pos_in_central_dir = s->offset_central_dir;
	s->num_file = 0;
	auto err(unzlocal_FileInfoInternal(file, &s->cur_file_info, &s->cur_file_info_internal));
	s->current_file_ok = (err == UNZ_OK);
	if(info) *info = s->cur_file_info;
	return err;
}

extern int ZEXPORT unzGoToNextFile(unzFile file, unz_file_info* info) {
	if(!file) return UNZ_PARAMERROR;
	auto s((unz_s*)file);
	if(!s->current_file_ok) return UNZ_END_OF_LIST_OF_FILE;
	if(s->gi.number_entry != 0xffff) if(s->num_file + 1 == s->gi.number_entry) return UNZ_END_OF_LIST_OF_FILE;
	s->pos_in_central_dir += SIZECENTRALDIRITEM + s->cur_file_info.sfilename + s->cur_file_info.sextra + s->cur_file_info.scomment;
	s->num_file++;
	auto err(unzlocal_FileInfoInternal(file, &s->cur_file_info, &s->cur_file_info_internal));
	s->current_file_ok = (err == UNZ_OK);
	if(info) *info = s->cur_file_info;
	return err;
}

extern int ZEXPORT unzLocateFile(unzFile file, unz_file_info* info, int index, cstr szFileName, int iCaseSensitivity) {
	if(!file) return UNZ_PARAMERROR;
	auto s((unz_s*)file);
	if(!s->current_file_ok) return UNZ_END_OF_LIST_OF_FILE;
	auto num_fileSaved(s->num_file);
	auto pos_in_central_dirSaved(s->pos_in_central_dir);
	auto cur_file_infoSaved(s->cur_file_info);
	auto cur_file_info_internalSaved(s->cur_file_info_internal);
	auto err(unzGoToFirstFile(file, info));
	while(err == UNZ_OK) {
		if(szFileName) {
			if(unzStringFileNameCompare(info->name, szFileName, iCaseSensitivity) == 0) return UNZ_OK;
		} else {
			if(index-- <= 0) return UNZ_OK;
		}
		err = unzGoToNextFile(file, info);
	}
	s->num_file = num_fileSaved;
	s->pos_in_central_dir = pos_in_central_dirSaved;
	s->cur_file_info = cur_file_infoSaved;
	s->cur_file_info_internal = cur_file_info_internalSaved;
	return err;
}

extern int ZEXPORT unzCountFiles(unzFile file) {
	if(!file) return 0;
	auto s((unz_s*)file);
	if(!s->current_file_ok) return 0;
	auto num_fileSaved(s->num_file);
	auto pos_in_central_dirSaved(s->pos_in_central_dir);
	auto cur_file_infoSaved(s->cur_file_info);
	auto cur_file_info_internalSaved(s->cur_file_info_internal);
	auto err(unzGoToFirstFile(file, nullptr));
	while(err == UNZ_OK) err = unzGoToNextFile(file, nullptr);
	auto index(s->num_file);
	s->num_file = num_fileSaved;
	s->pos_in_central_dir = pos_in_central_dirSaved;
	s->cur_file_info = cur_file_infoSaved;
	s->cur_file_info_internal = cur_file_info_internalSaved;
	return index;
}

local int unzlocal_CheckCurrentFileCoherencyHeader(unz_s* s, uInt* piSizeVar, uLong* poffset_local_extrafield, uInt* psize_local_extrafield) {
	uLong uMagic, uData, uFlags, size_filename, size_extra_field;
	*piSizeVar = 0;
	*poffset_local_extrafield = 0;
	*psize_local_extrafield = 0;
	if(ZSEEK(s->z_filefunc, s->filestream, s->cur_file_info_internal.offset_curfile + s->byte_before_the_zipfile, ZLIB_FILEFUNC_SEEK_SET) != 0) return UNZ_ERRNO;
	if(unzlocal_getLong(&s->z_filefunc, s->filestream, &uMagic) != UNZ_OK) return UNZ_ERRNO;
	if(uMagic != 0x04034b50) return UNZ_BADZIPFILE;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &uData) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &uFlags) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &uData) != UNZ_OK) return UNZ_ERRNO;
	if(uData != s->cur_file_info.cmethod) return UNZ_BADZIPFILE;
	if((s->cur_file_info.cmethod != 0) && (s->cur_file_info.cmethod != Z_DEFLATED)) return UNZ_BADZIPFILE;
	if(unzlocal_getLong(&s->z_filefunc, s->filestream, &uData) != UNZ_OK) return UNZ_ERRNO;
	if(unzlocal_getLong(&s->z_filefunc, s->filestream, &uData) != UNZ_OK) return UNZ_ERRNO;
	if((uData != s->cur_file_info.crc) && ((uFlags & 8) == 0)) return UNZ_BADZIPFILE;
	if(unzlocal_getLong(&s->z_filefunc, s->filestream, &uData) != UNZ_OK) return UNZ_ERRNO;
	if((uData != s->cur_file_info.csize) && ((uFlags & 8) == 0)) return UNZ_BADZIPFILE;
	if(unzlocal_getLong(&s->z_filefunc, s->filestream, &uData) != UNZ_OK) return UNZ_ERRNO;
	if((uData != s->cur_file_info.usize) && ((uFlags & 8) == 0)) return UNZ_BADZIPFILE;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &size_filename) != UNZ_OK) return UNZ_ERRNO;
	if(size_filename != s->cur_file_info.sfilename) return UNZ_BADZIPFILE;
	*piSizeVar += (uInt)size_filename;
	if(unzlocal_getShort(&s->z_filefunc, s->filestream, &size_extra_field) != UNZ_OK) return UNZ_ERRNO;
	*poffset_local_extrafield = s->cur_file_info_internal.offset_curfile + SIZEZIPLOCALHEADER + size_filename;
	*psize_local_extrafield = (uInt)size_extra_field;
	*piSizeVar += (uInt)size_extra_field;
	return UNZ_OK;
}

int local_unzReadFile(unzFile file, voidp buf, unsigned len) {
	int err = UNZ_OK;
	uInt iRead = 0;
	unz_s* s;
	file_in_zip_read_info_s* pfile_in_zip_read_info;
	if(!file) return UNZ_PARAMERROR;
	s = (unz_s*)file;
	pfile_in_zip_read_info = s->pfile_in_zip_read;
	if(!pfile_in_zip_read_info) return UNZ_PARAMERROR;
	if(!pfile_in_zip_read_info->read_buffer) return UNZ_END_OF_LIST_OF_FILE;
	if(len == 0) return 0;
	pfile_in_zip_read_info->stream.next_out = (Bytef*)buf;
	pfile_in_zip_read_info->stream.avail_out = (uInt)len;
	if((len > pfile_in_zip_read_info->rest_read_uncompressed) && (!(pfile_in_zip_read_info->raw))) pfile_in_zip_read_info->stream.avail_out = (uInt)pfile_in_zip_read_info->rest_read_uncompressed;
	if((len > pfile_in_zip_read_info->rest_read_compressed + pfile_in_zip_read_info->stream.avail_in) && (pfile_in_zip_read_info->raw))
		pfile_in_zip_read_info->stream.avail_out = (uInt)pfile_in_zip_read_info->rest_read_compressed + pfile_in_zip_read_info->stream.avail_in;
	while(pfile_in_zip_read_info->stream.avail_out > 0) {
		if((pfile_in_zip_read_info->stream.avail_in == 0) && (pfile_in_zip_read_info->rest_read_compressed > 0)) {
			uInt uReadThis = UNZ_BUFSIZE;
			if(pfile_in_zip_read_info->rest_read_compressed < uReadThis) uReadThis = (uInt)pfile_in_zip_read_info->rest_read_compressed;
			if(uReadThis == 0) return UNZ_EOF;
			if(ZSEEK(pfile_in_zip_read_info->z_filefunc, pfile_in_zip_read_info->filestream, pfile_in_zip_read_info->pos_in_zipfile + pfile_in_zip_read_info->byte_before_the_zipfile, ZLIB_FILEFUNC_SEEK_SET) != 0)
				return UNZ_ERRNO;
			if(ZREAD(pfile_in_zip_read_info->z_filefunc, pfile_in_zip_read_info->filestream, pfile_in_zip_read_info->read_buffer, uReadThis) != uReadThis)
				return UNZ_ERRNO;
#ifndef NOUNCRYPT
			if(s->encrypted)             {
				uInt i;
				for(i = 0; i < uReadThis; i++) pfile_in_zip_read_info->read_buffer[i] = (char)zdecode(s->keys, s->pcrc_32_tab, pfile_in_zip_read_info->read_buffer[i]);
			}
#endif
			pfile_in_zip_read_info->pos_in_zipfile += uReadThis;
			pfile_in_zip_read_info->rest_read_compressed -= uReadThis;
			pfile_in_zip_read_info->stream.next_in = (Bytef*)pfile_in_zip_read_info->read_buffer;
			pfile_in_zip_read_info->stream.avail_in = (uInt)uReadThis;
		}
		if((pfile_in_zip_read_info->compression_method == 0) || (pfile_in_zip_read_info->raw)) {
			uInt uDoCopy, i;
			if((pfile_in_zip_read_info->stream.avail_in == 0) && (pfile_in_zip_read_info->rest_read_compressed == 0))
				return (iRead == 0) ? UNZ_EOF : iRead;
			uDoCopy = (pfile_in_zip_read_info->stream.avail_out < pfile_in_zip_read_info->stream.avail_in ? pfile_in_zip_read_info->stream.avail_out : pfile_in_zip_read_info->stream.avail_in);
			for(i = 0; i < uDoCopy; i++) *(pfile_in_zip_read_info->stream.next_out + i) = *(pfile_in_zip_read_info->stream.next_in + i);
			pfile_in_zip_read_info->crc32 = crc32(pfile_in_zip_read_info->crc32, pfile_in_zip_read_info->stream.next_out, uDoCopy);
			pfile_in_zip_read_info->rest_read_uncompressed -= uDoCopy;
			pfile_in_zip_read_info->stream.avail_in -= uDoCopy;
			pfile_in_zip_read_info->stream.avail_out -= uDoCopy;
			pfile_in_zip_read_info->stream.next_out += uDoCopy;
			pfile_in_zip_read_info->stream.next_in += uDoCopy;
			pfile_in_zip_read_info->stream.total_out += uDoCopy;
			iRead += uDoCopy;
		} else {
			uLong uTotalOutBefore, uTotalOutAfter;
			const Bytef* bufBefore;
			uLong uOutThis;
			int flush = Z_SYNC_FLUSH;
			uTotalOutBefore = pfile_in_zip_read_info->stream.total_out;
			bufBefore = pfile_in_zip_read_info->stream.next_out;
			err = inflate(&pfile_in_zip_read_info->stream, flush);
			if(err >= 0 && pfile_in_zip_read_info->stream.msg) err = Z_DATA_ERROR;
			uTotalOutAfter = pfile_in_zip_read_info->stream.total_out;
			uOutThis = uTotalOutAfter - uTotalOutBefore;
			pfile_in_zip_read_info->crc32 = crc32(pfile_in_zip_read_info->crc32, bufBefore, (uInt)(uOutThis));
			pfile_in_zip_read_info->rest_read_uncompressed -= uOutThis;
			iRead += (uInt)(uTotalOutAfter - uTotalOutBefore);
			if(err == Z_STREAM_END) return (iRead == 0 ? UNZ_EOF : iRead);
			if(err != Z_OK) break;
		}
	}
	return (err == Z_OK ? iRead : err);
}

int local_unzCloseFile(unzFile file) {
	file_in_zip_read_info_s* pfile_in_zip_read_info;
	if(!file) return UNZ_PARAMERROR;
	auto s((unz_s*)file);
	pfile_in_zip_read_info = s->pfile_in_zip_read;
	if(!pfile_in_zip_read_info) return UNZ_PARAMERROR;
	if((pfile_in_zip_read_info->rest_read_uncompressed == 0) && !pfile_in_zip_read_info->raw) {
		if(pfile_in_zip_read_info->crc32 != pfile_in_zip_read_info->crc32_wait) return UNZ_CRCERROR;
	}
	TRYFREE(pfile_in_zip_read_info->read_buffer);
	pfile_in_zip_read_info->read_buffer = nullptr;
	if(pfile_in_zip_read_info->stream_initialised) inflateEnd(&pfile_in_zip_read_info->stream);
	pfile_in_zip_read_info->stream_initialised = 0;
	TRYFREE(pfile_in_zip_read_info);
	s->pfile_in_zip_read = nullptr;
	return UNZ_OK;
}

extern int ZEXPORT unzUnzipToMem(unzFile file, int index, u8** ptr, unsigned* size, cstr password) {
	uInt iSizeVar;
	uLong offset_local_extrafield;
	uInt  size_local_extrafield;
#ifndef NOUNCRYPT
	char source[12];
#else
	if(password) return UNZ_PARAMERROR;
#endif
	if(!file) return UNZ_PARAMERROR;
	auto s((unz_s*)file);
	if(!s->current_file_ok) return UNZ_PARAMERROR;
	if(s->pfile_in_zip_read) local_unzCloseFile(file);
	unz_file_info info;
	auto err(unzLocateFile(file, &info, index, nullptr, 0));
	if(err != UNZ_OK) return err;
	if(unzlocal_CheckCurrentFileCoherencyHeader(s, &iSizeVar, &offset_local_extrafield, &size_local_extrafield) != UNZ_OK) return UNZ_BADZIPFILE;
	auto pfile_in_zip_read_info((file_in_zip_read_info_s*)ALLOC(sizeof(file_in_zip_read_info_s)));
	if(!pfile_in_zip_read_info) return UNZ_INTERNALERROR;
	auto len(info.usize); auto buf(new u8[len]);
	if(size) *size = len; if(ptr) *ptr = buf;
	pfile_in_zip_read_info->read_buffer = (char*)ALLOC(UNZ_BUFSIZE);
	pfile_in_zip_read_info->offset_local_extrafield = offset_local_extrafield;
	pfile_in_zip_read_info->size_local_extrafield = size_local_extrafield;
	pfile_in_zip_read_info->pos_local_extrafield = 0;
	pfile_in_zip_read_info->raw = 0;
	if(!pfile_in_zip_read_info->read_buffer) {
		TRYFREE(pfile_in_zip_read_info);
		return UNZ_INTERNALERROR;
	}
	pfile_in_zip_read_info->stream_initialised = 0;
	if((s->cur_file_info.cmethod != 0) && (s->cur_file_info.cmethod != Z_DEFLATED)) err = UNZ_BADZIPFILE;
	pfile_in_zip_read_info->crc32_wait = s->cur_file_info.crc;
	pfile_in_zip_read_info->crc32 = 0;
	pfile_in_zip_read_info->compression_method = s->cur_file_info.cmethod;
	pfile_in_zip_read_info->filestream = s->filestream;
	pfile_in_zip_read_info->z_filefunc = s->z_filefunc;
	pfile_in_zip_read_info->byte_before_the_zipfile = s->byte_before_the_zipfile;
	pfile_in_zip_read_info->stream.total_out = 0;
	if(s->cur_file_info.cmethod == Z_DEFLATED) {
		pfile_in_zip_read_info->stream.zalloc = nullptr;
		pfile_in_zip_read_info->stream.zfree = nullptr;
		pfile_in_zip_read_info->stream.opaque = nullptr;
		pfile_in_zip_read_info->stream.next_in = nullptr;
		pfile_in_zip_read_info->stream.avail_in = 0;
		err = inflateInit2(&pfile_in_zip_read_info->stream, -MAX_WBITS);
		if(err == Z_OK) pfile_in_zip_read_info->stream_initialised = 1;
		else {
			TRYFREE(pfile_in_zip_read_info);
			return err;
		}
	}
	pfile_in_zip_read_info->rest_read_compressed = s->cur_file_info.csize;
	pfile_in_zip_read_info->rest_read_uncompressed = s->cur_file_info.usize;
	pfile_in_zip_read_info->pos_in_zipfile = s->cur_file_info_internal.offset_curfile + SIZEZIPLOCALHEADER + iSizeVar;
	pfile_in_zip_read_info->stream.avail_in = (uInt)0;
	s->pfile_in_zip_read = pfile_in_zip_read_info;
#ifndef NOUNCRYPT
	if(password) {
		int i;
		s->pcrc_32_tab = (const unsigned long*)get_crc_table();
		init_keys(password, s->keys, s->pcrc_32_tab);
		if(ZSEEK(s->z_filefunc, s->filestream, s->pfile_in_zip_read->pos_in_zipfile + s->pfile_in_zip_read->byte_before_the_zipfile, SEEK_SET) != 0)
			return UNZ_INTERNALERROR;
		if(ZREAD(s->z_filefunc, s->filestream, source, 12) < 12) return UNZ_INTERNALERROR;
		for(i = 0; i < 12; i++) zdecode(s->keys, s->pcrc_32_tab, source[i]);
		s->pfile_in_zip_read->pos_in_zipfile += 12;
		s->encrypted = 1;
	}
#endif
	err = local_unzReadFile(file, buf, len);
	return (err == (int)len ? local_unzCloseFile(file) : err);
}

extern int ZEXPORT unzClose(unzFile file) {
	if(!file) return UNZ_PARAMERROR;
	auto s((unz_s*)file);
	if(s->pfile_in_zip_read) local_unzCloseFile(file);
	ZCLOSE(s->z_filefunc, s->filestream);
	TRYFREE(s);
	return UNZ_OK;
}

extern int ZEXPORT unzUnzipToFile(unzFile file, int index, cstr path, cstr password) {
	u8* buf(nullptr); unsigned size(0);
	auto err(unzUnzipToMem(file, index, &buf, &size, password));
	if(err == UNZ_OK) {
		zFile f(path, false, false);
		f.write(buf, size);
	}
	delete[] buf;
	return err;
}
