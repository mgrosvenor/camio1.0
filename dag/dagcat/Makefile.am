lib_LTLIBRARIES = libdagcat.la

libdagcat_la_CPPFLAGS = -I${top_builddir}/include -I${top_srcdir}/include -I${top_srcdir}/include/dagcat -I${top_srcdir}/include/adt
libdagcat_la_CFLAGS = @DAG_CFLAGS@ @BYTESWAP@ @LARGEFILE@ 
libdagcat_la_SOURCES = \
	cat_interface.c

libdagcat_la_LIBADD = ${top_builddir}/lib/libdag.la \
		       ${top_builddir}/lib/libdagconf/libdagconf.la	


libdagcat_la_LDFLAGS = -version-number @DAG_SO_VERSION@
