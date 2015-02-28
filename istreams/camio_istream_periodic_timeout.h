/*
 * camio_periodic_timeout.h
 *
 *  Created on: Nov 14, 2012
 *      Author: root
 */

#ifndef CAMIO_ISTREAM_PERIODIC_TIMEOUT_H_
#define CAMIO_ISTREAM_PERIODIC_TIMEOUT_H_

#include "camio_istream.h"

#define CAMIO_ISTREAM_PERIODIC_TIMEOUT_BLOCKING    1
#define CAMIO_ISTREAM_PERIODIC_TIMEOUT_NONBLOCKING 0

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/


typedef struct {
    //No params
} camio_istream_periodic_timeout_params_t;

typedef struct {
    camio_istream_t istream;
    int is_closed;                      //Has close be called?
    uint64_t expiries;                  //Number of expiries since the timer was set
    size_t read_size;                   //Size of the last read
    camio_istream_periodic_timeout_params_t* params;  //Parameters passed in from the outside
    int blocking;
} camio_istream_periodic_timeout_t;




/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_istream_t* camio_istream_periodic_timeout_new( const camio_descr_t* opts,  camio_istream_periodic_timeout_params_t* params);


#endif /* CAMIO_ISTREAM_PERIODIC_TIMEOUT_H_ */
