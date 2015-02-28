/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag_platform_solaris.h,v 1.14 2007/02/12 03:12:40 sfd Exp $
 */

#ifndef DAG_PLATFORM_SOLARIS_H
#define DAG_PLATFORM_SOLARIS_H

#if defined(__SVR4) && defined (__sun)

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif /* HAVE_CONFIG_H */

/* Solaris headers. */
#include <sys/ddi.h>

/* POSIX headers. */
#include <net/if.h>
#include <sys/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/un.h>
#include <regex.h>
#include <pthread.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <syslog.h>


/* C Standard Library headers. */
#include <inttypes.h>
#include <stdbool.h>


#ifndef INLINE
#define INLINE inline
#endif /* INLINE */

#ifndef PRIu64
#define PRIu64 "llu"
#endif /* PRIu64 */

#ifndef PRId64
#define PRId64 "lld"
#endif /* PRId64 */

#ifndef PRIx64
#define PRIx64 "llx"
#endif /* PRIx64 */

#define timersub(a, b, result)                            \
  do {                                                    \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;         \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;      \
    if ((result)->tv_usec < 0) {                          \
      --(result)->tv_sec;                                 \
      (result)->tv_usec += 1000000;                       \
    }                                                     \
  } while (0)


/* libedit header default. */
#ifndef HAVE_EDITLINE
#define HAVE_EDITLINE 0
#endif /* HAVE_EDITLINE */


/* Byteswap code. */
#if defined(BYTESWAP)
#include <byteswap.h>
#else

static inline uint64_t
bswap_64 (uint64_t x)
{
	return (((x & 0x00000000000000ffll) << 56) |
			((x & 0x000000000000ff00ll) << 40) |
			((x & 0x0000000000ff0000ll) << 24) |
			((x & 0x00000000ff000000ll) <<  8) |
			((x & 0x000000ff00000000ll) >>  8) |
			((x & 0x0000ff0000000000ll) >> 24) |
			((x & 0x00ff000000000000ll) >> 40) |
			((x & 0xff00000000000000ll) >> 56));
}	
#endif /* BYTESWAP */

/* Check IP checksum (for IP packets). */
#define IN_CHKSUM(IP) ip_sum_calc_solaris((uint8_t *)(IP))


/* Routines. */
uint16_t ip_sum_calc_solaris(uint8_t* buff);


#endif /* Solaris */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* DAG_PLATFORM_SOLARIS_H */ 
