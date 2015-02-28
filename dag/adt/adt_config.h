/* ------------------------------------------------------------------------------
 adt_config from Koryn's Units.

 Configuration file for cross-platform work.

 Copyright (c) 2001-2005, Koryn Grant <koryn@clear.net.nz>.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 *	Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.
 *	Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution.
 *	Neither the name of the author nor the names of other contributors
	may be used to endorse or promote products derived from this software without
	specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
------------------------------------------------------------------------------ */

#ifndef ADT_CONFIG_H
#define ADT_CONFIG_H

/* Sets of orthogonal options:
	feature flags (derived from TARGET_XXX):
	POLL_SELECT -- set to 1 if using Cygwin networking rather than WinSock.
	USE_POSIX_THREADS -- use POSIX pthreads.
	PROVIDE_DLFCN -- provide an interface for the POSIX dlfcn.h routines.
	USE_MACOSX_DLFCN -- provide an implementation of the POSIX dlfcn.h for Mac OS X systems.
	USE_WIN_DLFCN -- provide an implementation of the POSIX dlfcn.h for Windows systems.
	USE_WIN_FILE_IO -- use Windows File i/o calls (CreateFile, ReadFile, WriteFile, GetFileSize, CloseHandle).
	USE_WINSOCK -- use WinSock rather than POSIX sockets.
	USE_WIN_SLEEP -- use Windows' Sleep().
	USE_WIN_TIME -- use Windows' GetTickCount() for millisecond system timer.
	USE_WIN_THREADS -- use Windows' threads.
	USE_MAC_DEBUGSTR -- in assertions, use DebugStr rather than fprintf().
	USE_MAC_FSSPEC -- use Mac OS 9 FSSpecs rather than POSIX file descriptors.
	USE_MACOS9_HANDLES -- use Handles and the NewPtr()/DisposePtr() calls rather than malloc()/free().
 */

#if !defined(__cygwin__) && !defined(__linux__) && !defined(__FreeBSD__) && !(defined(__APPLE__) && defined(__ppc__)) && !defined(_WIN32)
/* TARGET wasn't set. */
/* Default values (POSIX). */
#define POLL_SELECT 0
#define USE_MAC_DEBUGSTR 0
#define USE_MAC_FSSPEC 0
#define USE_MACOS9_HANDLES 0
#define USE_WINSOCK 0
#define USE_WIN_SLEEP 0
#define USE_WIN_THREADS 0
#define USE_WIN_TIME 0
#define USE_POSIX_THREADS 1
#define USE_MACOSX_DLFCN 0
#define USE_WIN_DLFCN 0
#define USE_POSIX_FILE_IO 1
#define USE_WIN_FILE_IO 0
#endif /* Default values (POSIX). */


#if defined(__cygwin__)
#define USE_MAC_DEBUGSTR 0
#define USE_MAC_FSSPEC 0
#define USE_MACOS9_HANDLES 0
#define USE_WINSOCK 1
#define USE_WIN_SLEEP 0
#define USE_WIN_THREADS 1
#define USE_WIN_TIME 1
#define USE_POSIX_THREADS 0
#define USE_MACOSX_DLFCN 0
#define USE_WIN_DLFCN 1 /* Cygwin has its own dlfcn.h -> LoadLibrary() et al mapping, or we can use Win32 directly. */
#define USE_POSIX_FILE_IO 1
#define USE_WIN_FILE_IO 0 /* Cygwin has its own POSIX file i/o -> CreateFile() et al mapping, or we can use Win32 directly. */

/* POLL_SELECT is purely to work around problems with select() in Cygwin's sockets API, and is not necessary when using WinSock. */
#if USE_WINSOCK
#define POLL_SELECT 0
#else
#define POLL_SELECT 1
#endif /* USE_WINSOCK */
#endif /* __cygwin__ */


