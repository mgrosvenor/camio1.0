/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ NETMAP card input stream
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
#include <sys/ioctl.h>
#include <linux/ethtool.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/poll.h>

#include "camio_istream_netmap.h"
#include "../camio_errors.h"
#include "../camio_util.h"
#include "../clocks/camio_time.h"

#include "../netmap/netmap.h"
#include "../netmap/netmap_user.h"

//Based on version in nm_util.c
//This version is much faster as the socket fd and ifreq are cached at startup
static void do_ioctl_flags(const char* ifname, size_t flags)
{
    int socket_fd;
    struct ifreq ifr;

    //Get a control socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        eprintf_exit(CAMIO_ERR_SOCKET, "Could not open device control socket\n");
    }

    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

    ifr.ifr_flags = flags & 0xffff;

    if( ioctl(socket_fd, SIOCSIFFLAGS, &ifr) ){
        eprintf_exit(CAMIO_ERR_IOCTL, "Could not set interface flags 0x%08x\n", flags);
    }

    close(socket_fd);
}


static void do_ioctl_ethtool(const char* ifname, int subcmd)
{

    int socket_fd;
    struct ifreq ifr;

    //Get a control socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        eprintf_exit(CAMIO_ERR_SOCKET, "Could not open device control socket\n");
    }

    bzero(&ifr, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

    struct ethtool_value eval;
    eval.cmd = subcmd;
    eval.data = 0;
    ifr.ifr_data = (caddr_t)&eval;

    if( ioctl(socket_fd, SIOCETHTOOL, &ifr) ){
        eprintf_exit(CAMIO_ERR_IOCTL, "Could not set ethtool value\n");
    }

}

