
#include "zstandard/zstandard.h"
#include "zstandard/zZip.h"

#ifdef NO_ERRNO_H
	extern int errno;
#else
	#include <errno.h>
#endif

#ifndef local
	#define local static
#endif

#ifndef VERSIONMADEBY
	#define VERSIONMADEBY   (0x0)
#endif

#ifndef Z_BUFSIZE
	#define Z_BUFSIZE (16384)
#endif

#ifndef Z_MAXFILENAMEINZIP
	#define Z_MAXFILENAMEINZIP (256)
#endif

#ifndef ALLOC
	#define ALLOC(size) malloc(size)
#endif
#ifndef TRYFREE
	#define TRYFREE(p) { if (p) free(p); }
#endif

#ifndef SEEK_CUR
	#define SEEK_CUR    1
#endif

#ifndef SEEK_END
	#define SEEK_END    2
#endif

#ifndef SEEK_SET
	#define SEEK_SET    0
#endif

#define SIZEDATA_INDATABLOCK	(4096 - (4 * 4))
#define LOCALHEADERMAGIC		0x04034b50
#define CENTRALHEADERMAGIC		0x02014b50
#define ENDHEADERMAGIC			0x06054b50
#define FLAG_LOCALHEADER_OFFSET 0x06
#define CRC_LOCALHEADER_OFFSET  0x0e
#define SIZECENTRALHEADER		0x2e

typedef struct linkedlist_datablock_internal_s {
	struct linkedlist_datablock_internal_s* next_datablock;
	uLong  avail_in_this_block;
	uLong  filled_in_this_block;
	uLong  unused;
	unsigned char data[SIZEDATA_INDATABLOCK];
} linkedlist_datablock_internal;

typedef struct linkedlist_data_s {
	linkedlist_datablock_internal* first_block;
	linkedlist_datablock_internal* last_block;
} linkedlist_data;

typedef struct {
	z_stream stream;
	int  stream_initialised;
	uInt pos_in_buffered_data;
	uLong pos_local_header;
	char* central_header;
	uLong size_centralheader;
	uLong flag;
	int  method;
	int  raw;
	Byte buffered_data[Z_BUFSIZE];
	uLong dosDate;
	uLong crc32;
	int  encrypt;
#ifndef NOCRYPT
	unsigned long keys[3];
	const unsigned long* pcrc_32_tab;
	int crypt_header_size;
#endif
} curfile_info;

typedef struct {
	zlib_filefunc_def z_filefunc;
	voidpf filestream;
	linkedlist_data central_dir;
	int  in_opened_file_inzip;
	curfile_info ci;
	uLong begin_pos;
	uLong add_position_when_writting_offset;
	uLong number_entry;
#ifndef NO_ADDFILEINEXISTINGZIP
	char* globalcomment;
#endif
} zip_internal;

#ifndef NOCRYPT
	#define INCLUDECRYPTINGCODE_IFCRYPTALLOWED
	#include <zstandard/zCrypt.h>
#endif

local linkedlist_datablock_internal* allocate_new_datablock() {
	linkedlist_datablock_internal* ldi;
	ldi = (linkedlist_datablock_internal*)
		ALLOC(sizeof(linkedlist_datablock_internal));
	if(ldi != NULL)     {
		ldi->next_datablock = NULL;
		ldi->filled_in_this_block = 0;
		ldi->avail_in_this_block = SIZEDATA_INDATABLOCK;
	}
	return ldi;
}

local void free_datablock(linkedlist_datablock_internal* ldi) {
	while(ldi) {
		linkedlist_datablock_internal* ldinext = ldi->next_datablock;
		TRYFREE(ldi);
		ldi = ldinext;
	}
}

local void init_linkedlist(linkedlist_data* ll) {
	ll->first_block = ll->last_block = nullptr;
}

