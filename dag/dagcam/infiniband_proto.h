/*
 * Copyright (c) 2007-2009 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: infiniband_proto.h,v 1.7 2009/06/10 02:45:56 karthik Exp $
 * Author: Vladimir Minkov 
 */

#ifndef INFINIBAND_PROTO_H
#define INFINIBAND_PROTO_H

/* Common headers. */
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <inttypes.h>
#elif defined(_WIN32)
#include <wintypedefs.h>
#else
#error Compiling on an unsupported platform - please contact <support@endace.com> for assistance.
#endif /* Platform-specific code. */


/**
 * \file
 * \brief High-level filter rules token used for infiband or other filtering protocol 
 *
 * Declares the Token used by the lexical gramer for protocol used for filtering 
 * as well as the variable structure for each rule
 * 
 */

	/** 
	 * Infiniband rule structure 
	 * this stracture will be returned after each rule scanned 
	 * 
	 */
typedef struct infiniband_filter_rule_s {

	/* LRH Header classification fields */
		/* The following 3 fields are not supported in the current 
		   Infiniband filtering version : link_version 4bits, virtual_lane 4 bits , packet_length : 11 bits;
		*/
	/* drop 1(true) , accept 0(false) */
    int action ;
    uint16_t user_tag;
    uint8_t user_class; /* used for steering on some versions at the moment 0*/
    
	/** 
	* Infiniband LRH header 
	 */
    struct  { /* 4 bits service level*/
	    uint8_t data;
	    uint8_t mask;
    } service_level; /**< Service level 4 bits */
    
    struct  { /* 2 bits LNH */
	    uint8_t data;
	    uint8_t mask; 
    } lnh ;
    
    struct  { 
	    uint16_t data;
	    uint16_t mask;
    } src_local_id;  /**< Source local ID 16 bits data and mask */
    
    struct  { /* 16 bits dlid*/
	    uint16_t data;
	    uint16_t mask;
    } dest_local_id; /**< Destination local 16 bits */

    /** 
     * BTH Header classification fields 
    */ 
    struct { 
	    uint8_t data;
	    uint8_t mask; 
    } opcode; /**< Opcode 8 bits */
    
    struct { /* 24 bits destination QP*/
	    uint32_t data;
	    uint32_t mask;
    }dest_qp ;

	/** DETH Header classification fields */
    struct { /* 24 bits source QP*/
	    uint32_t data;
	    uint32_t mask;
    }src_qp ;

	/** LINK Packet Identification Bit - 1 BIT**/
	struct  { /* 1 bits Link idendification */
	    uint8_t data;
	    uint8_t mask; 
    } link_id ;
} infiniband_filter_rule_t, *infiniband_filter_rule_p;
 
typedef enum  
{
    kTypeUnknown,
    k32Bit,
    k128Bit,
}ip_addr_type_t;
typedef struct 
{
    uint16_t ipv4_supported :1;
    uint16_t ipv6_supported :1;
    uint16_t vlan_skipping_supported :1;
    uint16_t vlan_filtering_supported :1;
    uint16_t vlan_tag_supported :1;
    uint16_t mpls_skipping_supported :1;
    uint16_t mpls_filtering_supported :1;
    uint16_t width576_supported :1;
    uint16_t width288_supported :1;
} ipf_capabilities_t;

typedef struct 
{
    uint16_t ipv4_supported :1;
    uint16_t ipv6_supported :1;
    uint16_t vlan_skipping_supported :1;
    uint16_t vlan_filtering_supported :1;
    uint16_t vlan_tag_supported :1;
    uint16_t mpls_skipping_supported :1;
    uint16_t mpls_filtering_supported :1;
    uint16_t width576_supported :1;
    uint16_t width288_supported :1;
} bfs_capabilities_t;

