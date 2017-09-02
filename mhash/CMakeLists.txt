include_directories(include)
include_directories(lib)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

if(WIN32)
	configure_file (include/mutils/mhash_config.win32.h include/mutils/mhash_config.h COPYONLY)
else()
	# ad-hoc
	configure_file (include/mutils/mhash_config.linux.h include/mutils/mhash_config.h COPYONLY)
endif()

set (SOURCES
	lib/adler32.c
	lib/crc32.c
	lib/gosthash.c
	lib/haval.c
	lib/keygen.c
	lib/keygen.h
	lib/keygen_asis.c
	lib/keygen_hex.c
	lib/keygen_mcrypt.c
	lib/keygen_s2k.c
	lib/md2.c
	lib/md4.c
	lib/md5.c
	lib/mhash.c
	lib/ripemd.c
	lib/sha1.c
	lib/sha256.c
	lib/sha256_sha224.c
	lib/sha512_sha384.c
	lib/snefru.c
	lib/stdfns.c
	lib/tiger.c
	lib/tiger_sboxes.c
	lib/whirlpool.c
)

set (HEADERS
	include/mutils/mglobal.h
	include/mutils/mhash.h
	${CMAKE_CURRENT_BINARY_DIR}/include/mutils/mhash_config.h
	include/mutils/mincludes.h
	include/mutils/mtypes.h
	include/mutils/mutils.h
	lib/libdefs.h
	lib/mhash_adler32.h
	lib/mhash_crc32.h
	lib/mhash_gost.h
	lib/mhash_haval.h
	lib/mhash_int.h
	lib/mhash_md2.h
	lib/mhash_md4.h
	lib/mhash_md5.h
	lib/mhash_ripemd.h
	lib/mhash_sha1.h
	lib/mhash_sha256.h
	lib/mhash_sha256_sha224.h
	lib/mhash_sha512_sha384.h
	lib/mhash_snefru.h
	lib/mhash_tiger.h
	lib/mhash_whirlpool.h
)

add_library(mhash STATIC ${SOURCES} ${HEADERS})
