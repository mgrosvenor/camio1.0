/*
 * Copyright (c) 2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag_platform_netbsd.h,v 1.5 2007/02/12 03:12:40 sfd Exp $
 */

#ifndef DAG_PLATFORM_NETBSD_H
#define DAG_PLATFORM_NETBSD_H

#if defined(__NetBSD__)

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif /* HAVE_CONFIG_H */

/* POSIX headers. */
#include <sys/types.h>
#include <sys/socket.h>
#include <net/ethertypes.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/dlt.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/param.h>
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
#include <machine/endian.h>
#if defined(__byte_swap_int)
#define bswap_64(x) __bswap64(x)
#else
#define bswap_64(x)                               \
    (__extension__                                \
     ({ union { __extension__ uint64_t __ll;      \
     uint32_t __l[2]; } __w, __r;                 \
     __w.__ll = (x);                              \
     __r.__l[0] = __byte_swap_long (__w.__l[1]);  \
     __r.__l[1] = __byte_swap_long (__w.__l[0]);  \
     __r.__ll; }))
#endif /* __byte_swap_int */

#define bswap_32(x) __byte_swap_long(x)

#endif /* BYTESWAP */

/* Check IP checksum (for IP packets). */
#include <machine/in_cksum.h>
#define IN_CHKSUM(IP) in_cksum_hdr((struct ip *)IP)

#endif /* __NetBSD__ */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* DAG_PLATFORM_NETBSD_H */ 
