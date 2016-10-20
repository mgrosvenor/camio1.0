/*
 * Copyright  (C) Matthew P. Grosvenor, 2016, All Rights Reserved
 *
 * Camio 1.0 ExaNIC card input stream
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
#include <exanic/exanic.h>
#include <exanic/time.h>
#include <net/if.h>
#include <exanic/exanic.h>
#include <exanic/config.h>
#include <exanic/port.h>
#include <exanic/fifo_rx.h>
#include <exanic/time.h>
#include <exanic/filter.h>
#include <sys/ioctl.h>


#include "camio_istream_exa.h"
#include "../camio_errors.h"
#include "../camio_util.h"
#include "../clocks/camio_time.h"

#include "camio/dag/dagapi.h"


#define EXANIC_PORTS 4 //HACK! Assumes ExaNIC x4

static void set_promiscuous_mode(exanic_t *exanic, int port_number, int enable)
{
    struct ifreq ifr;
    int fd;

    memset(&ifr, 0, sizeof(ifr));
    if (exanic_get_interface_name(exanic, port_number, ifr.ifr_name,
            sizeof(ifr.ifr_name)) == -1)
        return;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) != -1)
    {
        if (enable)
            ifr.ifr_flags |= IFF_PROMISC;
        else
            ifr.ifr_flags &= ~IFF_PROMISC;

        ioctl(fd, SIOCSIFFLAGS, &ifr);
    }
    close(fd);
}


int64_t camio_istream_exa_open(camio_istream_t* this, const camio_descr_t* descr ){
    camio_istream_exa_t* priv = this->priv;
    int exa_fd = -1;

    if(unlikely((size_t)descr->opt_head)){
        eprintf_exit(CAMIO_ERR_UNKNOWN_OPT, "Option(s) supplied, but none expected\n");
    }

    if(unlikely(!descr->query)){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "No device supplied\n");
    }

    priv->exanic = exanic_acquire_handle(descr->query);
    printf("Opening %s\n", descr->query);
    if(unlikely(priv->exanic == NULL)){
        eprintf_exit(CAMIO_ERR_FILE_OPEN, "Could not open file \"%s\". Error=%s\n", descr->query, strerror(errno));
    }

    //Hack! Asumes an ExaNIC x4
    for(int i = 0; i < EXANIC_PORTS; i++){
        priv->exanic_rx[i] = exanic_acquire_rx_buffer(priv->exanic,  i, 0);
        if(unlikely(priv->exanic_rx[i] == NULL)){
            wprintf(CAMIO_ERR_FILE_OPEN, "Warning Could not attach to port \"%lu\". Error=%s\n",i, strerror(errno));
            continue;
        }

        set_promiscuous_mode(priv->exanic,i,1);
    }

    this->fd = exa_fd;
    priv->is_closed = 0;
    return CAMIO_ERR_NONE;
}


void camio_istream_exa_close(camio_istream_t* this){
    camio_istream_exa_t* priv = this->priv;

    for(int i = 0; i < EXANIC_PORTS;i++){
        if(priv->exanic_rx[i]){
            exanic_release_rx_buffer(priv->exanic_rx[i]);
            set_promiscuous_mode(priv->exanic,i,0);
        }
    }

    exanic_release_handle(priv->exanic);

    priv->is_closed = 1;
}




static int64_t prepare_next(camio_istream_t* this){
    camio_istream_exa_t* priv = this->priv;

    //Simple case, there's already data waiting
    if(unlikely(priv->data_size > 0)){
        return priv->data_size;
    }

    //Is there new data?
    for(int i = 0; i < EXANIC_PORTS; i++){
        const int p = (priv->port + i) % EXANIC_PORTS;

        if(priv->exanic_rx[p] == NULL){
            continue;
        }



//      >0 The length of the frame acquired
//      0 No frame currently available
//      -EXANIC_RX_FRAME_ABORTED Frame aborted by sender
//      -EXANIC_RX_FRAME_CORRUPT Frame failed hardware CRC check
//      -EXANIC_RX_FRAME_HWOVFL Frame lost due to hardware overflow
//      (e.g. insufficient PCIe/memory bandwidth)
//      -EXANIC_RX_FRAME_SWOVFL Frame lost due to software overflow
//      (e.g. scheduling issue)
//      -EXANIC_RX_FRAME_TRUNCATED Supplied buffer was too short

        uint32_t timestamp_lo = 0;
        dag_record_t* erf = (dag_record_t*)&priv->exa_data[0];
        char* ether_head = (char*)&erf->rec.eth.dst;
        const size_t ether_head_offset = ether_head - priv->exa_data;


        ssize_t result = exanic_receive_frame(priv->exanic_rx[p], ether_head, DATA_BUFF- ether_head_offset,  &timestamp_lo);
        if(result > 0){
            const uint64_t timestamp = exanic_timestamp_to_counter(priv->exanic, timestamp_lo);
            erf->ts    = timestamp; //HACK! Note this shoudld be in fixed point format..
            erf->type  = 0; //Hack? Maybe should have something here?
            erf->flags.iface = p; //Low bits are the port number
            erf->flags.reserved = 1; //Tell the reciever this is an unusual timestamp
            erf->rlen  = htons(ether_head_offset +  result);
            erf->lctr  = 0;
            erf->wlen  = htons((uint16_t)result);

            priv->port = p + 1;
            priv->data_size = result;
            //printf("Got data of size =%li on port =%i\n",priv->data_size,p);
            return priv->data_size;
        }

    }

    return 0;
}

int64_t camio_istream_exa_ready(camio_istream_t* this){
    camio_istream_exa_t* priv = this->priv;
    if(priv->data_size || priv->is_closed){
        return 1;
    }

    return prepare_next(this);
}


int64_t camio_istream_exa_start_read(camio_istream_t* this, uint8_t** out){
    camio_istream_exa_t* priv = this->priv;
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

//    camio_time_t current;
//    camio_exatime_to_time(&current,priv->exa_data);
//    //this->clock->set(this->&current);

    *out = (uint8_t*)priv->exa_data;
    return priv->data_size;
}


int64_t camio_istream_exa_end_read(camio_istream_t* this, uint8_t* free_buff){
    camio_istream_exa_t* priv = this->priv;
    priv->data_size = 0;
    return 0;
}


void camio_istream_exa_delete(camio_istream_t* this){
    this->close(this);
    camio_istream_exa_t* priv = this->priv;
    free(priv);
}

/* ****************************************************
 * Construction
 */