#if defined(__FreeBSD__)
#define POLL_SELECT 0
#define USE_MAC_DEBUGSTR 0
#define USE_MAC_FSSPEC 0
#define USE_MACOS9_HANDLES 0
#define USE_WINSOCK 0
#define USE_WIN_SLEEP 0
#define USE_WIN_THREADS 0
#define USE_WIN_TIME 0
#define USE_POSIX_THREADS 1
#define USE_MACOSX_DLFCN 0
#define USE_WIN_DLFCN 0
#define USE_POSIX_FILE_IO 1
#define USE_WIN_FILE_IO 0
#endif /* __FreeBSD__ */


#if defined(__linux__)
#define POLL_SELECT 0
#define USE_MAC_DEBUGSTR 0
#define USE_MAC_FSSPEC 0
#define USE_MACOS9_HANDLES 0
#define USE_WINSOCK 0
#define USE_WIN_SLEEP 0
#define USE_WIN_THREADS 0
#define USE_WIN_TIME 0
#define USE_POSIX_THREADS 1
#define USE_MACOSX_DLFCN 0
#define USE_WIN_DLFCN 0
#define USE_POSIX_FILE_IO 1
#define USE_WIN_FILE_IO 0
#endif /* __linux__ */


#ifdef TARGET_MACOS9
#define POLL_SELECT 0
#define USE_MAC_DEBUGSTR 1
#define USE_MAC_FSSPEC 1
#define USE_MACOS9_HANDLES 1
#define USE_WINSOCK 0
#define USE_WIN_SLEEP 0
#define USE_WIN_THREADS 0
#define USE_POSIX_THREADS 0
#define USE_MACOSX_DLFCN 0
#define USE_WIN_DLFCN 0
#define USE_POSIX_FILE_IO 0
#define USE_WIN_FILE_IO 0
#endif /* TARGET_MACOS9 */


#if (defined(__APPLE__) && defined(__ppc__))
/* Mac OS X 10.3 and greater have socklen_t */
#define POLL_SELECT 0
#define USE_MAC_DEBUGSTR 0
#define USE_MAC_FSSPEC 0
#define USE_MACOS9_HANDLES 0
#define USE_WINSOCK 0
#define USE_WIN_SLEEP 0
#define USE_WIN_THREADS 0
#define USE_WIN_TIME 0
#define USE_POSIX_THREADS 1
#define USE_MACOSX_DLFCN 1
#define USE_WIN_DLFCN 0
#define USE_POSIX_FILE_IO 1
#define USE_WIN_FILE_IO 0
#endif /* TARGET_MACOSX */


#if defined(_WIN32)
#define POLL_SELECT 0
#define USE_MAC_DEBUGSTR 0
#define USE_MAC_FSSPEC 0
#define USE_MACOS9_HANDLES 0
#define USE_WINSOCK 1
#define USE_WIN_SLEEP 1
#define USE_WIN_THREADS 1
#define USE_WIN_TIME 1
#define USE_POSIX_THREADS 0
#define USE_MACOSX_DLFCN 0
#define USE_WIN_DLFCN 1
#define USE_POSIX_FILE_IO 0
#define USE_WIN_FILE_IO 1
#endif /* _WIN32 */


/* Set some derived flags. */
#if (USE_MACOSX_DLFCN || USE_WIN_DLFCN)
#define PROVIDE_DLFCN 1
#else
#define PROVIDE_DLFCN 0
#endif

/* Sanity checking for mutually exclusive flags. */
#if (USE_WIN_DLFCN && USE_MACOSX_DLFCN)
#error Cannot have both USE_WIN_DLFCN and USE_MACOSX_DLFCN set.
#endif

#if (USE_WIN_THREADS && USE_POSIX_THREADS)
#error Cannot have both USE_WIN_THREADS and USE_POSIX_THREADS set.
#endif

#if (USE_WIN_FILE_IO && USE_POSIX_FILE_IO)
#error Cannot have both USE_WIN_FILE_IO and USE_POSIX_FILE_IO set.
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


	const char* adt_config_string(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_CONFIG_H */
