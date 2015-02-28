/*
 * camio_periodic_timeout_fast.h
 *
 *  Created on: Nov 14, 2012
 *      Author: root
 */

#ifndef CAMIO_ISTREAM_PERIODIC_TIMEOUT_FAST_H_
#define CAMIO_ISTREAM_PERIODIC_TIMEOUT_FAST_H_

#include "camio_istream.h"

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/


typedef struct {
    int clock_type;
} camio_istream_periodic_timeout_fast_params_t;

typedef struct {
    camio_istream_t istream;
    int is_closed;                      //Has close be called?
    uint64_t is_ready;
    uint64_t ns_aim;
    uint64_t period;
    int clock_type;
    uint64_t result;
    camio_istream_periodic_timeout_fast_params_t* params;  //Parameters passed in from the outside

} camio_istream_periodic_timeout_fast_t;




/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_istream_t* camio_istream_periodic_timeout_fast_new( const camio_descr_t* opts,  camio_istream_periodic_timeout_fast_params_t* params);


#endif /* CAMIO_ISTREAM_PERIODIC_TIMEOUT_FAST_H_ */
