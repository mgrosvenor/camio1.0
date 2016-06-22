/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ ring (newline separated) input stream
 *
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <memory.h>
#include <stdint.h>

#include "camio_istream_ring.h"
#include "../camio_errors.h"
#include "../camio_util.h"


//TODO XXX: These should be passed as options
#define CAMIO_ISTREAM_RING_SIZE (4 * 1024 * 1024) //4MB
#define CAMIO_ISTREAM_RING_SLOT_SIZE (4 * 1024)  //4K

int64_t camio_istream_ring_open(camio_istream_t* this, const camio_descr_t* descr ){
    camio_istream_ring_t* priv = this->priv;
    int ring_fd = -1;
    volatile uint8_t* ring = NULL;

    if(unlikely((size_t)descr->opt_head)){
        eprintf_exit(CAMIO_ERR_UNKNOWN_OPT, "Option(s) supplied, but none expected\n");
    }

    if(unlikely(!descr->query)){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "No filename supplied\n");
    }

    ring_fd = open(descr->query, O_RDONLY);
    //If that fails, try opening and creating the file
    if(unlikely(ring_fd < 0)){
        ring_fd = open(descr->query, O_RDWR | O_CREAT | O_TRUNC , (mode_t)(0666));
        if(unlikely(ring_fd < 0)){
            eprintf_exit(CAMIO_ERR_FILE_OPEN, "Could not open file \"%s\". Error=%s\n", descr->query, strerror(errno));
        }

        //Resize the file
        if(lseek(ring_fd, CAMIO_ISTREAM_RING_SIZE -1, SEEK_SET) < 0){
            eprintf_exit(CAMIO_ERR_FILE_LSEEK, "Could not resize file for shared region \"%s\". Error=%s\n", descr->query, strerror(errno));
        }

        if(write(ring_fd, "", 1) < 0){
            eprintf_exit(CAMIO_ERR_FILE_WRITE, "Could not resize file for shared region \"%s\". Error=%s\n", descr->query, strerror(errno));
        }

        ring = mmap( NULL, CAMIO_ISTREAM_RING_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, ring_fd, 0);
        if(unlikely(ring == MAP_FAILED)){
            eprintf_exit(CAMIO_ERR_MMAP, "Could not memory map ring file \"%s\". Error=%s\n", descr->query, strerror(errno));
        }

        //Initialize the ring with 0
        memset((uint8_t*)ring, 0, CAMIO_ISTREAM_RING_SIZE);
    }else{
        ring = mmap( NULL, CAMIO_ISTREAM_RING_SIZE, PROT_READ, MAP_SHARED, ring_fd, 0);
        if(unlikely(ring == MAP_FAILED)){
            eprintf_exit(CAMIO_ERR_MMAP, "Could not memory map ring file \"%s\". Error=%s\n", descr->query, strerror(errno));
        }
    }


    priv->ring_size = CAMIO_ISTREAM_RING_SIZE;
    this->fd = ring_fd;
    priv->ring = ring;
    priv->curr = ring;
    priv->is_closed = 0;

    return CAMIO_ERR_NONE;
}


void camio_istream_ring_close(camio_istream_t* this){
    camio_istream_ring_t* priv = this->priv;
    munmap((void*)priv->ring, priv->ring_size);
    close(this->fd);
    priv->is_closed = 1;
}




static int prepare_next(camio_istream_ring_t* priv){

    //Simple case, there's already data waiting
    if(unlikely(priv->read_size)){
        return priv->read_size;
    }

    //Is there new data?
    register uint64_t curr_sync_count = *((volatile uint64_t*)(priv->curr + CAMIO_ISTREAM_RING_SLOT_SIZE - sizeof(uint64_t)));
    if( likely(curr_sync_count == priv->sync_counter)){
        const uint64_t data_len  = *((volatile uint64_t*)(priv->curr + CAMIO_ISTREAM_RING_SLOT_SIZE - 2* sizeof(uint64_t)));
        priv->read_size = data_len;
        return data_len;
    }

    if( likely(curr_sync_count > priv->sync_counter)){
        wprintf(CAMIO_ERR_UNKNOWN_OSTREAM, "Ring overflow. Catching up now. Dropping payloads from %lu to %lu\n", priv->sync_counter, curr_sync_count -1);
        priv->sync_counter = curr_sync_count;
        const uint64_t data_len  = *((volatile uint64_t*)(priv->curr + CAMIO_ISTREAM_RING_SLOT_SIZE - 2* sizeof(uint64_t)));
        priv->read_size = data_len;
        return data_len;
    }

    return 0;
}