static int add_data_in_datablock(linkedlist_data* ll, const void* buf, uLong len) {
	linkedlist_datablock_internal* ldi;
	const unsigned char* from_copy;
	if(!ll) return ZIP_INTERNALERROR;
	if(!ll->last_block) {
		ll->first_block = ll->last_block = allocate_new_datablock();
		if(!ll->first_block) return ZIP_INTERNALERROR;
	}
	ldi = ll->last_block;
	from_copy = (unsigned char*)buf;
	while(len > 0) {
		uInt copy_this, i;
		unsigned char* to_copy;
		if(ldi->avail_in_this_block == 0) {
			ldi->next_datablock = allocate_new_datablock();
			if(!ldi->next_datablock) return ZIP_INTERNALERROR;
			ldi = ldi->next_datablock;
			ll->last_block = ldi;
		}
		copy_this = (ldi->avail_in_this_block < len ? (uInt)ldi->avail_in_this_block : (uInt)len);
		to_copy = &(ldi->data[ldi->filled_in_this_block]);
		for(i = 0; i < copy_this; i++) *(to_copy + i) = *(from_copy + i);
		ldi->filled_in_this_block += copy_this;
		ldi->avail_in_this_block -= copy_this;
		from_copy += copy_this;
		len -= copy_this;
	}
	return ZIP_OK;
}

/****************************************************************************/

#ifndef NO_ADDFILEINEXISTINGZIP

local int ziplocal_putValue (const zlib_filefunc_def* pzlib_filefunc_def, voidpf filestream, uLong x, int nbByte) {
	unsigned char buf[4];
	int n;
	for(n = 0; n < nbByte; n++) {
		buf[n] = (unsigned char)(x & 0xff);
		x >>= 8;
	}
	if(x) { for(n = 0; n < nbByte; n++) buf[n] = 0xff; }
	return (ZWRITE(*pzlib_filefunc_def, filestream, buf, nbByte) != (uLong)nbByte ? ZIP_ERRNO : ZIP_OK);
}

local void ziplocal_putValue_inmemory(void* dest, uLong x, int nbByte) {
	unsigned char* buf = (unsigned char*)dest;
	int n;
	for(n = 0; n < nbByte; n++) {
		buf[n] = (unsigned char)(x & 0xff);
		x >>= 8;
	}
	if(x != 0) { for(n = 0; n < nbByte; n++) buf[n] = 0xff; }
}

/****************************************************************************/

static uLong ziplocal_TmzDateToDosDate(uLong* fa, int fd) {
#ifdef WIN32
	struct _stat32 st { 0 };
	auto mt(_time32(nullptr));
	if(fd) _fstat32(fd, &st);
	mt = st.st_mtime;
	auto t(_localtime32(&mt));
#else
	struct stat st { 0 };
	auto mt(time(nullptr));
	if(fd) fstat(fd, &st);
	mt = st.st_mtim.tv_sec;
	auto t(localtime(&mt));
#endif
	*fa = st.st_mode;
	uLong dosdate, dostime;
	dosdate =  ((t->tm_year - 80) & 0x7f) << 9;
	dosdate |= ((t->tm_mon + 1)   & 0xf)  << 5;
	dosdate |= (t->tm_mday        & 0x1f);
	dostime =  ((t->tm_hour       & 0x1f) << 11);
	dostime |= ((t->tm_min        & 0x3f) << 5);
	dostime |= ((t->tm_sec * 2)   & 0x1f);
	return (dostime | (uLong)(dosdate << 16));
}

/****************************************************************************/

local int ziplocal_getByte(const zlib_filefunc_def* pzlib_filefunc_def, voidpf filestream, int* pi) {
	unsigned char c;
	int err = (int)ZREAD(*pzlib_filefunc_def, filestream, &c, 1);
	if(err == 1) { *pi = (int)c; return ZIP_OK; }
	return (ZERROR(*pzlib_filefunc_def, filestream) ? ZIP_ERRNO : ZIP_EOF);
}

