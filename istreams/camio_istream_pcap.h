/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ pcap socket input stream
 *
 */

#ifndef CAMIO_ISTREAM_PCAP_H_
#define CAMIO_ISTREAM_PCAP_H_

#include "camio_istream.h"

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/

typedef struct {
    //No params at this stage
} camio_istream_pcap_params_t;

typedef struct {
    camio_istream_t istream;
    uint8_t* buffer;
    size_t buffer_size;
    size_t bytes_read;
    int fd;
    int is_closed;                      //Has close be called?
    camio_istream_pcap_params_t* params;  //Parameters passed in from the outside

} camio_istream_pcap_t;



/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_istream_t* camio_istream_pcap_new( const camio_descr_t* opts,  camio_istream_pcap_params_t* params);


#endif /* CAMIO_ISTREAM_PCAP_H_ */
