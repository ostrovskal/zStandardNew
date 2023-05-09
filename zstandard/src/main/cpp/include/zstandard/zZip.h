    
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

#if defined(STRICTZIP) || defined(STRICTZIPUNZIP)
    typedef struct TagzipFile__ { int unused; } zipFile__;
    typedef zipFile__ *zipFile;
#else
    typedef void* zipFile;
#endif

#define ZIP_OK                          (0)
#define ZIP_EOF                         (0)
#define ZIP_ERRNO                       (Z_ERRNO)
#define ZIP_PARAMERROR                  (-102)
#define ZIP_BADZIPFILE                  (-103)
#define ZIP_INTERNALERROR               (-104)
#define ZIP_BADFILE                     (-105)

#ifndef DEF_MEM_LEVEL
#  if MAX_MEM_LEVEL >= 8
#    define DEF_MEM_LEVEL 8
#  else
#    define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#  endif
#endif

#define APPEND_STATUS_CREATE        (0)
#define APPEND_STATUS_CREATEAFTER   (1)
#define APPEND_STATUS_ADDINZIP      (2)

extern HZIP* ZEXPORT zipOpen(cstr pathname, int append, cstr* globalcomment = nullptr, zlib_filefunc_def* pzlib_filefunc_def = nullptr);
extern int ZEXPORT zipAddFile(zipFile file, cstr filename, int method = Z_DEFLATED, int level = Z_BEST_COMPRESSION, cstr password = nullptr,
                              int raw = 0, int windowBits = -MAX_WBITS, int memLevel = DEF_MEM_LEVEL,
                              int strategy = Z_DEFAULT_STRATEGY, uLong crcForCtypting = 0);
extern int ZEXPORT zipAddMem(zipFile file, cstr filename, const void* buf, unsigned len, int method = Z_DEFLATED, int level = Z_BEST_COMPRESSION, cstr password = nullptr,
                             int fd = 0, int raw = 0, int windowBits = -MAX_WBITS, int memLevel = DEF_MEM_LEVEL,
                             int strategy = Z_DEFAULT_STRATEGY, uLong crcForCtypting = 0);
extern int ZEXPORT zipCloseFile(zipFile file, uLong uncompressed_size, uLong crc32);
extern int ZEXPORT zipClose(zipFile file);

#ifdef __cplusplus
}
#endif
