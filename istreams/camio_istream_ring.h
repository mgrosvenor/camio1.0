/*
 * camio_ring.h
 *
 *  Created on: Nov 14, 2012
 *      Author: root
 */

#ifndef CAMIO_ISTREAM_RING_H_
#define CAMIO_ISTREAM_RING_H_

#include "camio_istream.h"

#define CAMIO_ISTREAM_RING_BLOCKING    1
#define CAMIO_ISTREAM_RING_NONBLOCKING 0

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/


typedef struct {
    //No params yet
} camio_istream_ring_params_t;

typedef struct {
    camio_istream_t istream;
    int is_closed;                       //Has close be called?
    volatile uint8_t* ring;              //Pointer to the head of the ring
    size_t ring_size;                    //Size of the ring buffer
    volatile uint8_t* curr;              //Current slot in the ring
    size_t read_size;                    //Size of the current read waiting (if any)
    uint64_t sync_counter;               //Synchronization counter
    uint64_t index;                      //Current index into the buffer
    camio_istream_ring_params_t* params;  //Parameters passed in from the outside

} camio_istream_ring_t;




/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_istream_t* camio_istream_ring_new( const camio_descr_t* opts,  camio_istream_ring_params_t* params);


#endif /* CAMIO_ISTREAM_RING_H_ */