int64_t camio_istream_netmap_open(camio_istream_t* this, const camio_descr_t* descr ){
    camio_istream_netmap_t* priv = this->priv;
    int netmap_fd = -1;

    if(unlikely((size_t)descr->opt_head)){
        eprintf_exit(CAMIO_ERR_UNKNOWN_OPT, "Option(s) supplied, but none expected\n");
    }

    if(unlikely(!descr->query)){
        eprintf_exit(CAMIO_ERR_NULL_PTR, "No device supplied\n");
    }
    const char* iface = descr->query;


    if(priv->params && priv->params->nm_mem){
        priv->nm_mem      = priv->params->nm_mem;
        priv->mem_size    = priv->params->nm_mem_size;
        priv->nm_offset   = priv->params->nm_offset;
        priv->nm_rx_rings = priv->params->nm_rx_rings;
        priv->nm_tx_rings = priv->params->nm_tx_rings;
        priv->nm_rx_slots = priv->params->nm_rx_slots;
        priv->nm_tx_slots = priv->params->nm_tx_slots;
        priv->nm_ringid   = priv->params->nm_ringid;
        netmap_fd      = priv->params->fd;
    }
    else{

        struct nmreq req;

        //Request a specific interface
        bzero(&req, sizeof(req));
        req.nr_version = NETMAP_API;
        strncpy(req.nr_name, iface, sizeof(req.nr_name));
        req.nr_ringid = 0; //All hw rings

        //Open the netmap device
        netmap_fd = open("/dev/netmap", O_RDWR);
       
	//printf("Opening %s\n", "/dev/netmap = %i\n", netmap_fd);
        if(unlikely(netmap_fd < 0)){
            eprintf_exit(CAMIO_ERR_FILE_OPEN, "Could not open file \"%s\". Error=%s\n", "/dev/netmap", strerror(errno));
        }

        if(ioctl(netmap_fd, NIOCREGIF, &req)){
            eprintf_exit(CAMIO_ERR_IOCTL, "Could not register netmap interface %s\n", descr->query);
        }

        priv->mem_size = req.nr_memsize;
        printf("Memsize  = %u\n"
                "Ringid   = %u\n"
                "Offset   = %u\n"
                "rx rings = %u\n"
                "rx slots = %u\n",
                req.nr_memsize / 1024 / 1204,
                req.nr_ringid,
                req.nr_offset,
                req.nr_rx_rings,
                req.nr_rx_slots);


        priv->nm_mem = mmap(0, priv->mem_size, PROT_WRITE | PROT_READ, MAP_SHARED, netmap_fd, 0);
        if(unlikely(priv->nm_mem == MAP_FAILED)){
            eprintf_exit(CAMIO_ERR_MMAP, "Could not memory map blob file \"%s\". Error=%s\n", descr->query, strerror(errno));
        }

        priv->nm_offset = req.nr_offset;
        priv->nm_rx_rings = req.nr_rx_rings;
        priv->nm_rx_slots = req.nr_rx_slots;
        priv->nm_tx_rings = req.nr_tx_rings;
        priv->nm_tx_slots = req.nr_tx_slots;
        priv->nm_ringid   = req.nr_ringid;
    }

    printf("nm_mem=%p\n", priv->nm_mem);

    priv->nifp = NETMAP_IF(priv->nm_mem, priv->nm_offset);
    printf("nifp=%p\n", priv->nifp);

    int i = 0;
    //printf("nifp at offset %d, %d tx %d rx rings %s\n",
     //       priv->nm_offset, priv->nm_tx_rings, priv->nm_rx_rings,
     //       priv->nm_ringid & NETMAP_PRIV_MEM ? "PRIVATE" : "common" );
    for (i = 0; i <= priv->nm_tx_rings; i++) {
        //printf("   TX%d at 0x%lx\n", i, (char *)NETMAP_TXRING(priv->nifp, i) - (char *)priv->nifp);
    }
    for (i = 0; i <= priv->nm_rx_rings; i++) {
        //printf("   RX%d at 0x%lx\n", i, (char *)NETMAP_RXRING(priv->nifp, i) - (char *)priv->nifp);
    }


    //Make sure the interface is up an promiscuious
    do_ioctl_flags(descr->query, IFF_UP | IFF_PROMISC );

    //Turn off all the offload features on the card
    do_ioctl_ethtool(descr->query, ETHTOOL_SGSO);
    do_ioctl_ethtool(descr->query, ETHTOOL_STSO);
    do_ioctl_ethtool(descr->query, ETHTOOL_SRXCSUM);
    do_ioctl_ethtool(descr->query, ETHTOOL_STXCSUM);

    //Do we want to use the local linux qeues or the netmap harware queues
    if(priv->params->use_local){
        priv->begin = priv->nm_rx_rings;
        priv->end   = priv->nm_rx_rings + 1;
    }
    else{
        priv->begin = 0;
        priv->end   = priv->nm_rx_rings;
    }


    //Netmap lays out memory so that all packet buffers are interchangeable.
    //All packets begin at packet_buffer_bottom and are arranged in an array
    //of 2K offset from there. We need to get the address of the bottom of this
    //to calculate relative packets
    struct netmap_ring *rxring = NETMAP_RXRING(priv->nifp, priv->begin);
    priv->packet_buff_bottom = ((uint8_t *)(rxring) + (rxring)->buf_ofs);

    //printf("---->\n");
    i = 0;
    for(; i < 8; i++){
       // printf("rx ring %i at %li\n",i, (char*)NETMAP_RXRING(priv->nifp, i) - (char*)priv->nifp);
    }

    //printf("<:---->\n");

    this->fd = netmap_fd;
    priv->is_closed = 0;
    return CAMIO_ERR_NONE;
}


void camio_istream_netmap_close(camio_istream_t* this){
    camio_istream_netmap_t* priv = this->priv;
    priv->is_closed = 1;
    //    if(priv->nm_mem){
    //        munmap(priv->nm_mem, priv->mem_size);
    //        priv->nm_mem   = NULL;
    //        priv->mem_size = 0;
    //    }
    //ioctl(this->fd, NIOCUNREGIF, NULL);
    //close(this->fd);
}

static int prepare_next(camio_istream_t* this){
    camio_istream_netmap_t* priv = this->priv;


    //Simple case, there's already data waiting
    if(unlikely((size_t)priv->packet)){
	//printf("Packet waiting %lu\n", priv->packet_size);
        return priv->packet_size;
    }


    size_t i = 0;
    for (i = priv->begin; i < priv->end; i++) {
        //printf("Looking at ring %i\n", i);
	struct netmap_ring *ring = NETMAP_RXRING(priv->nifp, i);
        
	//printf("ring=%p\n",ring);
	if(likely(nm_ring_space(ring) > 0)){
            //printf("ring idx=%lu, ring->cur=%u, nm_ring_space=%u | ", i, ring->cur, nm_ring_space(ring) );
            priv->packet        = NETMAP_BUF(ring, ring->slot[ring->cur].buf_idx);
            priv->packet_size   = ring->slot[ring->cur].len;
            priv->nm_slot       = &ring->slot[ring->cur];
            priv->ring          = ring; //Keep the ring for later
            //printf("slot.len=%lu | ", priv->packet_size );
            //printf("buf_idx=%u\n", ring->slot[ring->cur].buf_idx);
            return 1;
        }
    }

    //We've run out, call this and hope it's better next time
    ioctl(this->fd, NIOCRXSYNC, NULL);
    return 0 ;
}

