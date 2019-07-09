cmake_minimum_required(VERSION 3.6)
project(libunrar)

set (unrar_SOURCES
		strlist.cpp
		strfn.cpp
		pathfn.cpp
		smallfn.cpp
		global.cpp
		file.cpp
		filefn.cpp
		filcreat.cpp
		archive.cpp
		arcread.cpp
		arcmem.cpp
		unicode.cpp
		system.cpp
		isnt.cpp
		crypt.cpp
		crc.cpp
		rawread.cpp
		encname.cpp
		match.cpp
		timefn.cpp
		threadpool.cpp
		rdwrfn.cpp
		consio.cpp
		options.cpp
		errhnd.cpp
		rarvm.cpp
		secpassword.cpp
		rijndael.cpp
		getbits.cpp
		sha1.cpp
		sha256.cpp
		blake2s.cpp
		hash.cpp
		extinfo.cpp
		extract.cpp
		volume.cpp
		list.cpp
		find.cpp
		unpack.cpp
#		unpack15.cpp
#		unpack20.cpp
		unpack30.cpp
		unpack50.cpp
		unpack50frag.cpp
		unpack50mt.cpp
		headers.cpp
		rs16.cpp
		ui.cpp
		ulinks.cpp
		cmddata.cpp
		filestr.cpp
		scantree.cpp
		dll.cpp
		qopen.cpp)

#set(LOCAL_MODULE_TAGS eng debug)
#set(LOCAL_LDLAGS "${LOCAL_LDLAGS} -pie -fPIE")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRARDLL -DLITTLE_ENDIAN -DRAR_SMP -fPIC -fexceptions -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -UALLOW_EXCEPTIONS ")
add_library(static_unrar STATIC
            ${unrar_SOURCES})

set(CMAKE_CXX_STANDARD 11)
target_link_libraries(static_unrar)