local int ziplocal_getShort(const zlib_filefunc_def* pzlib_filefunc_def, voidpf filestream, uLong* pX) {
	uLong x; int i;
	int err = ziplocal_getByte(pzlib_filefunc_def, filestream, &i);
	x = (uLong)i;
	if(err == ZIP_OK) err = ziplocal_getByte(pzlib_filefunc_def, filestream, &i);
	x += ((uLong)i) << 8;
	*pX = (err == ZIP_OK ? x : 0);
	return err;
}

local int ziplocal_getLong(const zlib_filefunc_def* pzlib_filefunc_def, voidpf filestream, uLong* pX) {
	uLong x;
	int i, err;
	err = ziplocal_getByte(pzlib_filefunc_def, filestream, &i);
	x = (uLong)i;
	if(err == ZIP_OK) err = ziplocal_getByte(pzlib_filefunc_def, filestream, &i);
	x += ((uLong)i) << 8;
	if(err == ZIP_OK) err = ziplocal_getByte(pzlib_filefunc_def, filestream, &i);
	x += ((uLong)i) << 16;
	if(err == ZIP_OK) err = ziplocal_getByte(pzlib_filefunc_def, filestream, &i);
	x += ((uLong)i) << 24;
	*pX = (err == ZIP_OK ? x : 0);
	return err;
}

#ifndef BUFREADCOMMENT
	#define BUFREADCOMMENT (0x400)
#endif

local uLong ziplocal_SearchCentralDir(const zlib_filefunc_def* pzlib_filefunc_def, voidpf filestream) {
	unsigned char* buf;
	uLong uSizeFile, uBackRead, uMaxBack = 0xffff, uPosFound = 0;
	if(ZSEEK(*pzlib_filefunc_def, filestream, 0, ZLIB_FILEFUNC_SEEK_END) != 0) return 0;
	uSizeFile = ZTELL(*pzlib_filefunc_def, filestream);
	if(uMaxBack > uSizeFile) uMaxBack = uSizeFile;
	buf = (unsigned char*)ALLOC(BUFREADCOMMENT + 4);
	if(!buf) return 0;
	uBackRead = 4;
	while(uBackRead < uMaxBack) {
		uLong uReadSize, uReadPos;
		int i;
		uBackRead = (uBackRead + BUFREADCOMMENT > uMaxBack ? uMaxBack : uBackRead + BUFREADCOMMENT);
		uReadPos = uSizeFile - uBackRead;
		uReadSize = ((BUFREADCOMMENT + 4) < (uSizeFile - uReadPos)) ? (BUFREADCOMMENT + 4) : (uSizeFile - uReadPos);
		if(ZSEEK(*pzlib_filefunc_def, filestream, uReadPos, ZLIB_FILEFUNC_SEEK_SET)) break;
		if(ZREAD(*pzlib_filefunc_def, filestream, buf, uReadSize) != uReadSize) break;
		for(i = (int)uReadSize - 3; (i--) > 0;)
			if(((*(buf + i)) == 0x50) && ((*(buf + i + 1)) == 0x4b) && ((*(buf + i + 2)) == 0x05) && ((*(buf + i + 3)) == 0x06)) {
				uPosFound = uReadPos + i;
				break;
			}
		if(uPosFound != 0) break;
	}
	TRYFREE(buf);
	return uPosFound;
}
#endif

