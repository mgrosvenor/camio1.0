/*
 * camio_ostream_netmap.h
 *
 *  Created on: Nov 15, 2012
 *      Author: root
 */

#ifndef CAMIO_OSTREAM_NETMAP_H_
#define CAMIO_OSTREAM_NETMAP_H_

#include "camio_ostream.h"

/********************************************************************
 *                  PRIVATE DEFS
 ********************************************************************/


typedef struct {
    void* nm_mem; //Can take a memory argument as a parameter to ensure all instances are working
                  //from the same pointer range. This will ensure that zero-copy optimisations are
                  //possible
    size_t nm_mem_size;
    uint32_t nm_offset;
    uint32_t nm_rx_rings;
    uint32_t nm_rx_slots;
    uint32_t nm_tx_rings;
    uint32_t nm_tx_slots;
    uint32_t nm_ringid;

    size_t burst_size;
    int use_local;
    int    fd;
} camio_ostream_netmap_params_t;

typedef struct {
    int is_closed;
    void* nm_mem;
    uint32_t nm_offset;
    uint32_t nm_rx_rings;
    uint32_t nm_rx_slots;
    uint32_t nm_tx_rings;
    uint32_t nm_tx_slots;
    uint32_t nm_ringid;

    size_t mem_size;
    struct netmap_if *nifp;
    struct netmap_ring  *tx;
    size_t begin;
    size_t end;

    //size_t available_buffs;
    void* packet;
    size_t packet_size;
    size_t slot_num;
    size_t ring_num;
    size_t ring_count;
    size_t slot_count;
    size_t total_slots;

    void* packet_buff_bottom;
    int rings[256];

    void* assigned_buffer;
    size_t  assigned_buffer_sz;
    size_t burst_size;

    camio_ostream_t ostream;
    camio_ostream_netmap_params_t* params;      //Parameters from the outside world

} camio_ostream_netmap_t;



/********************************************************************
 *                  PUBLIC DEFS
 ********************************************************************/

camio_ostream_t* camio_ostream_netmap_new( const camio_descr_t* opts,  camio_ostream_netmap_params_t* params);



#endif /* CAMIO_OSTREAM_NETMAP_H_ */
