/* 
 * Copyright (c) 2003 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Ltd and no part
 * of it may be redistributed, published or disclosed except as outlined
 * in the written contract supplied with this product.
 *
 * $Id: ipf_v2_filter_proto.l,v 1.4 2009/05/12 23:00:33 jomi Exp $ 
 * This file defines the key words which will create the high level rule 
 */ 
%option prefix="ipf_v2"
%option yylineno
%option stack
%option nounput
%option noyywrap
%option noinput
%x S_ipf_v2_rule S_l2_proto S_mpls_top S_mpls_bottom S_vlan_1 S_vlan_2 S_dst_ip S_src_ip S_ip_prot S_src_port S_dst_port S_tcp_flags S_comment S_label_cnt S_iface
BINARY_BIT_MASK [0-1\-\*]
DECIMAL_8bits [0-9]{1,3}
SPACE [ \t]*

HEX_8bits 0\x[0-9a-hA-H]{1,2}
BINARY_PATTERN [ \t]*[ \t0-1\-\*]+[ \t]*
HEX_PATTERN_16bits [ \t]*0x[0-9a-fA-F\-\*]{4,4}[ \t]*

%{
#include <stdlib.h>
#include "infiniband_proto.h"
#include "dag_config.h"

#define out stdout

	/*
	 * Debugging the gramer defines and general output 
	 * IPFV2_RULE_PRINT: will output the actual recognized rules with out extra lines and comments 
	 * IPFV2LEX_DEBUG: Used for intial of the rule and print out of the result 
	 * IPFV2LEX_DEBUG_R: used for the actual rules and individual states 
	 * IPFV2LEX_DEBUG_COMMENT : used for comments debbuging and empty lines 
	*/
//#define IPFV2_RULE_PRINT 1
//#define IPFV2LEX_DEBUG 1
//#define IPFV2LEX_DEBUG_R 1
//#define IPFV2LEX_DEBUG_COMMENT 1

//this is done so multipl parsers to be able to be used in the same application 
//each pareser just have to change this define and the prefix in the option 

#define yy_filter_rule ipf_v2_filter_rule

ipf_v2_filter_rule_t yy_filter_rule;
static int filter_rule_width = 576 ;/* to distinguish 144 bit or 576 bit rules */
static  ipf_capabilities_t *ipf_card_cap = NULL;
/* for the mutual exclusion between mpls and vlan filtering */
static uint8_t is_mpls_filtering = 0;
static uint8_t is_vlan_filtering = 0;
static uint32_t last_line_num = 0;


%}

	/* supporting fuunction to be moved to some part of the library  */
