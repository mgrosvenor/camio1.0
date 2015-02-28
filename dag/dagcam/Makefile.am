lib_LTLIBRARIES = libinfinicam.la

BUILT_SOURCES = bfs_filter_proto.c bfs_parse.h infinicamproto.c infini_parse.h ipf_v2_filter_proto.c ipf_v2_parse.h
#to be marked out 
#CLEANFILES = infinicamproto.c infini_parse.h *~
LEX_OUTPUT_ROOT=lex.infini
LFLAGS = --header-file="infini_parse.h"
ipf_v2_filter_proto.c: ipf_v2_filter_proto.l
		$(LEX) --header-file="ipf_v2_parse.h" ipf_v2_filter_proto.l 
		sed '/^#/ s|lex.ipf_v2.c|ipf_v2_filter_proto.c|' lex.ipf_v2.c >ipf_v2_filter_proto.c
		rm -f lex.ipf_v2.c

infinicamproto.c:infinicamproto.l 
		$(LEX) --header-file="infini_parse.h" infinicamproto.l 
		sed '/^#/ s|lex.infini.c|infinicamproto.c|' lex.infini.c >infinicamproto.c
		rm -f lex.infini.c

bfs_filter_proto.c:bfs_filter_proto.l 
		$(LEX) --header-file="bfs_parse.h" bfs_filter_proto.l 
		sed '/^#/ s|lex.bfs.c|bfs_filter_proto.c|' lex.bfs.c > bfs_filter_proto.c
		rm -f lex.bfs.c

#lex adds extern and include for some functions from unistd.h so -Wno-redundant-decls is added to fix a warning 
#other possible solution is to change the lex behaviour 

libinfinicam_la_CPPFLAGS = -I${top_builddir}/include -I${top_srcdir}/include -I${top_srcdir}/include/dagcam -I${top_srcdir}/include/unified_filter
libinfinicam_la_CFLAGS = @DAG_CFLAGS@ @BYTESWAP@ @LARGEFILE@ -Wno-redundant-decls
libinfinicam_la_SOURCES = \
	idt75k_lib.c \
	idt75k_wrapper.c \
	infini_parse.h \
	idt52k_lib.c \
	idt52k_wrapper.c \
	bfs_lib.c \
	bfs_wrapper.c \
	bfs_parse.h \
	bfs_filter_proto.l \
	infinicamproto.l \
	ipf_v2_parse.h \
	ipf_v2_filter_proto.l\
	filter_common.c \
	ipf_v2_filter_impl.c\
	infiniband_filter_impl.c \
	bfs_filter_impl.c 


libinfinicam_la_LIBADD = ${top_builddir}/lib/libdag.la  
#commented out to resolve cycling dependencies, Libdagconfig depends on this library
#			 ${top_builddir}/lib/libdagconf/libdagconf.la


libinfinicam_la_LDFLAGS = -version-number @DAG_SO_VERSION@

noinst_HEADERS = \
	filter_common.h \
	ipf_v2_filter_impl.h \
	infiniband_filter_impl.h \
	bfs_filter_impl.h