int64_t camio_istream_ring_ready(camio_istream_t* this){
    camio_istream_ring_t* priv = this->priv;
    if(priv->read_size || priv->is_closed){
        return 1;
    }

    return prepare_next(priv);
}

int64_t camio_istream_ring_start_read(camio_istream_t* this, uint8_t** out){
    camio_istream_ring_t* priv = this->priv;
    *out = NULL;

    if(unlikely(priv->is_closed)){
        return 0;
    }

    //Called read without calling ready, they must want to block/spin waiting for data
    if(unlikely(!priv->read_size)){
        while(!prepare_next(priv)){
            __asm__ __volatile__("pause"); //Tell the CPU we're spinning
        }
    }

    *out = (uint8_t*)priv->curr;
    size_t result = priv->read_size;
    return result;
}


int64_t camio_istream_ring_end_read(camio_istream_t* this, uint8_t* free_buff){
    camio_istream_ring_t* priv = this->priv;

    register uint64_t curr_sync_count = *((volatile uint64_t*)(priv->curr + CAMIO_ISTREAM_RING_SLOT_SIZE - sizeof(uint64_t)));
    if( unlikely(curr_sync_count != priv->sync_counter)){
        //wprintf(CAMIO_ERR_BUFFER_OVERRUN, "Detected overrun in ring buffer sync count is now=%lu, expected sync count=%lu\n", curr_sync_count, priv->sync_counter);
        priv->sync_counter = curr_sync_count;
        priv->read_size = 0;
        return -1;
    }

    priv->read_size = 0;
    priv->sync_counter++;
    priv->index = (priv->index + 1) % (CAMIO_ISTREAM_RING_SIZE / CAMIO_ISTREAM_RING_SLOT_SIZE);
    priv->curr  = priv->ring + (priv->index * CAMIO_ISTREAM_RING_SLOT_SIZE);


    return 0;
}


void camio_istream_ring_delete(camio_istream_t* this){
    this->close(this);
    camio_istream_ring_t* priv = this->priv;
    free(priv);
}

/* ****************************************************
 * Construction
 */

camio_istream_t* camio_istream_ring_construct(camio_istream_ring_t* priv, const camio_descr_t* descr,  camio_istream_ring_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"ring stream supplied is null\n");
    }

    //Initialize the local variables
    priv->is_closed         = 1;
    priv->ring              = NULL;
    priv->ring_size         = 0;
    priv->curr              = NULL;
    priv->read_size         = 0;
    priv->sync_counter      = 1; //We will expect 1 when the first write occurs
    priv->index             = 0;
    priv->params            = params;

    //Populate the function members
    priv->istream.priv          = priv; //Lets us access private members
    priv->istream.open          = camio_istream_ring_open;
    priv->istream.close         = camio_istream_ring_close;
    priv->istream.start_read    = camio_istream_ring_start_read;
    priv->istream.end_read      = camio_istream_ring_end_read;
    priv->istream.ready         = camio_istream_ring_ready;
    priv->istream.delete        = camio_istream_ring_delete;
    priv->istream.fd            = -1;

    //Call open, because its the obvious thing to do now...
    priv->istream.open(&priv->istream, descr);

    //Return the generic istream interface for the outside world to use
    return &priv->istream;

}

camio_istream_t* camio_istream_ring_new( const camio_descr_t* descr,  camio_istream_ring_params_t* params){
    camio_istream_ring_t* priv = malloc(sizeof(camio_istream_ring_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for ring istream creation\n");
    }
    return camio_istream_ring_construct(priv, descr,  params);
}