%{
	
#define BINARY_TOKEN_TO_DATAMASK_64( token, bits, data,  mask ) 			\
 do {									\
	unsigned int count = 0;						\
	unsigned int index;						\
	int          buf_index;						\
	int         shift = 0;								\
    uint64_t *tmp_data = data ;               \
    uint64_t *tmp_mask = mask;                \
	if ( token == NULL ) return 0; 					\
	/* protection to work only with 1 to 64 bits */ 		\
	if ( (bits > 128) || (bits < 1) ) return 0; 			\
	if(  bits > 64 ) shift = 64;								\
	count = strlen(token);						\
									\
	/* we require this due we store everything in host order*/ 	\
	buf_index = bits - 1;						\
									\
	/* Create the entry in host byte order then convert to network byte order. */ \
	for (index = 0; index < count; index++)				\
	{								  \
                                     \
		if ((token[index] == '{') || (token[index] == '}'))	\
		{							\
			/* Do nothing. */				\
		} 							\
		else if ((token[index] == ' ') || (token[index] == '\t'))\
		{							\
			/* Do nothing. added for spaces*/		\
		}							\
		else if (token[index] == '0')				\
		{							\
			/* Data bit is already 0. */			\
			*data &= ~(1LL << (buf_index -shift));			\
									\
			/* Set mask bit to 1. */			\
			*mask |= (1LL << (buf_index - shift));			\
									\
			buf_index--;					\
		}							\
		else if (token[index] == '1')				\
		{							\
			/* Set data bit to 1. */			\
			*data |= (1LL << (buf_index -shift) );			\
			/* Set mask bit to 1. */			\
			*mask |= (1LL << (buf_index-shift));			\
									\
			buf_index--;					\
		}							\
		else if( (token[index] == '-') || (token[index] == '*'))	\
		{							\
			/* Set data bit to  0. */			\
			*data &= ~(1LL << (buf_index -shift));			\
			/* Set Mask bit to  0. */			\
			*mask &= ~(1LL << (buf_index-shift));			\
			buf_index--;					\
		}							\
		else							\
		{							\
			return 0;					\
		};							\
    if( buf_index == 63)            \
        {                               \
            data = tmp_data + 1;                     \
            mask = tmp_mask+ 1;                         \
            shift = 0;                      \
        }                       \
	};								\
									\
	/* assert(buf_index == -1);	*/				\
	/* case we provided more bits then allowed */			\
	if (buf_index != -1)						\
		return 0;						\
									\
	return 1;							\
} while(0);  								
	

#define BINARY_TOKEN_TO_DATAMASK( token, bits, data,  mask ) 			\
 do {									\
	unsigned int count = 0;						\
	unsigned int index;						\
	int          buf_index;						\
									\
	if ( token == NULL ) return 0; 					\
	/* protection to work only with 1 to 64 bits */ 		\
	if ( (bits > 64) || (bits < 1) ) return 0; 			\
									\
	count = strlen(token);						\
									\
	/* we require this due we store everything in host order*/ 	\
	buf_index = bits - 1;						\
									\
	/* Create the entry in host byte order then convert to network byte order. */ \
	for (index = 0; index < count; index++)				\
	{								\
		if ((token[index] == '{') || (token[index] == '}'))	\
		{							\
			/* Do nothing. */				\
		} 							\
		else if ((token[index] == ' ') || (token[index] == '\t'))\
		{							\
			/* Do nothing. added for spaces*/		\
		}							\
		else if (token[index] == '0')				\
		{							\
			/* Data bit is already 0. */			\
			*data &= ~(1 << buf_index);			\
									\
			/* Set mask bit to 1. */			\
			*mask |= (1 << buf_index);			\
									\
			buf_index--;					\
		}							\
		else if (token[index] == '1')				\
		{							\
			/* Set data bit to 1. */			\
			*data |= (1 << buf_index);			\
			/* Set mask bit to 1. */			\
			*mask |= (1 << buf_index);			\
									\
			buf_index--;					\
		}							\
		else if( (token[index] == '-') || (token[index] == '*'))	\
		{							\
			/* Set data bit to  0. */			\
			*data &= ~(1 << buf_index);			\
			/* Set Mask bit to  0. */			\
			*mask &= ~(1 << buf_index);			\
			buf_index--;					\
		}							\
		else							\
		{							\
			return 0;					\
		};							\
	};								\
									\
	/* assert(buf_index == -1);	*/				\
	/* case we provided more bits then allowed */			\
	if (buf_index != -1)						\
		return 0;						\
									\
	return 1;							\
} while(0);  			
//TODO: FIXME:
#define HEX_TOKEN_TO_DATAMASK( token, bits, data,  mask ) 		\
 do {									\
	unsigned int count = 0;						\
	unsigned int index;						\
	int          buf_index;						\
	uint64_t nible_mask;						\
									\
	if ( token == NULL ) return 0; 					\
	/* protection to work only with 1 to 64 bits */ 		\
	if ( (bits > 64) || (bits < 1) ) return 0; 			\
									\
	count = strlen(token);						\
	/* protection from empty strings */				\
	if ( count < 1 ) return 0; 					\
									\
	/* we require this due we store everything in host order*/ 	\
	buf_index = 0;							\
	/* Create the entry in host byte order then  we invert the proccecing. */ \
	for (index = count-1; index >= 0; index--)				\
	{								\
	/* calculates the nible mask in case we have 11 bits 2 nibles(16) */	\
		nible_mask = 0xF; 					\
		if( (bits - buf_index) < 4 ) {				\
			nible_mask = 0xF >>  (bits - buf_index);	\
		}; 							\
									\
		if ((token[index] == '{') || (token[index] == '}'))	\
		{							\
			/* Do nothing. */				\
		} 							\
		else if( (token[index] == ' ') || (token[index] == '\t') || (token[index] == 0))	\
		{							\
			/* Do nothing. added for spaces*/		\
		}							\
		else if ((token[index] == 'x') || (token[index] == '\t'))\
		{							\
			/* we expect 0x000 procesing backwards 		\
			 and as soon as we get to x we are finished */	\
			return 1;					\
		}							\
		else if( (token[index] >= '0')  && (token[index] < '9') )	\
		{							\
			/* Set data bits. */				\
			*data |= ((token[index]-'0') << buf_index)&nible_mask;	\
			*data &= ~(((token[index]-'0') << buf_index)&nible_mask);	\
			/* Set mask 4 bits to 0xf. */			\
			*mask |= (0xF << buf_index);			\
			buf_index+=4;					\
		}							\
		else if( (token[index] >= 'a') || (token[index] <= 'f'))	\
		{							\
			/* Set data bits. */				\
			*data |= ((token[index]-'a') << buf_index)&nible_mask;	\
			*data &= ~(((token[index]-'a') << buf_index)&nible_mask);	\
			/* Set mask 4 bits to 0xf. */			\
			*mask |= (nible_mask << buf_index);		\
									\
			buf_index+=4;					\
		}							\
		else if (token[index] == '-'|| token[index] == '*')	\
		{							\
			/* Address bit is already 0. */			\
			/* Mask bit is already 0. */			\
			buf_index+=4;					\
		}							\
		else							\
		{							\
			return 0;					\
		};							\
	};								\
									\
	/* case we provided more or less nibles then allowed */		\
	if ((buf_index - bits) > 0 )					\
		return 0;						\
	if( (bits - buf_index) > 3 )					\
		return 0;						\
									\
	return 1;							\
} while(0);  								

		
/**
 * Internal Functions to parse a bit mask in format "-1-0-" and to return mask and data value works with up to 64 bit data and mask 
 * the matching will ignore the following symbols '{' , space , tab, '}'
 *     ------------------------------------------
 * Where '-' can be replaced with either '0' or '1'.
 *
 * @param[in]  token    Pointer to the string token being parsed.
 * @param[in]  bits     how many bits are expected and how many bits will be returned in the result 
 * @param[in]  data     pinter to 64 bit data which will be returned 
 * @param[in]  mask     pinter to 64 bit data which will be returned 
 *
 * @returns data         is the data in host order 
 * @returns mask         is the mask in host order 
 * @returns              Returns true if the token contained a valid value, otherwise
 *                       false is returned.
 *
 * @sa 
 */

static int bit_entry_8(const char * token, int bits, uint8_t * data, uint8_t * mask )
{

	BINARY_TOKEN_TO_DATAMASK( token, bits, data, mask );
};

static int bit_entry_16(const char * token, int bits, uint16_t * data, uint16_t * mask )
{

	BINARY_TOKEN_TO_DATAMASK( token, bits, data, mask );
};

static  int bit_entry_32(const char * token, int bits, uint32_t * data, uint32_t * mask )
{

	BINARY_TOKEN_TO_DATAMASK( token, bits, data, mask);
}
#if 0
static  int bit_entry_64(const char * token, int bits, uint64_t * data, uint64_t * mask )
{

	BINARY_TOKEN_TO_DATAMASK_64( token, bits, data, mask);
}
	 
static int hex_entry_16(const char * token, int bits, uint16_t * data, uint16_t * mask )
{

	HEX_TOKEN_TO_DATAMASK( token, bits, data, mask );
};
#endif

static  int bit_entry_128(const char * token, int bits, uint64_t * data, uint64_t * mask )
{

	BINARY_TOKEN_TO_DATAMASK_64( token, bits, data, mask);
}
static int parse_ip_addr(const char * token, uint64_t * data, uint64_t * mask)
{
    
    if ( (kTypeUnknown == yy_filter_rule.ip_address_type) || (k32Bit == yy_filter_rule.ip_address_type) )
    {
        if (bit_entry_32 ( token, 32, (uint32_t*) data, (uint32_t*) mask) )
        {
            yy_filter_rule.ip_address_type = k32Bit;
            return 1;
        }
    }
    memset( data, 0, sizeof(uint64_t) ) ;
    memset ( mask, 0, sizeof ( uint64_t) );
    if ( (kTypeUnknown == yy_filter_rule.ip_address_type) || (k128Bit == yy_filter_rule.ip_address_type) )
    {
        if ( bit_entry_128(token, 128, data, mask) )
        {
            /*its a 128 bit ip address */
            yy_filter_rule.ip_address_type = k128Bit;
            return 1;
        }
    }
    /* some error - its not 32 bit or 128 bit */
    return 0;
}
%}