camio_istream_t* camio_istream_exa_construct(camio_istream_exa_t* priv, const camio_descr_t* descr,  camio_istream_exa_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"exa stream supplied is null\n");
    }

    //Initialize the local variables
    priv->is_closed         = 1;
    priv->exanic            = NULL;
    priv->data_size			= 0;
    priv->params            = params;
    priv->exanic_rx[0]      = NULL;
    priv->exanic_rx[1]      = NULL;
    priv->exanic_rx[2]      = NULL;
    priv->exanic_rx[3]      = NULL;

    //Populate the function members
    priv->istream.priv          = priv; //Lets us access private members
    priv->istream.open          = camio_istream_exa_open;
    priv->istream.close         = camio_istream_exa_close;
    priv->istream.start_read    = camio_istream_exa_start_read;
    priv->istream.end_read      = camio_istream_exa_end_read;
    priv->istream.ready         = camio_istream_exa_ready;
    priv->istream.delete        = camio_istream_exa_delete;
    priv->istream.fd            = -1;

    //Call open, because its the obvious thing to do now...
    priv->istream.open(&priv->istream, descr);

    //Return the generic istream interface for the outside world to use
    return &priv->istream;

}

camio_istream_t* camio_istream_exa_new( const camio_descr_t* descr,  camio_istream_exa_params_t* params){
    camio_istream_exa_t* priv = malloc(sizeof(camio_istream_exa_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for exa istream creation\n");
    }
    return camio_istream_exa_construct(priv, descr,  params);
}



