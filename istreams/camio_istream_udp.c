/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ udp socket input stream
 *
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "camio_istream_udp.h"
#include "../camio_errors.h"



int camio_istream_udp_open(camio_istream_t* this, const camio_descr_t* descr ){
    camio_istream_udp_t* priv = this->priv;
    char ip_addr[17]; //IP addr is worst case, 16 bytes long (255.255.255.255)
    char udp_port[6]; //UDP port is wost case, 5 bytes long (65536)
    int udp_sock_fd;

    if(descr->opt_head){
         eprintf_exit(CAMIO_ERR_UNKNOWN_OPT, "Option(s) supplied, but none expected\n");
     }

    if(!descr->query){
        eprintf_exit(CAMIO_ERR_SOCKET, "No address supplied\n");
    }

    const size_t query_len = strlen(descr->query);
    if(query_len > 22){
        eprintf_exit(CAMIO_ERR_SOCKET, "IP Address supplied is >22 chars \"%s\"\n", descr->query);
    }

    //Find the IP:port
    size_t i = 0;
    for(; i < query_len; i++ ){
        if(descr->query[i] == ':'){
            memcpy(ip_addr,descr->query,i);
            ip_addr[i] = '\0';
            memcpy(udp_port,&descr->query[i+1],query_len - i -1);
            udp_port[query_len - i -1] = '\0';
            break;
        }
    }


    if(query_len > 22){
        eprintf_exit(CAMIO_ERR_SOCKET, "IP Address supplied does not have a \":\" for the port number \"%s\"\n", descr->query);
    }


    priv->buffer = malloc(getpagesize() * 1024); //Allocate 4Mb for the buffer
    if(!priv->buffer){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "Failed to allocate message buffer\n");
    }
    priv->buffer_size = getpagesize() * 1024;

    /* Open the udp socket MAC/PHY layer output stage */
    udp_sock_fd = socket(AF_INET,SOCK_DGRAM,0);
    if (udp_sock_fd < 0 ){
        eprintf_exit(CAMIO_ERR_SOCKET,"Could not open udp socket. Error = %s\n",strerror(errno));
    }

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port        = htons(strtol(udp_port,NULL,10));

    printf("%X\n", addr.sin_addr.s_addr);

    if( bind(udp_sock_fd, (struct sockaddr *)&addr, sizeof(addr)) ){
         eprintf_exit(CAMIO_ERR_BIND,"Could not bind raw socket. Error = %s\n",strerror(errno));
    }

    int RCVBUFF_SIZE = 512 * 1024 * 1024;
    if (setsockopt(udp_sock_fd, SOL_SOCKET, SO_RCVBUF, &RCVBUFF_SIZE, sizeof(RCVBUFF_SIZE)) < 0) {
        eprintf_exit(CAMIO_ERR_SOCK_OPT,"Could not set socket option. Error = %s\n",strerror(errno));
    }


    priv->addr = addr;
    this->fd = udp_sock_fd;
    priv->is_closed = 0;
    return CAMIO_ERR_NONE;

}


void camio_istream_udp_close(camio_istream_t* this){
    camio_istream_udp_t* priv = this->priv;
    close(this->fd);
    free(priv->buffer);
}

static void set_fd_blocking(int fd, int blocking){
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1){
        eprintf_exit(CAMIO_ERR_FILE_FLAGS, "Could not get file flags\n");
    }

    if (blocking){
        flags &= ~O_NONBLOCK;
    }
    else{
        flags |= O_NONBLOCK;
    }

    if( fcntl(fd, F_SETFL, flags) == -1){
        eprintf_exit(CAMIO_ERR_FILE_FLAGS, "Could not set file flags\n");
    }
}

static int prepare_next(camio_istream_udp_t* priv, int blocking){
    if(priv->bytes_read){
        return priv->bytes_read;
    }

    set_fd_blocking(priv->istream.fd, blocking);

    int bytes = recv(priv->istream.fd,priv->buffer,priv->buffer_size, 0);
    if( bytes < 0){
        eprintf_exit(CAMIO_ERR_RCV,"Could not receive from socket. Error = %s\n",strerror(errno));
    }

    priv->bytes_read = bytes;
    return bytes;

}

int camio_istream_udp_ready(camio_istream_t* this){
    camio_istream_udp_t* priv = this->priv;
    if(priv->bytes_read || priv->is_closed){
        return 1;
    }

    return prepare_next(priv,0);
}


static int camio_istream_udp_start_read(camio_istream_t* this, uint8_t** out){
    *out = NULL;

    camio_istream_udp_t* priv = this->priv;
    if(priv->is_closed){
        return 0;
    }

    //Called read without calling ready, they must want to block
    if(!priv->bytes_read){
        if(!prepare_next(priv,1)){
            return 0;
        }
    }

    *out = priv->buffer;
    size_t result = priv->bytes_read; //Strip off the newline
    priv->bytes_read = 0;

    return  result;
}


int camio_istream_udp_end_read(camio_istream_t* this, uint8_t* free_buff){
    return 0; //Always true for socket I/O
}


void camio_istream_udp_delete(camio_istream_t* this){
    this->close(this);
    camio_istream_udp_t* priv = this->priv;
    free(priv);
}

/* ****************************************************
 * Construction
 */

camio_istream_t* camio_istream_udp_construct(camio_istream_udp_t* priv, const camio_descr_t* descr,  camio_istream_udp_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"udp stream supplied is null\n");
    }
    //Initialize the local variables
    priv->is_closed         = 1;
    priv->buffer            = NULL;
    priv->buffer_size       = 0;
    priv->bytes_read        = 0;
    priv->params            = params;


    //Populate the function members
    priv->istream.priv          = priv; //Lets us access private members
    priv->istream.open          = camio_istream_udp_open;
    priv->istream.close         = camio_istream_udp_close;
    priv->istream.start_read    = camio_istream_udp_start_read;
    priv->istream.end_read      = camio_istream_udp_end_read;
    priv->istream.ready         = camio_istream_udp_ready;
    priv->istream.delete        = camio_istream_udp_delete;
    priv->istream.fd            = -1;

    //Call open, because its the obvious thing to do now...
    priv->istream.open(&priv->istream, descr);

    //Return the generic istream interface for the outside world to use
    return &priv->istream;

}

camio_istream_t* camio_istream_udp_new( const camio_descr_t* descr,  camio_istream_udp_params_t* params){
    camio_istream_udp_t* priv = malloc(sizeof(camio_istream_udp_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for udp istream creation\n");
    }
    return camio_istream_udp_construct(priv, descr,  params);
}





