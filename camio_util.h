/*
 * camio_util.h
 *
 *  Created on: Nov 18, 2012
 *      Author: mgrosvenor
 */

#ifndef CAMIO_UTIL_H_
#define CAMIO_UTIL_H_

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define MIN(x,y) ( (x) < (y) ?  (x) : (y))
#define MAX(x,y) ( (x) > (y) ?  (x) : (y))

#endif /* CAMIO_UTIL_H_ */
