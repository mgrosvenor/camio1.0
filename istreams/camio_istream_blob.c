/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ blob (newline separated) input stream
 *
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <memory.h>
#include <sys/stat.h>

#include "camio_istream_blob.h"
#include "../camio_errors.h"
#include "../camio_util.h"


int64_t camio_istream_blob_open(camio_istream_t* this, const camio_descr_t* descr ){
    camio_istream_blob_t* priv = this->priv;

    if(unlikely((size_t)descr->opt_head)){
        eprintf_exit(CAMIO_ERR_UNKNOWN_OPT, "Option(s) supplied, but none expected\n");
    }

    if(unlikely(!descr->query)){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "No filename supplied\n");
    }

    this->fd = open(descr->query, O_RDONLY);
    if(unlikely(this->fd < 0)){
        eprintf_exit(CAMIO_ERR_FILE_OPEN, "Could not open file \"%s\". Error=%s\n", descr->query, strerror(errno));
    }

    //Get the file size
    struct stat st;
    stat(descr->query, &st);
    priv->blob_size = st.st_size;

    if(priv->blob_size)
    {
        //Map the whole thing into memory
        priv->blob = mmap( NULL, priv->blob_size, PROT_READ, MAP_SHARED, this->fd, 0);
        if(unlikely(priv->blob == MAP_FAILED)){
            eprintf_exit(CAMIO_ERR_MMAP, "Could not memory map blob file \"%s\". Error=%s\n", descr->query, strerror(errno));
        }

        priv->is_closed = 0;
    }

    return CAMIO_ERR_NONE;
}


void camio_istream_blob_close(camio_istream_t* this){
    camio_istream_blob_t* priv = this->priv;
    munmap((void*)priv->blob, priv->blob_size);
    close(this->fd);
    priv->is_closed = 1;
}

static int64_t prepare_next(camio_istream_blob_t* priv){
    if(priv->offset == priv->blob_size || priv->is_closed){
        priv->read_size = 0;
        return 0; //Nothing more to read
    }

    //There is data, it is unread
    priv->read_size = priv->blob_size;
    return priv->read_size;
}

int64_t camio_istream_blob_ready(camio_istream_t* this){
    camio_istream_blob_t* priv = this->priv;

    //This cannot fail
    prepare_next(priv);
    return 1;
}

int64_t camio_istream_blob_start_read(camio_istream_t* this, uint8_t** out){
    *out = NULL;

    camio_istream_blob_t* priv = this->priv;
    if(unlikely(priv->is_closed)){
        return 0;
    }

    prepare_next(priv);

    if(unlikely(!priv->read_size)){
        *out = NULL;
        return 0;
    }

    *out = priv->blob;
    priv->offset = priv->blob_size;

    return priv->blob_size;
}


int64_t camio_istream_blob_end_read(camio_istream_t* this, uint8_t* free_buff){
    return 0; //Always true for memory I/O
}



void camio_istream_blob_delete(camio_istream_t* this){
    this->close(this);
    camio_istream_blob_t* priv = this->priv;
    free(priv);
}

/* ****************************************************
 * Construction
 */

camio_istream_t* camio_istream_blob_construct(camio_istream_blob_t* priv, const camio_descr_t* descr, camio_istream_blob_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"blob stream supplied is null\n");
    }

    //Initialize the local variables
    priv->is_closed         = 1;
    priv->blob              = NULL;
    priv->blob_size         = 0;
    priv->read_size         = 0;
    priv->offset            = 0;
    priv->params            = params;

    //Populate the function members
    priv->istream.priv          = priv; //Lets us access private members
    priv->istream.open          = camio_istream_blob_open;
    priv->istream.close         = camio_istream_blob_close;
    priv->istream.start_read    = camio_istream_blob_start_read;
    priv->istream.end_read      = camio_istream_blob_end_read;
    priv->istream.ready         = camio_istream_blob_ready;
    priv->istream.delete        = camio_istream_blob_delete;
    priv->istream.fd            = -1;

    //Call open, because its the obvious thing to do now...
    priv->istream.open(&priv->istream, descr);

    //Return the generic istream interface for the outside world to use
    return &priv->istream;

}

camio_istream_t* camio_istream_blob_new( const camio_descr_t* descr, camio_istream_blob_params_t* params){
    camio_istream_blob_t* priv = malloc(sizeof(camio_istream_blob_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for blob istream creation\n");
    }
    return camio_istream_blob_construct(priv, descr, params);
}





