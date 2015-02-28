/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ ring (newline separated) output stream
 *
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <memory.h>

#include "../camio_util.h"
#include "../camio_errors.h"

#include "camio_ostream_ring.h"


//TODO XXX: These should be passed as options
#define CAMIO_OSTREAM_RING_SIZE (4 * 1024 * 1024) //4MB
#define CAMIO_OSTREAM_RING_SLOT_SIZE (4 * 1024)  //4K

int camio_ostream_ring_open(camio_ostream_t* this, const camio_descr_t* descr ){
    camio_ostream_ring_t* priv = this->priv;
    int ring_fd = -1;
    volatile uint8_t* ring = NULL;

    if(descr->opt_head){
        eprintf_exit(CAMIO_ERR_UNKNOWN_OPT, "Option(s) supplied, but none expected\n");
    }

    if(!descr->query){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "No filename supplied\n");
    }

    //Make a local copy of the filename in case the descr pointer goes away (probable)
    size_t filename_len = strlen(descr->query);
    priv->filename = malloc(filename_len + 1);
    memcpy(priv->filename,descr->query, filename_len);
    priv->filename[filename_len] = '\0'; //Make sure it's null terminated


    ring_fd = open(descr->query, O_RDWR);
    //If that fails, try opening and creating the file
    if(unlikely(ring_fd < 0)){
        ring_fd = open(descr->query, O_RDWR | O_CREAT | O_TRUNC, (mode_t)(0666));
        if(unlikely(ring_fd < 0)){
            eprintf_exit(CAMIO_ERR_FILE_OPEN, "Could not open file \"%s\". Error=%s\n", descr->query, strerror(errno));
        }

        //Resize the file
        if(lseek(ring_fd, CAMIO_OSTREAM_RING_SIZE -1, SEEK_SET) < 0){
            eprintf_exit(CAMIO_ERR_FILE_LSEEK, "Could not resize file for shared region \"%s\". Error=%s\n", descr->query, strerror(errno));
        }

        if(write(ring_fd, "", 1) < 0){
            eprintf_exit(CAMIO_ERR_FILE_WRITE, "Could not resize file for shared region \"%s\". Error=%s\n", descr->query, strerror(errno));
        }

        ring = mmap( NULL, CAMIO_OSTREAM_RING_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, ring_fd, 0);
        if(unlikely(ring == MAP_FAILED)){
            eprintf_exit(CAMIO_ERR_MMAP, "Could not memory map ring file \"%s\". Error=%s\n", descr->query, strerror(errno));
        }

        //Initialize the ring with 0
        memset((uint8_t*)ring, 0, CAMIO_OSTREAM_RING_SIZE);
    }
    else{
        ring = mmap( NULL, CAMIO_OSTREAM_RING_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, ring_fd, 0);
        if(unlikely(ring == MAP_FAILED)){
            eprintf_exit(CAMIO_ERR_MMAP, "Could not memory map ring file \"%s\". Error=%s\n", descr->query, strerror(errno));
        }
    }


    priv->ring_size = CAMIO_OSTREAM_RING_SIZE;
    this->fd = ring_fd;
    priv->ring = ring;
    priv->curr = ring;
    priv->is_closed = 0;

    return CAMIO_ERR_NONE;
}

void camio_ostream_ring_close(camio_ostream_t* this){
    camio_ostream_ring_t* priv = this->priv;
    munmap((void*)priv->ring, priv->ring_size);
    close(this->fd);
    unlink(priv->filename); //Delete the file so reader can't get confused
    priv->is_closed = 1;
}



//Returns a pointer to a space of size len, ready for data
//Returns NULL if this is impossible
uint8_t* camio_ostream_ring_start_write(camio_ostream_t* this, size_t len ){
    camio_ostream_ring_t* priv = this->priv;
    if(len > CAMIO_OSTREAM_RING_SLOT_SIZE - 8){
        wprintf(0, "Length supplied (%lu) is greater than slot size (%lu, corruption is likely if you proceed.\n", len, CAMIO_OSTREAM_RING_SIZE - 2 * sizeof(uint64_t) );
        return NULL;

    }

    return (uint8_t*)priv->curr;
}

