//#LINKFLAGS=-lrt
/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ periodic timeout input stream
 *
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/timerfd.h>
#include <time.h>
#include <sys/time.h>


#include "camio_istream_periodic_timeout_fast.h"
#include "../camio_errors.h"
#include "../camio_util.h"


static inline uint64_t timespec_to_ns(struct timespec* ts){
    return ts->tv_sec * 1000 * 1000 * 1000 + ts->tv_nsec;
}


//static inline void ns_to_timespec(uint64_t ns, struct timespec* ts){
//    ts->tv_sec = ns / (1000 * 1000 * 1000);
//    ts->tv_nsec = ns % (1000 * 1000 * 1000);
//}


int camio_istream_periodic_timeout_fast_open(camio_istream_t* this, const camio_descr_t* opts ){
    camio_istream_periodic_timeout_fast_t* priv = this->priv;


    if(opts->opt_head){
        eprintf_exit(CAMIO_ERR_UNKNOWN_OPT, "Option(s) supplied, but none expected\n");
    }

    //Parse the time spec
    if(!opts->query){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "No timer specification supplied expected nanoseconds format\n");
    }

    if(priv->params){
        priv->clock_type = priv->params->clock_type;
    }
    else{
        priv->clock_type = CLOCK_MONOTONIC_COARSE;
    }

    uint64_t temp = 0;
    size_t i = 0;
    for(; opts->query[i] != '\0'; i++){
        temp *= 10;
        temp += (opts->query[i] - '0');
    }
    priv->period = temp;

    struct timespec ts_now;
    clock_gettime(priv->clock_type,&ts_now);
    const uint64_t ns_now = timespec_to_ns(&ts_now);
    priv->ns_aim = ns_now + priv->period;



    //Set the file descriptor
    this->fd = -1;

    priv->is_closed = 0;
    return CAMIO_ERR_NONE;
}


void camio_istream_periodic_timeout_fast_close(camio_istream_t* this){
    camio_istream_periodic_timeout_fast_t* priv = this->priv;
    priv->is_closed = 1;
}



static int prepare_next(camio_istream_periodic_timeout_fast_t* priv){
    if(priv->is_ready){
        return 1;
    }

    struct timespec ts_now;
    clock_gettime(priv->clock_type,&ts_now);
    const uint64_t ns_now = timespec_to_ns(&ts_now);
    if(unlikely(ns_now >= priv->ns_aim)){
        //printf("Timer fired\n");
        priv->ns_aim    = ns_now + priv->period;
        priv->is_ready  = 1;
        priv->result    = ns_now;
        return 1;
    }

    return 0;
}

int camio_istream_periodic_timeout_fast_ready(camio_istream_t* this){
    camio_istream_periodic_timeout_fast_t* priv = this->priv;
    if(priv->is_ready || priv->is_closed){
        return 1;
    }

    return prepare_next(priv);
}

int camio_istream_periodic_timeout_fast_start_read(camio_istream_t* this, uint8_t** out){
    camio_istream_periodic_timeout_fast_t* priv = this->priv;
    if(unlikely(priv->is_closed == 1)){
        return 0;
    }

    //Called read without calling ready, they must want to block
    if(unlikely(!priv->is_ready)){
        while(!prepare_next(priv)){
            //spin waiting for this
        }
    }

    *out = (uint8_t*)&priv->result;
    return sizeof(priv->result);
}


int camio_istream_periodic_timeout_fast_end_read(camio_istream_t* this, uint8_t* free_buff){
    camio_istream_periodic_timeout_fast_t* priv = this->priv;
    priv->is_ready = 0;
    return 0;
}


void camio_istream_periodic_timeout_fast_delete(camio_istream_t* this){
    this->close(this);
    camio_istream_periodic_timeout_fast_t* priv = this->priv;
    free(priv);
}

/* ****************************************************
 * Construction
 */

camio_istream_t* camio_istream_periodic_timeout_fast_construct(camio_istream_periodic_timeout_fast_t* priv, const camio_descr_t* opts,  camio_istream_periodic_timeout_fast_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"periodic_timeout_fast stream supplied is null\n");
    }
    //Initialize the local variables
    priv->is_closed         = 1;
    priv->is_ready          = 0;
    priv->period            = 0;
    priv->ns_aim            = 0;
    priv->params            = params;

    //Populate the function members
    priv->istream.priv          = priv; //Lets us access private members
    priv->istream.open          = camio_istream_periodic_timeout_fast_open;
    priv->istream.close         = camio_istream_periodic_timeout_fast_close;
    priv->istream.start_read    = camio_istream_periodic_timeout_fast_start_read;
    priv->istream.end_read      = camio_istream_periodic_timeout_fast_end_read;
    priv->istream.ready         = camio_istream_periodic_timeout_fast_ready;
    priv->istream.delete        = camio_istream_periodic_timeout_fast_delete;
    priv->istream.fd            = -1;

    //Call open, because its the obvious thing to do now...
    priv->istream.open(&priv->istream, opts);

    //Return the generic istream interface for the outside world to use
    return &priv->istream;

}

camio_istream_t* camio_istream_periodic_timeout_fast_new( const camio_descr_t* opts,  camio_istream_periodic_timeout_fast_params_t* params){
    camio_istream_periodic_timeout_fast_t* priv = malloc(sizeof(camio_istream_periodic_timeout_fast_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for periodic_timeout_fast istream creation\n");
    }
    return camio_istream_periodic_timeout_fast_construct(priv, opts,  params);
}





