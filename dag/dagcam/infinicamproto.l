/*
 * Copyright (c) 2003 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Ltd and no part
 * of it may be redistributed, published or disclosed except as outlined
 * in the written contract supplied with this product.
 *
 * $Id: infinicamproto.l,v 1.12 2009/06/10 02:45:56 karthik Exp $
 * This file defines the key words which will create the high level rule 
 */
%option prefix="infini"
%option yylineno
%option stack
%option nounput
%option noyywrap
%option noinput
%x S_infini_rule S_sl S_lnh S_slid S_dlid S_opcode S_dest_qp S_src_qp S_comment 
BINARY_BIT_MASK [0-1\-\*]
DECIMAL_8bits [0-9]{1,3}
SPACE [ \t]*

HEX_8bits 0\x[0-9a-hA-H]{1,2}
BINARY_PATTERN [ \t]*[ \t0-1\-\*]+[ \t]*
HEX_PATTERN_16bits [ \t]*0x[0-9a-fA-F\-\*]{4,4}[ \t]*

%{
#include <stdio.h>
#include <stdlib.h>
#include "infiniband_proto.h"

static FILE *out;

	/*
	 * Debugging the gramer defines and general output 
	 * INFINI_RULE_PRINT: will output the actual recognized rules with out extra lines and comments 
	 * INFINILEX_DEBUG: Used for intial of the rule and print out of the result 
	 * INFINILEX_DEBUG_R: used for the actual rules and individual states 
	 * INFINILEX_DEBUG_COMMENT : used for comments debbuging and empty lines 
	*/
#define INFINI_RULE_PRINT 0
#define INFINILEX_DEBUG 0
#define INFINILEX_DEBUG_R 0
#define INFINILEX_DEBUG_COMMENT 0

//this is done so multipl parsers to be able to be used in the same application 
//each pareser just have to change this define and the prefix in the option 

#define yy_filter_rule infini_filter_rule

infiniband_filter_rule_t yy_filter_rule;

uint32_t 	yy_infini_tag;
uint32_t 	yy_infini_action;
uint32_t 	yy_infini_steer;


%}

	/* supporting fuunction to be moved to some part of the library  */
%{
	
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

int bit_entry_8(const char * token, int bits, uint8_t * data, uint8_t * mask )
{

	BINARY_TOKEN_TO_DATAMASK( token, bits, data, mask );
};

int bit_entry_16(const char * token, int bits, uint16_t * data, uint16_t * mask )
{

	BINARY_TOKEN_TO_DATAMASK( token, bits, data, mask );
};

int bit_entry_32(const char * token, int bits, uint32_t * data, uint32_t * mask )
{

	BINARY_TOKEN_TO_DATAMASK( token, bits, data, mask);
}
int bit_entry_64(const char * token, int bits, uint64_t * data, uint64_t * mask )
{

	BINARY_TOKEN_TO_DATAMASK( token, bits, data, mask);
}
	 
int hex_entry_16(const char * token, int bits, uint16_t * data, uint16_t * mask )
{

	HEX_TOKEN_TO_DATAMASK( token, bits, data, mask );
};


%}

%% 
	/* ignore spaces at any level */		
<*>[ \t]*	;

	
 /* Start of rule and a TAG recognition */
<INITIAL>^[ \t]*([0-9]{1,6}) {
			//clear the current rule to all zero
			memset(&yy_filter_rule,0,sizeof(yy_filter_rule));
	 		yy_filter_rule.user_tag = atoi(yytext);
			BEGIN(S_infini_rule);
	#if INFINILEX_DEBUG
	 		fprintf(out,"START RULE: user tag: %d line_no: %d",yy_filter_rule.user_tag,yylineno );
	 		fprintf(out, "\n");
	#endif 
		}
	/* Action for the filter */		