/************************************************************/
extern HZIP* ZEXPORT zipOpen(const char* pathname, int append, const char** globalcomment, zlib_filefunc_def* pzlib_filefunc_def) {
	zip_internal ziinit;
	zip_internal* zi;
	int err = ZIP_OK;
	if(!pzlib_filefunc_def)
		fill_fopen_filefunc(&ziinit.z_filefunc);
	else
		ziinit.z_filefunc = *pzlib_filefunc_def;
	ziinit.filestream = (*(ziinit.z_filefunc.zopen_file))(ziinit.z_filefunc.opaque, pathname, (append == APPEND_STATUS_CREATE ?
		 (ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_CREATE) : (ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_EXISTING)));
	if(!ziinit.filestream) return nullptr;
	ziinit.begin_pos = ZTELL(ziinit.z_filefunc, ziinit.filestream);
	ziinit.in_opened_file_inzip = 0;
	ziinit.ci.stream_initialised = 0;
	ziinit.number_entry = 0;
	ziinit.add_position_when_writting_offset = 0;
	init_linkedlist(&(ziinit.central_dir));
	zi = (zip_internal*)ALLOC(sizeof(zip_internal));
	if(!zi) {
		ZCLOSE(ziinit.z_filefunc, ziinit.filestream);
		return nullptr;
	}
#ifndef NO_ADDFILEINEXISTINGZIP
	ziinit.globalcomment = nullptr;
	if(append == APPEND_STATUS_ADDINZIP) {
		uLong byte_before_the_zipfile, size_central_dir, offset_central_dir, central_pos, uL;
		uLong number_disk, number_disk_with_CD, number_entry, number_entry_CD, size_comment;
		central_pos = ziplocal_SearchCentralDir(&ziinit.z_filefunc, ziinit.filestream);
		if(!central_pos) err = ZIP_ERRNO;
		if(ZSEEK(ziinit.z_filefunc, ziinit.filestream, central_pos, ZLIB_FILEFUNC_SEEK_SET) != 0) err = ZIP_ERRNO;
		if(ziplocal_getLong(&ziinit.z_filefunc, ziinit.filestream, &uL) != ZIP_OK) err = ZIP_ERRNO;
		if(ziplocal_getShort(&ziinit.z_filefunc, ziinit.filestream, &number_disk) != ZIP_OK) err = ZIP_ERRNO;
		if(ziplocal_getShort(&ziinit.z_filefunc, ziinit.filestream, &number_disk_with_CD) != ZIP_OK) err = ZIP_ERRNO;
		if(ziplocal_getShort(&ziinit.z_filefunc, ziinit.filestream, &number_entry) != ZIP_OK) err = ZIP_ERRNO;
		if(ziplocal_getShort(&ziinit.z_filefunc, ziinit.filestream, &number_entry_CD) != ZIP_OK) err = ZIP_ERRNO;
		if((number_entry_CD != number_entry) || (number_disk_with_CD != 0) || (number_disk != 0)) err = ZIP_BADZIPFILE;
		if(ziplocal_getLong(&ziinit.z_filefunc, ziinit.filestream, &size_central_dir) != ZIP_OK) err = ZIP_ERRNO;
		if(ziplocal_getLong(&ziinit.z_filefunc, ziinit.filestream, &offset_central_dir) != ZIP_OK) err = ZIP_ERRNO;
		if(ziplocal_getShort(&ziinit.z_filefunc, ziinit.filestream, &size_comment) != ZIP_OK) err = ZIP_ERRNO;
		if((central_pos < offset_central_dir + size_central_dir) && (err == ZIP_OK)) err = ZIP_BADZIPFILE;
		if(err != ZIP_OK) { ZCLOSE(ziinit.z_filefunc, ziinit.filestream); return nullptr; }
		if(size_comment > 0) {
			ziinit.globalcomment = (char*)ALLOC(size_comment + 1);
			if(ziinit.globalcomment)             {
				size_comment = ZREAD(ziinit.z_filefunc, ziinit.filestream, ziinit.globalcomment, size_comment);
				ziinit.globalcomment[size_comment] = 0;
			}
		}
		byte_before_the_zipfile = central_pos - (offset_central_dir + size_central_dir);
		ziinit.add_position_when_writting_offset = byte_before_the_zipfile;
		uLong size_central_dir_to_read = size_central_dir;
		size_t buf_size = SIZEDATA_INDATABLOCK;
		void* buf_read = (void*)ALLOC(buf_size);
		if(ZSEEK(ziinit.z_filefunc, ziinit.filestream, offset_central_dir + byte_before_the_zipfile, ZLIB_FILEFUNC_SEEK_SET) != 0) err = ZIP_ERRNO;
		while((size_central_dir_to_read > 0) && (err == ZIP_OK)) {
			uLong read_this = SIZEDATA_INDATABLOCK;
			if(read_this > size_central_dir_to_read) read_this = size_central_dir_to_read;
			if(ZREAD(ziinit.z_filefunc, ziinit.filestream, buf_read, read_this) != read_this) err = ZIP_ERRNO;
			if(err == ZIP_OK) err = add_data_in_datablock(&ziinit.central_dir, buf_read, (uLong)read_this);
			size_central_dir_to_read -= read_this;
		}
		TRYFREE(buf_read);
		ziinit.begin_pos = byte_before_the_zipfile;
		ziinit.number_entry = number_entry_CD;
		if(ZSEEK(ziinit.z_filefunc, ziinit.filestream, offset_central_dir + byte_before_the_zipfile, ZLIB_FILEFUNC_SEEK_SET) != 0) err = ZIP_ERRNO;
	}
	if(globalcomment) *globalcomment = ziinit.globalcomment;
#endif
	if(err != ZIP_OK) {
#ifndef NO_ADDFILEINEXISTINGZIP
		TRYFREE(ziinit.globalcomment);
#endif
		TRYFREE(zi);
		return nullptr;
	}
	*zi = ziinit;
	return new HZIP(nullptr, zi);
}

