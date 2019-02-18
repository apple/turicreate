#ifndef cm_zlib_mangle_h
#define cm_zlib_mangle_h

/*

This header file mangles all symbols exported from the zlib library.
It is included in all files while building the zlib library.  Due to
namespace pollution, no zlib headers should be included in .h files in
cm.

The following command was used to obtain the symbol list:

nm libcmzlib.so |grep " [TRD] "

This is the way to recreate the whole list:

nm libcmzlib.so |grep " [TRD] " | awk '{ print "#define "$3" cm_zlib_"$3 }'

REMOVE the "_init" and "_fini" entries.

*/

#define adler32 cm_zlib_adler32
#define adler32_combine cm_zlib_adler32_combine
#define compress cm_zlib_compress
#define compress2 cm_zlib_compress2
#define compressBound cm_zlib_compressBound
#define crc32 cm_zlib_crc32
#define crc32_combine cm_zlib_crc32_combine
#define get_crc_table cm_zlib_get_crc_table
#define deflate cm_zlib_deflate
#define deflateBound cm_zlib_deflateBound
#define deflateCopy cm_zlib_deflateCopy
#define deflateEnd cm_zlib_deflateEnd
#define deflateInit2_ cm_zlib_deflateInit2_
#define deflateInit_ cm_zlib_deflateInit_
#define deflateParams cm_zlib_deflateParams
#define deflatePrime cm_zlib_deflatePrime
#define deflateReset cm_zlib_deflateReset
#define deflateSetDictionary cm_zlib_deflateSetDictionary
#define deflateSetHeader cm_zlib_deflateSetHeader
#define deflateTune cm_zlib_deflateTune
#define deflate_copyright cm_zlib_deflate_copyright
#define gzclearerr cm_zlib_gzclearerr
#define gzclose cm_zlib_gzclose
#define gzdirect cm_zlib_gzdirect
#define gzdopen cm_zlib_gzdopen
#define gzeof cm_zlib_gzeof
#define gzerror cm_zlib_gzerror
#define gzflush cm_zlib_gzflush
#define gzgetc cm_zlib_gzgetc
#define gzgets cm_zlib_gzgets
#define gzopen cm_zlib_gzopen
#define gzprintf cm_zlib_gzprintf
#define gzputc cm_zlib_gzputc
#define gzputs cm_zlib_gzputs
#define gzread cm_zlib_gzread
#define gzrewind cm_zlib_gzrewind
#define gzseek cm_zlib_gzseek
#define gzsetparams cm_zlib_gzsetparams
#define gztell cm_zlib_gztell
#define gzungetc cm_zlib_gzungetc
#define gzwrite cm_zlib_gzwrite
#define inflate_fast cm_zlib_inflate_fast
#define inflate cm_zlib_inflate
#define inflateCopy cm_zlib_inflateCopy
#define inflateEnd cm_zlib_inflateEnd
#define inflateGetHeader cm_zlib_inflateGetHeader
#define inflateInit2_ cm_zlib_inflateInit2_
#define inflateInit_ cm_zlib_inflateInit_
#define inflatePrime cm_zlib_inflatePrime
#define inflateReset cm_zlib_inflateReset
#define inflateSetDictionary cm_zlib_inflateSetDictionary
#define inflateSync cm_zlib_inflateSync
#define inflateSyncPoint cm_zlib_inflateSyncPoint
#define inflate_copyright cm_zlib_inflate_copyright
#define inflate_table cm_zlib_inflate_table
#define _dist_code cm_zlib__dist_code
#define _length_code cm_zlib__length_code
#define _tr_align cm_zlib__tr_align
#define _tr_flush_block cm_zlib__tr_flush_block
#define _tr_init cm_zlib__tr_init
#define _tr_stored_block cm_zlib__tr_stored_block
#define _tr_tally cm_zlib__tr_tally
#define uncompress cm_zlib_uncompress
#define zError cm_zlib_zError
#define z_errmsg cm_zlib_z_errmsg
#define zcalloc cm_zlib_zcalloc
#define zcfree cm_zlib_zcfree
#define zlibCompileFlags cm_zlib_zlibCompileFlags
#define zlibVersion cm_zlib_zlibVersion

#endif