int64_t camio_istream_netmap_ready(camio_istream_t* this){
    camio_istream_netmap_t* priv = this->priv;
    if(priv->packet_size || priv->is_closed){
        return 1;
    }

    return prepare_next(this);
}



int64_t camio_istream_netmap_start_read(camio_istream_t* this, uint8_t** out){
    camio_istream_netmap_t* priv = this->priv;
    *out = NULL;

    if(unlikely(priv->is_closed)){
        return 0;
    }

    if(unlikely(!priv->packet_size)){

        //Called read without calling ready, they must want to block/spin waiting for data .. ok
        while(!prepare_next(this)){
            //There are no packets, call syncronise
            //struct pollfd fds[1] = {{0}};
            //fds[0].fd = this->fd;
            //fds[0].events = (POLLIN);
            //printf("Polling for packets on %i...\n", this->fd);
            //if( poll(fds, 1, -1) < 0){
            //    eprintf_exit_simple("Poll error!\n");
            //}
            //printf("Success!\na");      
        }
    }

    *out = (uint8_t*)priv->packet;
    return priv->packet_size;
}

size_t count = 0; 
int64_t camio_istream_netmap_end_read(camio_istream_t* this,uint8_t* free_buff){
    camio_istream_netmap_t* priv = this->priv;

    if(free_buff){
        size_t offset = ((void*)free_buff - priv->packet_buff_bottom) / 2048;
        priv->nm_slot->buf_idx  = offset;
        //printf("istream: fast path with offset=%lu --from ostream\n",offset);
        priv->nm_slot->len      = 2048;
        priv->nm_slot->flags   |= NS_BUF_CHANGED;

        //Make sure the free buffers get back
        if(count && count % 512 == 0){        
            ioctl(this->fd, NIOCRXSYNC, NULL);
            count++;
        }
    }

    //Advance the ring pointer now that we're done
    priv->ring->cur = nm_ring_next(priv->ring, priv->ring->cur);
    priv->ring->head = priv->ring->cur;
    priv->packet_size = 0;
    priv->packet      = NULL;
    return 0;
}


void camio_istream_netmap_delete(camio_istream_t* this){
    this->close(this);
    camio_istream_netmap_t* priv = this->priv;
    free(priv);
}

/* ****************************************************
 * Construction
 */

camio_istream_t* camio_istream_netmap_construct(camio_istream_netmap_t* priv, const camio_descr_t* descr,  camio_istream_netmap_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"netmap stream supplied is null\n");
    }

    //Initialize the local variables
    priv->is_closed             = 0;
    priv->nm_mem                = NULL;
    priv->mem_size              = 0;
    priv->nifp                  = NULL;
    priv->rx                    = NULL;
    priv->begin                 = 0;
    priv->end                   = 0;
    priv->packets_waiting       = 0;
    priv->packet                = 0;
    priv->packet_size           = 0;
    priv->packet_buff_bottom    = NULL;
    priv->nm_slot               = NULL;
    priv->ring                  = NULL;
    priv->params                = params;


    //Populate the function members
    priv->istream.priv          = priv; //Lets us access private members
    priv->istream.open          = camio_istream_netmap_open;
    priv->istream.close         = camio_istream_netmap_close;
    priv->istream.start_read    = camio_istream_netmap_start_read;
    priv->istream.end_read      = camio_istream_netmap_end_read;
    priv->istream.ready         = camio_istream_netmap_ready;
    priv->istream.delete        = camio_istream_netmap_delete;
    priv->istream.fd            = -1;

    //Call open, because its the obvious thing to do now...
    priv->istream.open(&priv->istream, descr);

    //Return the generic istream interface for the outside world to use
    return &priv->istream;

}

camio_istream_t* camio_istream_netmap_new( const camio_descr_t* descr,  camio_istream_netmap_params_t* params){
    camio_istream_netmap_t* priv = malloc(sizeof(camio_istream_netmap_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for netmap istream creation\n");
    }
    return camio_istream_netmap_construct(priv, descr,  params);
}