typedef struct ipf_v2_filter_rule_s {
     int action ;
    uint16_t user_tag;
    uint8_t user_class; /* used for steering on some versions at the moment 0*/
    ip_addr_type_t ip_address_type;/* used to differntiate between 32 bit and 128 bit addresses */
    uint8_t rule_set;
    /* iface */
    struct {
        uint8_t data;
        uint8_t mask;
        }iface;
    /*l2-proto*/
    struct {
        uint16_t data;
        uint16_t mask;
        }l2_proto;
    /* mpls-top*/
    struct {
        uint32_t data;
        uint32_t mask;
        }mpls_top;
    /*mpls-btm*/
    struct {
        uint32_t data;
        uint32_t mask;
        }mpls_bottom;
    /* vlan-1*/
    struct {
        uint16_t data;
        uint16_t mask;
        }vlan_1;
    /* vlan-2*/
    struct {
        uint16_t data;
        uint16_t mask;
        }vlan_2;
    /*dst-ip */
    struct {
        uint64_t data[2];
        uint64_t mask[2];
    }dst_ip;
    /*src-ip */
    struct {
        uint64_t data[2];
        uint64_t mask[2];
    }src_ip;
    /*ip-prot */
    struct {
        uint8_t data;
        uint8_t mask;
    }ip_prot;
    /* src-port*/
    struct {
        uint16_t data;
        uint16_t mask;
        }src_port;
    /* dst-port*/
    struct {
        uint16_t data;
        uint16_t mask;
        }dst_port;
    /*tcp-flags*/
    struct {
        uint8_t data;
        uint8_t mask;
        }tcp_flags;
    /*label-cnt*/
    struct {
        uint8_t data;
        uint8_t mask;
        }label_cnt;
} ipf_v2_filter_rule_t, *ipf_v2_filter_rule_p;

typedef struct bfs_filter_rule_s {
     int action ;
    uint16_t user_tag;
    uint8_t user_class; /* used for steering on some versions at the moment 0*/
    ip_addr_type_t ip_address_type;/* used to differntiate between 32 bit and 128 bit addresses */
    uint8_t rule_set;
    /* iface */
    struct {
        uint8_t data;
        uint8_t mask;
        }iface;
    /*l2-proto*/
    struct {
        uint16_t data;
        uint16_t mask;
        }l2_proto;
    /* mpls-top*/
    struct {
        uint32_t data;
        uint32_t mask;
        }mpls_top;
    /*mpls-btm*/
    struct {
        uint32_t data;
        uint32_t mask;
        }mpls_bottom;
    /* vlan-1*/
    struct {
        uint16_t data;
        uint16_t mask;
        }vlan_1;
    /* vlan-2*/
    struct {
        uint16_t data;
        uint16_t mask;
        }vlan_2;
    /*dst-ip */
    struct {
        uint64_t data[2];
        uint64_t mask[2];
    }dst_ip;
    /*src-ip */
    struct {
        uint64_t data[2];
        uint64_t mask[2];
    }src_ip;
    /*ip-prot */
    struct {
        uint8_t data;
        uint8_t mask;
    }ip_prot;
    /* src-port*/
    struct {
        uint16_t data;
        uint16_t mask;
        }src_port;
    /* dst-port*/
    struct {
        uint16_t data;
        uint16_t mask;
        }dst_port;
    /*tcp-flags*/
    struct {
        uint8_t data;
        uint8_t mask;
        }tcp_flags;
    /*label-cnt*/
    struct {
        uint8_t data;
        uint8_t mask;
        }label_cnt;
} bfs_filter_rule_t, *bfs_filter_rule_p;


int get_ifp_v2_filtering_capabilities ( const char* dagname, ipf_capabilities_t *cap);
void set_ipf_capabiity(ipf_capabilities_t *cap );
void set_ipf_v2_rule_width(int width);

int get_bfs_filtering_capabilities ( const char* dagname, bfs_capabilities_t *cap);
void set_bfs_capabiity(bfs_capabilities_t *cap );
void set_bfs_rule_width(int width);

/** common scanners results for the protocol */
#define T_ERROR -1		/**> Error while the scanner recognizes the rule common */
#define T_ERROR_PARAMETER -2 	/**> Error while the scanner recognises the parameters  */
#define T_ERROR_CONVERT -3 	/**> Error while the scanner convert the parameter  */

#define T_RULE_CONTINUE 2	/**> Recognized a multi line rule the result is partial and has to continue on the new line */
#define T_RULE_DONE 1		/**> the rule has been recognised and the filter varibale is finished */

/* used for both debuging and for LOG messages 
* TODO:
* At the moment interanly the scanner_out will be set to stdout 
*/ 
int scanner_set_stdout(FILE *scanner_out);
/* variable which is cleared after each rule begin(correctly recognized user tag)
 * and set after each recognition including the partially recognized rules
 */
extern infiniband_filter_rule_t infini_filter_rule;
extern ipf_v2_filter_rule_t ipf_v2_filter_rule;
extern bfs_filter_rule_t bfs_filter_rule;

#endif /* INFINIBAND_PROTO_H */
