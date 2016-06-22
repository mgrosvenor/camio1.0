/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Fe2+ netmap (newline separated) output stream
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
#include <time.h>
#include <sys/time.h>
#include <sys/poll.h>


#include "../camio_errors.h"
#include "../camio_util.h"
#include "../clocks/camio_time.h"
#include "../netmap/netmap.h"
#include "../netmap/netmap_user.h"

#include "camio_ostream_netmap.h"

//static void hex_dump(void *data, int size)
//{
//    /* dumps size bytes of *data to stdout. Looks like:
//     * [0000] 75 6E 6B 6E 6F 77 6E 20
//     *                  30 FF 00 00 00 00 39 00 unknown 0.....9.
//     * (in a single line of course)
//     */
//
//    unsigned char *p = data;
//    unsigned char c;
//    int n;
//    char bytestr[4] = {0};
//    char addrstr[10] = {0};
//    char hexstr[ 16*3 + 5] = {0};
//    char charstr[16*1 + 5] = {0};
//    for(n=1;n<=size;n++) {
//        if (n%16 == 1) {
//            /* store address for this line */
//            snprintf(addrstr, sizeof(addrstr), "%.4x",
//               ((unsigned int)p-(unsigned int)data) );
//        }
//
//        c = *p;
//        if (isalnum(c) == 0) {
//            c = '.';
//        }
//
//        /* store hex str (for left side) */
//        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
//        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);
//
//        /* store char str (for right side) */
//        snprintf(bytestr, sizeof(bytestr), "%c", c);
//        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);
//
//        if(n%16 == 0) {
//            /* line completed */
//            printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
//            hexstr[0] = 0;
//            charstr[0] = 0;
//        } else if(n%8 == 0) {
//            /* half line: add whitespaces */
//            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
//            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
//        }
//        p++; /* next byte */
//    }
//
//    if (strlen(hexstr) > 0) {
//        /* print rest of buffer if not empty */
//        printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
//    }
//}

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

int camio_ostream_netmap_open(camio_ostream_t* this, const camio_descr_t* descr ){
    camio_ostream_netmap_t* priv = this->priv;
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
        printf("Opening %s\n", "/dev/netmap");
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
    printf("nifp at offset %d, %d tx %d rx rings %s\n",
            priv->nm_offset, priv->nm_tx_rings, priv->nm_rx_rings,
            priv->nm_ringid & NETMAP_PRIV_MEM ? "PRIVATE" : "common" );
    for (i = 0; i <= priv->nm_tx_rings; i++) {
        printf("   TX%d at 0x%lx\n", i, (char *)NETMAP_TXRING(priv->nifp, i) - (char *)priv->nifp);
    }
    for (i = 0; i <= priv->nm_rx_rings; i++) {
        printf("   RX%d at 0x%lx\n", i, (char *)NETMAP_RXRING(priv->nifp, i) - (char *)priv->nifp);
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
        priv->begin = priv->nm_tx_rings;
        priv->end   = priv->nm_tx_rings + 1;
    }
    else{
        priv->begin = 0;
        priv->end   = priv->nm_tx_rings;
    }

    //Netmap lays out memory so that all packet buffers are interchangeable.
    //All packets begin at packet_buffer_bottom and are arranged in an array
    //of 2K offset from there. We need to get the address of the bottom of this
    //to calculate relative packets
    struct netmap_ring *txring = NETMAP_TXRING(priv->nifp, priv->begin);
    priv->packet_buff_bottom = ((uint8_t *)(txring) + (txring)->buf_ofs);

    for(i = 0; i < priv->end; i++){
        printf("tx ring %i at %p\n",i, NETMAP_TXRING(priv->nifp, i));
    }

    //Get ready to use the packet buffs
    for(i = 0; i < priv->nm_tx_rings;i++){
		priv->rings[i] = priv->nm_tx_slots;
    }
    priv->total_slots		= priv->nm_tx_rings * priv->nm_tx_slots;
   //priv->available_buffs	= req.nr_tx_rings * req.nr_tx_rings;
    priv->ring_num			= 0;
    priv->slot_num			= priv->rings[priv->ring_num];
    priv->ring_count		= priv->nm_tx_rings;
    priv->slot_count		= priv->nm_tx_slots;

    this->fd = netmap_fd;
    priv->is_closed = 0;

	for (i = priv->begin; i < priv->end; i++) {
		struct netmap_ring *ring = NETMAP_TXRING(priv->nifp, i);
		printf("%i space=%u\n", i, nm_ring_space(ring) );
	}


    return CAMIO_ERR_NONE;
}