%% 
	/* ignore spaces at any level */		
<*>[ \t]*	;

	
 /* Start of rule and a TAG recognition */
<INITIAL>^[ \t]*([0-9]{1,6}) {
			//clear the current rule to all zero
			memset(&yy_filter_rule,0,sizeof(yy_filter_rule));
	 		yy_filter_rule.user_tag = atoi(yytext);
			BEGIN(S_ipf_v2_rule);
	#if IPFV2LEX_DEBUG
	 		fprintf(out,"START RULE: user tag: %d line_no: %d",yy_filter_rule.user_tag,yylineno );
	 		fprintf(out, "\n");
	#endif 
		}
	/* Action for the filter */		
<S_ipf_v2_rule>accept|pass|reject|drop	{
			yy_filter_rule.action = ( (yytext[0] == 'a') || (yytext[0] == 'p') )?0:1;
			yy_filter_rule.user_class = 0;
			#if IPFV2_RULE_PRINT
			fprintf( out,"ACTION: %d line_no: %d\n",yy_filter_rule.action, yylineno );
			#endif 
		};
<S_ipf_v2_rule>all	{
            int tmp_action  = yy_filter_rule.action ;
            uint16_t tmp_user_tag = yy_filter_rule.user_tag;
            uint8_t tmp_user_class = yy_filter_rule.user_class;
			memset(&yy_filter_rule, 0 , sizeof(ipf_v2_filter_rule_t));
			#if IPFV2_RULE_PRINT
			//fprintf( out,"Steer: %d line_no: %d\n",yy_filter_rule.user_class , yylineno );
			#endif
            yy_filter_rule.action = tmp_action;
            yy_filter_rule.user_tag = tmp_user_tag;
            yy_filter_rule.user_class = tmp_user_class;
		};
	
<S_ipf_v2_rule>red	{
			yy_filter_rule.user_class |= 0x01;
			#if IPFV2_RULE_PRINT
			fprintf( out,"Steer: %d line_no: %d\n",yy_filter_rule.user_class , yylineno );
			#endif
		};
<S_ipf_v2_rule>blue	{
			yy_filter_rule.user_class |= 0x02;
			#if IPFV2_RULE_PRINT
			fprintf( out,"Steer: %d line_no: %d\n",yy_filter_rule.user_class , yylineno );
			#endif
		};
	
<S_ipf_v2_rule>(iface){SPACE} { 	
			BEGIN(S_iface); 
		}
