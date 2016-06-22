/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ DAG card input stream
 *
 */



#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <memory.h>
#include <netinet/in.h>

#include "camio_istream_dag.h"
#include "../camio_errors.h"
#include "../camio_util.h"
#include "../clocks/camio_time.h"

#include "../dag/dagapi.h"

int64_t camio_istream_dag_open(camio_istream_t* this, const camio_descr_t* descr ){
    camio_istream_dag_t* priv = this->priv;
    int dag_fd = -1;

    if(unlikely((size_t)descr->opt_head)){
        eprintf_exit(CAMIO_ERR_UNKNOWN_OPT, "Option(s) supplied, but none expected\n");
    }

    if(unlikely(!descr->query)){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "No device supplied\n");
    }

    dag_fd = dag_open(descr->query);
    printf("Opening %s\n", descr->query);
    if(unlikely(dag_fd < 0)){
        eprintf_exit(CAMIO_ERR_FILE_OPEN, "Could not open file \"%s\". Error=%s\n", descr->query, strerror(errno));
    }

    if(dag_attach_stream(dag_fd, priv->dag_stream, 0, 0) < 0){
        eprintf_exit(CAMIO_ERR_FILE_OPEN, "Could not attach to dag stream \"%lu\". Error=%s\n", priv->dag_stream, strerror(errno));
    }

    if(dag_start_stream(dag_fd, priv->dag_stream) < 0){
        eprintf_exit(CAMIO_ERR_FILE_OPEN, "Could not start dag stream \"%lu\". Error=%s\n", priv->dag_stream, strerror(errno));
    }


    struct timeval maxwait;
    struct timeval poll;
    timerclear(&maxwait);
    timerclear(&poll);
    dag_setpollparams(0,&maxwait,&poll);

    this->fd = dag_fd;
    priv->is_closed = 0;
    return CAMIO_ERR_NONE;
}


void camio_istream_dag_close(camio_istream_t* this){
    camio_istream_dag_t* priv = this->priv;
    dag_stop_stream(this->fd, priv->dag_stream);
    dag_detach_stream(this->fd, priv->dag_stream);
    dag_close(this->fd);
    priv->is_closed = 1;
}




static int64_t prepare_next(camio_istream_t* this){
    camio_istream_dag_t* priv = this->priv;

    //Simple case, there's already data waiting
    if(unlikely((size_t)priv->dag_data)){
        return priv->data_size;
    }

    //Is there new data?
    dag_record_t* data = (dag_record_t*)dag_rx_stream_next_record(this->fd, priv->dag_stream);
    if( likely((size_t)data)){
        //Is this an ERF? Or did something wierd happen?
        if ( unlikely(data->type == 0)){
            wprintf(CAMIO_ERR_NOT_AN_ERF, "Not ERF payload \n");
            return 0;
        }

        priv->dag_data = data;
        priv->data_size = ntohs(data->rlen);
        return priv->data_size;
    }

    return 0;
}

int64_t camio_istream_dag_ready(camio_istream_t* this){
    camio_istream_dag_t* priv = this->priv;
    if(priv->data_size || priv->is_closed){
        return 1;
    }

    return prepare_next(this);
}

//Convert fixed point dagtime to nanoseconds since 1970
void camio_dagtime_to_time(camio_time_t* time_out, dag_record_t* dag_record){
    time_out->counter = dag_record->ts;
}


int64_t camio_istream_dag_start_read(camio_istream_t* this, uint8_t** out){
    camio_istream_dag_t* priv = this->priv;
    *out = NULL;

    if(unlikely(priv->is_closed)){
        return 0;
    }

    //Called read without calling ready, they must want to block/spin waiting for data
    if(unlikely(!priv->data_size)){
        while(!prepare_next(this)){
            __asm__ __volatile__("pause"); //Tell the CPU we're spinning
        }
    }

    camio_time_t current;
    camio_dagtime_to_time(&current,priv->dag_data);
    //this->clock->set(this->&current);

    *out = (uint8_t*)priv->dag_data;
    return priv->data_size;
}


int64_t camio_istream_dag_end_read(camio_istream_t* this, uint8_t* free_buff){
    camio_istream_dag_t* priv = this->priv;
    priv->data_size = 0;
    priv->dag_data = NULL;
    return 0;
}


void camio_istream_dag_delete(camio_istream_t* this){
    this->close(this);
    camio_istream_dag_t* priv = this->priv;
    free(priv);
}

/* ****************************************************
 * Construction
 */

camio_istream_t* camio_istream_dag_construct(camio_istream_dag_t* priv, const camio_descr_t* descr,  camio_istream_dag_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"dag stream supplied is null\n");
    }

    //Initialize the local variables
    priv->is_closed         = 1;
    priv->dag_stream        = 0;
    priv->dag_data          = NULL;
    priv->data_size			= 0;
    priv->params            = params;

    //Populate the function members
    priv->istream.priv          = priv; //Lets us access private members
    priv->istream.open          = camio_istream_dag_open;
    priv->istream.close         = camio_istream_dag_close;
    priv->istream.start_read    = camio_istream_dag_start_read;
    priv->istream.end_read      = camio_istream_dag_end_read;
    priv->istream.ready         = camio_istream_dag_ready;
    priv->istream.delete        = camio_istream_dag_delete;
    priv->istream.fd            = -1;

    //Call open, because its the obvious thing to do now...
    priv->istream.open(&priv->istream, descr);

    //Return the generic istream interface for the outside world to use
    return &priv->istream;

}

camio_istream_t* camio_istream_dag_new( const camio_descr_t* descr,  camio_istream_dag_params_t* params){
    camio_istream_dag_t* priv = malloc(sizeof(camio_istream_dag_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for dag istream creation\n");
    }
    return camio_istream_dag_construct(priv, descr,  params);
}



