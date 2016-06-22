/*
 * Copyright  (C) Matthew P. Grosvenor, 2012, All Rights Reserved
 *
 * Input stream factory definition
 *
 */

#include <string.h>

#include "camio_istream.h"
#include "../camio_errors.h"

#include "camio_istream_log.h"
#include "camio_istream_raw.h"
#include "camio_istream_udp.h"
#include "camio_istream_ring.h"
#include "camio_istream_pcap.h"
#include "camio_istream_periodic_timeout.h"
#include "camio_istream_periodic_timeout_fast.h"
#include "camio_istream_blob.h"
#include "camio_istream_netmap.h"

//#ifdef HAVE_DAG_
#include "camio_istream_dag.h"
//#endif //HAVE_DAG_


#include "camio_istream_exa.h"
camio_istream_t* camio_istream_new(const char* description, void* parameters){
    camio_istream_t* result = NULL;
    camio_descr_t descr;
    camio_descr_construct(&descr);
    camio_descr_parse(description,&descr);

    if(strcmp(descr.protocol,"log") == 0 ){
        result = camio_istream_log_new(&descr,parameters);
    }
    else if(strcmp(descr.protocol,"std-log") == 0 ){
        camio_istream_log_params_t params = { .fd = STDIN_FILENO};
        result = camio_istream_log_new(&descr, &params);
    }
    else if(strcmp(descr.protocol,"raw") == 0 ){
        result = camio_istream_raw_new(&descr,parameters);
    }
    else if(strcmp(descr.protocol,"ring") == 0 ){
        result = camio_istream_ring_new(&descr,parameters);
    }
    else if(strcmp(descr.protocol,"udp") == 0 ){
        result = camio_istream_udp_new(&descr,parameters);
    }
    else if(strcmp(descr.protocol,"periodic") == 0 ){
        result = camio_istream_periodic_timeout_new(&descr,parameters);
    }
    else if(strcmp(descr.protocol,"period_fast") == 0 ){
        result = camio_istream_periodic_timeout_fast_new(&descr,parameters);
    }
    else if(strcmp(descr.protocol,"blob") == 0 ){
        result = camio_istream_blob_new(&descr,parameters);
    }

//    else if(strcmp(descr.protocol,"pcap") == 0 ){
//        result = camio_istream_pcap_new(&descr,NULL);
//    }
//#ifdef HAVE_DAG_
    else if(strcmp(descr.protocol,"dag") == 0 ){
        result = camio_istream_dag_new(&descr,parameters);
    }
    else if(strcmp(descr.protocol,"exa") == 0 ){
        result = camio_istream_exa_new(&descr,parameters);
    }
    else if(strcmp(descr.protocol,"nmap") == 0 ){
        result = camio_istream_netmap_new(&descr,parameters);
    }
//#endif /*camio_ISTREAM_BUILD_WITH_DAG_*/
    else{
        eprintf_exit(CAMIO_ERR_UNKNOWN_ISTREAM,"Could not create istream from description \"%s\" \n", description);
    }

    camio_descr_destroy(&descr);
    return result;

}