local int zipFlushWriteBuffer(zip_internal* zi) {
	int err = ZIP_OK;
	if(zi->ci.encrypt) {
#ifndef NOCRYPT
		uInt i; int t;
		for(i = 0; i < zi->ci.pos_in_buffered_data; i++) zi->ci.buffered_data[i] = (u8)zencode(zi->ci.keys, zi->ci.pcrc_32_tab, zi->ci.buffered_data[i], t);
#endif
	}
	if(ZWRITE(zi->z_filefunc, zi->filestream, zi->ci.buffered_data, zi->ci.pos_in_buffered_data) != zi->ci.pos_in_buffered_data) err = ZIP_ERRNO;
	zi->ci.pos_in_buffered_data = 0;
	return err;
}

extern int ZEXPORT zipCloseFile(zipFile file, uLong uncompressed_size, uLong crc32) {
	zip_internal* zi;
	uLong compressed_size;
	int err = ZIP_OK;
	if(!file) return ZIP_PARAMERROR;
	zi = (zip_internal*)file;
	if(zi->in_opened_file_inzip == 0) return ZIP_PARAMERROR;
	zi->ci.stream.avail_in = 0;
	if((zi->ci.method == Z_DEFLATED) && (!zi->ci.raw))
		while(err == ZIP_OK) {
			uLong uTotalOutBefore;
			if(zi->ci.stream.avail_out == 0) {
				if(zipFlushWriteBuffer(zi) == ZIP_ERRNO) err = ZIP_ERRNO;
				zi->ci.stream.avail_out = (uInt)Z_BUFSIZE;
				zi->ci.stream.next_out = zi->ci.buffered_data;
			}
			uTotalOutBefore = zi->ci.stream.total_out;
			err = deflate(&zi->ci.stream, Z_FINISH);
			zi->ci.pos_in_buffered_data += (uInt)(zi->ci.stream.total_out - uTotalOutBefore);
		}
	if(err == Z_STREAM_END) err = ZIP_OK; /* this is normal */
	if((zi->ci.pos_in_buffered_data > 0) && (err == ZIP_OK))
		if(zipFlushWriteBuffer(zi) == ZIP_ERRNO) err = ZIP_ERRNO;
	if((zi->ci.method == Z_DEFLATED) && (!zi->ci.raw)) {
		err = deflateEnd(&zi->ci.stream);
		zi->ci.stream_initialised = 0;
	}
	if(!zi->ci.raw) {
		crc32 = (uLong)zi->ci.crc32;
		uncompressed_size = (uLong)zi->ci.stream.total_in;
	}
	compressed_size = (uLong)zi->ci.stream.total_out;
#ifndef NOCRYPT
	compressed_size += zi->ci.crypt_header_size;
#endif
	ziplocal_putValue_inmemory(zi->ci.central_header + 16, crc32, 4); /*crc*/
	ziplocal_putValue_inmemory(zi->ci.central_header + 20, compressed_size, 4);
	if(zi->ci.stream.data_type == Z_ASCII) ziplocal_putValue_inmemory(zi->ci.central_header + 36, (uLong)Z_ASCII, 2);
	ziplocal_putValue_inmemory(zi->ci.central_header + 24, uncompressed_size, 4);
	if(err == ZIP_OK) err = add_data_in_datablock(&zi->central_dir, zi->ci.central_header, (uLong)zi->ci.size_centralheader);
	free(zi->ci.central_header);
	if(err == ZIP_OK) {
		long cur_pos_inzip = ZTELL(zi->z_filefunc, zi->filestream);
		if(ZSEEK(zi->z_filefunc, zi->filestream, zi->ci.pos_local_header + 14, ZLIB_FILEFUNC_SEEK_SET) != 0) err = ZIP_ERRNO;
		if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, crc32, 4);
		if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, compressed_size, 4);
		if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, uncompressed_size, 4);
		if(ZSEEK(zi->z_filefunc, zi->filestream, cur_pos_inzip, ZLIB_FILEFUNC_SEEK_SET) != 0) err = ZIP_ERRNO;
	}
	zi->number_entry++;
	zi->in_opened_file_inzip = 0;
	return err;
}

