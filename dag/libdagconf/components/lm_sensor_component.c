/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: lm_sensor_component.c,v 1.35.2.2 2009/10/22 00:25:03 darren.lim Exp $
 */
 
/**
 * @file lm_sensor_component.c
 *
 * This componentis for communicating with LM93 & LM63 sensors on a card via a SMBus (System Management Bus).
 * See http://www.smbus.org/specs for protocol specification. This is exposed by a firmware interface (RAW_SMBUS)
 * however we don't make this a component because it is unlikely that a user of the Config API will need
 * direct access to the SMBus.
 *
 * This component is design to replace both the LM63 and LM93 existing components, they are currently rather
 * limited and don't have the ability to distingish which device (either and LM93 or LM63) they are talking
 * to. This is a problem becuase the firmware doesn't tell us which device we have, rather it just has an
 * enumeration entry for a "Hardware Monitor", at the time of writing this included either an LM93 or LM63 only.
 * Currently the only way we can tell which module we have is to send request on the different addresses used
 * by the LM93 (01011xx) and LM63 (1001100), and check the result.
 * 
 *
 */
#include <float.h>
#include <math.h>

#include "dagutil.h"
#include "dag_platform.h"
#include "../include/component_types.h"
#include "../include/attribute_types.h"
#include "../include/card_types.h"
#include "../include/component.h"
#include "../include/card.h"
#include "../include/attribute.h"
#include "../include/attribute_factory.h"
#include "../include/components/mini_mac_statistics_component.h"
#include "../include/modules/generic_read_write.h"








/** Constant used to indicate that the analog to digital sensor is not used on the device. */
#define AD_UNUSED   FLT_MAX


/**
 * Structure that describes the features supported by a particular LM sensor on the different cards,
 * this is needed because all the cards have different temperature sensor configurations and voltage
 * hook ups (on the LM93).
 *
 */
typedef struct lmsensor_feature_s
{
	dag_card_t	card_type;	/**< The PCI card type value. */

	uint32_t	board_rev;	/**< Revision of the board corrsponding to the lm sensor information, e.g. A=0, B=1, B1=1, C=2, etc */

	uint16_t	lm_index;	/**< The index of the LM sensor, for example the 5.2xsa has both an LM93 and LM63,
					*   this field is used to determine which device we are talking about.
					*/

	int32_t		tmp_sens_1;	/**< A non-zero value indicates the first temperature sensor is available */
	int32_t		tmp_sens_2;	/**< A non-zero value indicates the second temperature sensor is available,
					*   Note that if neither temperature sensors are present then the on-board
					*   sensor is used, this is for cards like the 8.2X which has 2 LM63s next
					*   the chips, with neither of the temperatre diodes attached.
	                                */
	
	float	ad_multipliers[16];	/**< The multipliers to use for the analog to digital conversion pins, basically the
					*   value read from the register is a percentage value of the voltage within a given
					*   range, these multipliers are used to get the actual value. The multipliers
					*   depend on the hardware configuration because sometimes a resistor divider setup
					*   is used, and that needs to be taken into account when calculating the voltage.
					*   This value can be set to the constant AD_UNUSED, if the A2D pin is not attached
					*   in hardware.
					*
					*   Note: LM63 sensors don't have any A2D pins, so this field is ignored.
					*
					*   The following are the tyical formulas for calculating the actual voltages:
					*     AD_IN1:   (1.236f / 255f) * ((R1 + R2) / R2)   = R1 & R2 are the resistors used in the divider network
					*     AD_IN2:   (1.236f / 255f) * ((R1 + R2) / R2)   = R1 & R2 are the resistors used in the divider network
					*     AD_IN3:   (1.236f / 255f) * ((R1 + R2) / R2)   = R1 & R2 are the resistors used in the divider network
					*     AD_IN4:   1.20f / 192.0f
					*     AD_IN5:   1.50f / 192.0f
					*     AD_IN6:   1.50f / 192.0f
					*     AD_IN7:   1.20f / 192.0f
					*     AD_IN8:   1.20f / 192.0f
					*     AD_IN9:   3.30f / 192.0f
					*     AD_IN10:  5.00f / 192.0f
					*     AD_IN11:  2.50f / 192.0f
					*     AD_IN12:  1.969f / 192.0f
					*     AD_IN13:  0.984f / 192.0f
					*     AD_IN14:  0.984f / 192.0f
					*     AD_IN15:  0.309f / 64.0f
					*     AD_IN16:  3.300f / 192.0f
					* 
					*/
	
	const char *ad_descriptions[16];/**< The descriptions for all the valid voltage inputs, if there is no description or
					*   the A2D line is not used then set the description to the NULL.
					*/

	float	ad_lowerr[16];		/**< the voltage below which (lt) we consider an undervolt Error. AD_UNUSED for not used */
	float	ad_lowwarn[16];		/**< the voltage below which (lt) we consider an undervolt Warning. AD_UNUSED for not used */
	float	ad_highwarn[16];	/**< the voltage above which (gt) we consider an overvolt Warning. AD_UNUSED for not used */
	float	ad_higherr[16];		/**< the voltage above which (gt) we consider an overvolt Error. AD_UNUSED for not used */

/*			note		   Additional information regarding which schematics were used as a reference */
} lmsensor_feature_t;






/**
 * Table that maps LM sensor features to given card types. Note that most of
 * of the voltage multipliers will be the same accross all the cards, new multipliers
 * are only needed if a resistor divider network is used on the inputs. You will
 * need to refer to the schematics for more information.
 *
 * Notes:
 *
 * 5.2SXA  - Has a LM93 with the temp diodes hooked up to each of the FPGA dies.
 *         - An LM63 is also present, but the co-pro image enumerates it and it is for the TCAM.
 *
 * 7.1s    - The second temperature input on the LM93 is for the IXP processor.
 *         - Both the fan control outputs are hooked up.
 *
 * 8.2X    - Three temperature sensors are used; LM93 for main FPGA, and two LM63 next the
 *           FPGA and bridge chip.
 *
 * CoPros  - 
 *
 */
