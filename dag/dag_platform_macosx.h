/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag_platform_macosx.h,v 1.12 2007/02/12 03:12:40 sfd Exp $
 */

#ifndef DAG_PLATFORM_MACOSX_H
#define DAG_PLATFORM_MACOSX_H

#if (defined(__APPLE__) && defined(__ppc__))

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif /* HAVE_CONFIG_H */

/* CAUTION: Mac OS X is much pickier about header file ordering than Linux! */

/* POSIX headers. */
#include <sys/types.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/un.h>
#include <regex.h>
#include <semaphore.h>
#include <syslog.h>

/* C Standard Library headers. */
#include <inttypes.h>
#include <stdbool.h>


#ifndef PRIu64
#define PRIu64 "llu"
#endif /* PRIu64 */

#ifndef PRId64
#define PRId64 "lld"
#endif /* PRId64 */

#ifndef PRIx64
#define PRIx64 "llx"
#endif /* PRIx64 */

#ifndef INLINE
#define INLINE inline
#endif /* INLINE */


/* libedit header default. */
#ifndef HAVE_EDITLINE
#define HAVE_EDITLINE 0
#endif /* HAVE_EDITLINE */


/* Byteswap code. */
#if defined(BYTESWAP)
#include <byteswap.h>
#else



static inline uint64_t bswap_64 (uint64_t x) {
return (((x & 0x00000000000000ffll) << 56) |
		((x & 0x000000000000ff00ll) << 40) |
		((x & 0x0000000000ff0000ll) << 24) |
		((x & 0x00000000ff000000ll) <<  8) |
		((x & 0x000000ff00000000ll) >>  8) |
		((x & 0x0000ff0000000000ll) >> 24) |
		((x & 0x00ff000000000000ll) >> 40) |
		((x & 0xff00000000000000ll) >> 56));
}

static inline uint32_t bswap_32 (uint32_t x) {
return (((x & 0x000000ffll) << 24) |
		((x & 0x0000ff00ll) << 8) |
		((x & 0x00ff0000ll) >> 8) |
		((x & 0xff000000ll) >> 24));
}



#endif /* BYTESWAP */

/* Check IP checksum (for IP packets). */
#define IN_CHKSUM(IP) ip_sum_calc_macosx((uint8_t *)(IP))


/* Routines. */
uint16_t ip_sum_calc_macosx(uint8_t* buff);


#endif /* Mac OS X */



#endif /* DAG_PLATFORM_MACOSX_H */ 