<S_iface>\{{BINARY_PATTERN}\}	{ 
		
		if (! bit_entry_8 ( yytext, 2 , 
			&yy_filter_rule.iface.data, 
			&yy_filter_rule.iface.mask) )
		{		
			fprintf( out, "ERROR: Bad iface parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if IPFV2_RULE_PRINT
		fprintf( out, "iface 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.iface.data, yy_filter_rule.iface.mask, yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_ipf_v2_rule>iface{SPACE}[0-9]	{ 	
				yy_filter_rule.iface.data  = atoi(&yytext[5]);
                yy_filter_rule.iface.mask  = 0x3;
                if ( 144 == filter_rule_width)
                {
                    if ( yy_filter_rule.iface.data > 1 )
                    {
                        fprintf( out, "Error: Not allowed in 144 bit filter. Iface could be 0 or 1.  line_no: %d\n",yylineno );
			            return T_ERROR;
                    };
                };
            #if IPFV2_RULE_PRINT
			fprintf( out, "iface 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.iface.data, yy_filter_rule.iface.mask, yylineno );
			#endif
		}

<S_ipf_v2_rule>(iface){SPACE} { 	
			BEGIN(S_iface); 
		}
<S_iface>\{{BINARY_PATTERN}\}	{ 
		
		if (! bit_entry_8 ( yytext, 2 , 
			&yy_filter_rule.iface.data, 
			&yy_filter_rule.iface.mask) )
		{		
			fprintf( out, "ERROR: Bad iface parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if IPFV2_RULE_PRINT
		fprintf( out, "iface 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.iface.data, yy_filter_rule.iface.mask, yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_iface>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after iface  '{---}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_iface>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
		}
				
<S_ipf_v2_rule>l2-proto{SPACE}	{ 	
			BEGIN(S_l2_proto); 
		}
			
<S_l2_proto>\{{BINARY_PATTERN}\}	{ 
		if (! bit_entry_16 ( yytext, 16 , 
			&yy_filter_rule.l2_proto.data, 
			&yy_filter_rule.l2_proto.mask) )
		{		
			fprintf( out, "ERROR: Bad l2-proto parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if IPFV2_RULE_PRINT
		fprintf( out, "l2-proto 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.l2_proto.data, yy_filter_rule.l2_proto.mask, yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_l2_proto>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after l2-proto  '{---- ---- ---- ---- }' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_l2_proto>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	

				
<S_ipf_v2_rule>(mpls-top){SPACE}	{ 	
			BEGIN(S_mpls_top); 
		}
			
<S_mpls_top>\{{BINARY_PATTERN}\}	{ 
        if ( 144 == filter_rule_width)
        {
            fprintf( out, "Error: Not allowed in 144 bit filter.  line_no: %d\n",yylineno );
			return T_ERROR;
        };

        if ( (ipf_card_cap) && !ipf_card_cap->mpls_filtering_supported )
        {
            fprintf( out, "Error: mpls filtering is not supported by the F/W. line_no: %d\n",yylineno );
			return T_ERROR;
        };
        /* check for vlan filtering in the ruleset */
		if ( is_vlan_filtering )
        {
            fprintf( out, "ERROR: MPLS filtering not allowed in line  %d. Previous declaration of VLAN filtering in line %d\n",yylineno, last_line_num );
			return T_ERROR;
        };
    
        if (! bit_entry_32 ( yytext, 32 , 
			&yy_filter_rule.mpls_top.data, 
			&yy_filter_rule.mpls_top.mask) )
		{		
			fprintf( out, "ERROR: Bad mpls-top parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
        is_mpls_filtering = 1;
        last_line_num = yylineno;
		#if IPFV2_RULE_PRINT
		fprintf( out, "mpls-top 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.mpls_top.data, yy_filter_rule.mpls_top.mask, yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_mpls_top>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after mpls-top  '{---- ---- ---- ---- ---- ---- ---- ----}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_mpls_top>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	

<S_ipf_v2_rule>(mpls-btm){SPACE}	{ 	
			BEGIN(S_mpls_bottom); 
		}
<S_mpls_bottom>\{{BINARY_PATTERN}\}	{ 
        if ( 144 == filter_rule_width)
        {
            fprintf( out, "Error: Not allowed in 144 bit filter.  line_no: %d\n",yylineno );
			return T_ERROR;
        };
        
        if ((ipf_card_cap) &&  !ipf_card_cap->mpls_filtering_supported )
        {
            fprintf( out, "Error: mpls filtering is not supported by the F/W. line_no: %d\n",yylineno );
			return T_ERROR;
        };
		 /* check for vlan filtering in the ruleset */
		if ( is_vlan_filtering )
        {
            fprintf( out, "ERROR: MPLS filtering not allowed in line  %d. Previous declaration of VLAN filtering in line %d\n",yylineno, last_line_num );
			return T_ERROR;
        };
		if (! bit_entry_32 ( yytext, 32 , 
			&yy_filter_rule.mpls_bottom.data, 
			&yy_filter_rule.mpls_bottom.mask) )
		{		
			fprintf( out, "ERROR: Bad mpls-btm parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
         
        is_mpls_filtering = 1;
        last_line_num = yylineno;
		#if IPFV2_RULE_PRINT
		fprintf( out, "mpls-btm 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.mpls_bottom.data, yy_filter_rule.mpls_bottom.mask, yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_mpls_bottom>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after mpls-btm  '{---- ---- ---- ---- ---- ---- ---- ----}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_mpls_bottom>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	
    /* vlan-1 */
<S_ipf_v2_rule>(vlan-1){SPACE}	{ 	
			BEGIN(S_vlan_1); 
		}
<S_vlan_1>\{{BINARY_PATTERN}\}	{ 
        if ( 144 == filter_rule_width)
        {
            fprintf( out, "Error: Not allowed in 144 bit filter.  line_no: %d\n",yylineno );
			return T_ERROR;
        };
        if ( (ipf_card_cap) && !ipf_card_cap->vlan_filtering_supported )
        {
            fprintf( out, "Error: VLAN filtering is not supported by the F/W. line_no: %d\n",yylineno );
			return T_ERROR;
        };
		/* check for mpls filtering in the ruleset */
		if ( is_mpls_filtering )
        {
            fprintf( out, "ERROR: VLAN filtering not allowed in line  %d. Previous declaration of MPLS filtering in line %d\n",yylineno, last_line_num );
			return T_ERROR;
        };
		
		if (! bit_entry_16 ( yytext, 16 , 
			&yy_filter_rule.vlan_1.data, 
			&yy_filter_rule.vlan_1.mask) )
		{		
			fprintf( out, "ERROR: Bad vlan-1 parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
        
        is_vlan_filtering = 1;
        last_line_num = yylineno;
		#if IPFV2_RULE_PRINT
		fprintf( out, "vlan-1 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.vlan_1.data, yy_filter_rule.vlan_1.mask, yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_vlan_1>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after vlan-1  '{---- ---- ---- ---- }' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_vlan_1>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	
				
    /* vlan-2 */

<S_ipf_v2_rule>(vlan-2){SPACE}	{ 	
			BEGIN(S_vlan_2); 
		}
<S_vlan_2>\{{BINARY_PATTERN}\}	{ 
        if ( 144 == filter_rule_width)
        {
            fprintf( out, "Error: Not allowed in 144 bit filter.  line_no: %d\n",yylineno );
			return T_ERROR;
        };
       /* check for mpls filtering in the ruleset */
		if ( (ipf_card_cap) && !ipf_card_cap->vlan_filtering_supported )
        {
            fprintf( out, "Error: VLAN filtering is not supported by the F/W. line_no: %d\n",yylineno );
			return T_ERROR;
        };
        if ( is_mpls_filtering )
        {
            fprintf( out, "ERROR: VLAN filtering not allowed in line  %d. Previous declaration of MPLS filtering in line %d\n",yylineno, last_line_num );
			return T_ERROR;
        };
		if (! bit_entry_16 ( yytext, 16 , 
			&yy_filter_rule.vlan_2.data, 
			&yy_filter_rule.vlan_2.mask) )
		{		
			fprintf( out, "ERROR: Bad vlan-2 parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
        
        is_vlan_filtering = 1;
        last_line_num = yylineno;
		#if IPFV2_RULE_PRINT
		fprintf( out, "vlan-2 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.vlan_2.data, yy_filter_rule.vlan_2.mask, yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_vlan_2>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after vlan-2  '{---- ---- ---- ----}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_vlan_2>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	

    /* dst-ip */

<S_ipf_v2_rule>(dst-ip){SPACE}	{ 	
			BEGIN(S_dst_ip); 
		}
<S_dst_ip>\{{BINARY_PATTERN}\}	{ 

		if (! parse_ip_addr ( yytext, yy_filter_rule.dst_ip.data, 
			yy_filter_rule.dst_ip.mask)) 
		{		
			fprintf( out, "ERROR: Bad dst-ip parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
        if ( k128Bit == yy_filter_rule.ip_address_type)
        {
            if ( 144 == filter_rule_width)
            {
                fprintf( out, "Error: Not allowed in 144 bit filter.  line_no: %d\n",yylineno );
                return T_ERROR;
            };
            if ((ipf_card_cap) &&  ((!ipf_card_cap->ipv6_supported) || ( !ipf_card_cap->width576_supported)) )
            {
                fprintf( out, "ERROR: Firmware does not support 128 bit dst-ip parameter line_no: %d\n",yylineno );
                return T_ERROR;
            }
        };
		#if IPFV2_RULE_PRINT
        if( k128Bit != yy_filter_rule.ip_address_type )
    		  fprintf( out, "dst-ip 0x%04x,0x%04x line_no: %d\n",(uint32_t)yy_filter_rule.dst_ip.data[0], (uint32_t)yy_filter_rule.dst_ip.mask[0], yylineno );
        else
            fprintf( out, "dst-ip128 0x%016"PRIx64"%016"PRIx64",0x%016"PRIx64"%016"PRIx64" line_no: %d\n",yy_filter_rule.dst_ip.data[0],yy_filter_rule.dst_ip.data[1], yy_filter_rule.dst_ip.mask[0],yy_filter_rule.dst_ip.mask[1], yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_dst_ip>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after dst-ip  '{---- ---- ---- ---- ---- ---- ---- ----}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_dst_ip>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	

 /* src-ip */

<S_ipf_v2_rule>(src-ip){SPACE}	{ 	
			BEGIN(S_src_ip); 
		}
<S_src_ip>\{{BINARY_PATTERN}\}	{ 
		
		if (! parse_ip_addr ( yytext, yy_filter_rule.src_ip.data, 
			yy_filter_rule.src_ip.mask) )
		{		
			fprintf( out, "ERROR: Bad src-ip parameter line_no: %d\n",yylineno );
            fprintf( out, "Make sure that you have not mixed 32 bit and 128bit addresses\n");
			return T_ERROR;
		};
        if ( k128Bit == yy_filter_rule.ip_address_type)
        {
            if ( 144 == filter_rule_width)
            {
                fprintf( out, "Error: Not allowed in 144 bit filter.  line_no: %d\n",yylineno );
                return T_ERROR;
            };
            if ((ipf_card_cap) && ( (!ipf_card_cap->ipv6_supported) || ( !ipf_card_cap->width576_supported) ) )
            {
                fprintf( out, "ERROR: Firmware does not support 128 bit src-ip parameter line_no: %d\n",yylineno );
                return T_ERROR;
            }
        };
		#if IPFV2_RULE_PRINT
		if( k128Bit != yy_filter_rule.ip_address_type )
    		  fprintf( out, "src-ip 0x%04x,0x%04x line_no: %d\n",(uint32_t)yy_filter_rule.src_ip.data[0], (uint32_t)yy_filter_rule.src_ip.mask[0], yylineno );
        else
            fprintf( out, "src-ip 0x%016"PRIx64"%016"PRIx64",0x%016"PRIx64"%016"PRIx64" line_no: %d\n",yy_filter_rule.src_ip.data[0],yy_filter_rule.src_ip.data[1], yy_filter_rule.src_ip.mask[0],yy_filter_rule.src_ip.mask[1], yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_src_ip>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after src-ip  '{---- ---- ---- ---- ---- ---- ---- ----}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_src_ip>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	

    /* ip-prot */

<S_ipf_v2_rule>(ip-prot){SPACE}	{ 	
			BEGIN(S_ip_prot); 
		}
<S_ip_prot>\{{BINARY_PATTERN}\}	{ 
		
		if (! bit_entry_8 ( yytext, 8 , 
			&yy_filter_rule.ip_prot.data, 
			&yy_filter_rule.ip_prot.mask) )
		{		
			fprintf( out, "ERROR: Bad ip-prot parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if IPFV2_RULE_PRINT
		fprintf( out, "ip-prot 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.ip_prot.data, yy_filter_rule.ip_prot.mask, yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_ip_prot>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after ip-prot  '{---- ---- }' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_ip_prot>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	

    /* src-port */

<S_ipf_v2_rule>(src-port){SPACE}	{ 	
			BEGIN(S_src_port); 
		}
<S_src_port>\{{BINARY_PATTERN}\}	{ 
		
		if (! bit_entry_16 ( yytext, 16 , 
			&yy_filter_rule.src_port.data, 
			&yy_filter_rule.src_port.mask) )
		{		
			fprintf( out, "ERROR: Bad src-port parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if IPFV2_RULE_PRINT
		fprintf( out, "src-port 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.src_port.data, yy_filter_rule.src_port.mask, yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_src_port>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after src-port  '{---- ---- ---- ---- }' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_src_port>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	

    /* dst-port */

<S_ipf_v2_rule>(dst-port){SPACE}	{ 	
			BEGIN(S_dst_port); 
		}
<S_dst_port>\{{BINARY_PATTERN}\}	{ 
		
		if (! bit_entry_16 ( yytext, 16 , 
			&yy_filter_rule.dst_port.data, 
			&yy_filter_rule.dst_port.mask) )
		{		
			fprintf( out, "ERROR: Bad dst-port parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if IPFV2_RULE_PRINT
		fprintf( out, "dst-port 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.dst_port.data, yy_filter_rule.dst_port.mask, yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_dst_port>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after dst-port  '{---- ---- ---- ---- }' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_dst_port>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}

    /* tcp-flags */

<S_ipf_v2_rule>(tcp-flags){SPACE}	{ 	
			BEGIN(S_tcp_flags); 
		}
<S_tcp_flags>\{{BINARY_PATTERN}\}	{ 
		
		if (! bit_entry_8 ( yytext, 8 , 
			&yy_filter_rule.tcp_flags.data, 
			&yy_filter_rule.tcp_flags.mask) )
		{		
			fprintf( out, "ERROR: Bad tcp-flags parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if IPFV2_RULE_PRINT
		fprintf( out, "tcp-flags 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.tcp_flags.data, yy_filter_rule.tcp_flags.mask, yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_tcp_flags>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after tcp-flags  '{---- ---- }' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_tcp_flags>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}

<S_ipf_v2_rule>(label-cnt){SPACE}	{ 	
			BEGIN(S_label_cnt); 
		}
<S_label_cnt>\{{BINARY_PATTERN}\}	{ 
		if ( 144 == filter_rule_width)
        {
            fprintf( out, "Error: Not allowed in 144 bit filter.  line_no: %d\n",yylineno );
			return T_ERROR;
        };
		if (! bit_entry_8 ( yytext, 3 , 
			&yy_filter_rule.label_cnt.data, 
			&yy_filter_rule.label_cnt.mask) )
		{		
			fprintf( out, "ERROR: Bad label-cnt parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if IPFV2_RULE_PRINT
		fprintf( out, "label-cnt 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.label_cnt.data, yy_filter_rule.label_cnt.mask, yylineno );
		#endif 
		BEGIN(S_ipf_v2_rule);
	}
<S_label_cnt>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after label-cnt  '{---}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_label_cnt>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	

<S_ipf_v2_rule>ip	{ 	
				yy_filter_rule.ip_prot.data = 0x00;
                yy_filter_rule.ip_prot.mask = 0x00;
            #if IPFV2_RULE_PRINT
			fprintf( out,"ip ( ip_prot = 0x00) line_no: %d\n", yylineno );
			#endif
		}	

<S_ipf_v2_rule>tcp	{ 	
				yy_filter_rule.ip_prot.data = 0x06;
                yy_filter_rule.ip_prot.mask = 0xff;
            #if IPFV2_RULE_PRINT
			fprintf( out,"tcp ( ip_prot = 0x06) line_no: %d\n", yylineno );
			#endif
		}	

<S_ipf_v2_rule>udp	{ 	
				yy_filter_rule.ip_prot.data = 0x11;
                yy_filter_rule.ip_prot.mask = 0xff;
            #if IPFV2_RULE_PRINT
			fprintf( out,"udp ( ip_prot = 0x11) line_no: %d\n", yylineno );
			#endif
		}	
<S_ipf_v2_rule>icmp	{ 	
				yy_filter_rule.ip_prot.data = 0x01;
                yy_filter_rule.ip_prot.mask = 0xff;
            #if IPFV2_RULE_PRINT
			fprintf( out,"icmp ( ip_prot = 0x01) line_no: %d\n", yylineno );
			#endif
		}	
<S_ipf_v2_rule>igmp	{ 	
				yy_filter_rule.ip_prot.data = 0x02;
                yy_filter_rule.ip_prot.mask = 0xff;
            #if IPFV2_RULE_PRINT
			fprintf( out,"igmp ( ip_prot = 0x02) line_no: %d\n", yylineno );
			#endif
		}	
	 

	/* Implementation for adding comments in the rules 
	 *  single line commnet // to the end of the line 
	 * multiline comment from / * to the * / at any place multiple lines 
	 * and the status is at the point of begining of comment
	 */


<*>\/\/.*\n	{	
	#if IPFV2LEX_DEBUG_COMMENT
			fprintf( out, "Single line Comment lineno:%d ", yylineno-1); 
			ECHO;
			fprintf( out,"\n");
	#endif
		}	
<*>\#.*\n	{	
	#if IPFV2LEX_DEBUG_COMMENT
			fprintf( out, "Single line Comment lineno:%d ", yylineno-1); 
			ECHO;
			fprintf( out,"\n");
	#endif
		}	
<*>\/\*		{
			yy_push_state(S_comment);
	#if IPFV2LEX_DEBUG_COMMENT
			fprintf( out, "SATRT Multy line Commentlineno:%d\n", yylineno); 
	#endif 
		}
  /* warning there is a rule which may overight this one <*>.  */				
<S_comment>\*	 ; 
<S_comment>\*\/	{
			yy_pop_state();
	#if IPFV2LEX_DEBUG_COMMENT
			fprintf( out, "FINISH Multy line Commentlineno:%d\n", yylineno); 
	#endif
  
		}
<S_comment>\n	 ; 
<S_comment>[^\*]*  ; 

	/* Common things like:
	* A. concatenate multiple lines for one rule \ \n 
	* B. unrecognized charecters any 
	* C. new line and new rule \n,
	*/
  
<*>\\{SPACE}[\n]	{
			#if IPFV2LEX_DEBUG_COMMENT
			fprintf( out, "RULE continue to a new lineno:%d\n", yylineno ); 
			#endif 
			}
<*>.	{
			fprintf( out, "\nUnrecognized character: %s lineno:%d\n", yytext, yylineno ); 
			return T_ERROR;
	}

<INITIAL>\n	{
	#if IPFV2LEX_DEBUG_COMMENT
		fprintf( out,"Empty line just skip:%d\n",yylineno-1);
	#endif 
	}
	
<S_ipf_v2_rule>\n	{
	#if IPFV2LEX_DEBUG
		fprintf( out, "User tag: %d, action: %s, steering: %d \n", 
   	    		yy_filter_rule.user_tag, (yy_filter_rule.action == 0)?"through":"drop" , yy_filter_rule.user_class );
        fprintf( out, "iface 0x%04x,0x%04x \n",yy_filter_rule.iface.data, yy_filter_rule.iface.mask);
        fprintf( out, "l2-proto 0x%04x,0x%04x\n",yy_filter_rule.l2_proto.data, yy_filter_rule.l2_proto.mask);
        fprintf( out, "mpls-top 0x%04x,0x%04x\n",yy_filter_rule.mpls_top.data, yy_filter_rule.mpls_top.mask);
        fprintf( out, "mpls-btm 0x%04x,0x%04x\n",yy_filter_rule.mpls_bottom.data, yy_filter_rule.mpls_bottom.mask);
        fprintf( out, "vlan-1 0x%04x,0x%04x\n",yy_filter_rule.vlan_1.data, yy_filter_rule.vlan_1.mask);
        fprintf( out, "vlan-2 0x%04x,0x%04x\n",yy_filter_rule.vlan_2.data, yy_filter_rule.vlan_2.mask);
        if ( k128Bit != yy_filter_rule.ip_address_type )
        {
            fprintf( out, "dst-ip 0x%04x,0x%04x\n",(uint32_t)yy_filter_rule.dst_ip.data[0], (uint32_t)yy_filter_rule.dst_ip.mask[0]);
            fprintf( out, "src-ip 0x%04x,0x%04x\n",(uint32_t) yy_filter_rule.src_ip.data[0], (uint32_t) yy_filter_rule.src_ip.mask[0]);
        }
        else 
        {
            fprintf( out, "dst-ip128 0x%016"PRIx64"%016"PRIx64",0x%016"PRIx64"%016"PRIx64" line_no: %d\n",yy_filter_rule.dst_ip.data[0],yy_filter_rule.dst_ip.data[1], yy_filter_rule.dst_ip.mask[0],yy_filter_rule.dst_ip.mask[1], yylineno );
            fprintf( out, "src-ip 0x%016"PRIx64"%016"PRIx64",0x%016"PRIx64"%016"PRIx64" line_no: %d\n",yy_filter_rule.src_ip.data[0],yy_filter_rule.src_ip.data[1], yy_filter_rule.src_ip.mask[0],yy_filter_rule.src_ip.mask[1], yylineno );
        }
        fprintf( out, "ip-prot 0x%04x,0x%04x\n",yy_filter_rule.ip_prot.data, yy_filter_rule.ip_prot.mask);
        fprintf( out, "src-port 0x%04x,0x%04x\n",yy_filter_rule.src_port.data, yy_filter_rule.src_port.mask);
        fprintf( out, "dst-port 0x%04x,0x%04x\n",yy_filter_rule.dst_port.data, yy_filter_rule.dst_port.mask);
        fprintf( out, "tcp-flags 0x%04x,0x%04x\n",yy_filter_rule.tcp_flags.data, yy_filter_rule.tcp_flags.mask);
		fprintf( out,"END of rule: on lineno:%d\n",yylineno-1);
	#endif
		BEGIN(INITIAL);
		/* 
		 * This can be used to change the default behavior 
		 * if you disable the return will retutn at the end of the file or error 
		 * usfull for debugging to be set to 0 or in case we change the sacnner to return set instead of a single rule
		 */
		#if 1
		 return T_RULE_DONE;
		#endif 
	}
<S_ipf_v2_rule><<EOF>>	{
	#if IPFV2LEX_DEBUG
		fprintf( out, "User tag: %d, action: %s, steering: %d \n", 
   	    		yy_filter_rule.user_tag, (yy_filter_rule.action == 0)?"through":"drop" , yy_filter_rule.user_class );
        fprintf( out, "iface 0x%04x,0x%04x \n",yy_filter_rule.iface.data, yy_filter_rule.iface.mask);
        fprintf( out, "l2-proto 0x%04x,0x%04x\n",yy_filter_rule.l2_proto.data, yy_filter_rule.l2_proto.mask);
        fprintf( out, "mpls-top 0x%04x,0x%04x\n",yy_filter_rule.mpls_top.data, yy_filter_rule.mpls_top.mask);
        fprintf( out, "mpls-btm 0x%04x,0x%04x\n",yy_filter_rule.mpls_bottom.data, yy_filter_rule.mpls_bottom.mask);
        fprintf( out, "vlan-1 0x%04x,0x%04x\n",yy_filter_rule.vlan_1.data, yy_filter_rule.vlan_1.mask);
        fprintf( out, "vlan-2 0x%04x,0x%04x\n",yy_filter_rule.vlan_2.data, yy_filter_rule.vlan_2.mask);
        if ( k128Bit != yy_filter_rule.ip_address_type )
        {
            fprintf( out, "dst-ip 0x%04x,0x%04x\n",(uint32_t)yy_filter_rule.dst_ip.data[0], (uint32_t)yy_filter_rule.dst_ip.mask[0]);
            fprintf( out, "src-ip 0x%04x,0x%04x\n",(uint32_t) yy_filter_rule.src_ip.data[0], (uint32_t) yy_filter_rule.src_ip.mask[0]);
        }
        else 
        {
            fprintf( out, "dst-ip128 0x%016"PRIx64"%016"PRIx64",0x%016"PRIx64"%016"PRIx64" line_no: %d\n",yy_filter_rule.dst_ip.data[0],yy_filter_rule.dst_ip.data[1], yy_filter_rule.dst_ip.mask[0],yy_filter_rule.dst_ip.mask[1], yylineno );
            fprintf( out, "src-ip 0x%016"PRIx64"%016"PRIx64",0x%016"PRIx64"%016"PRIx64" line_no: %d\n",yy_filter_rule.src_ip.data[0],yy_filter_rule.src_ip.data[1], yy_filter_rule.src_ip.mask[0],yy_filter_rule.src_ip.mask[1], yylineno );
        }
        fprintf( out, "ip-prot 0x%04x,0x%04x\n",yy_filter_rule.ip_prot.data, yy_filter_rule.ip_prot.mask);
        fprintf( out, "src-port 0x%04x,0x%04x\n",yy_filter_rule.src_port.data, yy_filter_rule.src_port.mask);
        fprintf( out, "dst-port 0x%04x,0x%04x\n",yy_filter_rule.dst_port.data, yy_filter_rule.dst_port.mask);
        fprintf( out, "tcp-flags 0x%04x,0x%04x\n",yy_filter_rule.tcp_flags.data, yy_filter_rule.tcp_flags.mask);
		fprintf( out,"END of rule: on lineno:%d\n",yylineno-1);
	#endif
		BEGIN(INITIAL);

		/* 
         *   Adding the end of file rules so that it accepts the last rule in a file if there 
         *   is no  new line 
		 */
		return T_RULE_DONE;
	}
	
%%
/*			
int scanner_set_stdout(FILE *scanner_out) {
		out = scanner_out;
		return 0;
}
*/

int get_scanner_topstate_ipfv2(void) {
	return yy_top_state();
}

void set_ipf_capabiity(ipf_capabilities_t *cap )
{
    ipf_card_cap = cap;
}

void set_ipf_v2_rule_width(int width)
{
    filter_rule_width  = width;
}