void camio_ostream_netmap_close(camio_ostream_t* this){
    camio_ostream_netmap_t* priv = this->priv;
    priv->is_closed = 1;
   
    printf("Flushing...\n");
    ioctl(this->fd, NIOCTXSYNC, NULL);

    /* final part: wait all the TX queues to be empty. */
//    size_t i = 0;
//    for (i = priv->begin; i < priv->end; i++) {
//    	struct netmap_ring* txring = NETMAP_TXRING(priv->nifp, i);
//
//    	while (nm_tx_pending(txring)) {
//    		printf("ring=%i, count=%u pending=%u\n", i, nm_ring_space(txring), nm_tx_pending(txring));
//    		if(ioctl(this->fd, NIOCTXSYNC, NULL) < 0){
//    			//goto done;
//    			return;
//    		}
//    		usleep(1); /* wait 1 tick */
//    	}
//    }

    //HACK!!
    sleep(2);

    printf("Finished flushing\n");

}





struct netmap_slot* get_free_slot(camio_ostream_t* this ){
    camio_ostream_netmap_t* priv = this->priv;
    struct netmap_slot* result = NULL;
    struct pollfd fds[1];
    fds[0].fd = this->fd;
    fds[0].events = (POLLOUT);


    size_t i;
    while(1){
    	//printf("Looking for slots between %i and %i\n", priv->begin, priv->end);
		for (i = priv->begin; i < priv->end; i++) {
			//printf("Looking at %i\n", i);
			//ringid = (i + offset) % priv->end;
			struct netmap_ring *ring = NETMAP_TXRING(priv->nifp, i);
			//printf("space=%lu\n",nm_ring_space(ring) );
			if(nm_ring_space(ring) > 0){
				//ring->avail--;
				priv->ring_num = i;
				priv->slot_num = ring->cur;
				result = &ring->slot[ring->cur];
				//printf("(Avail =%u)Packet buffer of size %lu is available on ring %lu at slot %u (%u) buff idx=%u\n", ring->avail, result->len, i, ring->cur,ring->avail, result->buf_idx );
                //printf("Slot at buffer idx=%u\n",  result->buf_idx );
				return result;
			}
        }
        
		//printf("Polling for more slots...\n");
        if(poll(fds, 1, 10000) <= 0){
		    eprintf_exit(CAMIO_ERR_SEND,"Failed on poll %s\n", strerror(errno));
        }

    }

    return result;

}

static inline uint8_t* get_buffer(camio_ostream_netmap_t* priv, struct netmap_slot* slot){


    struct netmap_ring *ring = NETMAP_TXRING(priv->nifp, priv->ring_num);
    
    priv->packet = NETMAP_BUF(ring, slot->buf_idx);
    priv->packet_size = ring->slot->len;
    

    return  priv->packet;

}

//Returns a pointer to a space of size len, ready for data
uint8_t* camio_ostream_netmap_start_write(camio_ostream_t* this, size_t len ){
    camio_ostream_netmap_t* priv = this->priv;



 if(len > 2 * 1024){
        return NULL;
    }

    if(unlikely((size_t)priv->packet)){
        return priv->packet;
    }

    struct netmap_slot* slot = get_free_slot(this);
    return get_buffer(priv, slot);
}

//Returns non-zero if a call to start_write will be non-blocking
int camio_ostream_netmap_ready(camio_ostream_t* this){
    //Not implemented
    eprintf_exit(CAMIO_ERR_NOT_IMPL, "\n");
    return 0;
}



//Commit the data to the buffer previously allocated
//Len must be equal to or less than len called with start_write
//returns a pointer to a
size_t pcount = 0; 
uint8_t* camio_ostream_netmap_end_write(camio_ostream_t* this, size_t len){
    camio_ostream_netmap_t* priv = this->priv;
    void* result = NULL;   

    if(unlikely(len > 2* 1024)){
        eprintf_exit(CAMIO_ERR_FILE_WRITE, "The supplied length %lu is greater than the buffer size %lu\n", len, priv->packet_size);
    }

    //This is the fast path, for zero copy operation need and assigned buffer
    if(likely((size_t)priv->assigned_buffer)){
        struct netmap_slot* slot = get_free_slot(this);
        uint8_t* buffer = get_buffer(priv,  slot);

        //That comes from the netmap buffer range
        if(likely(priv->assigned_buffer >= priv->packet_buff_bottom && priv->assigned_buffer <= priv->nm_mem + priv->mem_size)){
            size_t offset = (priv->assigned_buffer - priv->packet_buff_bottom) / 2048;

            //printf("ostream: fast path with offset=%lu --input from istream\n", offset);

            slot->buf_idx = offset;
            slot->len     = len;
            slot->flags  |= NS_BUF_CHANGED;
            result        = buffer; //We've taken the buffer from the assignment and are holding on to it so give back another one
        
            //Reset everything
            struct netmap_ring *ring = NETMAP_TXRING(priv->nifp, priv->ring_num);
            ring->cur                = nm_ring_next(ring, ring->cur);
            priv->packet             = NULL;
            priv->packet_size        = 0;
            priv->assigned_buffer    = NULL;
            goto done;
        }
        else{
            //No fast path, looks like we have to copy
            memcpy(buffer,priv->assigned_buffer,len);
            priv->assigned_buffer    = NULL;
            priv->assigned_buffer_sz = 0;
            //Fall through to start_write() handling below
        }
    }

    struct netmap_ring *ring = NETMAP_TXRING(priv->nifp, priv->ring_num);
    ring->slot[priv->slot_num].len = len;
    ring->cur = nm_ring_next(ring, ring->cur);
    ring->head = ring->cur;
    priv->packet = NULL;
    priv->packet_size = 0;

    if(nm_ring_empty(ring)){
	     //printf("Setting report\n");
         ring->slot->flags |= NS_REPORT;
         ioctl(this->fd, NIOCTXSYNC, NULL);
    }


done:    
    if(unlikely(priv->burst_size && pcount && pcount % priv->burst_size == 0)){
        ioctl(this->fd, NIOCTXSYNC, NULL);
    }
    pcount++;


    return result;
}


