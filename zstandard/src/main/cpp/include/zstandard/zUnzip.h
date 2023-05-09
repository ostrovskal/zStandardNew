
#pragma once

#ifdef __cplusplus
    extern "C" {
#endif

#ifndef _ZLIB_H
    #include <zip/zlib.h>
#endif

#ifndef _ZLIBIOAPI_H
    #include "zIOfile.h"
#endif

#if defined(STRICTUNZIP) || defined(STRICTZIPUNZIP)
    typedef struct TagunzFile__ { int unused; } unzFile__;
    typedef unzFile__ *unzFile;
#else
    typedef void* unzFile;
#endif

#define UNZ_OK                          (0)
#define UNZ_END_OF_LIST_OF_FILE         (-100)
#define UNZ_ERRNO                       (Z_ERRNO)
#define UNZ_EOF                         (0)
#define UNZ_PARAMERROR                  (-102)
#define UNZ_BADZIPFILE                  (-103)
#define UNZ_INTERNALERROR               (-104)
#define UNZ_CRCERROR                    (-105)

typedef struct unz_global_info_s {
    uLong number_entry;
    uLong size_comment;
} unz_global_info;

typedef struct unz_file_info_s {
    uLong version;
    uLong version_needed;
    uLong flag;
    uLong cmethod;
    uLong dosDate;
    uLong crc;
    uLong csize;
    uLong usize;
    uLong sfilename;
    uLong sextra;
    uLong scomment;
    uLong disk_num_start;
    uLong attr;
    uLong mtime;
    char name[260];
} unz_file_info;

extern HZIP* ZEXPORT unzOpen(cstr path, zlib_filefunc_def* pzlib_filefunc_def = nullptr);
extern int ZEXPORT unzClose(unzFile file);
extern int ZEXPORT unzGoToFirstFile(unzFile file, unz_file_info* info);
extern int ZEXPORT unzGoToNextFile(unzFile file, unz_file_info* info);
extern int ZEXPORT unzLocateFile(unzFile file, unz_file_info* info, int index, cstr szFileName, int iCaseSensitivity);
extern int ZEXPORT unzCountFiles(unzFile file);
extern int ZEXPORT unzFileInfo(unzFile file, unz_file_info *info);
extern int ZEXPORT unzUnzipToFile(unzFile file, int index, cstr path, cstr password = nullptr);
extern int ZEXPORT unzUnzipToMem(unzFile file, int index, u8** ptr, unsigned* size, cstr password = nullptr);

#ifdef __cplusplus
}
#endif