//Returns non-zero if a call to start_write will be non-blocking
int camio_ostream_ring_ready(camio_ostream_t* this){
    //Not implemented
    eprintf_exit(CAMIO_ERR_NOT_IMPL, "\n");
    return 0;
}


//Commit the data to the buffer previously allocated
//Len must be equal to or less than len called with start_write
uint8_t* camio_ostream_ring_end_write(camio_ostream_t* this, size_t len){
    camio_ostream_ring_t* priv = this->priv;

    if(len > CAMIO_OSTREAM_RING_SLOT_SIZE - 2 * sizeof(uint64_t)){
        eprintf_exit(0, "Length supplied (%lu) is greater than slot size (%lu, corruption is likely.\n", len, CAMIO_OSTREAM_RING_SIZE - 2 * sizeof(uint64_t) );
        return NULL;
    }

    //Memory copy is done implicitly here
    if(priv->assigned_buffer){
        memcpy((uint8_t*)priv->curr,priv->assigned_buffer,len);
        priv->assigned_buffer    = NULL;
        priv->assigned_buffer_sz = 0;
    }


    priv->sync_count++;
    *(volatile uint64_t*)(priv->curr + CAMIO_OSTREAM_RING_SLOT_SIZE-2*sizeof(uint64_t)) = len;
    *(volatile uint64_t*)(priv->curr + CAMIO_OSTREAM_RING_SLOT_SIZE-1*sizeof(uint64_t)) = priv->sync_count; //Write is now committed

    priv->index = (priv->index + 1) % ( CAMIO_OSTREAM_RING_SIZE /  CAMIO_OSTREAM_RING_SLOT_SIZE);
    priv->curr  = priv->ring + (priv->index * CAMIO_OSTREAM_RING_SLOT_SIZE);

    return NULL;
}


void camio_ostream_ring_delete(camio_ostream_t* ostream){
    ostream->close(ostream);
    camio_ostream_ring_t* priv = ostream->priv;
    free(priv);
}

//Is this stream capable of taking over another stream buffer
int camio_ostream_ring_can_assign_write(camio_ostream_t* this){
    return 1;
}

//Assign the write buffer to the stream
int camio_ostream_ring_assign_write(camio_ostream_t* this, uint8_t* buffer, size_t len){
    camio_ostream_ring_t* priv = this->priv;

    if(!buffer){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"Assigned buffer is null.");
    }

    priv->assigned_buffer    = buffer;
    priv->assigned_buffer_sz = len;

    return 0;
}


/* ****************************************************
 * Construction heavy lifting
 */

camio_ostream_t* camio_ostream_ring_construct(camio_ostream_ring_t* priv, const camio_descr_t* descr,  camio_ostream_ring_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"ring stream supplied is null\n");
    }
    //Initialize the local variables
    priv->is_closed             = 1;
    priv->ring                  = NULL;
    priv->ring_size             = 0;
    priv->curr                  = NULL;
    priv->sync_count            = 0;
    priv->index                 = 0;
    priv->assigned_buffer       = NULL;
    priv->assigned_buffer_sz    = 0;
    priv->params                = params;


    //Populate the function members
    priv->ostream.priv              = priv; //Lets us access private members from public functions
    priv->ostream.open              = camio_ostream_ring_open;
    priv->ostream.close             = camio_ostream_ring_close;
    priv->ostream.start_write       = camio_ostream_ring_start_write;
    priv->ostream.end_write         = camio_ostream_ring_end_write;
    priv->ostream.ready             = camio_ostream_ring_ready;
    priv->ostream.delete            = camio_ostream_ring_delete;
    priv->ostream.can_assign_write  = camio_ostream_ring_can_assign_write;
    priv->ostream.assign_write      = camio_ostream_ring_assign_write;
    priv->ostream.fd                = -1;

    //Call open, because its the obvious thing to do now...
    priv->ostream.open(&priv->ostream, descr);

    //Return the generic ostream interface for the outside world
    return &priv->ostream;

}

camio_ostream_t* camio_ostream_ring_new( const camio_descr_t* descr,  camio_ostream_ring_params_t* params){
    camio_ostream_ring_t* priv = malloc(sizeof(camio_ostream_ring_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for ostream ring creation\n");
    }
    return camio_ostream_ring_construct(priv, descr,  params);
}