void camio_ostream_netmap_flush(camio_ostream_t* this){
    //camio_ostream_netmap_t* priv = this->priv;

     //printf("Flushing...\n");
     /* flush any remaining packets */
     ioctl(this->fd, NIOCTXSYNC, NULL);

     /* final part: wait all the TX queues to be empty. */
     /*size_t i = 0;
     for (i = priv->begin; i < priv->end; i++) {
         struct netmap_ring* txring = NETMAP_TXRING(priv->nifp, i);
          while (nm_tx_pending(txring)) {
             //printf("ring=%i, count=%u\n", i, nm_ring_space(txring));
             ioctl(this->fd, NIOCTXSYNC, NULL);
             //usleep(1); // wait 1 tick 
         }
     }i*/

    //printf("Done\n\n");


}



void camio_ostream_netmap_delete(camio_ostream_t* ostream){
    ostream->close(ostream);
    camio_ostream_netmap_t* priv = ostream->priv;
    free(priv);
}

//Is this stream capable of taking over another stream buffer
int camio_ostream_netmap_can_assign_write(camio_ostream_t* this){
    return 0;
}

//Assign the write buffer to the stream
int camio_ostream_netmap_assign_write(camio_ostream_t* this, uint8_t* buffer, size_t len){
    camio_ostream_netmap_t* priv = this->priv;

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

camio_ostream_t* camio_ostream_netmap_construct(camio_ostream_netmap_t* priv, const camio_descr_t* descr,  camio_ostream_netmap_params_t* params){
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"netmap stream supplied is null\n");
    }
    //Initialize the local variables
    priv->is_closed             = 0;
    priv->nm_mem                = NULL;
    priv->mem_size              = 0;
    priv->nifp                  = NULL;
    priv->tx                    = NULL;
    priv->begin                 = 0;
    priv->end                   = 0;
    //priv->available_buffs       = 0;
    priv->packet                = 0;
    priv->packet_size           = 0;
    priv->slot_num              = 0;
    priv->ring_num              = 0;
    priv->assigned_buffer       = NULL;
    priv->assigned_buffer_sz    = 0;
    priv->packet_buff_bottom    = NULL;
    priv->params                = params;
    priv->burst_size            = 0;


    //Populate the function members
    priv->ostream.priv              = priv; //Lets us access private members from public functions
    priv->ostream.open              = camio_ostream_netmap_open;
    priv->ostream.close             = camio_ostream_netmap_close;
    priv->ostream.start_write       = camio_ostream_netmap_start_write;
    priv->ostream.end_write         = camio_ostream_netmap_end_write;
    priv->ostream.ready             = camio_ostream_netmap_ready;
    priv->ostream.delete            = camio_ostream_netmap_delete;
    priv->ostream.can_assign_write  = camio_ostream_netmap_can_assign_write;
    priv->ostream.assign_write      = camio_ostream_netmap_assign_write;
    priv->ostream.flush             = camio_ostream_netmap_flush;
    priv->ostream.fd                = -1;

    //Call open, because its the obvious thing to do now...
    priv->ostream.open(&priv->ostream, descr);

    //Return the generic ostream interface for the outside world
    return &priv->ostream;

}

camio_ostream_t* camio_ostream_netmap_new( const camio_descr_t* descr,  camio_ostream_netmap_params_t* params){
    camio_ostream_netmap_t* priv = malloc(sizeof(camio_ostream_netmap_t));
    if(!priv){
        eprintf_exit(CAMIO_ERR_NULL_PTR,"No memory available for ostream netmap creation\n");
    }
    return camio_ostream_netmap_construct(priv, descr,  params);
}