extern int ZEXPORT zipAddMem(zipFile file, cstr filename, const void* buf, unsigned len, int method, int level, cstr password,
							 int fd, int raw, int windowBits, int memLevel, int strategy, uLong crcForCrypting) {
	uInt i; uLong fa;
	int err = ZIP_OK;
#ifdef NOCRYPT
	if(password) return ZIP_PARAMERROR;
#endif
	if(!file) return ZIP_PARAMERROR;
	if(!filename) return ZIP_PARAMERROR;
	if(method && method != Z_DEFLATED) return ZIP_PARAMERROR;
	auto zi((zip_internal*)file);
	if(zi->in_opened_file_inzip == 1) {
		err = zipCloseFile(file, 0, 0);
		if(err != ZIP_OK) return err;
	}
	auto size_filename((uInt)strlen(filename));
	zi->ci.dosDate = ziplocal_TmzDateToDosDate(&fa, fd);
	zi->ci.flag = 0;
	if((level == 8) || (level == 9)) zi->ci.flag |= 2;
	if(level == 2) zi->ci.flag |= 4;
	if(level == 1) zi->ci.flag |= 6;
	if(password) zi->ci.flag |= 1;
	zi->ci.crc32 = 0;
	zi->ci.method = method;
	zi->ci.encrypt = 0;
	zi->ci.stream_initialised = 0;
	zi->ci.pos_in_buffered_data = 0;
	zi->ci.raw = raw;
	zi->ci.pos_local_header = ZTELL(zi->z_filefunc, zi->filestream);
	zi->ci.size_centralheader = SIZECENTRALHEADER + size_filename;
	zi->ci.central_header = (char*)ALLOC((uInt)zi->ci.size_centralheader);
	ziplocal_putValue_inmemory(zi->ci.central_header, (uLong)CENTRALHEADERMAGIC, 4);
	ziplocal_putValue_inmemory(zi->ci.central_header + 4, (uLong)VERSIONMADEBY, 2);
	ziplocal_putValue_inmemory(zi->ci.central_header + 6, (uLong)20, 2);
	ziplocal_putValue_inmemory(zi->ci.central_header + 8, (uLong)zi->ci.flag, 2);
	ziplocal_putValue_inmemory(zi->ci.central_header + 10, (uLong)zi->ci.method, 2);
	ziplocal_putValue_inmemory(zi->ci.central_header + 12, (uLong)zi->ci.dosDate, 4);
	ziplocal_putValue_inmemory(zi->ci.central_header + 16, (uLong)0, 4); /*crc*/
	ziplocal_putValue_inmemory(zi->ci.central_header + 20, (uLong)0, 4); /*compr size*/
	ziplocal_putValue_inmemory(zi->ci.central_header + 24, (uLong)0, 4); /*uncompr size*/
	ziplocal_putValue_inmemory(zi->ci.central_header + 28, (uLong)size_filename, 2);
	ziplocal_putValue_inmemory(zi->ci.central_header + 30, (uLong)0, 2);
	ziplocal_putValue_inmemory(zi->ci.central_header + 32, (uLong)0, 2);
	ziplocal_putValue_inmemory(zi->ci.central_header + 34, (uLong)0, 2); /*disk nm start*/
	ziplocal_putValue_inmemory(zi->ci.central_header + 36, fa, 2);
	ziplocal_putValue_inmemory(zi->ci.central_header + 38, fa, 4);
	ziplocal_putValue_inmemory(zi->ci.central_header + 42, (uLong)zi->ci.pos_local_header - zi->add_position_when_writting_offset, 4);
	for(i = 0; i < size_filename; i++) *(zi->ci.central_header + SIZECENTRALHEADER + i) = *(filename + i);
	if(!zi->ci.central_header) return ZIP_INTERNALERROR;
	err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)LOCALHEADERMAGIC, 4);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)20, 2);/* version needed to extract */
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)zi->ci.flag, 2);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)zi->ci.method, 2);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)zi->ci.dosDate, 4);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)0, 4); /* crc 32, unknown */
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)0, 4); /* compressed size, unknown */
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)0, 4); /* uncompressed size, unknown */
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)size_filename, 2);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)0, 2);
	if(err == ZIP_OK && size_filename > 0) if(ZWRITE(zi->z_filefunc, zi->filestream, filename, size_filename) != size_filename) err = ZIP_ERRNO;
	zi->ci.stream.avail_in = (uInt)0;
	zi->ci.stream.avail_out = (uInt)Z_BUFSIZE;
	zi->ci.stream.next_out = zi->ci.buffered_data;
	zi->ci.stream.total_in = 0;
	zi->ci.stream.total_out = 0;
	if((err == ZIP_OK) && (zi->ci.method == Z_DEFLATED) && (!zi->ci.raw)) {
		zi->ci.stream.zalloc = (alloc_func)0;
		zi->ci.stream.zfree = (free_func)0;
		zi->ci.stream.opaque = (voidpf)0;
		if(windowBits > 0) windowBits = -windowBits;
		err = deflateInit2(&zi->ci.stream, level, Z_DEFLATED, windowBits, memLevel, strategy);
		if(err == Z_OK) zi->ci.stream_initialised = 1;
	}