static lmsensor_feature_t   g_lm_feature_map[] = {


	/* The following is a typical setup for the A2D multiplers, in almost all cases you will not need to modify
	 * the values, the exceptions are where a voltage dividor setup is used on the input pins (typically on the first
	 * three A2D pins). Refer to the schematic and the help text in the lmsensor_feature_t type declaration for more
	 * information.
	 */

//	{ kDagCard,	0,	1,	0,	{ 0.006350f,	0.009690f,	0.009690f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.006250f,	0.017180f,	0.026040f,	0.013028f,	0.010255f,	0.005125f,	0.005125f,	0.004828f,	0.017187f },
//						{ "12V",	"5V",		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		"3V3"	},
//						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
//						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
//						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
//						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },


/*card		rev	idx	sens1	sens2	  AD1		AD2		AD3		AD4		AD5		AD6		AD7		AD8		AD9		AD10		AD11		AD12		AD13		AD14		AD15		AD16		note			*/
/*========================================================================================================================================================================================================================================================================================================*/
{kDag43ge,	0,	0,	1,	0,	{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },	/*D1*/
						{ NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },

{kDag452e,	3,	0,	1,	0,	{ 0.063578f,	0.01516f,	0.01516f,	AD_UNUSED,	0.007813f,	0.007813f,	0.019548f,	AD_UNUSED,	0.017188f,	0.026042f,	0.013021f,	AD_UNUSED,	0.005125f,	0.005125f,	AD_UNUSED,	0.017188f },
		/*D*/				{ "12V",	"2V5_LVDS",	"2V5_AUX",	NULL,		"1V5",		"1V8",		"2V5_RIO_BUF",	NULL,		"3V3",		"5V",		"3V_PCI",	NULL,		"0V75_REF_BUF",	"0V75_VTERM",	NULL,		"3V3"	  },
						{ 6.0f,		2.375f,		2.375f,		AD_UNUSED,	1.425f,		1.70f,		2.375f,		AD_UNUSED,	3.135f,		2.60f,		2.85f,		AD_UNUSED,	0.7125f,	0.7125f,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	2.4f,		2.4f,		AD_UNUSED,	1.44f,		1.718f,		2.40f,		AD_UNUSED,	3.168f,		2.65f,		2.88f,		AD_UNUSED,	0.72f,		0.72f,		AD_UNUSED,	3.168f	  },
						{ 13.0f,	2.6f,		2.6f,		AD_UNUSED,	1.56f,		1.82f,		2.60f,		AD_UNUSED,	3.432f,		5.45f,		3.12f,		AD_UNUSED,	0.78f,		0.78f,		AD_UNUSED,	3.432f	  },
						{ 13.1f,	2.625f,		2.625f,		AD_UNUSED,	1.575f,		1.90f,		2.625f,		AD_UNUSED,	3.465f,		5.50f,		3.15f,		AD_UNUSED,	0.7875f,	0.7875f,	AD_UNUSED,	3.465f	  } },
						/*PCI		XC2VP30		XC2VP30				XC2VP30		CY7C1415BV18	XC2VP30				XC2VP30		MAX1945		XC2VP30				XC2VP30		CY7C1415BV18			XC2VP30	  */
						/*								CY7C1415BV18							XC2C64															  */

{kDag452e,	0,	0,	1,	0,	{ 0.063578f,	0.022537f,	AD_UNUSED,	0.006250f,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.004828f,	0.017188f },	/*C*/
						{ "12V",	"5V",		NULL,		"1V5",		NULL,		NULL,		NULL,		NULL,		"3V3",		"2V5_RIO_BUF",	"2V5",		"1V8",		"0V75_REF_BUF",	"0V75_TERM",	"3V_PCI",	"3V3"	  },
						{ 6.0f,		2.60f,		AD_UNUSED,	1.425f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.135f,		2.375f,		2.375f,		1.70f,		0.7125f,	0.7125f,	2.85f,		3.135f	  },
						{ 10.0f,	2.65f,		AD_UNUSED,	1.44f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.168f,		2.40f,		2.40f,		1.718f,		0.72f,		0.72f,		2.88f,		3.168f	  },
						{ 13.0f,	5.45f,		AD_UNUSED,	1.56f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.432f,		2.60f,		2.60f,		1.82f,		0.78f,		0.78f,		3.12f,		3.432f	  },
						{ 13.1f,	5.50f,		AD_UNUSED,	1.575f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.465f,		2.625f,		2.625f,		1.90f,		0.7875f,	0.7875f,	3.15f,		3.465f	  } },
						/*PCI		MAX1945R			XC2VP30										XC2VP30		XC2VP30		XC2VP30		CY7C1313BV18	XC2VP30		CY7C1313BV18	XC2VP30		XC2VP30	  */
						/*						CY7C1313BV18																								  */

{kDag452z,	3,	0,	1,	0,	{ 0.063578f,	0.01516f,	0.01516f,	AD_UNUSED,	0.007813f,	0.007813f,	0.019548f,	AD_UNUSED,	0.017188f,	0.026042f,	0.013021f,	AD_UNUSED,	0.005125f,	0.005125f,	AD_UNUSED,	0.017188f },
		/*D*/				{ "12V",	"2V5_LVDS",	"2V5_AUX",	NULL,		"1V5",		"1V8",		"2V5_RIO_BUF",	NULL,		"3V3",		"5V",		"3V_PCI",	NULL,		"0V75_REF_BUF",	"0V75_VTERM",	NULL,		"3V3"	  },
						{ 6.0f,		2.375f,		2.375f,		AD_UNUSED,	1.425f,		1.70f,		2.375f,		AD_UNUSED,	3.135f,		2.60f,		2.85f,		AD_UNUSED,	0.7125f,	0.7125f,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	2.4f,		2.4f,		AD_UNUSED,	1.44f,		1.718f,		2.40f,		AD_UNUSED,	3.168f,		2.65f,		2.88f,		AD_UNUSED,	0.72f,		0.72f,		AD_UNUSED,	3.168f	  },
						{ 13.0f,	2.6f,		2.6f,		AD_UNUSED,	1.56f,		1.82f,		2.60f,		AD_UNUSED,	3.432f,		5.45f,		3.12f,		AD_UNUSED,	0.78f,		0.78f,		AD_UNUSED,	3.432f	  },
						{ 13.1f,	2.625f,		2.625f,		AD_UNUSED,	1.575f,		1.90f,		2.625f,		AD_UNUSED,	3.465f,		5.50f,		3.15f,		AD_UNUSED,	0.7875f,	0.7875f,	AD_UNUSED,	3.465f	  } },
						/*PCI		XC2VP30		XC2VP30				XC2VP30		CY7C1415BV18	XC2VP30				XC2VP30		MAX1945		XC2VP30				XC2VP30		CY7C1415BV18			XC2VP30	  */
						/*								CY7C1415BV18							XC2C64															  */

{kDag452z,	0,	0,	1,	0,	{ 0.063578f,	0.022537f,	AD_UNUSED,	0.006250f,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.004828f,	0.017188f },	/* refer to kDag452e schematic, C */
						{ "12V",	"5V",		NULL,		"1V5",		NULL,		NULL,		NULL,		NULL,		"3V3",		"2V5_RIO_BUF",	"2V5",		"1V8",		"0V75_REF_BUF",	"0V75_TERM",	"3V_PCI",	"3V3"	  },
						{ 6.0f,		2.60f,		AD_UNUSED,	1.425f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.135f,		2.375f,		2.375f,		1.70f,		0.7125f,	0.7125f,	2.85f,		3.135f	  },
						{ 10.0f,	2.65f,		AD_UNUSED,	1.44f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.168f,		2.40f,		2.40f,		1.718f,		0.72f,		0.72f,		2.88f,		3.168f	  },
						{ 13.0f,	5.45f,		AD_UNUSED,	1.56f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.432f,		2.60f,		2.60f,		1.882f,		0.78f,		0.78f,		3.12f,		3.432f	  },
						{ 13.1f,	5.50f,		AD_UNUSED,	1.575f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.465f,		2.625f,		2.625f,		1.90f,		0.7875f,	0.7875f,	3.15f,		3.465f	  } },
						/*PCI		MAX1945R			XC2VP30										XC2VP30		XC2VP30		XC2VP30		CY7C1313BV18	XC2VP30		CY7C1313BV18	XC2VP30		XC2VP30	  */
						/*						CY7C1313BV18																								  */

{kDag454e,	3,	0,	1,	0,	{ 0.063578f,	0.01516f,	0.01516f,	AD_UNUSED,	0.007813f,	0.007813f,	0.019548f,	AD_UNUSED,	0.017188f,	0.026042f,	0.013021f,	AD_UNUSED,	0.005125f,	0.005125f,	AD_UNUSED,	0.017188f },
		/*D*/				{ "12V",	"2V5_LVDS",	"2V5_AUX",	NULL,		"1V5",		"1V8",		"2V5_RIO_BUF",	NULL,		"3V3",		"5V",		"3V_PCI",	NULL,		"0V75_REF_BUF",	"0V75_VTERM",	NULL,		"3V3"	  },
						{ 6.0f,		2.375f,		2.375f,		AD_UNUSED,	1.425f,		1.70f,		2.375f,		AD_UNUSED,	3.135f,		2.60f,		2.85f,		AD_UNUSED,	0.7125f,	0.7125f,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	2.4f,		2.4f,		AD_UNUSED,	1.44f,		1.718f,		2.40f,		AD_UNUSED,	3.168f,		2.65f,		2.88f,		AD_UNUSED,	0.72f,		0.72f,		AD_UNUSED,	3.168f	  },
						{ 13.0f,	2.6f,		2.6f,		AD_UNUSED,	1.56f,		1.82f,		2.60f,		AD_UNUSED,	3.432f,		5.45f,		3.12f,		AD_UNUSED,	0.78f,		0.78f,		AD_UNUSED,	3.432f	  },
						{ 13.1f,	2.625f,		2.625f,		AD_UNUSED,	1.575f,		1.90f,		2.625f,		AD_UNUSED,	3.465f,		5.50f,		3.15f,		AD_UNUSED,	0.7875f,	0.7875f,	AD_UNUSED,	3.465f	  } },
						/*PCI		XC2VP30		XC2VP30				XC2VP30		CY7C1415BV18	XC2VP30				XC2VP30		MAX1945		XC2VP30				XC2VP30		CY7C1415BV18			XC2VP30	  */
						/*								CY7C1415BV18							XC2C64															  */

{kDag454e,	0,	0,	1,	0,	{ 0.063578f,	0.022537f,	AD_UNUSED,	0.006250f,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.004828f,	0.017188f },	/*C1*/
						{ "12V",	"5V",		NULL,		"1V5",		NULL,		NULL,		NULL,		NULL,		"3V3",		"2V5_RIO_BUF",	"2V5_AUX",	"1V8",		"0V75_REF_BUF",	"0V75_TERM",	"3V_PCI",	"3V3"	  },
						{ 6.0f,		2.60f,		AD_UNUSED,	1.425f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.135f,		2.375f,		2.375f,		1.70f,		0.7125f,	0.7125f,	2.85f,		3.135f	  },
						{ 10.0f,	2.65f,		AD_UNUSED,	1.44f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.168f,		2.40f,		2.40f,		1.718f,		0.72f,		0.72f,		2.88f,		3.168f	  },
						{ 13.0f,	5.45f,		AD_UNUSED,	1.56f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.432f,		2.60f,		2.60f,		1.882f,		0.78f,		0.78f,		3.12f,		3.432f	  },
						{ 13.1f,	5.50f,		AD_UNUSED,	1.575f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.465f,		2.625f,		2.625f,		1.90f,		0.7875f,	0.7875f,	3.15f,		3.465f	  } },
						/*PCI		MAX1945R			XC2VP30										XC2VP30		XC2VP30		XC2VP30		CY7C1313BV18	XC2VP30		CY7C1313BV18	XC2VP30		XC2VP30	  */
						/*						CY7C1313BV18																								  */

{kDag452z8,	3,	0,	1,	0,	{ 0.063578f,	0.01516f,	0.01516f,	AD_UNUSED,	0.007813f,	0.007813f,	0.019548f,	AD_UNUSED,	0.017188f,	0.026042f,	0.013021f,	AD_UNUSED,	0.005125f,	0.005125f,	AD_UNUSED,	0.017188f },
		/*D*/				{ "12V",	"2V5_LVDS",	"2V5_AUX",	NULL,		"1V5",		"1V8",		"2V5_RIO_BUF",	NULL,		"3V3",		"5V",		"3V_PCI",	NULL,		"0V75_REF_BUF",	"0V75_VTERM",	NULL,		"3V3"	  },
						{ 6.0f,		2.375f,		2.375f,		AD_UNUSED,	1.425f,		1.70f,		2.375f,		AD_UNUSED,	3.135f,		2.60f,		2.85f,		AD_UNUSED,	0.7125f,	0.7125f,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	2.4f,		2.4f,		AD_UNUSED,	1.44f,		1.718f,		2.40f,		AD_UNUSED,	3.168f,		2.65f,		2.88f,		AD_UNUSED,	0.72f,		0.72f,		AD_UNUSED,	3.168f	  },
						{ 13.0f,	2.6f,		2.6f,		AD_UNUSED,	1.56f,		1.82f,		2.60f,		AD_UNUSED,	3.432f,		5.45f,		3.12f,		AD_UNUSED,	0.78f,		0.78f,		AD_UNUSED,	3.432f	  },
						{ 13.1f,	2.625f,		2.625f,		AD_UNUSED,	1.575f,		1.90f,		2.625f,		AD_UNUSED,	3.465f,		5.50f,		3.15f,		AD_UNUSED,	0.7875f,	0.7875f,	AD_UNUSED,	3.465f	  } },
						/*PCI		XC2VP30		XC2VP30				XC2VP30		CY7C1415BV18	XC2VP30				XC2VP30		MAX1945		XC2VP30				XC2VP30		CY7C1415BV18			XC2VP30	  */
						/*								CY7C1415BV18							XC2C64															  */

{kDag452z8,	0,	0,	1,	0,	{ 0.063578f,	0.022537f,	AD_UNUSED,	0.006250f,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.004828f,	0.017188f },	/* refer to kDag454e schematic, C1 */
						{ "12V",	"5V",		NULL,		"1V5",		NULL,		NULL,		NULL,		NULL,		"3V3",		"2V5_RIO_BUF",	"2V5_AUX",	"1V8",		"0V75_REF_BUF",	"0V75_TERM",	"3V_PCI",	"3V3"	  },
						{ 6.0f,		2.60f,		AD_UNUSED,	1.425f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.135f,		2.375f,		2.375f,		1.70f,		0.7125f,	0.7125f,	2.85f,		3.135f	  },
						{ 10.0f,	2.65f,		AD_UNUSED,	1.44f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.168f,		2.40f,		2.40f,		1.718f,		0.72f,		0.72f,		2.88f,		3.168f	  },
						{ 13.0f,	5.45f,		AD_UNUSED,	1.56f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.432f,		2.60f,		2.60f,		1.882f,		0.78f,		0.78f,		3.12f,		3.432f	  },
						{ 13.1f,	5.50f,		AD_UNUSED,	1.575f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.465f,		2.625f,		2.625f,		1.90f,		0.7875f,	0.7875f,	3.15f,		3.465f	  } },
						/*PCI		MAX1945R			XC2VP30										XC2VP30		XC2VP30		XC2VP30		CY7C1313BV18	XC2VP30		CY7C1313BV18	XC2VP30		XC2VP30	  */
						/*						CY7C1313BV18																								  */

{kDag452cf,	0,	0,	1,	0,	{ 0.063578f,	0.027628f,	0.013161f,	0.006250f,	AD_UNUSED,	AD_UNUSED,	0.006250f,	0.006250f,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.004828f,	0.017188f },
		/*A*/				{ "12V",	"5V_FAN",	"3V PCI",	"1V5",		NULL,		NULL,		"1V2_DVDD_PHYA","1V2_DVDD_PHYB","3V3",		"2V5_RIO_BUF",	"2V5_AUX",	"1V8",		"0V75_REF_BUF",	"0V75_TERM",	"2V5",		"3V3"	  },
						{ 6.0f,		2.60f,		2.85f,		1.425f,		AD_UNUSED,	AD_UNUSED,	1.14f,		1.14f,		3.135f,		2.375f,		2.375f,		1.70f,		0.7125f,	0.7125f,	2.375f,		3.135f	  },
						{ 10.0f,	2.65f,		2.88f,		1.44f,		AD_UNUSED,	AD_UNUSED,	1.152f,		1.152f,		3.168f,		2.40f,		2.40f,		1.718f,		0.72f,		0.72f,		2.40f,		3.168f	  },
						{ 13.0f,	5.45f,		3.12f,		1.56f,		AD_UNUSED,	AD_UNUSED,	1.248f,		1.248f,		3.432f,		2.60f,		2.60f,		1.882f,		0.78f,		0.78f,		2.60f,		3.432f	  },
						{ 13.1f,	5.50f,		3.15f,		1.575f,		AD_UNUSED,	AD_UNUSED,	1.26f,		1.26f,		3.465f,		2.625f,		2.625f,		1.90f,		0.7875f,	0.7875f,	2.625f,		3.465f	  } },
						/*PCI		FAN		XC2VP30		XC2VP30						88E1112		88E1112		XC2VP30		XC2VP30		XC2VP30		CY7C1313BV18	XC2VP30		CY7C1313BV18	EG-2121CA	XC2VP30	  */
						/*						CY7C1313BV18																								  */

{kDag452gf,	0,	0,	1,	0,	{ 0.063578f,	0.022537f,	AD_UNUSED,	0.006250f,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.004828f,	0.017188f },	/*B_SX*/
						{ "12V",	"5V",		NULL,		"1V5",		NULL,		NULL,		NULL,		NULL,		"3V3",		"2V5_RIO_BUF",	"2V5_AUX",	"1V8",		"0V75_REF_BUF",	"0V75_TERM",	"3V_PCI",	"3V3"	  },
						{ 6.0f,		2.60f,		AD_UNUSED,	1.425f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.135f,		2.375f,		2.375f,		1.70f,		0.7125f,	0.7125f,	2.85f,		3.135f	  },
						{ 10.0f,	2.65f,		AD_UNUSED,	1.44f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.168f,		2.40f,		2.40f,		1.718f,		0.72f,		0.72f,		2.88f,		3.168f	  },
						{ 13.0f,	5.45f,		AD_UNUSED,	1.56f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.432f,		2.60f,		2.60f,		1.882f,		0.78f,		0.78f,		3.12f,		3.432f	  },
						{ 13.1f,	5.50f,		AD_UNUSED,	1.575f,		AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	3.465f,		2.625f,		2.625f,		1.90f,		0.7875f,	0.7875f,	3.15f,		3.465f	  } },
						/*PCI		MAX1945R			XC2VP30										XC2VP30		XC2VP30		XC2VP30		CY7C1313BV18	XC2VP30		CY7C1313BV18	XC2VP30		XC2VP30	  */
						/*						CY7C1313BV18																								  */

/*card		rev	idx	sens1	sens2	  AD1		AD2		AD3		AD4		AD5		AD6		AD7		AD8		AD9		AD10		AD11		AD12		AD13		AD14		AD15		AD16	*/
/*========================================================================================================================================================================================================================================================================================================*/
{kDag52x,	0,	0,	1,	0,	{ 0.063578f,	AD_UNUSED,	AD_UNUSED,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.006250f,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.004828f,	0.017188f },	/*D*/
						{ "12V",	NULL,		NULL,		"1V2",		"1V8_CPLDCORE",	"1V5_FIFO",	"VDD12A",	"1V5_XGMII_HSTL","3V3",		"3V_PCI",	"2V5_AUX",	"1V8_FIFO",	"0V75_QDR_TERM","1V2_PHYBUF",	"2V5_PREPHY",	"3V3"	  },
						{ 6.0f,		AD_UNUSED,	AD_UNUSED,	1.14f,		1.70f,		1.40f,		1.14f,		1.40f,		3.135f,		2.85f,		2.375f,		1.70f,		0.7125f,	1.14f,		2.30f,		3.135f	  },
						{ 10.0f,	AD_UNUSED,	AD_UNUSED,	1.152f,		1.718f,		1.415f,		1.152f,		1.44f,		3.168f,		2.88f,		2.38f,		1.718f,		0.72f,		1.152f,		2.325f,		3.168f	  },
						{ 13.0f,	AD_UNUSED,	AD_UNUSED,	1.248f,		1.882f,		1.785f,		1.248f,		1.56f,		3.432f,		3.12f,		2.61f,		1.882f,		0.78f,		1.248f,		2.675f,		3.432f	  },
						{ 13.1f,	AD_UNUSED,	AD_UNUSED,	1.26f,		1.90f,		1.80f,		1.26f,		1.60f,		3.465f,		3.15f,		2.625f,		1.90f,		0.7875f,	1.26f,		2.70f,		3.465f	  } },
						/*PCI						XC4VLX40	XC2C64A		XC4VLX40	VSC8476		XC4VLX40	XC4VLX40	XC4VLX40	XC4VLX40	uPD44325364	uPD44325364	VSC8476		LT1764		XC4VLX40  */
						/*										uPD44325364			VSC8476		uPD44325364													uPD44325364 */
						/*																VSC8476														VSC8476	  */

{kDag52sxa,	0,	0,	1,	1,	{ 0.063578f,	0.009694f,	0.009694f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	AD_UNUSED,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	AD_UNUSED,	AD_UNUSED,	0.009656f,	0.017188f },	/*C*/
						{ "12V",	"1V8_TCAM",	"1V8_DDR",	"1V2",		"1V8_CPLDCORE",	"0V9_DDR_VTERM","0V9_QDR36_TERM",NULL,		"3V3",		"3V_PCI",	"2V5_AUX",	"2V5_LVDS",	NULL,		NULL,		"1V8_PREPHY",	"3V3"	  },
						{ 6.0f,		1.71f,		1.70f,		1.14f,		1.70f,		0.85f,		0.85f,		AD_UNUSED,	3.135f,		2.85f,		2.375f,		2.375f,		AD_UNUSED,	AD_UNUSED,	1.71f,		3.135f	  },
						{ 10.0f,	1.728f,		1.718f,		1.152f,		1.718f,		0.86f,		0.86f,		AD_UNUSED,	3.168f,		2.88f,		2.40f,		2.40f,		AD_UNUSED,	AD_UNUSED,	1.728f,		3.168f	  },
						{ 13.0f,	1.872f,		1.882f,		1.248f,		1.882f,		0.94f,		0.94f,		AD_UNUSED,	3.432f,		3.12f,		2.62f,		2.62f,		AD_UNUSED,	AD_UNUSED,	1.872f,		3.432f	  },
						{ 13.1f,	1.89f,		1.90f,		1.26f,		1.90f,		0.95f,		0.95f,		AD_UNUSED,	3.465f,		3.15f,		2.625f,		2.625f,		AD_UNUSED,	AD_UNUSED,	1.89f,		3.465f	  } },
						/*PCI/PTH08T220	IDT75P52100	uPD44325364	XC4VSX55	XC2C128		XC4VSX55	uPD44325364			XC4VSX55	XC4VSX55	XC4VSX55	XC4VSX55					VSC8479		XC4VSX55  */
						/*																XC2C128														XC2C128	  */
						/*																GS8161Z36													GS8161Z36 */

{kDag52sxa,	0,	1,	0,	0,	{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },	/*C*/
						{ NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },

{kDag50s,	0,	0,	1,	0,	{ 0.063578f,	AD_UNUSED,	AD_UNUSED,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.006250f,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	AD_UNUSED,	0.005125f,	AD_UNUSED,	0.017188f },	/* refer to kDag50sg2 schematic, C */
						{ "12V",	NULL,		NULL,		"1V2",		"1V8_CPLDCORE",	"1V8_FRAMER",	"1V2_FRAMER",	"1V5_FIFO",	"3V3",		"3V_PCI",	"2V5_AUX",	"1V8_PREPHY",	NULL,		"0V75_QDR_TERM",NULL,		"3V3"	  },
						{ 6.0f,		AD_UNUSED,	AD_UNUSED,	1.14f,		1.70f,		1.71f,		1.14f,		1.40f,		3.135f,		2.85f,		2.375f,		1.71f,		AD_UNUSED,	0.7125f,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	AD_UNUSED,	AD_UNUSED,	1.152f,		1.718f,		1.728f,		1.152f,		1.415f,		3.168f,		2.88f,		2.38f,		1.728f,		AD_UNUSED,	0.72f	,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	AD_UNUSED,	AD_UNUSED,	1.248f,		1.882f,		1.872f,		1.248f,		1.785f,		3.432f,		3.12f,		2.61f,		1.872f,		AD_UNUSED,	0.78f,		AD_UNUSED,	3.432f	  },
						{ 13.1f,	AD_UNUSED,	AD_UNUSED,	1.26f,		1.90f,		1.89f,		1.26f,		1.80f,		3.465f,		3.15f,		2.625f,		1.89f,		AD_UNUSED,	0.7875f,	AD_UNUSED,	3.465f	  } },
						/*PCI		Spare		Spare		XC4VLX40	XC2C64A		XC4VLX40	AMCC3485	XC4VLX40	XC4VLX40	XC4VLX40	XC4VLX40	LT1764				uPD44325184	`		XC4VLX40  */
						/*										AMCC3485			uPD44325364	XC2C64A														XC2C64A	  */

{kDag50z,	0,	0,	1,	0,	{ 0.063578f,	AD_UNUSED,	AD_UNUSED,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.006250f,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	AD_UNUSED,	0.005125f,	AD_UNUSED,	0.017188f },	/* refer to kDag50sg2 schematic, C */
						{ "12V",	NULL,		NULL,		"1V2",		"1V8_CPLDCORE",	"1V8_FRAMER",	"1V2_FRAMER",	"1V5_FIFO",	"3V3",		"3V_PCI",	"2V5_AUX",	"1V8_PREPHY",	NULL,		"0V75_QDR_TERM",NULL,		"3V3"	  },
						{ 6.0f,		AD_UNUSED,	AD_UNUSED,	1.14f,		1.70f,		1.71f,		1.14f,		1.40f,		3.135f,		2.85f,		2.375f,		1.71f,		AD_UNUSED,	0.7125f,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	AD_UNUSED,	AD_UNUSED,	1.152f,		1.718f,		1.728f,		1.152f,		1.415f,		3.168f,		2.88f,		2.38f,		1.728f,		AD_UNUSED,	0.72f	,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	AD_UNUSED,	AD_UNUSED,	1.248f,		1.882f,		1.872f,		1.248f,		1.785f,		3.432f,		3.12f,		2.61f,		1.872f,		AD_UNUSED,	0.78f,		AD_UNUSED,	3.432f	  },
						{ 13.1f,	AD_UNUSED,	AD_UNUSED,	1.26f,		1.90f,		1.89f,		1.26f,		1.80f,		3.465f,		3.15f,		2.625f,		1.89f,		AD_UNUSED,	0.7875f,	AD_UNUSED,	3.465f	  } },
						/*PCI		Spare		Spare		XC4VLX40	XC2C64A		XC4VLX40	AMCC3485	XC4VLX40	XC4VLX40	XC4VLX40	XC4VLX40	LT1764				uPD44325184	`		XC4VLX40  */
						/*										AMCC3485			uPD44325364	XC2C64A														XC2C64A	  */

{kDag50dup,	0,	0,	1,	0,	{ 0.063578f,	AD_UNUSED,	AD_UNUSED,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.006250f,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	AD_UNUSED,	0.005125f,	AD_UNUSED,	0.017188f },	/* refer to kDag50sg2 schematic, C */
						{ "12V",	NULL,		NULL,		"1V2",		"1V8_CPLDCORE",	"1V8_FRAMER",	"1V2_FRAMER",	"1V5_FIFO",	"3V3",		"3V_PCI",	"2V5_AUX",	"1V8_PREPHY",	NULL,		"0V75_QDR_TERM",NULL,		"3V3"	  },
						{ 6.0f,		AD_UNUSED,	AD_UNUSED,	1.14f,		1.70f,		1.71f,		1.14f,		1.40f,		3.135f,		2.85f,		2.375f,		1.71f,		AD_UNUSED,	0.7125f,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	AD_UNUSED,	AD_UNUSED,	1.152f,		1.718f,		1.728f,		1.152f,		1.415f,		3.168f,		2.88f,		2.38f,		1.728f,		AD_UNUSED,	0.72f	,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	AD_UNUSED,	AD_UNUSED,	1.248f,		1.882f,		1.872f,		1.248f,		1.785f,		3.432f,		3.12f,		2.61f,		1.872f,		AD_UNUSED,	0.78f,		AD_UNUSED,	3.432f	  },
						{ 13.1f,	AD_UNUSED,	AD_UNUSED,	1.26f,		1.90f,		1.89f,		1.26f,		1.80f,		3.465f,		3.15f,		2.625f,		1.89f,		AD_UNUSED,	0.7875f,	AD_UNUSED,	3.465f	  } },
						/*PCI		Spare		Spare		XC4VLX40	XC2C64A		XC4VLX40	AMCC3485	XC4VLX40	XC4VLX40	XC4VLX40	XC4VLX40	LT1764				uPD44325184	`		XC4VLX40  */
						/*										AMCC3485			uPD44325364	XC2C64A														XC2C64A	  */

{kDag54s12,	0,	0,	1,	0,	{ 0.063578f,	AD_UNUSED,	AD_UNUSED,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.006250f,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	AD_UNUSED,	0.005125f,	AD_UNUSED,	0.017188f },	/* refer to kDag50sg2 schematic, C */
						{ "12V",	NULL,		NULL,		"1V2",		"1V8_CPLDCORE",	"1V8_FRAMER",	"1V2_FRAMER",	"1V5_FIFO",	"3V3",		"3V_PCI",	"2V5_AUX",	"1V8_PREPHY",	NULL,		"0V75_QDR_TERM",NULL,		"3V3"	  },
						{ 6.0f,		AD_UNUSED,	AD_UNUSED,	1.14f,		1.70f,		1.71f,		1.14f,		1.40f,		3.135f,		2.85f,		2.375f,		1.71f,		AD_UNUSED,	0.7125f,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	AD_UNUSED,	AD_UNUSED,	1.152f,		1.718f,		1.728f,		1.152f,		1.415f,		3.168f,		2.88f,		2.38f,		1.728f,		AD_UNUSED,	0.72f	,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	AD_UNUSED,	AD_UNUSED,	1.248f,		1.882f,		1.872f,		1.248f,		1.785f,		3.432f,		3.12f,		2.61f,		1.872f,		AD_UNUSED,	0.78f,		AD_UNUSED,	3.432f	  },
						{ 13.1f,	AD_UNUSED,	AD_UNUSED,	1.26f,		1.90f,		1.89f,		1.26f,		1.80f,		3.465f,		3.15f,		2.625f,		1.89f,		AD_UNUSED,	0.7875f,	AD_UNUSED,	3.465f	  } },
						/*PCI		Spare		Spare		XC4VLX40	XC2C64A		XC4VLX40	AMCC3485	XC4VLX40	XC4VLX40	XC4VLX40	XC4VLX40	LT1764				uPD44325184	`		XC4VLX40  */
						/*										AMCC3485			uPD44325364	XC2C64A														XC2C64A	  */

{kDag54sg48,	0,	0,	1,	0,	{ 0.063578f,	AD_UNUSED,	AD_UNUSED,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.006250f,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	AD_UNUSED,	0.005125f,	AD_UNUSED,	0.017188f },	/* refer to kDag50sg2 schematic, C */
						{ "12V",	NULL,		NULL,		"1V2",		"1V8_CPLDCORE",	"1V8_FRAMER",	"1V2_FRAMER",	"1V5_FIFO",	"3V3",		"3V_PCI",	"2V5_AUX",	"1V8_PREPHY",	NULL,		"0V75_QDR_TERM",NULL,		"3V3"	  },
						{ 6.0f,		AD_UNUSED,	AD_UNUSED,	1.14f,		1.70f,		1.71f,		1.14f,		1.40f,		3.135f,		2.85f,		2.375f,		1.71f,		AD_UNUSED,	0.7125f,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	AD_UNUSED,	AD_UNUSED,	1.152f,		1.718f,		1.728f,		1.152f,		1.415f,		3.168f,		2.88f,		2.38f,		1.728f,		AD_UNUSED,	0.72f	,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	AD_UNUSED,	AD_UNUSED,	1.248f,		1.882f,		1.872f,		1.248f,		1.785f,		3.432f,		3.12f,		2.61f,		1.872f,		AD_UNUSED,	0.78f,		AD_UNUSED,	3.432f	  },
						{ 13.1f,	AD_UNUSED,	AD_UNUSED,	1.26f,		1.90f,		1.89f,		1.26f,		1.80f,		3.465f,		3.15f,		2.625f,		1.89f,		AD_UNUSED,	0.7875f,	AD_UNUSED,	3.465f	  } },
						/*PCI		Spare		Spare		XC4VLX40	XC2C64A		XC4VLX40	AMCC3485	XC4VLX40	XC4VLX40	XC4VLX40	XC4VLX40	LT1764				uPD44325184	`		XC4VLX40  */
						/*										AMCC3485			uPD44325364	XC2C64A														XC2C64A	  */

{kDag50sg2a,	0,	0,	1,	1,	{ 0.063578f,	0.022537f,	0.009690f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	AD_UNUSED,	0.017188f,	AD_UNUSED,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.013110f,	0.017188f },	/*B2*/
						{ "12V_ISOL",	"5V",		"1V8",		"1V2",		"1V8_CPLD",	"1V8_FRAMER",	"1V2_FRAMER",	NULL,		"3V3",		NULL,		"2V5_AUX",	"0V9_FIFO_TERM","0V9_D/QDR_TERM","0V9_D/QDR_REF","3V_PCI",	"3V3"	  },
						{ 6.0f,		4.75f,		1.71f,		1.14f,		1.70f,		1.71f,		1.14f,		AD_UNUSED,	3.135f,		AD_UNUSED,	2.375f,		0.855f,		0.855f,		0.68f,		2.85f,		3.135f	  },
						{ 10.0f,	4.80f,		1.728f,		1.152f,		1.718f,		1.728f,		1.152f,		AD_UNUSED,	3.168f,		AD_UNUSED,	2.38f,		0.864f,		0.864f,		0.689f	,	2.88f,		3.168f	  },
						{ 13.0f,	5.20f 	,	1.872f,		1.248f,		1.882f,		1.872f,		1.248f,		AD_UNUSED,	3.432f,		AD_UNUSED,	2.61f,		0.936f,		0.936f,		0.941f,		3.12f,		3.432f	  },
						{ 13.1f,	5.25f,		1.89f,		1.26f,		1.90f,		1.89f,		1.26f,		AD_UNUSED,	3.465f,		AD_UNUSED,	2.625f,		0.945f,		0.945f,		0.95f,		3.15f,		3.465f	  } },
						/*Isolated	PCI		IDT75P52100	XC4VLX80	XC2C128		AMCC3485	AMCC3485			XC4VLX80			XC4VLX80	uPD44325364	uPD44325184	uPD44325184	XC4VLX80	XC4VLX80  */
						/*																XC4VSX55													XC4VSX55  */
						/*																XC2C128														XC2C128	  */

{kDag50sg2a,	0,	1,	0,	0,	{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },	/*B2*/
						{ NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },

{kDag50sg3a,	0,	0,	1,	1,	{ 0.063578f,	0.022537f,	0.009694f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	AD_UNUSED,	0.017188f,	AD_UNUSED,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.013110f,	0.017188f },	/* refer to kDag50sg2a schematic, B2 */
						{ "12V_ISOL",	"5V",		"1V8",		"1V2",		"1V8_CPLD",	"1V8_FRAMER",	"1V2_FRAMER",	NULL,		"3V3",		NULL,		"2V5_AUX",	"0V9_FIFO_TERM","0V9_D/QDR_TERM","0V9_D/QDR_REF","3V_PCI",	"3V3"	  },
						{ 6.0f,		4.75f,		1.71f,		1.14f,		1.70f,		1.71f,		1.14f,		AD_UNUSED,	3.135f,		AD_UNUSED,	2.375f,		0.855f,		0.855f,		0.68f,		2.85f,		3.135f	  },
						{ 10.0f,	4.80f,		1.728f,		1.152f,		1.718f,		1.728f,		1.152f,		AD_UNUSED,	3.168f,		AD_UNUSED,	2.38f,		0.864f,		0.864f,		0.689f	,	2.88f,		3.168f	  },
						{ 13.0f,	5.20f 	,	1.872f,		1.248f,		1.882f,		1.872f,		1.248f,		AD_UNUSED,	3.432f,		AD_UNUSED,	2.61f,		0.936f,		0.936f,		0.941f,		3.12f,		3.432f	  },
						{ 13.1f,	5.25f,		1.89f,		1.26f,		1.90f,		1.89f,		1.26f,		AD_UNUSED,	3.465f,		AD_UNUSED,	2.625f,		0.945f,		0.945f,		0.95f,		3.15f,		3.465f	  } },
						/*Isolated	PCI		IDT75P52100	XC4VLX80	XC2C128		AMCC3485	AMCC3485			XC4VLX80			XC4VLX80	uPD44325364	uPD44325184	uPD44325184	XC4VLX80	XC4VLX80  */
						/*																XC4VSX55													XC4VSX55  */
						/*																XC2C128														XC2C128	  */

{kDag50sg3a,	0,	1,	0,	0,	{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },	/* refer to kDag50sg2a schematic, B2 */
						{ NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },

{kDag54ga,	0,	0,	1,	1,	{ 0.063578f,	0.022537f,	0.009694f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	AD_UNUSED,	0.017188f,	AD_UNUSED,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.013110f,	0.017188f },	/* refer to kDag50sg2a schematic, B2 */
						{ "12V_ISOL",	"5V",		"1V8",		"1V2",		"1V8_CPLD",	"1V8_FRAMER",	"1V2_FRAMER",	NULL,		"3V3",		NULL,		"2V5_AUX",	"0V9_FIFO_TERM","0V9_D/QDR_TERM","0V9_D/QDR_REF","3V_PCI",	"3V3"	  },
						{ 6.0f,		4.75f,		1.71f,		1.14f,		1.70f,		1.71f,		1.14f,		AD_UNUSED,	3.135f,		AD_UNUSED,	2.375f,		0.855f,		0.855f,		0.68f,		2.85f,		3.135f	  },
						{ 10.0f,	4.80f,		1.728f,		1.152f,		1.718f,		1.728f,		1.152f,		AD_UNUSED,	3.168f,		AD_UNUSED,	2.38f,		0.864f,		0.864f,		0.689f	,	2.88f,		3.168f	  },
						{ 13.0f,	5.20f 	,	1.872f,		1.248f,		1.882f,		1.872f,		1.248f,		AD_UNUSED,	3.432f,		AD_UNUSED,	2.61f,		0.936f,		0.936f,		0.941f,		3.12f,		3.432f	  },
						{ 13.1f,	5.25f,		1.89f,		1.26f,		1.90f,		1.89f,		1.26f,		AD_UNUSED,	3.465f,		AD_UNUSED,	2.625f,		0.945f,		0.945f,		0.95f,		3.15f,		3.465f	  } },
						/*Isolated	PCI		IDT75P52100	XC4VLX80	XC2C128		AMCC3485	AMCC3485			XC4VLX80			XC4VLX80	uPD44325364	uPD44325184	uPD44325184	XC4VLX80	XC4VLX80  */
						/*																XC4VSX55													XC4VSX55  */
						/*																XC2C128														XC2C128	  */

{kDag54ga,	0,	1,	0,	0,	{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },	/* refer to kDag50sg2a schematic, B2 */
						{ NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },

{kDag54sa12,	0,	0,	1,	1,	{ 0.063578f,	0.022537f,	0.009694f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	AD_UNUSED,	0.017188f,	AD_UNUSED,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.013110f,	0.017188f },	/* refer to kDag50sg2a schematic, B2 */
						{ "12V_ISOL",	"5V",		"1V8",		"1V2",		"1V8_CPLD",	"1V8_FRAMER",	"1V2_FRAMER",	NULL,		"3V3",		NULL,		"2V5_AUX",	"0V9_FIFO_TERM","0V9_D/QDR_TERM","0V9_D/QDR_REF","3V_PCI",	"3V3"	  },
						{ 6.0f,		4.75f,		1.71f,		1.14f,		1.70f,		1.71f,		1.14f,		AD_UNUSED,	3.135f,		AD_UNUSED,	2.375f,		0.855f,		0.855f,		0.68f,		2.85f,		3.135f	  },
						{ 10.0f,	4.80f,		1.728f,		1.152f,		1.718f,		1.728f,		1.152f,		AD_UNUSED,	3.168f,		AD_UNUSED,	2.38f,		0.864f,		0.864f,		0.689f	,	2.88f,		3.168f	  },
						{ 13.0f,	5.20f 	,	1.872f,		1.248f,		1.882f,		1.872f,		1.248f,		AD_UNUSED,	3.432f,		AD_UNUSED,	2.61f,		0.936f,		0.936f,		0.941f,		3.12f,		3.432f	  },
						{ 13.1f,	5.25f,		1.89f,		1.26f,		1.90f,		1.89f,		1.26f,		AD_UNUSED,	3.465f,		AD_UNUSED,	2.625f,		0.945f,		0.945f,		0.95f,		3.15f,		3.465f	  } },
						/*Isolated	PCI		IDT75P52100	XC4VLX80	XC2C128		AMCC3485	AMCC3485			XC4VLX80			XC4VLX80	uPD44325364	uPD44325184	uPD44325184	XC4VLX80	XC4VLX80  */
						/*																XC4VSX55													XC4VSX55  */
						/*																XC2C128														XC2C128	  */

{kDag54sa12,	0,	1,	0,	0,	{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },	/* refer to kDag50sg2a schematic, B2 */
						{ NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },

{kDag54sga48,	0,	0,	1,	1,	{ 0.063578f,	0.022537f,	0.009694f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	AD_UNUSED,	0.017188f,	AD_UNUSED,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.013110f,	0.017188f },	/* refer to kDag50sg2a schematic, B2 */
						{ "12V_ISOL",	"5V",		"1V8",		"1V2",		"1V8_CPLD",	"1V8_FRAMER",	"1V2_FRAMER",	NULL,		"3V3",		NULL,		"2V5_AUX",	"0V9_FIFO_TERM","0V9_D/QDR_TERM","0V9_D/QDR_REF","3V_PCI",	"3V3"	  },
						{ 6.0f,		4.75f,		1.71f,		1.14f,		1.70f,		1.71f,		1.14f,		AD_UNUSED,	3.135f,		AD_UNUSED,	2.375f,		0.855f,		0.855f,		0.68f,		2.85f,		3.135f	  },
						{ 10.0f,	4.80f,		1.728f,		1.152f,		1.718f,		1.728f,		1.152f,		AD_UNUSED,	3.168f,		AD_UNUSED,	2.38f,		0.864f,		0.864f,		0.689f	,	2.88f,		3.168f	  },
						{ 13.0f,	5.20f 	,	1.872f,		1.248f,		1.882f,		1.872f,		1.248f,		AD_UNUSED,	3.432f,		AD_UNUSED,	2.61f,		0.936f,		0.936f,		0.941f,		3.12f,		3.432f	  },
						{ 13.1f,	5.25f,		1.89f,		1.26f,		1.90f,		1.89f,		1.26f,		AD_UNUSED,	3.465f,		AD_UNUSED,	2.625f,		0.945f,		0.945f,		0.95f,		3.15f,		3.465f	  } },
						/*Isolated	PCI		IDT75P52100	XC4VLX80	XC2C128		AMCC3485	AMCC3485			XC4VLX80			XC4VLX80	uPD44325364	uPD44325184	uPD44325184	XC4VLX80	XC4VLX80  */
						/*																XC4VSX55													XC4VSX55  */
						/*																XC2C128														XC2C128	  */

{kDag54sga48,	0,	1,	0,	0,	{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },	/* refer to kDag50sg2a schematic, B2 */
						{ NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },


/*card		rev	idx	sens1	sens2	  AD1		AD2		AD3		AD4		AD5		AD6		AD7		AD8		AD9		AD10		AD11		AD12		AD13		AD14		AD15		AD16	*/
/*========================================================================================================================================================================================================================================================================================================*/
{kDag62,	0,	0,	1,	0,	{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },	/*C*/
						{ NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },


/*card		rev	idx	sens1	sens2	  AD1		AD2		AD3		AD4		AD5		AD6		AD7		AD8		AD9		AD10		AD11		AD12		AD13		AD14		AD15		AD16		note			*/
/*========================================================================================================================================================================================================================================================================================================*/
{kDag71s,	0,	0,	1,	1,	{ 0.063578f,	AD_UNUSED,	0.009694f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.006250f,	0.017188f,	0.341583f,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	0.019361f,	0.017188f },	/* refer to the RevE_fixed schematic, E */
						{ "12V",	NULL,		"1V25_IXP_TERM","1V5",		"1V8_FIFOCORE",	"1V8_CPLD",	"1V2",		"0V75_VTERM_QDR","3V3",		"12VIN_MOLEX",	"2V5",		"2V5_IXP",	"1V2",		"0V75_REF",	"3V3_IXP",	"3V3"	  },	/* AD2 is for the 1V8_ANALOG on the schematic but this is a spare input */
						{ 6.0f,		AD_UNUSED,	1.1875f,	1.425f,		1.70f,		1.70f,		1.14f,		0.7125f,	3.135f,		6.0f,		2.375f,		2.375f,		1.14f,		0.7125f,	3.15f,		3.135f	  },
						{ 10.0f,	AD_UNUSED,	1.20f,		1.44f,		1.718f,		1.718f,		1.152f,		0.72f,		3.168f,		10.0f,		2.40f,		2.40f,		1.152f,		0.72f,		3.18f,		3.168f	  },
						{ 13.0f,	AD_UNUSED,	1.30f,		1.56f,		1.882f,		1.882f,		1.248f,		0.78f,		3.432f,		13.0f,		2.60f,		2.60f,		1.248f,		0.78f,		3.42f,		3.432f	  },
						{ 13.1f,	AD_UNUSED,	1.3125f,	1.575f,		1.90f,		1.90f,		1.26f,		1.7875f,	3.465f,		13.1f,		2.625f,		2.625f,		1.26f,		0.7875f,	3.45f,		3.465f	  } },
						/* PCI-e			IXP2350		XC2VP30		uPD44325184	XC2C64		IXP2350		uPD44325184	XC2VP30		LTM4600		XC2VP30		IXP2350		LTM4600		XC2VP30		IXP2350		XC2VP30	  */
						/*						XC3S2000-676					XC3S2000-676			XC2C64				XC3S2000-676			IXP2350		uPD44325184			XC2C64	  */
						/*						uPD44325184					88E1111				XC3S2000-676							XC3S2000-676	XC3S2000-676			XC3S2000-676 */

{kDag70ge,	0,	0,	1,	0,	{ 0.063578f,	0.009694f,	0.009694f,	0.006250f,	0.007813f,	0.007813f,	AD_UNUSED,	0.006250f,	0.017188f,	0.332078f,	0.013021f,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	0.017188f },	/*NOTE: Not in production, B */
						{ "12V",	"1V8_ANALOG",	"1V8_DIGITAL",	"1V5",		"1V8_CPLDCORE",	"1V8_FIFOCORE",	NULL,		"VTERM_QDR",	"3V3",		"2V5_RIO",	"2V5_AUX",	NULL,		NULL,		NULL,		NULL,		"3V3"	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },

{kDag74s,	0,	0,	1,	0,	{ 0.063578f,	0.004847f,	0.004847f,	0.006250f,	0.007813f,	AD_UNUSED,	0.006250f,	AD_UNUSED,	0.017188f,	AD_UNUSED,	0.013021f,	0.010255f,	0.005125f,	AD_UNUSED,	AD_UNUSED,	0.017188f },	/*A*/
						{ "12V",	"1V2_MGT_TX_BUF","1V2_MGT_RX_BUF","1V_CORE_BUF","1V8_BUF",	NULL,		"1V_AVCC_BUF",	NULL,		"3V3",		NULL,		"2V5_OSC_BUF",	"2V5_AUX_BUF",	"1V2_PLL_BUF",	NULL,		NULL,		"3V3"	  },
						{ 6.0f,		1.14f,		1.14f,		0.95f,		1.70f,		AD_UNUSED,	0.95f,		AD_UNUSED,	3.135f,		AD_UNUSED,	2.375f,		2.375f,		1.14f,		AD_UNUSED,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	1.152f,		1.152f,		0.96f,		1.718f,		AD_UNUSED,	0.96f,		AD_UNUSED,	3.168f,		AD_UNUSED,	2.38f,		2.38f,		1.152f,		AD_UNUSED,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	1.248f,		1.248f,		1.04f,		1.882f,		AD_UNUSED,	1.04f,		AD_UNUSED,	3.432f,		AD_UNUSED,	2.61f,		2.61f,		1.248f,		AD_UNUSED,	AD_UNUSED,	3.432f	  },
						{ 13.1f,	1.26f,		1.26f,		1.05f,		1.90f,		AD_UNUSED,	1.05f,		AD_UNUSED,	3.465f,		AD_UNUSED,	2.625f,		2.625f,		1.26f,		AD_UNUSED,	AD_UNUSED,	3.465f	  } },
						/*PCI		XC5VLX50T	XC5VLX50T	XC5VLX50T	uPD44325364	XC5VLX50T	XC5VLX50T			XC5VLX50T			XC5VLX50T	XC5VLX50T	XC5VLX50T					XC5VLX50T */	
						/*										uPD44325364					EPM570F256C5N													EPM570F256C5N */

{kDag74s48,	0,	0,	1,	0,	{ 0.063578f,	0.004847f,	0.004847f,	0.006250f,	0.007813f,	AD_UNUSED,	0.006250f,	AD_UNUSED,	0.017188f,	AD_UNUSED,	0.013021f,	0.010255f,	0.005125f,	AD_UNUSED,	AD_UNUSED,	0.017188f },	/*A*/
						{ "12V",	"1V2_MGT_TX_BUF","1V2_MGT_RX_BUF","1V_CORE_BUF","1V8_BUF",	NULL,		"1V_AVCC_BUF",	NULL,		"3V3",		NULL,		"2V5_OSC_BUF",	"2V5_AUX_BUF",	"1V2_PLL_BUF",	NULL,		NULL,		"3V3"	  },
						{ 6.0f,		1.14f,		1.14f,		0.95f,		1.70f,		AD_UNUSED,	0.95f,		AD_UNUSED,	3.135f,		AD_UNUSED,	2.375f,		2.375f,		1.14f,		AD_UNUSED,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	1.152f,		1.152f,		0.96f,		1.718f,		AD_UNUSED,	0.96f,		AD_UNUSED,	3.168f,		AD_UNUSED,	2.38f,		2.38f,		1.152f,		AD_UNUSED,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	1.248f,		1.248f,		1.04f,		1.882f,		AD_UNUSED,	1.04f,		AD_UNUSED,	3.432f,		AD_UNUSED,	2.61f,		2.61f,		1.248f,		AD_UNUSED,	AD_UNUSED,	3.432f	  },
						{ 13.1f,	1.26f,		1.26f,		1.05f,		1.90f,		AD_UNUSED,	1.05f,		AD_UNUSED,	3.465f,		AD_UNUSED,	2.625f,		2.625f,		1.26f,		AD_UNUSED,	AD_UNUSED,	3.465f	  } },
						/*PCI		XC5VLX50T	XC5VLX50T	XC5VLX50T	uPD44325364	XC5VLX50T	XC5VLX50T			XC5VLX50T			XC5VLX50T	XC5VLX50T	XC5VLX50T					XC5VLX50T */	
						/*										uPD44325364					EPM570F256C5N													EPM570F256C5N */

{kDag75g2,	0,	0,	1,	0,	{ 0.063578f,	0.004847f,	0.004847f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	AD_UNUSED,	0.017188f,	AD_UNUSED,	0.013021f,	0.010255f,	0.005125f,	AD_UNUSED,	AD_UNUSED,	0.017188f },	/*A*/
						{ "12V",	"1V2_MGT_TX_BUF","1V2_MGT_RX_BUF","1V_CORE_BUF","1V8_BUF",	"1V5_BUF",	"1V_AVCC_BUF",	NULL,		"3V3",		NULL,		"2V5_OSC_BUF",	"2V5_AUX_BUF",	"1V2_PLL_BUF",	NULL,		NULL,		"3V3"	  },
						{ 6.0f,		1.14f,		1.14f,		0.95f,		1.70f,		1.425f,		0.95f,		AD_UNUSED,	3.135f,		AD_UNUSED,	2.375f,		2.375f,		1.14f,		AD_UNUSED,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	1.152f,		1.152f,		0.96f,		1.718f,		1.44f,		0.96f,		AD_UNUSED,	3.168f,		AD_UNUSED,	2.38f,		2.38f,		1.152f,		AD_UNUSED,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	1.248f,		1.248f,		1.04f,		1.882f,		1.56f,		1.04f,		AD_UNUSED,	3.432f,		AD_UNUSED,	2.61f,		2.61f,		1.248f,		AD_UNUSED,	AD_UNUSED,	3.432f	  },
						{ 13.1f,	1.26f,		1.26f,		1.05f,		1.90f,		1.575f,		1.05f,		AD_UNUSED,	3.465f,		AD_UNUSED,	2.625f,		2.625f,		1.26f,		AD_UNUSED,	AD_UNUSED,	3.465f	  } },
						/*PCI		XC5VLX50T	XC5VLX50T	XC5VLX50T	uPD44325364	XC5VLX50T	XC5VLX50T			XC5VLX50T			XC5VLX50T	XC5VLX50T	XC5VLX50T					XC5VLX50T */	
						/*										uPD44325364					EPM570F256C5N													EPM570F256C5N */

{kDag75g4,	0,	0,	1,	0,	{ 0.063578f,	0.004847f,	0.004847f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	AD_UNUSED,	0.017188f,	AD_UNUSED,	0.013021f,	0.010255f,	0.005125f,	AD_UNUSED,	AD_UNUSED,	0.017188f },	/*A*/
						{ "12V",	"1V2_MGT_TX_BUF","1V2_MGT_RX_BUF","1V_CORE_BUF","1V8_BUF",	"1V5_BUF",	"1V_AVCC_BUF",	NULL,		"3V3",		NULL,		"2V5_OSC_BUF",	"2V5_AUX_BUF",	"1V2_PLL_BUF",	NULL,		NULL,		"3V3"	  },
						{ 6.0f,		1.14f,		1.14f,		0.95f,		1.70f,		1.425f,		0.95f,		AD_UNUSED,	3.135f,		AD_UNUSED,	2.375f,		2.375f,		1.14f,		AD_UNUSED,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	1.152f,		1.152f,		0.96f,		1.718f,		1.44f,		0.96f,		AD_UNUSED,	3.168f,		AD_UNUSED,	2.38f,		2.38f,		1.152f,		AD_UNUSED,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	1.248f,		1.248f,		1.04f,		1.882f,		1.56f,		1.04f,		AD_UNUSED,	3.432f,		AD_UNUSED,	2.61f,		2.61f,		1.248f,		AD_UNUSED,	AD_UNUSED,	3.432f	  },
						{ 13.1f,	1.26f,		1.26f,		1.05f,		1.90f,		1.575f,		1.05f,		AD_UNUSED,	3.465f,		AD_UNUSED,	2.625f,		2.625f,		1.26f,		AD_UNUSED,	AD_UNUSED,	3.465f	  } },
						/*PCI		XC5VLX50T	XC5VLX50T	XC5VLX50T	uPD44325364	XC5VLX50T	XC5VLX50T			XC5VLX50T			XC5VLX50T	XC5VLX50T	XC5VLX50T					XC5VLX50T */	
						/*										uPD44325364					EPM570F256C5N													EPM570F256C5N */
{kDag75be,	0,	0,	1,	0,	{ 0.063578f,	0.004847f,	0.004847f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	AD_UNUSED,	0.017188f,	AD_UNUSED,	0.013021f,	0.010255f,	0.005125f,	AD_UNUSED,	AD_UNUSED,	0.017188f },	/*A*/
						{ "12V",	"1V2_MGT_TX_BUF","1V2_MGT_RX_BUF","1V_CORE_BUF","1V8_BUF",	"1V5_BUF",	"1V_AVCC_BUF",	NULL,		"3V3",		NULL,		"2V5_OSC_BUF",	"2V5_AUX_BUF",	"1V2_PLL_BUF",	NULL,		NULL,		"3V3"	  },
						{ 6.0f,		1.14f,		1.14f,		0.95f,		1.70f,		1.425f,		0.95f,		AD_UNUSED,	3.135f,		AD_UNUSED,	2.375f,		2.375f,		1.14f,		AD_UNUSED,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	1.152f,		1.152f,		0.96f,		1.718f,		1.44f,		0.96f,		AD_UNUSED,	3.168f,		AD_UNUSED,	2.38f,		2.38f,		1.152f,		AD_UNUSED,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	1.248f,		1.248f,		1.04f,		1.882f,		1.56f,		1.04f,		AD_UNUSED,	3.432f,		AD_UNUSED,	2.61f,		2.61f,		1.248f,		AD_UNUSED,	AD_UNUSED,	3.432f	  },
						{ 13.1f,	1.26f,		1.26f,		1.05f,		1.90f,		1.575f,		1.05f,		AD_UNUSED,	3.465f,		AD_UNUSED,	2.625f,		2.625f,		1.26f,		AD_UNUSED,	AD_UNUSED,	3.465f	  } },
						/*PCI		XC5VLX50T	XC5VLX50T	XC5VLX50T	uPD44325364	XC5VLX50T	XC5VLX50T			XC5VLX50T			XC5VLX50T	XC5VLX50T	XC5VLX50T					XC5VLX50T */	
						/*										uPD44325364					EPM570F256C5N													EPM570F256C5N */
{kDag75ce,	0,	0,	1,	0,	{ 0.063578f,	0.004847f,	0.004847f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	AD_UNUSED,	0.017188f,	AD_UNUSED,	0.013021f,	0.010255f,	0.005125f,	AD_UNUSED,	AD_UNUSED,	0.017188f },	/*A*/
						{ "12V",	"1V2_MGT_TX_BUF","1V2_MGT_RX_BUF","1V_CORE_BUF","1V8_BUF",	"1V5_BUF",	"1V_AVCC_BUF",	NULL,		"3V3",		NULL,		"2V5_OSC_BUF",	"2V5_AUX_BUF",	"1V2_PLL_BUF",	NULL,		NULL,		"3V3"	  },
						{ 6.0f,		1.14f,		1.14f,		0.95f,		1.70f,		1.425f,		0.95f,		AD_UNUSED,	3.135f,		AD_UNUSED,	2.375f,		2.375f,		1.14f,		AD_UNUSED,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	1.152f,		1.152f,		0.96f,		1.718f,		1.44f,		0.96f,		AD_UNUSED,	3.168f,		AD_UNUSED,	2.38f,		2.38f,		1.152f,		AD_UNUSED,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	1.248f,		1.248f,		1.04f,		1.882f,		1.56f,		1.04f,		AD_UNUSED,	3.432f,		AD_UNUSED,	2.61f,		2.61f,		1.248f,		AD_UNUSED,	AD_UNUSED,	3.432f	  },
						{ 13.1f,	1.26f,		1.26f,		1.05f,		1.90f,		1.575f,		1.05f,		AD_UNUSED,	3.465f,		AD_UNUSED,	2.625f,		2.625f,		1.26f,		AD_UNUSED,	AD_UNUSED,	3.465f	  } },
						/*PCI		XC5VLX50T	XC5VLX50T	XC5VLX50T	uPD44325364	XC5VLX50T	XC5VLX50T			XC5VLX50T			XC5VLX50T	XC5VLX50T	XC5VLX50T					XC5VLX50T */	
						/*										uPD44325364					EPM570F256C5N													EPM570F256C5N */


/*card		rev	idx	sens1	sens2	  AD1		AD2		AD3		AD4		AD5		AD6		AD7		AD8		AD9		AD10		AD11		AD12		AD13		AD14		AD15		AD16	*/
/*========================================================================================================================================================================================================================================================================================================*/
{kDag810,	0,	0,	1,	0,	{ 0.063578f,	0.004847f,	0.004847f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.012500f,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	0.005125f,	AD_UNUSED,	AD_UNUSED,	0.017188f },	/* refer to 8.1sx schematic, C */
						{ "12V",	"1V2_MGT_TX_BUF","1V2_MGT_RX_BUF","1V_CORE_BUF","1V8_CPLD_BUF",	"VDD18D_BUF",	"1V_AVCC_BUF",	"1V8_XFP_BUF",	"2V_BUF",	"5V_BUF",	"2V5_LVDS_BUF",	"2V5_AUX_BUF",	"1V2_PLL_BUF",	NULL,		NULL,		"3V3"	  },
						{ 6.0f,		1.14f,		1.14f,		0.95f,		1.70f,		1.71f,		0.95f,		1.71f,		1.90f,		4.75f,		2.375f,		2.375f,		1.14f,		AD_UNUSED,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	1.152f,		1.152f,		0.96f,		1.718f,		1.728f,		0.96f,		1.728f,		1.92f,		4.80f,		2.38f,		2.38f,		1.152f,		AD_UNUSED,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	1.248f,		1.248f,		1.04f,		1.882f,		1.872f,		1.04f,		1.882f,		2.08f,		5.20f,		2.61f,		2.61f,		1.248f,		AD_UNUSED,	AD_UNUSED,	3.432f	  },
						{ 13.1f,	1.26f,		1.26f,		1.05f,		1.90f,		1.89f,		1.05f,		1.89f,		2.10f,		5.25f,		2.625f,		2.625f,		1.26f,		AD_UNUSED,	AD_UNUSED,	3.465f	  } },
						/*PCI		XC5VLX85T	XC5VLX85T	XC5VLX85T	XC2C64A		VSC8479		XC5VLX85T	XFP		XC5VLX85T_MGT	XFP		XC5VLX85T	XC5VLX85T	XC5VLX85T					XC5VLX85T */	
						/*																														XC2C64A	  */

{kDag82x,	0,	0,	1,	0,	{ 0.063578f,	0.013161f,	0.013161f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.006250f,	0.017188f,	0.026042f,	AD_UNUSED,	0.010255f,	0.005125f,	0.005125f,	0.004828f,	0.017188f },	/*B1*/
						{ "12V",	"2V5_AUX",	"3V0_PCI",	"1V5_PCIE",	"1V8_CPLDCORE",	"1V5_FRM",	"1V5_BRIDGE2",	"1V5_BRIDGE1",	"3V3",		"5V",		NULL,		"2V0_BUF",	"VDD12D_BUF",	"1V2_BUF",	"3V3_BRDG",	"3V3"	  },
						{ 6.0f,		2.375f,		2.85f,		1.46f,		1.70f,		1.40f,		1.425f,		1.425f,		3.135f,		4.75f,		AD_UNUSED,	1.90f,		1.14f,		1.14f,		3.00f,		3.135f	  },
						{ 10.0f,	2.38f,		2.88f,		1.465f,		1.718f,		1.44f,		1.44f,		1.44f,		3.168f,		4.80f,		AD_UNUSED,	1.92f,		1.152f,		1.152f,		3.033f,		3.168f	  },
						{ 13.0f,	2.61f,		3.12f,		1.535f,		1.882f,		1.56f,		1.56f,		1.56f,		3.432f,		5.20f,		AD_UNUSED,	2.08f,		1.248f,		1.248f,		3.567f,		3.432f	  },
						{ 13.1f,	2.625f,		3.15f,		1.55f,		1.90f,		1.60f,		1.575f,		1.575f,		3.465f,		5.25f,		AD_UNUSED,	2.10f,		1.26f,		1.26f,		3.60f,		3.465f	  } },
						/*PCI		XC4VSX35	XC4VSX35	QG41210 PCI-X	XC2C64A		XC4VSX35	QG41210	PCI-X	QG41210 PCI-X	XC4VSX35	XFP				XC4VSX35_MGT	VSC8476		XC4VSX35	QG41210 PCI-X	XC4VSX35  */
						/*										VSC8476						XC2C64A														XC2C64A	  */
						/*																VSC8476														VSC8476	  */
						/*																Strataflash													Strataflash */

{kDag82x,	0,	1,	0,	0,	{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },	/*B1*/
						{ NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },

{kDag82x,	0,	2,	0,	0,	{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },	/*B1*/
						{ NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL  	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },

{kDag82z,	0,	0,	1,	0,	{ 0.063578f,	0.013161f,	0.013161f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.006250f,	0.017188f,	0.026042f,	AD_UNUSED,	0.010255f,	0.005125f,	0.005125f,	0.004828f,	0.017188f },	/* refer to kDag82x schematic, B1 */
						{ "12V",	"2V5_AUX",	"3V0_PCI",	"1V5_PCIE",	"1V8_CPLD",	"1V5_FRM",	"1V5_BRG2",	"1V5_BRG1",	"3V3",		"5V",		NULL,		"2V0_BUF",	"VDD12D",	"1V2_BUF",	"3V3_BRDG",	"3V3"	  },
						{ 6.0f,		2.375f,		2.85f,		1.46f,		1.70f,		1.40f,		1.425f,		1.425f,		3.135f,		4.75f,		AD_UNUSED,	1.90f,		1.14f,		1.14f,		3.00f,		3.135f	  },
						{ 10.0f,	2.38f,		2.88f,		1.465f,		1.718f,		1.44f,		1.44f,		1.44f,		3.168f,		4.80f,		AD_UNUSED,	1.92f,		1.152f,		1.152f,		3.033f,		3.168f	  },
						{ 13.0f,	2.61f,		3.12f,		1.535f,		1.882f,		1.56f,		1.56f,		1.56f,		3.432f,		5.20f,		AD_UNUSED,	2.08f,		1.248f,		1.248f,		3.567f,		3.432f	  },
						{ 13.1f,	2.625f,		3.15f,		1.55f,		1.90f,		1.60f,		1.575f,		1.575f,		3.465f,		5.25f,		AD_UNUSED,	2.10f,		1.26f,		1.26f,		3.60f,		3.465f	  } },
						/*PCI		XC4VSX35	XC4VSX35	QG41210 PCI-X	XC2C64A		XC4VSX35	QG41210	PCI-X	QG41210 PCI-X	XC4VSX35	XFP				XC4VSX35_MGT	VSC8476		XC4VSX35	QG41210 PCI-X	XC4VSX35  */
						/*										VSC8476						XC2C64A														XC2C64A	  */
						/*																VSC8476														VSC8476	  */
						/*																Strataflash													Strataflash */

{kDag82z,	0,	1,	0,	0,	{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },	/* refer to kDag82x schematic, B1 */
						{ NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },

{kDag82z,	0,	2,	0,	0,	{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },	/* refer to kDag82x schematic, B1 */
						{ NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL	  },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED },
						{ AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED,	AD_UNUSED } },

{kDag840,	0,	0,	1,	0,	{ 0.063578f,	0.004847f,	0.004847f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.006250f,	AD_UNUSED,	AD_UNUSED,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	AD_UNUSED,	0.017188f },	/*C*/
						{ "12V",	"1V2_MGT_TX_BUF","1V2_MGT_RX_BUF","1V_CORE_BUF","1V8_CPLD_BUF",	"1V8_BUF",	"1V_AVCC_BUF",	"0V75_TERM",	NULL,		NULL,		"2V5_BUF",	"2V5_AUX_BUF",	"1V2_PLL_BUF",	"1V2_CAM_BUF",	NULL,		"3V3"	  },
						{ 6.0f,		1.14f,		1.14f,		0.95f,		1.70f,		1.70f,		0.95f,		0.7125f,	AD_UNUSED,	AD_UNUSED,	2.375f,		2.375f,		1.14f,		1.14f,		AD_UNUSED,	3.135f	  },
						{ 10.0f,	1.152f,		1.152f,		0.96f,		1.718f,		1.718f,		0.96f,		0.72f,		AD_UNUSED,	AD_UNUSED,	2.38f,		2.38f,		1.152f,		1.152f,		AD_UNUSED,	3.168f	  },
						{ 13.0f,	1.248f,		1.248f,		1.04f,		1.882f,		1.882f,		1.04f,		0.78f,		AD_UNUSED,	AD_UNUSED,	2.61f,		2.61f,		1.248f,		1.248f,		AD_UNUSED,	3.432f	  },
						{ 13.1f,	1.26f,		1.26f,		1.05f,		1.90f,		1.90f,		1.05f,		0.7875f,	AD_UNUSED,	AD_UNUSED,	2.625f,		2.625f,		1.26f,		1.26f,		AD_UNUSED,	3.465f	  } },
						/*PCI		XC5VLX110T	XC5VLX110T	XC5VLX110T	XC2C64A		GS8662D QDR	XC5VLX110T	GS8662D QDR					VSC3312		XC5VLX110T	XC5VLX110T	IDT75K72234			XC5VLX110T */
						/*																				IDT75K72234									XC2C64A	  */
						/*																				GS8161Z36B									VSC3312	  */
						/*																														IDT75K72234 */

{kDag850,	0,	0,	1,	0,	{ 0.063578f,	0.004828f,	0.004828f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	0.006250f,	AD_UNUSED,	AD_UNUSED,	0.013021f,	0.010254f,	0.005125f,	0.005125f,	AD_UNUSED,	0.017188f },	/*A*/
						{ "12V",	"1V2_MGT_TX_BUF","1V2_MGT_RX_BUF","1V_CORE_BUF","1V8_CPLD_BUF",	"1V8_BUF",	"1V_AVCC_BUF",	"0V75_TERM",	NULL,		NULL,		"2V5_BUF",	"2V5_AUX_BUF",	"1V_PLL_BUF",	"1V2_CAM_BUF",	NULL,		"3V3"	  },
						{ 6.0f,		1.14f,		1.14f,		0.95f,		1.70f,		1.70f,		0.95f,		0.7125f,	AD_UNUSED,	AD_UNUSED,	2.375f,		2.375f,		0.94f,		1.14f,		AD_UNUSED,	3.135f	  },
						{ 10.0f,	1.152f,		1.152f,		0.96f,		1.718f,		1.718f,		0.96f,		0.72f,		AD_UNUSED,	AD_UNUSED,	2.38f,		2.38f,		0.952f,		1.152f,		AD_UNUSED,	3.168f	  },
						{ 13.0f,	1.248f,		1.248f,		1.04f,		1.882f,		1.882f,		1.04f,		0.78f,		AD_UNUSED,	AD_UNUSED,	2.61f,		2.61f,		1.048f,		1.248f,		AD_UNUSED,	3.432f	  },
						{ 13.1f,	1.26f,		1.26f,		1.05f,		1.90f,		1.90f,		1.05f,		0.7875f,	AD_UNUSED,	AD_UNUSED,	2.625f,		2.625f,		1.06f,		1.26f,		AD_UNUSED,	3.465f	  } },
						/*PCI		XC5VLX110T	XC5VLX110T	XC5VLX110T	XC2C64A		GS8662D QDR	XC5VLX110T	GS8662D QDR					VSC3312		XC5VLX110T	XC5VLX110T	XC4VFX20			XC5VLX110T */
						/*																				IDT75K72234					IDT75K72234			XC2C64A */
						/*																														VSC3312 */
						/*																														IDT75K72234 */


/*card		rev	idx	sens1	sens2	  AD1		AD2		AD3		AD4		AD5		AD6		AD7		AD8		AD9		AD10		AD11		AD12		AD13		AD14		AD15		AD16	*/
/*========================================================================================================================================================================================================================================================================================================*/
{kDag91x2Rx,	0,	0,	1,	1,	{ 0.063578f,	0.004847f,	0.004847f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	AD_UNUSED,	0.017188f,	AD_UNUSED,	AD_UNUSED,	0.010255f,	0.005125f,	0.005125f,	AD_UNUSED,	0.017188f },	/* refer to 8.3sx schematic, A? */
						{ "12V",	"1V2_MGT_TX_BUF","1V2_MGT_RX_BUF","1V_CORE_BUF","1V8_CPLD_BUF",	"VDD15D_BUF",	"1V_AVCC_BUF",	NULL,		"3V3_BUF",	NULL,		NULL,		"2V5_AUX_BUF",	"1V2_PLL_BUF",	"1V2 PHY",	NULL,		"3V3"	  },
						{ 6.0f,		1.14f,		1.14f,		0.95f,		1.70f,		1.43f,		0.95f,		AD_UNUSED,	3.135f,		AD_UNUSED,	AD_UNUSED,	2.375f,		1.14f,		1.14f  ,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	1.152f,		1.152f,		0.96f,		1.718f,		1.46f,		0.96f,		AD_UNUSED,	3.168f,		AD_UNUSED,	AD_UNUSED,	2.38f,		1.152f,		1.152f ,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	1.248f,		1.248f,		1.04f,		1.882f,		1.54f,		1.04f,		AD_UNUSED,	3.432f,		AD_UNUSED,	AD_UNUSED,	2.61f,		1.248f,		1.248f ,	AD_UNUSED,	3.432f	  },
						{ 13.1f,	1.26f,		1.26f,		1.05f,		1.90f,		1.58f,		1.05f,		AD_UNUSED,	3.465f,		AD_UNUSED,	AD_UNUSED,	2.625f,		1.26f,		1.26f  ,	AD_UNUSED,	3.465f	  } },
						/*PCI		XC5VLX85T	XC5VLX85T	XC5VLX85T	XC2C64A		VSC8479		XC5VLX85T			XC5VLX85T_MGT					XC5VLX85T	XC5VLX85T					XC5VLX85T */	
						/*																														XC2C64A	  */

{kDag92x,	0,	0,	1,	0,	{ 0.063578f,	0.004847f,	0.004847f,	0.006250f,	0.007813f,	0.007813f,	0.006250f,	AD_UNUSED,	0.017188f,	0.026042f,	0.013021f,	0.010255f,	0.005125f,	0.005125f,	AD_UNUSED,	0.017188f },	/*B*/
						{ "12V",	"1V1_VCCGXB_TX","1V1_VCCGXB_RX","0V95_CORE",	"1V8",		"1V4_VCCHGXB",	"1V5_VCCPT",	NULL,		"3V3",		"2V5_OSC",	"2V5",		"2V5_VCCIO_INT","1V1_VCCLGXB",	"1V2 PHY",	NULL,		"3V3"	  },
						{ 6.0f,		1.045f,		1.045f,		0.9025f,	1.70f,		1.33f,		1.425f,		AD_UNUSED,	3.135f,		2.375f,		2.375f,		2.375f,		1.045f,		1.14f  ,	AD_UNUSED,	3.135f	  },
						{ 10.0f,	1.056f,		1.056f,		0.912f,		1.718f,		1.344f,		1.44f,		AD_UNUSED,	3.168f,		2.38f,		2.38f,		2.38f,		1.056f,		1.152f ,	AD_UNUSED,	3.168f	  },
						{ 13.0f,	1.144f,		1.144f,		0.988f,		1.882f,		1.456f,		1.56f,		AD_UNUSED,	3.432f,		2.61f,		2.61f,		2.61f,		1.144f,		1.248f ,	AD_UNUSED,	3.432f	  },
						{ 13.1f,	1.155f,		1.155f,		0.9975f,	1.90f,		1.47f,		1.575f,		AD_UNUSED,	3.465f,		2.625f,		2.625f,		2.625f,		1.155f,		1.26f  ,	AD_UNUSED,	3.465f	  } },
						/*PCI		EP4GX180H	EP4GX180H	EP4GX180H	EP4GX180H	EP4GX180H	EP4GX180H			LTC2861		LVDS 2.5V Osc	EP4GX180H	EP4GX180H	EP4GX180H	QT2225PRKD-1			LTC2861   */	
						/*								QT2225PRKD-1							LTC6905 Osc			QT2225PRKD-1									LTC6905 Osc */
						/*								CY7C1415BV18							LTC3413 DDR TR			EPM570F256C5N									LTC3413 DDR TR */
						/*								StrataFlash											StrataFlash										  */
						/*																				24LC256			*/
};



/**
 * The following is the register map for the LM93 devices.
 *
 */
typedef enum {

	/* misc */
	LM93_SMBUS_TEST         = 0x01,
	LM93_MANUF_ID           = 0x3E,
	LM93_VERSION            = 0x3F,

	/* bmc error status */
	LM93_BMC_ERROR_STATUS_1 = 0x40,
	LM93_BMC_ERROR_STATUS_2 = 0x41,

	/* temperatures */
	LM93_ZONE1_TEMP         = 0x50,
	LM93_ZONE2_TEMP         = 0x51,
	LM93_INTERNAL_TEMP      = 0x52,

	/* voltages */
	LM93_AD_IN1             = 0x56,
	LM93_AD_IN2             = 0x57,
	LM93_AD_IN3             = 0x58,
	LM93_AD_IN4             = 0x59,
	LM93_AD_IN5             = 0x5A,
	LM93_AD_IN6             = 0x5B,
	LM93_AD_IN7             = 0x5C,
	LM93_AD_IN8             = 0x5D,
	LM93_AD_IN9             = 0x5E,
	LM93_AD_IN10            = 0x5F,
	LM93_AD_IN11            = 0x60,
	LM93_AD_IN12            = 0x61,
	LM93_AD_IN13            = 0x62,
	LM93_AD_IN14            = 0x63,
	LM93_AD_IN15            = 0x64,
	LM93_AD_IN16            = 0x65,

	/* temperature limit registers */
	LM93_ZONE1_TEMP_MIN     = 0x78,
	LM93_ZONE1_TEMP_MAX     = 0x79,
	LM93_ZONE2_TEMP_MIN     = 0x7A,
	LM93_ZONE2_TEMP_MAX     = 0x7B,
	LM93_INTERNAL_TEMP_MIN  = 0x7C,
	LM93_INTERNAL_TEMP_MAX  = 0x7D,

	/* fan boost control */
	LM93_ZONE1_FANBOOST     = 0x80,
	LM93_ZONE2_FANBOOST     = 0x81,
	LM93_ZONE3_FANBOOST     = 0x82,
	LM93_ZONE4_FANBOOST     = 0x83,

	/* setup registers */
	LM93_MIN_PWM1           = 0xC3,
	LM93_MIN_PWM2           = 0xC4,
	LM93_PWM1_CTRL1         = 0xC8,
	LM93_PWM1_CTRL2         = 0xC9,
	LM93_PWM1_CTRL3         = 0xCA,
	LM93_PWM2_CTRL1         = 0xCC,
	LM93_PWM2_CTRL2         = 0xCD,
	LM93_PWM2_CTRL3         = 0xCE,
	LM93_ZONE1_BASE_TEMP    = 0xD0,
	LM93_ZONE2_BASE_TEMP    = 0xD1,
	LM93_INTERNAL_BASE_TEMP = 0xD2,
	LM93_LOOKUP_T1          = 0xD4,
	LM93_LOOKUP_T2          = 0xD5,
	LM93_LOOKUP_T3          = 0xD6,
	LM93_LOOKUP_T4          = 0xD7,
	LM93_LOOKUP_T5          = 0xD8,
	LM93_LOOKUP_T6          = 0xD9,
	LM93_LOOKUP_T7          = 0xDA,
	LM93_LOOKUP_T8          = 0xDB,
	LM93_LOOKUP_T9          = 0xDC,
	LM93_LOOKUP_T10         = 0xDD,
	LM93_LOOKUP_T11         = 0xDE,
	LM93_LOOKUP_T12         = 0xDF,

	/* general configuration registers */
	LM93_STATUS_CTRL        = 0xE2,
	LM93_CONFIG             = 0xE3,
	LM93_SLEEP              = 0xE4

} lm93_registers_t;


/**
 * The following is the register map for the LM63 devices.
 *
 */
typedef enum {

	/* temperatures */
	LM63_INTERNAL_TEMP      = 0x00,
	LM63_REMOTE_TEMP_MSB    = 0x01,
	LM63_REMOTE_TEMP_LSB    = 0x10,


	LM63_CONFIG              = 0x03,
	LM63_INTERNAL_TEMP_MAX   = 0x05,
	LM63_REMOTE_TEMP_MAX_MSB = 0x07,
	LM63_REMOTE_TEMP_MAX_LSB = 0x13,
	
/*	LM63_ALERT_STATUS        = 0x02,   defined in dagutil.h */
/*	LM63_ALERT_MASK          = 0x16,   defined in dagutil.h */
	
/*	LM63_PWM_RPM             = 0x4A,   defined in dagutil.h */
/*	LM63_SPINUP              = 0x4B,   defined in dagutil.h */
/*	LM63_PWM_VALUE           = 0x4C,   defined in dagutil.h */
	LM63_PWM_FREQ            = 0x4D,

/*	LM63_LOOKUP_T1           = 0x50,   defined in dagutil.h */
/*	LM63_LOOKUP_P1           = 0x51,   defined in dagutil.h */
/*	LM63_LOOKUP_T2           = 0x52,   defined in dagutil.h */
/*	LM63_LOOKUP_P2           = 0x53,   defined in dagutil.h */
/*	LM63_LOOKUP_T3           = 0x54,   defined in dagutil.h */
/*	LM63_LOOKUP_P3           = 0x55,   defined in dagutil.h */
/*	LM63_LOOKUP_T4           = 0x56,   defined in dagutil.h */
/*	LM63_LOOKUP_P4           = 0x57,   defined in dagutil.h */
/*	LM63_LOOKUP_T5           = 0x58,   defined in dagutil.h */
/*	LM63_LOOKUP_P5           = 0x59,   defined in dagutil.h */
/*	LM63_LOOKUP_T6           = 0x5A,   defined in dagutil.h */
/*	LM63_LOOKUP_P6           = 0x5B,   defined in dagutil.h */
/*	LM63_LOOKUP_T7           = 0x5C,   defined in dagutil.h */
/*	LM63_LOOKUP_P7           = 0x5D,   defined in dagutil.h */
/*	LM63_LOOKUP_T8           = 0x5E,   defined in dagutil.h */
/*	LM63_LOOKUP_P8           = 0x5F,   defined in dagutil.h */
	
	LM63_MANUF_ID            = 0xFE
	
} lm63ex_registers_t;


/**
 * The I2C / SMBus 7-bit address used for LM63 devices, note that this
 * address is always fixed and cannot be changed by hardware. The address
 * is put in the upper 7 bits, bit 0 is always 0.
 */
#define LM63_I2C_ADDRESS     0x98


/**
 * The I2C / SMBus 7-bit address used for LM93 devices, note that the
 * lower two bits of the address can be changed by hardware, however on
 * all DAG cards at the moment the lower two bits are always zero. The
 * address is put in the upper 7 bits, bit 0 is always 0.
 */
#define LM93_I2C_ADDRESS     0x58





/**
 * Structure describing details about an LM sensor device found during
 * enumeration of the bus. Note that not all the fields are initialised
 * at creation.
 */
typedef struct sens_details_s
{
	uint16_t  enum_idx;              /**< The index of the SMBus controller in the enumeration table */
	uint8_t   i2c_addr;              /**< The I2C / SMBus address of the device */
		
	uint32_t  raw_access : 1;        /**< Set if using RAW SMBus, otherwise clear */
	uint32_t  usable     : 1;        /**< Set if the device is deemed to be usable, i.e. it responded during initialisation */
	uint32_t  tmp_sens_1 : 1;        /**< Value taken from the feature table, used to indicate if the first remote diode is available */
	uint32_t  tmp_sens_2 : 1;        /**< Value taken from the feature table, used to indicate if the second remote diode is available */
	uint32_t  reserved   : 28;
		
} lm_sens_details_t;
 


/**
 * Structure used to store state information for the component. For the
 * LM sensors component we store the number of sensors and the type and
 * address of the interface to the sensor.
 *
 */
typedef struct lm_sens_state_s
{
	uint32_t           sens_count;	/**< The number of sensors on the card */
	lm_sens_details_t *sensors_p;	/**< Dynamically allocated array of LM sensors on the card */
		
} lm_sens_state_t;



/**
 * Structure used to store state information for the component. For the
 * LM sensors component we store the number of sensors and the type and
 * address of the interface to the sensor.
 *
 */
typedef struct voltage_attr_s
{
	uint32_t      address;          /**< Address of the sensor and register to read the value from, this should
	                                 *   be in the grw format which is; bits[31:16] = SMBus enum index,
	                                 *   bits[15:8] = I2C address and bits[7:0] = the register address.
	                                 */
									 
	float         multiplier;       /**< The multiplier to apply to the register value to get the actual voltage. */
	
	uint32_t      raw_access : 1;   /**< If set then access to the sensor is via a RAW SMBus controller. */
	uint32_t      reserved   : 31;
	
} voltage_attr_t;


/**
 * Structure used to store state information for the component. For the
 * error/warning voltages, we store whether we have such a thing, and the
 * threshold if we do have one.
 */
typedef struct errvoltage_attr_s
{
	uint32_t	enabled;	/**< true if this warning/error is enabled */
	float		threshold;	/**< the threshold value */
} errvoltage_attr_t;






/* Function prototypes */
static dag_err_t _lm_sensor_scan(DagCardPtr card, lm_sens_details_t *details_p);
static int lm_sensor_post_initialize(ComponentPtr component);
static void lm_sensor_dispose(ComponentPtr component);
static void lm_sensor_reset(ComponentPtr component);
static void lm_sensor_default(ComponentPtr component);
static void _lm_sensor_init_lm93(DagCardPtr card, lm_sens_details_t *details_p);
static void _lm_sensor_init_lm63(DagCardPtr card, lm_sens_details_t *details_p);
static void _lm_sensor_voltage_attr_dispose(AttributePtr self);
static void _lm_sensor_add_lm93_voltage_attr(DagCardPtr card, ComponentPtr component, lm_sens_details_t *sensor_p, uint32_t index, float multiplier, const char *description_p, uint32_t have_lowerr, float lowerr, uint32_t have_lowwarn, float lowwarn, uint32_t have_highwarn, float highwarn, uint32_t have_higherr, float higherr);
static void _lm_sensor_add_legacy_lm93_voltage_attr(DagCardPtr card, ComponentPtr component, lm_sens_details_t *sensor_p, float multiplier);




/**
 * This function is called during the initial setup of the Config API, it is
 * responsible for creating the new LM sensor component and returning a pointer
 * to it. This function is unlike the typical *_new_component function in that
 * it doesn't map 1 to 1 with in enumeration entry, instead it scans the enum
 * table for temperature sensors and if it finds any it creates the component
 * and adds the sensor(s) to it's internal state. If no sensors are found this
 * function returns a NULL pointer, and set the last error indicator to kDagErrNone.
 *
 *
 * @param[in]  card        Handle / pointer to the card object the component is
 *                         being created for.
 *
 * returns                 A pointer to the newly created component, if no entries
 *                         in the enum table are found then this function will
 *                         return NULL and set the last error indicator to kDagErrNone.
 *
 */
ComponentPtr
lm_sensor_get_new_component(DagCardPtr card)
{
	ComponentPtr       new_comp = NULL; 
	lm_sens_state_t   *state_p = NULL;
	lm_sens_details_t *details_p = NULL;
	dag_reg_t          registers[DAG_REG_MAX_ENTRIES];
	dag_reg_t          registers_raw[DAG_REG_MAX_ENTRIES];
	int                i = 0, j = 0;
	int                reg_count = 0;
	int		   reg_count_raw = 0;
	int                mon_reg_count = 0;

	
	
	/* first scan the enum table for any temperature sensors, SMB entries with version 1 */
	memset (registers, 0x00, sizeof(registers));
	reg_count = dag_reg_find ((char*)card_get_iom_address(card), DAG_REG_SMB, registers);
	for (i=0; i<reg_count; i++)
		if ( registers[i].version == 1 )
			mon_reg_count++;

	/* second scan the enum table for any raw SMBus controllers */
	memset (registers_raw, 0x00, sizeof(registers_raw));
	reg_count_raw = dag_reg_find ((char*)card_get_iom_address(card), DAG_REG_RAW_SMBBUS, registers_raw);
	for (i=0; i<reg_count_raw; i++)
		mon_reg_count++;
	
	
	/* no temperature sensors found */
	if ( mon_reg_count == 0 )
	{
		dag_config_set_last_error ((dag_card_ref_t)card, kDagErrNone);
		return NULL;
	}


	/* setup the private state of the component */
	details_p = (lm_sens_details_t*) malloc(sizeof(lm_sens_details_t) * mon_reg_count);
	if ( details_p == NULL )
	{
		dag_config_set_last_error ((dag_card_ref_t)card, kDagErrGeneral);
		return NULL;
	}
	memset (details_p, 0x00, (sizeof(lm_sens_details_t) * mon_reg_count));
	
	
	/* iterate over the sensor in the enum table and verify we can talk to them */
	for (i=0, j=0; i<reg_count; i++)
		if ( registers[i].version == 1 )
		{
			details_p[j].enum_idx  = (uint16_t) i;
			details_p[j].i2c_addr  = 0xFF;
			details_p[j].usable    = 0;

			/* currently only the 7.0g uses raw access for the SMBus interface */
			if (card_get_card_type(card) == kDag70ge || card_get_card_type(card) == kDag71s)	//FIXME: make this code non-card specific
				details_p[j].raw_access = 1;
			else
				details_p[j].raw_access = 0;
			
			
			/* scan the I2C bus for the address of the device and return it, a value of 0xFF indicates no reply */
			_lm_sensor_scan (card, &details_p[j]);
			j++;
		}
		
	/* iterate over the raw SMBus controllers */
	for (i=0; i<reg_count_raw; i++)
	{
		details_p[j].enum_idx  = (uint16_t) i;
		details_p[j].i2c_addr  = 0xFF;
		details_p[j].usable    = 0;

		/* currently only the 7.0g uses raw access for the SMBus interface */
		if (card_get_card_type(card) == kDag70ge || card_get_card_type(card) == kDag71s)	//FIXME: make this code non-card specific
			details_p[j].raw_access = 1;
		else
			details_p[j].raw_access = 0;
			
		/* scan the I2C bus for the address of the device and return it, a value of 0xFF indicates no reply */
		_lm_sensor_scan (card, &details_p[j]);
		j++;
	}


	
	/* once more chack that we have some usable sensors */
	for (i=0; i<mon_reg_count; i++)
		if ( details_p[i].usable )
			break;
			
	if ( i == mon_reg_count )
	{
		free (details_p);
		dag_config_set_last_error ((dag_card_ref_t)card, kDagErrNone);
		return NULL;
	}
		
	
	
	
	/* finally create the component */
	new_comp = component_init(kComponentHardwareMonitor, card);
	if (new_comp == NULL)
	{
		free (details_p);
		return NULL;
	}
	
	
	/* install the callback handlers for the component */
	component_set_dispose_routine (new_comp, lm_sensor_dispose);
	component_set_post_initialize_routine (new_comp, lm_sensor_post_initialize);
	component_set_reset_routine (new_comp, lm_sensor_reset);
	component_set_default_routine (new_comp, lm_sensor_default);
	component_set_name (new_comp, "hw_monitor");
	component_set_description (new_comp, "Card hardware monitoring.");

	
	/* set the component state structure */
	state_p = (lm_sens_state_t*) malloc(sizeof(lm_sens_state_t));
	state_p->sens_count = mon_reg_count;
	state_p->sensors_p  = details_p;
	component_set_private_state(new_comp, state_p);




	return new_comp;
}





/**
 * Scans the given I2C / SM bus for an LM63 or LM93 device using the standard address, 
 * if a valid response is found the usable flag in the supplied details structure is
 * set, and the i2c_addr field is updated with the correct address. If a valid response
 * was not received on any of the addresses, the usable flag is cleared and the i2c_addr
 * is set to 0xFF.
 *
 * @param[in]     card          Pointer / handle to the card we are working on.
 * @param[in,out] details_p     Pointer to the structure describing details about the device,
 *                              the enum_idx and raw_access fields are read by this function
 *                              and the usable and i2c_addr fields are written by this
 *                              function.
 *
 * @returns                     An error code indicating succes or failure.
 * @retval 
 *
 */
static dag_err_t
_lm_sensor_scan(DagCardPtr card, lm_sens_details_t *details_p)
{
	GenericReadWritePtr grw;
	uint32_t            address = 0;
	uint32_t            value = 0;
	size_t              i;
	grw_error_t         err;
	
	/* possible I2C addresses and their expected responses (Note we can't use 0x00
	 * as an expected response, because that is what grw_read returns if it fails).
	 */
	struct _tag_possible
	{
		uint8_t   i2c_addr;
		uint8_t   reg_addr;
		uint8_t   exp_value;
		
	} possibles[] = { { LM93_I2C_ADDRESS, LM93_MANUF_ID, 0x01 },
	                  { LM63_I2C_ADDRESS, LM63_MANUF_ID, 0x01 },
	                };


	/* invalidate the detials structure */
	details_p->i2c_addr = 0xFF;
	details_p->usable   = 0;


	/* go through the possible addresses ... */
	for ( i=0; i<dagutil_arraysize(possibles); i++ )
	{
		address  = possibles[i].reg_addr;
		address |= possibles[i].i2c_addr << 8;
		address |= details_p->enum_idx << 16;
		
		if (details_p->raw_access)
			grw = grw_init(card, address, grw_raw_smbus_read, grw_raw_smbus_write);
		else
			grw = grw_init(card, address, grw_drb_smbus_read, grw_drb_smbus_write);
 
		value = grw_read(grw);
		err = grw_get_last_error(grw);
		
		grw_dispose(grw);
		
		if (err != kGRWNoError)
			continue;
			
		if (value == possibles[i].exp_value)
		{
			details_p->i2c_addr = possibles[i].i2c_addr;
			details_p->usable   = 1;
			break;
		}
	}


	return kDagErrNone;
}





/**
 * Post initialisation callback, this is where all the attributes are registered. To register
 * the attributes, the list of sensors stored in the component state structure are iterated
 * over and the temperature and voltage attributes for each sensor are added.
 *
 * @param[in,out] component     Pointer / handle to the component being initialised.
 *
 * @returns                     Always returns 1.
 *
 */
static int
lm_sensor_post_initialize(ComponentPtr component)
{
	GenericReadWritePtr grw;
	DagCardPtr          card = component_get_card(component);
	AttributePtr        attr;
	uint32_t            mask = 0xffffffff;
	lm_sens_state_t    *state_p = component_get_private_state(component);
	lm_sens_details_t  *details_p = NULL;
	uint32_t            sens_count;
	size_t              i, j, k;
	uint32_t            lm_idx;
	int                 card_type;

	ComponentPtr root;
	ComponentPtr card_info;
	AttributePtr board_rev;
	uint32_t revision;
	uint8_t attr_added = 0;

	#define ADD_TEMPERATURE_ATTRIB(d,a) \
	{ \
		if ( (d).raw_access ) \
			grw = grw_init(card, ((a) | ((d).i2c_addr << 8) | ((d).enum_idx << 16)), grw_raw_smbus_read, NULL); \
		else \
			grw = grw_init(card, ((a) | ((d).i2c_addr << 8) | ((d).enum_idx << 16)), grw_drb_smbus_read, NULL); \
		attr = attribute_factory_make_attribute(kUint32AttributeTemperature, grw, &mask, 1); \
		component_add_attribute(component, attr); \
	}


	/* grab a pointer to the LM sensors and their numbers */
	details_p = state_p->sensors_p;
	sens_count = state_p->sens_count;
	

	
	/* add the temperature and voltage attributes to the component, this is based on the
	 * type of the card and the presence of the different entries in the enum table.
	 */
	card_type = card_get_card_type(card);

	/* Retrieve board revision value */
	root = card_get_root_component(card);
	assert (root != NULL);
	card_info = component_get_subcomponent(root, kComponentCardInfo, 0);
	assert (card_info != NULL);
	board_rev = component_get_attribute(card_info, kUint32AttributeBoardRevision);
	assert (board_rev != NULL);
	revision = *(uint32_t *)attribute_get_value (board_rev);

for (k = 0;  k < sens_count ; k++) 
{ 
	// clear the attrubute added for the k LM sensor (index) 
	attr_added = 0;	
	
	//Search for eaxct match of the board revision number LM sensor 
	for (i=0; i<dagutil_arraysize(g_lm_feature_map); i++) 
	{
		// Add voltage attributes based on card type and board revision, and ensure the attributes are only ever added once.		
		if ( (g_lm_feature_map[i].card_type == card_type) && (revision == g_lm_feature_map[i].board_rev) )
		{
			lm_idx = g_lm_feature_map[i].lm_index;
//			if ( lm_idx >= sens_count )
			if ( lm_idx != k )
				continue;
				
			/* check that the sensor is actually usable */
			if ( details_p[lm_idx].usable == 0 )
				continue;
			
			/* save the remote temperature sensor information */
			details_p[lm_idx].tmp_sens_1 = g_lm_feature_map[i].tmp_sens_1;
			details_p[lm_idx].tmp_sens_2 = g_lm_feature_map[i].tmp_sens_2;
			
			/* add the temperature and voltage attributes for the LM93 */
			if ( details_p[lm_idx].i2c_addr == LM93_I2C_ADDRESS )
			{
				if ( details_p[lm_idx].tmp_sens_1 )
					ADD_TEMPERATURE_ATTRIB (details_p[lm_idx], LM93_ZONE1_TEMP);
				if ( details_p[lm_idx].tmp_sens_2 )
					ADD_TEMPERATURE_ATTRIB (details_p[lm_idx], LM93_ZONE2_TEMP);
				if ( !details_p[lm_idx].tmp_sens_1 && !details_p[lm_idx].tmp_sens_2 )
					ADD_TEMPERATURE_ATTRIB (details_p[lm_idx], LM93_INTERNAL_TEMP);
				
				/* add the legacy voltage attribute */
				_lm_sensor_add_legacy_lm93_voltage_attr (card, component, &(details_p[lm_idx]), g_lm_feature_map[i].ad_multipliers[0]);
				
				/* add all the other voltages */
				for (j=0; j<16; j++)
					if ( g_lm_feature_map[i].ad_multipliers[j] != AD_UNUSED )
						_lm_sensor_add_lm93_voltage_attr (
							card, component, &(details_p[lm_idx]), j, 
							g_lm_feature_map[i].ad_multipliers[j], g_lm_feature_map[i].ad_descriptions[j], 
							(g_lm_feature_map[i].ad_lowerr[j] != AD_UNUSED), g_lm_feature_map[i].ad_lowerr[j],
							(g_lm_feature_map[i].ad_lowwarn[j] != AD_UNUSED), g_lm_feature_map[i].ad_lowwarn[j], 
							(g_lm_feature_map[i].ad_highwarn[j] != AD_UNUSED), g_lm_feature_map[i].ad_highwarn[j], 
							(g_lm_feature_map[i].ad_higherr[j] != AD_UNUSED), g_lm_feature_map[i].ad_higherr[j]
							);

			}
			
			
			/* add just the temperature attributes for the LM63 */
			else if ( details_p[lm_idx].i2c_addr == LM63_I2C_ADDRESS )
			{
				if ( details_p[lm_idx].tmp_sens_1 )
					ADD_TEMPERATURE_ATTRIB (details_p[lm_idx], LM63_REMOTE_TEMP_MSB)
				else
					ADD_TEMPERATURE_ATTRIB (details_p[lm_idx], LM63_INTERNAL_TEMP)
			}
			attr_added = 1;	
			break; // no point to conitnue due for one index(LM sensor), CARD, BOARD we have only one entry  
		};
	}; // end loop for exact match lm sensor for k index
	
	if (!attr_added) 
	{
		// only if no exat match found for the k index(LM sensor) 
	for (i=0; i<dagutil_arraysize(g_lm_feature_map); i++) {
		// Add voltage attributes based on card type and board revision, and ensure the attributes are only ever added once.
		if ( (g_lm_feature_map[i].card_type == card_type) && (revision > g_lm_feature_map[i].board_rev))
		{
			lm_idx = g_lm_feature_map[i].lm_index;
			if ( lm_idx != k ) continue;
				
			/* check that the sensor is actually usable */
			if ( details_p[lm_idx].usable == 0 )
				continue;
			
			/* save the remote temperature sensor information */
			details_p[lm_idx].tmp_sens_1 = g_lm_feature_map[i].tmp_sens_1;
			details_p[lm_idx].tmp_sens_2 = g_lm_feature_map[i].tmp_sens_2;
			
			/* add the temperature and voltage attributes for the LM93 */
			if ( details_p[lm_idx].i2c_addr == LM93_I2C_ADDRESS )
			{
				if ( details_p[lm_idx].tmp_sens_1 )
					ADD_TEMPERATURE_ATTRIB (details_p[lm_idx], LM93_ZONE1_TEMP);
				if ( details_p[lm_idx].tmp_sens_2 )
					ADD_TEMPERATURE_ATTRIB (details_p[lm_idx], LM93_ZONE2_TEMP);
				if ( !details_p[lm_idx].tmp_sens_1 && !details_p[lm_idx].tmp_sens_2 )
					ADD_TEMPERATURE_ATTRIB (details_p[lm_idx], LM93_INTERNAL_TEMP);
				
				/* add the legacy voltage attribute */
				_lm_sensor_add_legacy_lm93_voltage_attr (card, component, &(details_p[lm_idx]), g_lm_feature_map[i].ad_multipliers[0]);
				
				/* add all the other voltages */
				for (j=0; j<16; j++)
					if ( g_lm_feature_map[i].ad_multipliers[j] != AD_UNUSED )
						_lm_sensor_add_lm93_voltage_attr (
							card, component, &(details_p[lm_idx]), j, 
							g_lm_feature_map[i].ad_multipliers[j], g_lm_feature_map[i].ad_descriptions[j], 
							(g_lm_feature_map[i].ad_lowerr[j] != AD_UNUSED), g_lm_feature_map[i].ad_lowerr[j],
							(g_lm_feature_map[i].ad_lowwarn[j] != AD_UNUSED), g_lm_feature_map[i].ad_lowwarn[j], 
							(g_lm_feature_map[i].ad_highwarn[j] != AD_UNUSED), g_lm_feature_map[i].ad_highwarn[j], 
							(g_lm_feature_map[i].ad_higherr[j] != AD_UNUSED), g_lm_feature_map[i].ad_higherr[j]
							);

			}
			
			
			/* add just the temperature attributes for the LM63 */
			else if ( details_p[lm_idx].i2c_addr == LM63_I2C_ADDRESS )
			{
				if ( details_p[lm_idx].tmp_sens_1 )
					ADD_TEMPERATURE_ATTRIB (details_p[lm_idx], LM63_REMOTE_TEMP_MSB)
				else
					ADD_TEMPERATURE_ATTRIB (details_p[lm_idx], LM63_INTERNAL_TEMP)
			}
		break;

		};
	}; //end loop no exact match 	
		
	}; //if no exatc match for k
} /* loop through the indexes (LM sensors on the cards) */
	return 1;
}





/**
 * Callback called when the component is being destroyed, here we just free the
 * array allocated for the individual LM sensors.
 *
 * @param[in]  component        The component being destroyed
 *
 */
static void
lm_sensor_dispose(ComponentPtr component)
{
	lm_sens_state_t  *state_p = NULL;
	
	state_p = component_get_private_state(component);
	if ( state_p != NULL )
	{
		free (state_p->sensors_p);
		state_p->sensors_p = NULL;
		state_p->sens_count = 0;
	}
}



/**
 * Reset function for the component, for the LM sensor component there is nothing
 * required here.
 *
 * @param[in]  component        The component being reset
 *
 */
static void
lm_sensor_reset(ComponentPtr component)
{
}





/**
 * Callback called when the user calls default for the component, in the case where an
 * LM63 sensor device is present the temperature lookup map for the PWM output is
 * configured. For LM93 devices the default reset condition is maintained, which is
 * that the fan boost system is enabled (turns the fans on when the temperature reaches
 * 60 degrees for either temperature zone).
 *
 * @param[in]  component        The component that the default settings are being
 *                              applied to.
 *
 */
static void
lm_sensor_default(ComponentPtr component)
{
	DagCardPtr       card;
	uint32_t         i;
    lm_sens_state_t *state_p = component_get_private_state(component);
	
	
	/* Get a pointer to the card */
	card = component_get_card(component);
	
	
	/**
	 * Loop through the devices on the bus and setup their PWM controls for the temperature
	 * steps and so forth. Currently alarms are not configured for any the devices, this is
	 * to maintain backwards compatiblity and not cause any problems with cards that support
	 * the temperature alarm feature.
	 *
	 */
	for ( i=0; i<state_p->sens_count; i++ )
		if ( state_p->sensors_p[i].usable )
		{
			if ( state_p->sensors_p[i].i2c_addr == LM93_I2C_ADDRESS )
				_lm_sensor_init_lm93(card, &state_p->sensors_p[i]);
			else
				_lm_sensor_init_lm63(card, &state_p->sensors_p[i]);
		}


}




/**
 * Initialises the fan and temperature settings for the LM93 devices. This
 * function assumes that both PWM outputs are connected to fans, and that
 * the both temperature sensors are hooked up (if this is not the case it
 * doesn't hurt).
 *
 * For the LM93 sensors we start the fans at 60 degrees and then ramp them
 * up to full power at 75 degrees. I've noticed that the 5V fans don't kick
 * into life until at least half the PWM duty cycle is set (the 12V fans don't
 * have this problem).
 *
 * @param[in]  card             Pointer / handle to the card component that
 *                              we are talking to.
 * @param[in]  details_p        Pointer to the details about the sensor being
 *                              configured.
 *
 */
static void
_lm_sensor_init_lm93(DagCardPtr card, lm_sens_details_t *details_p)
{
	GenericReadWritePtr grw;
	uint32_t            address = 0;
	
	#define LM93_WRITE_BYTE(a, x)                                                               \
		grw_set_address(grw, ((a) | (details_p->i2c_addr << 8) | (details_p->enum_idx << 16))); \
		grw_write(grw, (x));


	/* Initialise the interface to the SMBus controller */
	address  = details_p->i2c_addr << 8;
	address |= details_p->enum_idx << 16;
		
	if (details_p->raw_access)
		grw = grw_init(card, address, grw_raw_smbus_read, grw_raw_smbus_write);
	else
		grw = grw_init(card, address, grw_drb_smbus_read, grw_drb_smbus_write);
 
 
	/* We don't care about the ambient fan boost control, because the fans are only attached
	 * to the FPGA, therefore they have no effect on the actual ambient temperature. The defaults
	 * for zone 1 and 2 are at 60 degrees, which is fine for our hardware.
	 */
	LM93_WRITE_BYTE(LM93_ZONE3_FANBOOST, 0x7F);
	LM93_WRITE_BYTE(LM93_ZONE4_FANBOOST, 0x7F);
	
 

	/* Map PWM1 & PWM2 to be controlled by ZONE1 & ZONE2 temperatures respectively */
	LM93_WRITE_BYTE(LM93_PWM1_CTRL1, 0x01);
	LM93_WRITE_BYTE(LM93_PWM2_CTRL1, 0x02);


	/* If we are missing one or more remote sensors then we force the matching PWM output to be 100% all the time */
	if ( details_p->tmp_sens_1 == 0 )
	{
		LM93_WRITE_BYTE(LM93_PWM1_CTRL2, 0xd1);
	}
	if ( details_p->tmp_sens_2 == 0 )
	{
		LM93_WRITE_BYTE(LM93_PWM2_CTRL2, 0xd1);
	}
		

	/* Setup the spin control, this helps kicking the 5V fans into life from a standing start (1 second at full power) */
	LM93_WRITE_BYTE(LM93_PWM1_CTRL3, 0xAD);
	LM93_WRITE_BYTE(LM93_PWM2_CTRL3, 0xAD);
	

	/* Setup the hystersis for the fan control (4 degrees either side) */
	LM93_WRITE_BYTE(LM93_MIN_PWM1, 0x04);
	LM93_WRITE_BYTE(LM93_MIN_PWM2, 0x04);


	/* Setup the auto-fan register so that zone 1 and 2 fans kick into
	 * life at 60 degrees C and linearly ratchets up to full power at
	 * 66 degrees C.
	 */
	LM93_WRITE_BYTE(LM93_ZONE1_BASE_TEMP, 60);
	LM93_WRITE_BYTE(LM93_ZONE2_BASE_TEMP, 60);


	/* the fan control table */
	LM93_WRITE_BYTE(LM93_LOOKUP_T1,  0x00);
	LM93_WRITE_BYTE(LM93_LOOKUP_T2,  0x00);
	LM93_WRITE_BYTE(LM93_LOOKUP_T3,  0x00);
	LM93_WRITE_BYTE(LM93_LOOKUP_T4,  0x00);
	LM93_WRITE_BYTE(LM93_LOOKUP_T5,  0x00);
	LM93_WRITE_BYTE(LM93_LOOKUP_T6,  0x00);
	LM93_WRITE_BYTE(LM93_LOOKUP_T7,  0x11);
	LM93_WRITE_BYTE(LM93_LOOKUP_T8,  0x11);
	LM93_WRITE_BYTE(LM93_LOOKUP_T9,  0x11);
	LM93_WRITE_BYTE(LM93_LOOKUP_T10, 0x11);
	LM93_WRITE_BYTE(LM93_LOOKUP_T11, 0x11);
	LM93_WRITE_BYTE(LM93_LOOKUP_T12, 0x11);



	/* set the configuration register, which jabs the start bit  */
	LM93_WRITE_BYTE(LM93_CONFIG, BIT0);


	/* clear the sleep state */
	LM93_WRITE_BYTE(LM93_SLEEP, 0);

	grw_dispose(grw);
}



/**
 * Initialises the fan and temperature settings for the LM63 devices. This
 * function assumes that the PWM output is hooked up to a fan and as such
 * sets up the fan operation based on the input from the temperature sensor.
 *
 * For the LM93 sensors we start the fans at 60 degrees and then ramp them
 * up to full power at 75 degrees. I've noticed that the 5V fans don't kick
 * into life until at least half the PWM duty cycle is set (the 12V fans don't
 * have this problem).
 *
 * @param[in]  card             Pointer / handle to the card component that
 *                              we are talking to.
 * @param[in]  details_p        Pointer to the details about the sensor being
 *                              configured.
 *
 */
static void
_lm_sensor_init_lm63(DagCardPtr card, lm_sens_details_t *details_p)
{
	GenericReadWritePtr grw;
	uint32_t            address = 0;
	
	#define LM63_WRITE_BYTE(a, x)                                                               \
		grw_set_address(grw, ((a) | (details_p->i2c_addr << 8) | (details_p->enum_idx << 16))); \
		grw_write(grw, (x));


	/* Initialise the interface to the SMBus controller */
	address  = details_p->i2c_addr << 8;
	address |= details_p->enum_idx << 16;
		
	if (details_p->raw_access)
		grw = grw_init(card, address, grw_raw_smbus_read, grw_raw_smbus_write);
	else
		grw = grw_init(card, address, grw_drb_smbus_read, grw_drb_smbus_write);
 
 
  	LM63_WRITE_BYTE(LM63_PWM_RPM, 0x20);

	LM63_WRITE_BYTE(LM63_SPINUP, 0x25);               /* spin-up at 100% for 0.8s */
	LM63_WRITE_BYTE(LM63_PWM_FREQ, 0x17);             /* set the default frequency setting */

	LM63_WRITE_BYTE(LM63_LOOKUP_T1, 0x00);            /* 0C */
	LM63_WRITE_BYTE(LM63_LOOKUP_P1, 0x00);            /* 0% */
	LM63_WRITE_BYTE(LM63_LOOKUP_T2, 0x30);            /* 48C */
	LM63_WRITE_BYTE(LM63_LOOKUP_P2, 0x10);            /* 25% */
	LM63_WRITE_BYTE(LM63_LOOKUP_T3, 0x38);            /* 56C */
	LM63_WRITE_BYTE(LM63_LOOKUP_P3, 0x20);            /* 50% */
	LM63_WRITE_BYTE(LM63_LOOKUP_T4, 0x40);            /* 64C */
	LM63_WRITE_BYTE(LM63_LOOKUP_P4, 0x30);            /* 75% */
	LM63_WRITE_BYTE(LM63_LOOKUP_T5, 0x50);            /* 80C */
	LM63_WRITE_BYTE(LM63_LOOKUP_P5, 0x3f);            /* 100% */
	LM63_WRITE_BYTE(LM63_LOOKUP_T6, 0x60);            /* 96C */
	LM63_WRITE_BYTE(LM63_LOOKUP_P6, 0x3f);            /* 100% */
	LM63_WRITE_BYTE(LM63_LOOKUP_T7, 0x70);            /* 112C */
	LM63_WRITE_BYTE(LM63_LOOKUP_P7, 0x3f);            /* 100% */
	LM63_WRITE_BYTE(LM63_LOOKUP_T8, 0x80);            /* 128C */
	LM63_WRITE_BYTE(LM63_LOOKUP_P8, 0x3f);            /* 100% */


	/* If there is no remote temperature sensor (which is currently the case on the 5.2sxa and 8.2x), then
	 * we should enable the alarm for the remote temperature or use the PWM lookup table. We can't use the
	 * PWM table because you can only source the temperature information for the table from the remote diode.
	 */
	if ( details_p->tmp_sens_1 == 0 )
	{
		LM63_WRITE_BYTE(LM63_PWM_RPM, 0x20);          /* enable manual override of the PWM output */
		LM63_WRITE_BYTE(LM63_PWM_VALUE, 0x3F);        /* set full power PWM output */
	}
	else
	{
		LM63_WRITE_BYTE(LM63_PWM_RPM, 0x00);          /* disable write access to the lookup table, and enable the PWM lookup table */
	}
	
	LM63_WRITE_BYTE(LM63_CONFIG, 0x01);               /* trigger alarm on temperature threshold */
	LM63_WRITE_BYTE(LM63_INTERNAL_TEMP_MAX, 0x55);    /* 85C - local high temperature */
	LM63_WRITE_BYTE(LM63_REMOTE_TEMP_MAX_MSB, 0x63);  /* 110C - remote high temperature */
	
	
	/* Enable the alarm output for both the internal and remote temperatures if a remote sensor is
	 * present, otherwise just the internal temperature.
	 */
	if ( details_p->tmp_sens_1 == 0 )
	{
		LM63_WRITE_BYTE(LM63_ALERT_MASK, 0xBF);           /* local high */
	}
	else
	{
		LM63_WRITE_BYTE(LM63_ALERT_MASK, 0xAF);           /* local and remote high */
	}

	grw_dispose(grw);
}






/**
 * Dispose function that frees the state structure allocated when the attribute
 * was created.
 *
 * @param[in]  self             Pointer / handle to the attribute being disposed of.
 *
 */
static void
_lm_sensor_voltage_attr_dispose(AttributePtr self)
{
	voltage_attr_t *attr_state_p = attribute_get_private_state(self);

	if ( attr_state_p )
		free (attr_state_p);
		
	attribute_set_private_state(self, NULL);
}



/**
 * Callback called to get the value of the attribute. This function uses the generic
 * reader and writer functions to read a byte from the remote device, and then converts
 * the value read into an actual voltage using the multiplier stored in the attribute
 * state structure.
 *
 * @param[in]  self             Pointer / handle to the attribute being read.
 *
 *
 */
static void*
_lm_sensor_voltage_attr_get(AttributePtr self)
{
	voltage_attr_t      *attr_state_p = attribute_get_private_state(self);
	DagCardPtr           card = attribute_get_card(self);
    GenericReadWritePtr  grw;
	grw_error_t          err;
	int                  attempts;
    uint32_t             reg_val;
	static float         voltage = 0.0f;
	

	/* initialise the access */
    if ( attr_state_p->raw_access )
		grw = grw_init (card, attr_state_p->address, grw_raw_smbus_read, NULL);
	else
		grw = grw_init (card, attr_state_p->address, grw_drb_smbus_read, NULL);


	/* try three times to read the value then give up */
	attempts = 3;
	do {
		
		/* read the value from the register, then dispose of the reader object */
		reg_val = grw_read(grw);

		/* check for an error and if so try once more then give up with a return error code set */
		err = grw_get_last_error(grw);
		if ( err == kGRWNoError )
			break;
	
	} while (--attempts);
	
	grw_dispose(grw);
	
	
	/* check if we succeeded in reading the value */
	if ( attempts == 0 )
	{
		card_set_last_error (card, kDagErrGeneral);
		
		/* set an obviously invalid value to indicate an error */
		voltage = FLT_MAX;
	}
	
	else
	{
		card_set_last_error (card, kDagErrNone);

		/* convert the value to an actual voltage */
		voltage = attr_state_p->multiplier * (float)(reg_val & 0xFF);
	}
	
	return (void*) &voltage;
}


/**
 * Dispose function that frees the state structure allocated when the attribute
 * was created.
 *
 * @param[in]  self             Pointer / handle to the attribute being disposed of.
 *
 */
static void
_lm_sensor_errvoltage_attr_dispose(AttributePtr self)
{
	errvoltage_attr_t *attr_state_ep = attribute_get_private_state(self);

	if ( attr_state_ep )
		free (attr_state_ep);
		
	attribute_set_private_state(self, NULL);
}



/**
 * Callback called to get the value of the attribute. This function returns the constant
 * held in the private state, if we have such a constant; or some sort of kErr if not
 *
 * @param[in]  self             Pointer / handle to the attribute being read.
 *
 *
 */
static void*
_lm_sensor_errvoltage_attr_get(AttributePtr self)
{
	errvoltage_attr_t      *attr_state_ep = attribute_get_private_state(self);
	DagCardPtr           card = attribute_get_card(self);
	static float         voltage = 0.0f; // Not threadsafe. cute.
	
	if( attr_state_ep->enabled ) { // got one
		card_set_last_error (card, kDagErrNone);
		voltage = attr_state_ep->threshold;
	} else {
		card_set_last_error (card, kDagErrGeneral);
		
		/* set an obviously invalid value to indicate an error */
		voltage = FLT_MAX;
	}
	return (void*) &voltage;
	
}



/**
 * Creates a new voltage attribute and adds it to the given component. The new attribute
 * is given the name stored in the description field and the register offset calculated
 * by using the index. The multiplier field is applied to the attribute before if is returned
 * to the caller.
 *
 * @param[in]  card             Pointer / handle to the card that we are talking to.
 * @param[in]  component        Pointer / handle to the component that we are talking to.
 * @param[in]  sensor_p         Pointer to the structure describing details about the sensor
 *                              being used.
 * @param[in]  index            The index of the analog to digital convertor to use.
 * @param[in]  multiplier       The multiplier to apply to the value read from the register.
 * @param[in]  description_p    Pointer to a string containing the description of the voltage.
 *
 */
static void
_lm_sensor_add_lm93_voltage_attr(DagCardPtr card, ComponentPtr component, lm_sens_details_t *sensor_p, uint32_t index, float multiplier, const char *description_p, uint32_t have_lowerr, float lowerr, uint32_t have_lowwarn, float lowwarn, uint32_t have_highwarn, float highwarn, uint32_t have_higherr, float higherr)
{
	AttributePtr    attr = NULL;
	voltage_attr_t *attr_state_p = NULL;
	errvoltage_attr_t *attr_state_ep = NULL;
	char            str_buf[128];

	uint32_t have_boundary[4] = {have_lowerr, have_lowwarn, have_highwarn, have_higherr};
	float boundary[4] = {lowerr, lowwarn, highwarn, higherr};
	const char *boundary_smsg[4] = {"low_err", "low_warn", "high_warn", "high_err"};
	const char *boundary_lmsg[4] = {"Undervolt Error Voltage", "Undervolt Warning Voltage", "Overvolt Warning Voltage", "Overvolt Error Voltage"};
	const dag_attribute_code_t boundary_attr[4] = {kFloatAttributeVoltageErrorLow, kFloatAttributeVoltageWarningLow, kFloatAttributeVoltageWarningHigh, kFloatAttributeVoltageErrorHigh};
	int b;

	for( b = 0; b< 4; b++ )
	{ 
		attr = attribute_init(boundary_attr[b]);
		if( attr == NULL ) 
		{
			if( b != 0 )
				dagutil_verbose( "Creating attributes for voltages; we probably just leaked RAM.\n");
			return; // We probably just leaked ram if B != 0
		}

		/* Set the name and description of the attribute */
		if ( description_p == NULL )
		{
			attribute_set_name (attr, boundary_smsg[b]);
			attribute_set_description (attr, boundary_lmsg[b]);
		}
		else
		{
			snprintf (str_buf, 128, "%s_%s", boundary_smsg[b], description_p);
			str_buf[127] = '\0';
			attribute_set_name (attr, str_buf);
		
			snprintf (str_buf, 128, "The %s of the %s line (in Volts).", boundary_lmsg[b], description_p);
			str_buf[127] = '\0';
			attribute_set_description (attr, str_buf);
		}
		
	
		/* Set the standard settings */
		attribute_set_config_status (attr, kDagAttrStatus);
		attribute_set_valuetype (attr, kAttributeFloat);
		attribute_set_to_string_routine (attr, attribute_float_to_string);

	
		/* Set the private state, which is just the address details and the mulitplier */
		attr_state_ep = (errvoltage_attr_t*) malloc(sizeof(errvoltage_attr_t));
		if( attr_state_ep == NULL ) {
			dagutil_panic("malloc failed for warning/error voltage private state.\n"); //FIXME
			return;
		}
		memset (attr_state_ep, 0x00, sizeof(errvoltage_attr_t));
	
		attr_state_ep->enabled = have_boundary[b];
		attr_state_ep->threshold = (have_boundary[b]) ? boundary[b] : FLT_MAX; // a sensible default?

		attribute_set_private_state (attr, attr_state_ep);


		/* Install the "get" callback handler for the attribute */
		attribute_set_getvalue_routine (attr, _lm_sensor_errvoltage_attr_get);
		attribute_set_dispose_routine (attr, _lm_sensor_errvoltage_attr_dispose);


		/* Finally add the attribute */
		component_add_attribute(component, attr);

		attr_state_ep = NULL; // reset for next time around the loop
		attr = NULL; // reset for next time around the loop
	}
    
	attr = attribute_init(kFloatAttributeVoltage);
	if ( attr == NULL )
		return;
	
	
	/* Set the name and description of the attribute */
	if ( description_p == NULL )
	{
		attribute_set_name (attr, "Voltage");
		attribute_set_description (attr, "The measured voltage level (in Volts).");
	}
	else
	{
		snprintf (str_buf, 128, "voltage_%s", description_p);
		str_buf[127] = '\0';
		attribute_set_name (attr, str_buf);
		
		snprintf (str_buf, 128, "The measured voltage level of the %s line (in Volts).", description_p);
		str_buf[127] = '\0';
		attribute_set_description (attr, str_buf);
	}
		
	
	/* Set the standard settings */
	attribute_set_config_status (attr, kDagAttrStatus);
	attribute_set_valuetype (attr, kAttributeFloat);
	attribute_set_to_string_routine (attr, attribute_float_to_string);

	
	/* Set the private state, which is just the address details and the mulitplier */
	attr_state_p = (voltage_attr_t*) malloc(sizeof(voltage_attr_t));
	memset (attr_state_p, 0x00, sizeof(voltage_attr_t));
	
	attr_state_p->address = ((LM93_AD_IN1 + index) | (sensor_p->i2c_addr << 8) | (sensor_p->enum_idx << 16));
	attr_state_p->multiplier = multiplier;
	attr_state_p->raw_access = sensor_p->raw_access;
	
	attribute_set_private_state (attr, attr_state_p);


	/* Install the "get" callback handler for the attribute */
	attribute_set_getvalue_routine (attr, _lm_sensor_voltage_attr_get);
	attribute_set_dispose_routine (attr, _lm_sensor_voltage_attr_dispose);


	/* Finally add the attribute */
	component_add_attribute(component, attr);
}



/**
 * Callback called to get the value of the attribute. This function uses the generic
 * reader and writer functions to read a byte from the remote device, and then converts
 * the value read into an percentage of the expected voltage.
 *
 * @param[in]  self             Pointer / handle to the attribute being read.
 *
 *
 */
static void*
_lm_sensor_voltage_attr_get_percentage(AttributePtr self)
{
	voltage_attr_t      *attr_state_p = attribute_get_private_state(self);
	DagCardPtr           card = attribute_get_card(self);
    GenericReadWritePtr  grw;
    uint32_t             reg_val;
	static uint32_t      percentage;


	/* initialise the access */
    if ( attr_state_p->raw_access )
		grw = grw_init (card, attr_state_p->address, grw_raw_smbus_read, NULL);
	else
		grw = grw_init (card, attr_state_p->address, grw_drb_smbus_read, NULL);


	/* read the value from the register, then dispose of the reader object */
	reg_val = grw_read(grw);
	
	grw_dispose(grw);
	
	
	/* convert the value to an actual voltage */
	percentage = (uint32_t) ((float)(attr_state_p->multiplier * (float)(reg_val & 0xFF)) / (float)(attr_state_p->multiplier * 192.0f) * 100.0f);
	
	return (void*) &percentage;
}



/**
 * Creates an old style voltage attribute and adds it to the given component. On previous
 * version of this component the voltage of the first interface was reported as a percentage,
 * however it was completely accurate because it didn't take into account the resistor dividor
 * network on the cards. However it is accurate enough, and since we are trying to be backwards
 * compatible this attribute is added, but this time with a more accurate calculation of the
 * voltage percentage.  This attribute is only for the first A2D which is the twelve volt
 * input present on all cards.
 *
 * @param[in]  card             Pointer / handle to the card that we are talking to.
 * @param[in]  component        Pointer / handle to the component that we are talking to.
 * @param[in]  sensor_p         Pointer to the structure describing details about the sensor
 *                              being used.
 * @param[in]  multiplier       The multiplier to apply to the value read from the register.
 *
 */
static void
_lm_sensor_add_legacy_lm93_voltage_attr(DagCardPtr card, ComponentPtr component, lm_sens_details_t *sensor_p, float multiplier)
{
	AttributePtr    attr = NULL;
	voltage_attr_t *attr_state_p = NULL;

    
	attr = attribute_init(kUint32AttributeVoltage);
	if ( attr == NULL )
		return;
	
	
	/* Set the name and description of the attribute */
	attribute_set_name(attr, "voltage");
	attribute_set_description(attr, "Voltage.");
		
	
	/* Set the standard settings */
	attribute_set_config_status (attr, kDagAttrStatus);
	attribute_set_valuetype (attr, kAttributeUint32);
	attribute_set_to_string_routine (attr, attribute_uint32_to_string);

	
	/* Set the private state, which is just the address details and the mulitplier */
	attr_state_p = (voltage_attr_t*) malloc(sizeof(voltage_attr_t));
	memset (attr_state_p, 0x00, sizeof(voltage_attr_t));
	
	attr_state_p->address = ((LM93_AD_IN1) | (sensor_p->i2c_addr << 8) | (sensor_p->enum_idx << 16));
	attr_state_p->multiplier = multiplier;
	attr_state_p->raw_access = sensor_p->raw_access;
	
	attribute_set_private_state (attr, attr_state_p);


	/* Install the "get" callback handler for the attribute */
	attribute_set_getvalue_routine (attr, _lm_sensor_voltage_attr_get_percentage);
	attribute_set_dispose_routine (attr, _lm_sensor_voltage_attr_dispose);


	/* Finally add the attribute */
	component_add_attribute(component, attr);
}