<S_infini_rule>accept|pass|reject|drop	{
			yy_filter_rule.action = ( (yytext[0] == 'a') || (yytext[0] == 'p') )?0:1;
			#if INFINI_RULE_PRINT
			fprintf( out,"ACTION: %d line_no: %d\n",yy_filter_rule.action, yylineno );
			#endif 
		};
	/* stream for the filter the current infiniband supports only 0*/
<S_infini_rule>class{SPACE}[0-3]	{
			yy_filter_rule.user_class = atoi(&yytext[6]);
			#if INFINI_RULE_PRINT
			fprintf( out,"Steer: %d line_no: %d\n",yy_infini_steer, yylineno );
			#endif
		};
	
	/* SLID Support  
	 * 
	 * 
	 */
<S_infini_rule>(slid|SLID){SPACE}	{ 	
				//call back method for specific FW if it is supported  can be added here and initialized before the scanner is used 
				//if (!yyval.yysupported(T_INFINIBAND_SLID) ) 
				//{ return or error message 
				//}
			BEGIN(S_slid); 
			#if INFINILEX_DEBUG_R
			ECHO; //example for debugging 
			fprintf( out, "->RECOGNIZED line_no: %d\n",yylineno );
			#endif 	  
		}
			
<S_slid>\{{BINARY_PATTERN}\}	{ 
		#if INFINILEX_DEBUG_R
		//example for debugging the expressions after the main key word is recognised
		ECHO;
		fprintf( out, "->RECOGNIZED line_no: %d\n",yylineno ); 
		#endif 
		
		if (! bit_entry_16 ( yytext, 16 , 
			&yy_filter_rule.src_local_id.data, 
			&yy_filter_rule.src_local_id.mask) )
		{	
			fprintf(out, "ERROR: Bad SLID parameter line_no: %d\n",yylineno );
			return T_ERROR_CONVERT;
		};
		#if INFINI_RULE_PRINT
		fprintf( out, "SLID 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.src_local_id.data, yy_filter_rule.src_local_id.mask, yylineno );
		#endif 
		BEGIN(S_infini_rule);
	}
	
<S_slid>\{{HEX_PATTERN_16bits}\}	{
		#if INFINILEX_DEBUG_R
		//used for debugging the expressions 
		ECHO;
		fprintf( out, "->RECOGNIZED line_no: %d\n",yylineno ); 
		#endif 
		
		if (! hex_entry_16 ( yytext, 16 , 
			&yy_filter_rule.src_local_id.data, 
			&yy_filter_rule.src_local_id.mask) )
		{		
			fprintf(out, "ERROR: Bad Hex pattern for SLID line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if INFINI_RULE_PRINT
		fprintf( out, "SLID 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.src_local_id.data, yy_filter_rule.src_local_id.mask, yylineno ); 
		#endif 
		BEGIN(S_infini_rule);
}

<S_slid>.	{
			fprintf(out,"ERROR: unexpected symbol %s on line: %d after SLID pattern '{----------------}' expected! \n",yytext, yylineno);
			return T_ERROR_CONVERT;
		}	
<S_slid>\n	{
			fprintf(out,"ERROR: unexpected end of line on line: %d ! \n", yylineno-1);
			return T_ERROR_CONVERT;
		}	
	
	/* 
	 * dlid rules
	 *
	 */
				
<S_infini_rule>(dlid|DLID){SPACE}	{ 	
			BEGIN(S_dlid); 
		}
			
<S_dlid>\{{BINARY_PATTERN}\}	{ 
		#if INFINILEX_DEBUG_R
		//used for debugging the expressions 
		ECHO;
		fprintf( out, "->RECOGNIZED line_no: %d\n",yylineno ); 
		#endif 
		
		if (! bit_entry_16 ( yytext, 16 , 
			&yy_filter_rule.dest_local_id.data, 
			&yy_filter_rule.dest_local_id.mask) )
		{		
			fprintf( out, "ERROR: Bad DLID parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if INFINI_RULE_PRINT
		fprintf( out, "DLID 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.dest_local_id.data, yy_filter_rule.dest_local_id.mask, yylineno );
		#endif 
		BEGIN(S_infini_rule);
	}
<S_dlid>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after DLID  '{----------------}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_dlid>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	
	
	/* 
	 * sl rules
	 *
	 */
				
<S_infini_rule>(sl|SL){SPACE}	{ 	
			BEGIN(S_sl); 
		}
			
<S_sl>\{{BINARY_PATTERN}\}	{ 
		
		if (! bit_entry_8 ( yytext, 4 , 
			&yy_filter_rule.service_level.data, 
			&yy_filter_rule.service_level.mask) )
		{		
			fprintf( out, "ERROR: Bad SL parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if INFINI_RULE_PRINT
		fprintf( out, "SL 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.service_level.data, yy_filter_rule.service_level.mask, yylineno );
		#endif 
		BEGIN(S_infini_rule);
	}
<S_sl>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after SL  '{----}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_sl>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	

				
	/* 
	 * lnh rules
	 *
	 */
				
<S_infini_rule>IBA-global	{
		yy_filter_rule.lnh.data = 0x3;
		yy_filter_rule.lnh.mask = 0x3;
		}
<S_infini_rule>IBA-local	{
		yy_filter_rule.lnh.data = 0x2;
		yy_filter_rule.lnh.mask = 0x3;
		}
<S_infini_rule>IP-non-IBA	{
		yy_filter_rule.lnh.data = 0x1;
		yy_filter_rule.lnh.mask = 0x3;
		}
<S_infini_rule>RAW-non-IBA	{
		yy_filter_rule.lnh.data = 0x0;
		yy_filter_rule.lnh.mask = 0x3;
		}
		
<S_infini_rule>(lnh|LNH){SPACE}	{ 
			BEGIN(S_lnh); 
		}			
<S_lnh>\{{BINARY_PATTERN}\}	{ 
		
		if (! bit_entry_8 ( yytext, 2 , &yy_filter_rule.lnh.data, 
			&yy_filter_rule.lnh.mask ) )
		{		
			fprintf( out, "ERROR: Bad LNH parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if INFINI_RULE_PRINT
		fprintf( out, "LNH 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.lnh.data, yy_filter_rule.lnh.mask, yylineno );
		#endif 
		BEGIN(S_infini_rule);
	}
<S_lnh>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after LNH  '{----}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_lnh>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	
	
	/* BTH Header */
	/* opcode related keywords
	 * RC,UC,RD,UD will setup 
	 * only the top 3 bits, to values according the spec, out of the 8 and the rest will be 0 for the core data
	 * For the mask the top 3 bits will be set to 1 and the rest will be 0(dont care)
	 * This is just a simplified interface for the opcode 
	 */
<S_infini_rule>RC|rc	{
		yy_filter_rule.opcode.data = 0x00;
		yy_filter_rule.opcode.mask = 0xE0;
		}
<S_infini_rule>UC|uc	{
		yy_filter_rule.opcode.data = 0x20;
		yy_filter_rule.opcode.mask = 0xE0;
		}
<S_infini_rule>RD|rd	{
		yy_filter_rule.opcode.data = 0x40;
		yy_filter_rule.opcode.mask = 0xE0;
		}
<S_infini_rule>UD|ud	{
		yy_filter_rule.opcode.data = 0x60;
		yy_filter_rule.opcode.mask = 0xE0;
		}
	

<S_infini_rule>(opcode|OPCODE){SPACE}	{ 
			BEGIN(S_opcode); 
		}			
<S_opcode>\{{BINARY_PATTERN}\}	{ 
		
		if (! bit_entry_8 ( yytext, 8 , 
		      	&yy_filter_rule.opcode.data, 
			&yy_filter_rule.opcode.mask ) )
		{		
			fprintf( out, "ERROR: Bad OPCODE parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if INFINI_RULE_PRINT
		fprintf( out, "OPCODE 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.opcode.data, yy_filter_rule.opcode.mask, yylineno );
		#endif 
		BEGIN(S_infini_rule);
	}
<S_opcode>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after OPCODE  '{----}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_opcode>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno);
			return T_ERROR;
	}	
	
<S_infini_rule>(dest-qp|DEST-QP){SPACE}	{ 
			BEGIN(S_dest_qp); 
		}			
<S_dest_qp>\{{BINARY_PATTERN}\}	{ 
		
		if (! bit_entry_32 ( yytext, 24 , 
			&yy_filter_rule.dest_qp.data, 
			&yy_filter_rule.dest_qp.mask ) )
		{		
			fprintf( out, "ERROR: Bad DEST-QP parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if INFINI_RULE_PRINT
		fprintf( out, "DEST_QP 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.dest_qp.data, yy_filter_rule.dest_qp.mask, yylineno );
		#endif
		BEGIN(S_infini_rule);
	}
<S_dest_qp>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after DEST-QP  '{----}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_dest_qp>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	
	/*
	 * DETH Header
	 * 
	 */

<S_infini_rule>(src-qp|SRC-QP){SPACE}	{ 
			BEGIN(S_src_qp); 
		}			
<S_src_qp>\{{BINARY_PATTERN}\}	{ 	
		if (! bit_entry_32 ( yytext, 24 , 
			&yy_filter_rule.src_qp.data, 
			&yy_filter_rule.src_qp.mask ) )
		{		
			fprintf( out, "ERROR: Bad SRC-QP parameter line_no: %d\n",yylineno );
			return T_ERROR;
		};
		#if INFINI_RULE_PRINT
		fprintf( out, "SRC-QP 0x%04x,0x%04x line_no: %d\n",yy_filter_rule.src_qp.data, yy_filter_rule.src_qp.mask, yylineno );
		#endif
		BEGIN(S_infini_rule);
	}
<S_src_qp>.	{
			fprintf( out,"ERROR: wrong format starting with %s on line: %d after SRC-QP  '{----}' expected! \n",yytext, yylineno);
			return T_ERROR;
	}	
<S_src_qp>\n	{
			fprintf(out,"ERROR: unexpected enf of line on line: %d ! \n", yylineno-1);
			return T_ERROR;
	}	
	
	/* Implementation for adding comments in the rules 
	 *  single line commnet // to the end of the line 
	 * multiline comment from / * to the * / at any place multiple lines 
	 * and the status is at the point of begining of comment
	 */
	/** Action for the filter */		
<S_infini_rule>data {
			yy_filter_rule.link_id.data = 1;
			yy_filter_rule.link_id.mask = 0x1;
			#if INFINI_RULE_PRINT
			fprintf( out,"ACTION: %d line_no: %d\n",yy_filter_rule.action, yylineno );
			#endif 
		};	

<S_infini_rule>link {
			yy_filter_rule.link_id.data = 0;
			yy_filter_rule.link_id.mask = 0x1;
			#if INFINI_RULE_PRINT
			fprintf( out,"ACTION: %d line_no: %d\n",yy_filter_rule.action, yylineno );
			#endif 
		};
	 
<*>\/\/.*\n	{	
	#if INFINILEX_DEBUG_COMMENT
			fprintf( out, "Single line Comment lineno:%d ", yylineno-1); 
			ECHO;
			fprintf( out,"\n");
	#endif
		}	
<*>\/\*		{
			yy_push_state(S_comment);
	#if INFINILEX_DEBUG_COMMENT
			fprintf( out, "SATRT Multy line Commentlineno:%d\n", yylineno); 
	#endif 
		}
  /* warning there is a rule which may overight this one <*>.  */				
<S_comment>\*	 ; 
<S_comment>\*\/	{
			yy_pop_state();
	#if INFINILEX_DEBUG_COMMENT
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
			#if INFINILEX_DEBUG_COMMENT
			fprintf( out, "RULE continue to a new lineno:%d\n", yylineno ); 
			#endif 
			}
<*>.	{
			fprintf( out, "\nUnrecognized character: %s lineno:%d\n", yytext, yylineno ); 
			return T_ERROR;
	}

<INITIAL>\n	{
	#if INFINILEX_DEBUG_COMMENT
		fprintf( out,"Empty line just skip:%d\n",yylineno-1);
	#endif 
	}
	
<S_infini_rule>\n	{
	#if INFINILEX_DEBUG
		fprintf( out, "User tag: %d, action: %s, steering: %d \n", 
   	    		yy_filter_rule.user_tag, (yy_filter_rule.action == 0)?"through":"drop" , yy_filter_rule.user_class );
		fprintf( out, "service_level data: 0x%01x, service_level mask: 0x%01x \n", 
   	    		yy_filter_rule.service_level.data, yy_filter_rule.service_level.mask );
		fprintf( out, "lnh.data: 0x%01x, lnh.mask: 0x%01x\n", 
			yy_filter_rule.lnh.data, yy_filter_rule.lnh.mask);
		fprintf( out, "src_local_id.data: 0x%04x, src_local_id.mask: 0x%04x\n", 
			yy_filter_rule.src_local_id.data, yy_filter_rule.src_local_id.mask);
	    	fprintf( out, "dest_local_id.data: 0x%04x, dest_local_id.mask: 0x%04x\n", 
			yy_filter_rule.dest_local_id.data, yy_filter_rule.dest_local_id.mask);
	    	fprintf( out, "opcode.data: 0x%01x, opcode.mask: 0x%01x\n", 
			yy_filter_rule.opcode.data, yy_filter_rule.opcode.mask);
	    	fprintf( out, "dest_qp.data: 0x%06x, dest_qp.mask: 0x%06x \n", 
			yy_filter_rule.dest_qp.data, yy_filter_rule.dest_qp.mask);
	/* DETH Header classification fields */
	    	fprintf( out, "src_qp.data: 0x%06x, src_qp.mask: 0x%06x \n", 
			yy_filter_rule.src_qp.data, yy_filter_rule.src_qp.mask);
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
<S_infini_rule><<EOF>>	{
	#if INFINILEX_DEBUG
		fprintf( out, "User tag: %d, action: %s, steering: %d \n", 
   	    		yy_filter_rule.user_tag, (yy_filter_rule.action == 0)?"through":"drop" , yy_filter_rule.user_class );
		fprintf( out, "service_level data: 0x%01x, service_level mask: 0x%01x \n", 
   	    		yy_filter_rule.service_level.data, yy_filter_rule.service_level.mask );
		fprintf( out, "lnh.data: 0x%01x, lnh.mask: 0x%01x\n", 
			yy_filter_rule.lnh.data, yy_filter_rule.lnh.mask);
		fprintf( out, "src_local_id.data: 0x%04x, src_local_id.mask: 0x%04x\n", 
			yy_filter_rule.src_local_id.data, yy_filter_rule.src_local_id.mask);
	    	fprintf( out, "dest_local_id.data: 0x%04x, dest_local_id.mask: 0x%04x\n", 
			yy_filter_rule.dest_local_id.data, yy_filter_rule.dest_local_id.mask);
	    	fprintf( out, "opcode.data: 0x%01x, opcode.mask: 0x%01x\n", 
			yy_filter_rule.opcode.data, yy_filter_rule.opcode.mask);
	    	fprintf( out, "dest_qp.data: 0x%06x, dest_qp.mask: 0x%06x \n", 
			yy_filter_rule.dest_qp.data, yy_filter_rule.dest_qp.mask);
	/* DETH Header classification fields */
	    	fprintf( out, "src_qp.data: 0x%06x, src_qp.mask: 0x%06x \n", 
			yy_filter_rule.src_qp.data, yy_filter_rule.src_qp.mask);
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
			
int scanner_set_stdout(FILE *scanner_out) {
		out = scanner_out;
		return 0;
}


int scanner_set_topstate(void) {
	return yy_top_state();
}
		