#ifndef NOCRYPT
	zi->ci.crypt_header_size = 0;
	if(err == Z_OK && password) {
		unsigned char bufHead[RAND_HEAD_LEN];
		unsigned int sizeHead;
		zi->ci.encrypt = 1;
		zi->ci.pcrc_32_tab = (const unsigned long*)get_crc_table();
		sizeHead = crypthead(password, bufHead, RAND_HEAD_LEN, zi->ci.keys, zi->ci.pcrc_32_tab, crcForCrypting);
		zi->ci.crypt_header_size = sizeHead;
		if(ZWRITE(zi->z_filefunc, zi->filestream, bufHead, sizeHead) != sizeHead) err = ZIP_ERRNO;
	}
#endif
	if(err != Z_OK) return err;

	zi->in_opened_file_inzip = 1;
	zi->ci.stream.next_in = (Bytef*)buf;
	zi->ci.stream.avail_in = len;
	zi->ci.crc32 = crc32(zi->ci.crc32, (const Bytef*)buf, len);
	while((err == ZIP_OK) && (zi->ci.stream.avail_in > 0)) {
		if(zi->ci.stream.avail_out == 0) {
			if(zipFlushWriteBuffer(zi) == ZIP_ERRNO) err = ZIP_ERRNO;
			zi->ci.stream.avail_out = (uInt)Z_BUFSIZE;
			zi->ci.stream.next_out = zi->ci.buffered_data;
		}
		if(err != ZIP_OK) break;
		if((zi->ci.method == Z_DEFLATED) && (!zi->ci.raw)) {
			uLong uTotalOutBefore = zi->ci.stream.total_out;
			err = deflate(&zi->ci.stream, Z_NO_FLUSH);
			zi->ci.pos_in_buffered_data += (uInt)(zi->ci.stream.total_out - uTotalOutBefore);
		} else {
			uInt copy_this;
			copy_this = (zi->ci.stream.avail_in < zi->ci.stream.avail_out ? zi->ci.stream.avail_in : zi->ci.stream.avail_out);
			for(i = 0; i < copy_this; i++) *(((char*)zi->ci.stream.next_out) + i) = *(((const char*)zi->ci.stream.next_in) + i);
			zi->ci.stream.avail_in -= copy_this;
			zi->ci.stream.avail_out -= copy_this;
			zi->ci.stream.next_in += copy_this;
			zi->ci.stream.next_out += copy_this;
			zi->ci.stream.total_in += copy_this;
			zi->ci.stream.total_out += copy_this;
			zi->ci.pos_in_buffered_data += copy_this;
		}
	}
	return zipCloseFile(file, 0, 0);
}

