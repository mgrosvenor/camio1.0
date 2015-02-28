/* File header. */
#include "adt_config.h"

/* ADT headers. */
#include "adt_debugging.h"
#include "adt_memory.h"

/* C Standard Library. */
#include <string.h>


static char* uConfigString = NULL;
static char* uConfigStrings[] = 
{
#if HAVE_DLFCN_H
	"HAVE_DLFCN_H=1;",
#else
	"HAVE_DLFCN_H=0;",
#endif

#if HAVE_POLL
	"HAVE_POLL=1;",
#else
	"HAVE_POLL=0;",
#endif

#if HAVE_SOCKLEN_T
	"HAVE_SOCKLEN_T=1;",
#else
	"HAVE_SOCKLEN_T=0;",
#endif

#if HAVE_SSIZE_T
	"HAVE_SSIZE_T=1;",
#else
	"HAVE_SSIZE_T=0;",
#endif

#if HAVE_UNISTD_H
	"HAVE_UNISTD_H=1;",
#else
	"HAVE_UNISTD_H=0;",
#endif

#if POLL_SELECT
	"POLL_SELECT=1;",
#else
	"POLL_SELECT=0;",
#endif

#if USE_MAC_DEBUGSTR
	"USE_MAC_DEBUGSTR=1;",
#else
	"USE_MAC_DEBUGSTR=0;",
#endif

#if USE_MAC_FSSPEC
	"USE_MAC_FSSPEC=1;",
#else
	"USE_MAC_FSSPEC=0;",
#endif

#if USE_MACOS9_HANDLES
	"USE_MACOS9_HANDLES=1;",
#else
	"USE_MACOS9_HANDLES=0;",
#endif

#if USE_WINSOCK
	"USE_WINSOCK=1;",
#else
	"USE_WINSOCK=0;",
#endif

#if USE_WIN_SLEEP
	"USE_WIN_SLEEP=1;",
#else
	"USE_WIN_SLEEP=0;",
#endif

#if USE_WIN_THREADS
	"USE_WIN_THREADS=1;",
#else
	"USE_WIN_THREADS=0;",
#endif

#if USE_POSIX_THREADS
	"USE_POSIX_THREADS=1;",
#else
	"USE_POSIX_THREADS=0;",
#endif

#if PROCESSOR_PPC
	"PROCESSOR_PPC;",
#else
	"PROCESSOR_X86;",
#endif

#if HAS_SYS_TIMES_H
	"HAS_SYS_TIMES_H=1;",
#else
	"HAS_SYS_TIMES_H=0;",
#endif

#if USE_WIN_TIME
	"USE_WIN_TIME=1;",
#else
	"USE_WIN_TIME=0;",
#endif

#if HAS_SYS_TIME_H
	"HAS_SYS_TIME_H=1;",
#else
	"HAS_SYS_TIME_H=0;",
#endif

#if USE_MACOSX_DLFCN
	"USE_MACOSX_DLFCN=1;",
#else
	"USE_MACOSX_DLFCN=0;",
#endif

#if USE_WIN_DLFCN
	"USE_WIN_DLFCN=1;",
#else
	"USE_WIN_DLFCN=0;",
#endif

#if PROVIDE_DLFCN
	"PROVIDE_DLFCN=1;",
#else
	"PROVIDE_DLFCN=0;",
#endif

#if USE_POSIX_FILE_IO
	"USE_POSIX_FILE_IO=1;",
#else
	"USE_POSIX_FILE_IO=0;",
#endif

#if USE_WIN_FILE_IO
	"USE_WIN_FILE_IO=1;"
#else
	"USE_WIN_FILE_IO=0;"
#endif
};


const char*
adt_config_string(void)
{
	if (!uConfigString)
	{
		size_t       length;
		unsigned int index;
		size_t       offset;
		unsigned int count = sizeof(uConfigStrings) / sizeof(uConfigStrings[0]);

		length = 0;
		for (index = 0; index < count; index++)
		{
			length += strlen(uConfigStrings[index]);
		}

		uConfigString = ADT_XALLOCATE(length + 1);

		offset = 0;
		for (index = 0; index < count; index++)
		{
			strncpy(&uConfigString[offset], uConfigStrings[index], length - offset);
			offset += strlen(uConfigStrings[index]);
		}

		uConfigString[length] = '\0';
	}

	return (const char*) uConfigString;
}
