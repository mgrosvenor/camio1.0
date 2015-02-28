/*
 * Copyright (c) 2003 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Ltd and no part
 * of it may be redistributed, published or disclosed except as outlined
 * in the written contract supplied with this product.
 *
 * $Id: dagopts.l,v 1.91 2009/06/23 06:55:23 vladimir Exp $
 */
%option noinput

%{

#include "dagtoken.h"
#ifdef _WIN32
#include <stdlib.h>
#endif

#define yylval dagoptlval
#define yylvalfl dagoptlvalfl

int yylval;
double yylvalfl;

%}

%option nounput

%%
%{
	yylval = 0; /* do this for every token not using yylval, to be on the safe side */
%}
default				{ return T_DEFAULT; }
default_ds3_atm		{ return T_DEFAULT_DS3_ATM; }
default_ds3_hdlc	{ return T_DEFAULT_DS3_HDLC; }
default_e3_hdlc		{ return T_DEFAULT_E3_HDLC; }
default_e3_hdlc_fract { return T_DEFAULT_E3_HDLC_FRACT; }
default_e3_atm		{ return T_DEFAULT_E3_ATM; }
default_kentrox		{ return T_DEFAULT_KENTROX; }
sonet					{ return T_SONET; }
sdh					{ return T_SDH; }
atm					{ return T_ATM; }
hdlc				{ return T_HDLC; }
ppp	    			{ return T_PPP; }
pos					{ return T_POS; }
raw					{ return T_RAW; }
(no)?errorflag		{ yylval = (yytext[0]!='n') ; return T_SUPPRESS_ERROR; }
oc1c?				{ return T_LINK_OC1; }
oc3c?				{ return T_LINK_OC3; }
oc12c?				{ return T_LINK_OC12; }
oc48c?				{ return T_LINK_OC48; }
oc192c?				{ return T_LINK_OC192; }
crc16				{ return T_POS_CRC16; }
terf_strip16		{ return T_TERF_STRIP_16; }
crc32				{ return T_POS_CRC32; }
terf_strip32		{ return T_TERF_STRIP_32; }
noterf_strip		{ return T_TERF_NOSTRIP; }
nodelay				{ return T_TR_TERF_NO_DELAY;}
relative			{ return T_TR_TERF_RELATIVE;}
(no)?txrxerror		{ yylval = (yytext[0]!='n') ; return T_TR_TERF_RX_ERROR; }
eth					{ yylval = 0 ; return T_ETH; }
10					{ return T_ETH_10; }
100					{ return T_ETH_100; }
1000				{ return T_ETH_1000; }
ge_status			{ return T_ETH_1000_STATUS; }
auto				{ yylval = 1 ; return T_ETH_AUTO; }
nni 				{ return T_SAR_NNI; }
uni 				{ return T_SAR_UNI; }
e1					{ return T_LINK_E1; }
e1_crc				{ return T_LINK_E1_CRC; }
e1_unframed			{ return T_LINK_E1_UNFRAMED; }
e3					{ return T_LINK_E3; }
e3_cc				{ return T_LINK_E3_CC; }
ds3_m23				{ return T_LINK_DS3_M23; }
ds3_m23_plcp		{ return T_LINK_DS3_M23_PLCP; }
ds3_m23_ff          { return T_LINK_DS3_M23_FF; }
ds3_m23_ef          { return T_LINK_DS3_M23_EF; }
ds3_m23_if          { return T_LINK_DS3_M23_IF; }
ds3_cbit			{ return T_LINK_DS3_CBIT; }
ds3_cbit_plcp		{ return T_LINK_DS3_CBIT_PLCP; }
ds3_cbit_ff			{ return T_LINK_DS3_CBIT_FF; }
ds3_cbit_ef			{ return T_LINK_DS3_CBIT_EF; }
ds3_cbit_if			{ return T_LINK_DS3_CBIT_IF; }
e3_g751				{ return T_LINK_E3_G751; }
e3_g751_if			{ return T_LINK_E3_G751_IF; }
e3_g751_plcp		{ return T_LINK_E3_G751_PLCP; }
t1					{ return T_LINK_T1; }
t1_esf				{ return T_LINK_T1_ESF; }
t1_sf				{ return T_LINK_T1_SF; }
t1_unframed			{ return T_LINK_T1_UNFRAMED; }
tu11				{ return T_LINK_TU11; }
tu12				{ return T_LINK_TU12; }
tunomap				{ return T_LINK_TUNOMAP; }
async				{ return T_LINK_ASYNC1; }
bitsync				{ return T_LINK_BIT_SYNC; }
bytesync1			{ return T_LINK_BYTE_SYNC1; }
bytesync2			{ return T_LINK_BYTE_SYNC2; }
coreon				{ yylval = 1; return T_LINK_CORE_ON; }
coreoff				{ return T_LINK_CORE_ON; }
clock_set           { return T_DUCK_SET_TO_HOST; }
clock_clear         { return T_DUCK_SET_TO_HOST; }
clear				{ return T_LINK_CLEAR; }
termext				{ return T_LINK_TERMEXT; }
term75				{ return T_LINK_TERM75; }
term100				{ return T_LINK_TERM100; }
term120				{ return T_LINK_TERM120; }
rxtermext			{ return T_LINK_RX_TERMEXT; }
txtermext			{ return T_LINK_TX_TERMEXT; }
b8zs				{ return T_LINK_B8ZS; }
ami					{ return T_LINK_AMI; }
vc3					{ return T_LINK_VC3; }
vc4					{ return T_LINK_VC4; }
vc4c				{ return T_LINK_VC4C; }
head		        { return T_TIMESTAMP_HEAD; }
tail			    { return T_TIMESTAMP_TAIL; }
(no)?tx_crc		{ yylval = (yytext[0]!='n') ; return T_TX_CRC; }
(no)?rx_monitor		{ yylval = (yytext[0]!='n') ; return T_RX_MONITOR_MODE; }
(no)?rx_equalizer	{ yylval = (yytext[0]!='n') ; return T_RX_EQUALIZER; }
(no)?rx_redundancy	{ yylval = (yytext[0]!='n') ; return T_REDUNDANCY_CONTROL; }
(no)?sfm	        { yylval = (yytext[0]!='n') ; return T_SFM; }
(no)?sfm_clock	    { yylval = (yytext[0]!='n') ; return T_SFM_CLOCK; }
(no)?pdhglobal		{ yylval = (yytext[0]!='n') ; return T_LINK_PDH_GLOBAL; }
(no)?sfppwr			{ yylval = (yytext[0]!='n') ; return T_LINK_SFPPWR; }
(no)?coreon			{ yylval = (yytext[0]!='n') ; return T_LINK_CORE_ON; }
(no)?rktiopwr         { yylval = (yytext[0]!='n') ; return T_LINK_RKTIOPWR; }
(no)?master			{ return (yytext[0]!='n') ? T_MASTER : T_SLAVE; }
(no)?slave			{ return (yytext[0]!='n') ? T_SLAVE  : T_MASTER; }
(no)?scramble		{ yylval = (yytext[0]!='n') ; return T_SONET_SCRAMBLE; }
(no)?ahec			{ yylval = (yytext[0]!='n') ; return T_ATM_ADD_HEC; }
(no)?descramble		{ yylval = (yytext[0]!='n') ; return T_ATM_DESCRAMBLE; }
(no)?afix			{ yylval = (yytext[0]!='n') ; return T_ATM_FIX_HEC; }
(no)?apass			{ yylval = (yytext[0]!='n') ; return T_ATM_PASS_HEC; }
(no)?aidle			{ yylval = (yytext[0]!='n') ; return T_ATM_PASS_IDLE; }
(no)?lcell			{ yylval = (yytext[0]!='n') ; return T_ATM_LCELL; }
(no)?ascramble		{ yylval = (yytext[0]!='n') ; return T_ATM_PLOAD_SCRAMBLE; }
(no)?nic			{ yylval = (yytext[1]!='o') ; return T_ETH_AUTO; }
(no)?auto_neg		{ yylval = (yytext[1]!='o') ; return T_ETH_AUTO; }
(w|l)an				{ yylval = (yytext[0]!='l') ; return T_ETH_WAN; }
(no)?varlen			{ yylval = (yytext[0]!='n') ; return T_GPP_VARLEN; }
(no)?drop			{ yylval = (yytext[0]!='n') ; return T_PBM_DROP; }
fixed				{ yylval = 0 ; return T_GPP_VARLEN; }
(no)?align64		{ yylval = (yytext[0]!='n') ; return T_GPP_ALIGN64; }
(no)?adm			{ yylval = (yytext[0]!='n') ; return T_LINK_ADM; }
(no)?crc			{ yylval = (yytext[0]!='n') ; return T_LINK_CRC; }
(no)?dcr			{ yylval = (yytext[0]!='n') ; return T_LINK_DCR; }
(no)?discard		{ yylval = (yytext[0]!='n') ; return T_LINK_DISCARD; }
(no)?eql			{ yylval = (yytext[0]!='n') ; return T_LINK_EQL; }
(no)?eql_f			{ yylval = (yytext[0]!='n') ; return T_D37D_FRAMER_LOOPBACK; }
(no)?fcl			{ yylval = (yytext[0]!='n') ; return T_LINK_FCL; }
(no)?h(igh)?gain	{ yylval = (yytext[0]!='n') ; return T_LINK_HGAIN; }
(no)?laser			{ yylval = (yytext[0]!='n') ; return T_LINK_LASER; }
(no)?lt0			{ yylval = (yytext[0]!='n') ; return T_LINK_LT0; }
(no)?lt1			{ yylval = (yytext[0]!='n') ; return T_LINK_LT1; }
(no)?m23			{ yylval = (yytext[0]!='n') ; return T_LINK_M23; }
(no)?ff00del        { yylval = (yytext[0]!='n') ; return T_FF_00_DEL; }
(no)?reset			{ yylval = (yytext[0]!='n') ; return T_LINK_RESET; }
(no)?muxeql |
(no)?lseql			{ yylval = (yytext[0]!='n') ; return T_LINK_LSEQL; }
(no)?digital_los	{ yylval = (yytext[0]!='n') ; return T_LINK_DIGITAL_LOS; }
(no)?analog_los		{ yylval = (yytext[0]!='n') ; return T_LINK_ANALOG_LOS; }
(no)?muxfcl |
(no)?lsfcl			{ yylval = (yytext[0]!='n') ; return T_LINK_LSFCL; }
(no)?pmax			{ yylval = (yytext[0]!='n') ; return T_POS_MAXCHECK; }
(no)?pmin			{ yylval = (yytext[0]!='n') ; return T_POS_MINCHECK; }
(no)?rxpkts			{ yylval = (yytext[0]!='n') ; return T_POS_RXPKTS; }
(no)?pscramble		{ yylval = (yytext[0]!='n') ; return T_POS_SCRAMBLE; }
(no)?txpkts			{ yylval = (yytext[0]!='n') ; return T_POS_TXPKTS; }
(no)?hec_correction { yylval = (yytext[0]!='n') ; return T_HEC_CORRECTION; }
(no)?crcstrip		{ yylval = (yytext[0]!='n') ; return T_POS_CRCSTRIP; }
(no)?ifaceswap		{ yylval = (yytext[0]!='n') ; return T_INTERFACE_SWAP; }
(no)?overlap		{ yylval = (yytext[0]!='n') ; return T_PBM_OVERLAP; }
(no)?cells			{ yylval = (yytext[0]!='n') ; return T_SAR_CELLS; }
(no)?frames			{ yylval = (yytext[0]!='n') ; return T_SAR_FRAMES; }
short=[0-9]+		{ yylval = strtoul(&yytext[6], NULL, 0) ; return T_POS_MINLEN; }
long=[0-9]+			{ yylval = strtoul(&yytext[5], NULL, 0) ; return T_POS_MAXLEN; }
slen=[0-9]+			{ yylval = strtoul(&yytext[5], NULL, 0) ; return T_GPP_SLEN; }
rawslen=[0-9]+			{ yylval = strtoul(&yytext[8], NULL, 0) ; return T_RAW_SLEN; }
fract=[0-9]+			{ yylval = strtoul(&yytext[6], NULL, 0) ; return T_LINK_E3_HDLC_FRACT; }
syncthresh=[0-9]+	{ yylval = strtoul(&yytext[11], NULL, 0) ; return T_DUCK_SYNC_THRESH; }
ncells=[0-9]+		{ yylval = strtoul(&yytext[7], NULL, 0) ; return T_ATM_NCELLS; }
mode=[0-9]+			{ yylval = strtoul(&yytext[5], NULL, 0) ; return T_LINK_MODE; }
link=[0-9]+			{ yylval = strtoul(&yytext[5], NULL, 0) ; return T_LINK_WHICH; }
link=all			{ yylval = 0xffff ; return T_LINK_WHICH; }
steer=(stream0|parity|crc|iface|colour|dsm)	{ yylval = 0; return T_STEER; }
(en|dis)ablea		{ yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_A; }
(en|dis)ableb		{ yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_B; }
(en|dis)ablec		{ yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_C; }
(en|dis)abled		{ yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_D; }
(en|dis)ablee       { yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_E; }
(en|dis)ablef       { yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_F; }
(en|dis)ableg       { yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_G; }
(en|dis)ableh       { yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_H; }
(en|dis)ablei       { yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_I; }
(en|dis)ablej       { yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_J; }
(en|dis)ablek       { yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_K; }
(en|dis)ablel       { yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_L; }
(en|dis)ablem       { yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_M; }
(en|dis)ablen       { yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_N; }
(en|dis)ableo       { yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_O; }
(en|dis)ablep       { yylval = (yytext[0]!='d') ; return T_GPP_ENABLE_P; }
mem=[0-9:]+			{ yylval = 0; return T_PBM_MEMCONFIG; }
rxtx				{ return T_PBM_RXTX; }
rxonly				{ return T_PBM_RXONLY; }
txonly				{ return T_PBM_TXONLY; }
rxmerge				{ return T_MUX_RXMERGE; }
rxsplit				{ return T_MUX_RXSPLIT; }
[pP](ort)?[a-hA-H]=[sS](tream)?[0-9]	{ yylval = 0; return T_MUX_CONFIG; }
(en|dis)ablesar		{ yylval = (yytext[0]!='d') ; return T_SAR_ENABLE; }
(no)?promisc        { yylval = (yytext[0]!='n') ; return T_ETH_PROMISC;}
10gbase_sr          {return T_ETH_10GBASESR; }
10gbase_lr          {return T_ETH_10GBASELR; }
10gbase_er          {return T_ETH_10GBASEER; }
sgmii               {return T_ETH_SGMII; }
gbic                {return T_ETH_GBIC; }
(no)?pathoam        { yylval = (yytext[0]!='n'); return T_SAR_PATH_OAM; }         
hlb=[0-9:.-]+			{ yylval = 0; return T_HLB_RANGE; }
idelay=[0-9]+			{ yylval = strtoul(&yytext[7], NULL, 0) ; return T_IDELAY; }
status=[0-9]+
drop=[0-9]+
tapmode                         { return T_INFINIBAND_TAPMODE;}
activemode                      { return T_INFINIBAND_ACTIVEMODE;}
monitormode 			{ return T_INFINIBAND_MONITORMODE;}
stm0				{ return T_LINK_STM0; }
stm1				{ return T_LINK_STM1; }
stm4				{ return T_LINK_STM4; }
stm16				{ return T_LINK_STM16; }
stm64				{ return T_LINK_STM64; }
[^ \t\n]+			{ return T_ERROR; }
.
%%

/*
 * Also define a yywrap.  Note that if we're using flex, it will
 * define a macro to map this identifier to dagoptwrap.
 */
int
yywrap(void)
{
	return 1;
}
