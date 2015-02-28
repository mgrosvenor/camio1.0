/*
 * camio_ostream_ring.h
 *
 *  Created on: Nov 15, 2012
 *      Author: root
 */

#ifndef CAMIO_OSTREAM_RING_H_
#define CAMIO_OSTREAM_RING_H_

#include "camio_ostream.h"

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/


typedef struct {
    //No params at this stage
} camio_ostream_ring_params_t;

typedef struct {
    camio_ostream_t ostream;
    char* filename;                         //Keep the file name so we can delete it
    int is_closed;              			//Has close be called?
    volatile uint8_t* ring;					//Pointer to the head of the ring
    size_t ring_size;                       //Size of the ring buffer
    volatile uint8_t* curr;                 //Current slot in the ring
    uint8_t* assigned_buffer;               //Assigned write buffer
    size_t assigned_buffer_sz;              //Assigned write buffer size
    uint64_t sync_count;                    //Synchronization counter
    uint64_t index;                         //Current slot in the ring
    camio_ostream_ring_params_t* params;     //Parameters from the outside world

} camio_ostream_ring_t;



/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_ostream_t* camio_ostream_ring_new( const camio_descr_t* opts,  camio_ostream_ring_params_t* params);



#endif /* CAMIO_OSTREAM_RING_H_ */