extern int ZEXPORT zipAddFile(zipFile file, const char* filename, int method, int level, const char* password,
							  int raw, int windowBits, int memLevel, int strategy, uLong crcForCrypting) {
	int err = ZIP_BADFILE;
	u32 mode(S_IREAD);
#ifdef WIN32
	mode |= _O_BINARY;
#endif
	auto fl(openf(filename, O_RDONLY, mode));
	if(fl) {
		struct stat st;
		fstat(fl, &st);
		auto len((int)st.st_size);
		auto buf(new unsigned char[len]);
		if(readf(fl, buf, len) == len) {
			auto uf(strrchr(filename, '/'));
			filename = uf ? uf + 1 : filename;
			auto wf(strrchr(filename, '\\'));
			filename = wf ? wf + 1 : filename;
			err = zipAddMem(file, filename, buf, len, method, level, password, fl, raw, windowBits, memLevel, strategy, crcForCrypting);
		}
		delete[] buf;
		closef(fl);
	};
	return err;
}

extern int ZEXPORT zipClose(zipFile file) {
	zip_internal* zi;
	int err = 0;
	uLong size_centraldir = 0, centraldir_pos_inzip;
	if(!file) return ZIP_PARAMERROR;
	zi = (zip_internal*)file;
	if(zi->in_opened_file_inzip == 1) err = zipCloseFile(file, 0, 0);
	centraldir_pos_inzip = ZTELL(zi->z_filefunc, zi->filestream);
	if(err == ZIP_OK) {
		linkedlist_datablock_internal* ldi = zi->central_dir.first_block;
		while(ldi) {
			if((err == ZIP_OK) && (ldi->filled_in_this_block > 0))
				if(ZWRITE(zi->z_filefunc, zi->filestream, ldi->data, ldi->filled_in_this_block) != ldi->filled_in_this_block) err = ZIP_ERRNO;
			size_centraldir += ldi->filled_in_this_block;
			ldi = ldi->next_datablock;
		}
	}
	free_datablock(zi->central_dir.first_block);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)ENDHEADERMAGIC, 4);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)0, 2);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)0, 2);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)zi->number_entry, 2);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)zi->number_entry, 2);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)size_centraldir, 4);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)(centraldir_pos_inzip - zi->add_position_when_writting_offset), 4);
	if(err == ZIP_OK) err = ziplocal_putValue(&zi->z_filefunc, zi->filestream, (uLong)0, 2);
	if(ZCLOSE(zi->z_filefunc, zi->filestream) != 0) if(err == ZIP_OK) err = ZIP_ERRNO;
#ifndef NO_ADDFILEINEXISTINGZIP
	TRYFREE(zi->globalcomment);
#endif
	TRYFREE(zi);
	return err;
}
