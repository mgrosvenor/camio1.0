lib_LTLIBRARIES = libdagima.la

libdagima_la_CPPFLAGS = -I${top_builddir}/include -I${top_srcdir}/include -I${top_srcdir}/include/adt
libdagima_la_CFLAGS = @DAG_CFLAGS@ @BYTESWAP@ @LARGEFILE@ 
libdagima_la_SOURCES = dag_ima.c 

libdagima_la_LIBADD = ${top_builddir}/lib/dagema/libdagema.la

libdagima_la_LDFLAGS = -version-number @DAG_SO_VERSION@
