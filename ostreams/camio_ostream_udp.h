/*
 * camio_ostream_udp.h
 *
 *  Created on: Nov 15, 2012
 *      Author: root
 */

#ifndef CAMIO_OSTREAM_UDP_H_
#define CAMIO_OSTREAM_UDP_H_

#include <netinet/in.h>

#include "camio_ostream.h"

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/


typedef struct {
    //No params at this stage
} camio_ostream_udp_params_t;

typedef struct {
    camio_ostream_t ostream;
    struct sockaddr_in addr;                //Destination address/port
    int is_closed;                          //Has close be called?
    uint8_t* buffer;                           //Space to build output
    size_t buffer_size;                     //Size of output buffer
    uint8_t* assigned_buffer;                  //Assigned write buffer
    size_t assigned_buffer_sz;              //Assigned write buffer size
    camio_ostream_udp_params_t* params;      //Parameters from the outside world

} camio_ostream_udp_t;



/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_ostream_t* camio_ostream_udp_new( const camio_descr_t* opts,  camio_ostream_udp_params_t* params);



#endif /* CAMIO_OSTREAM_UDP_H_ */
