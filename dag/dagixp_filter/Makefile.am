lib_LTLIBRARIES = libdagixp_filter.la

libdagixp_filter_la_CPPFLAGS = \
	-I${top_builddir}/include\
	-I${srcdir} \
	-I${top_srcdir}/include \
	-I${top_srcdir}/include/dagixp_filter \
	-I${top_srcdir}/include/adt \
	-I${srcdir}/cards
libdagixp_filter_la_CFLAGS = @DAG_CFLAGS@ @BYTESWAP@ @LARGEFILE@
libdagixp_filter_la_SOURCES = \
	cards/d71s_feature.c \
	cards/d71s_filter.h \
	cards/d71s_load.c \
	df_construct.c \
	df_feature.c \
	df_init.c \
	df_internal.h \
	df_load.c \
	df_meta.c \
	df_types.h \
	df_util.c
 
libdagixp_filter_la_LIBADD = ${top_builddir}/lib/adt/libadt.la
 

libdagixp_filter_la_LDFLAGS = -version-number @DAG_SO_VERSION@
