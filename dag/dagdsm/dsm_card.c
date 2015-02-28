/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dsm_card.c,v 1.25 2008/04/16 04:58:56 jomi Exp $
*
**********************************************************************/

/**********************************************************************
* 
* DESCRIPTION:  
*
*
* NOTES:       LUT and COLUR are used interchangably in the comments, both
*              terms refer to the lookup table in the FPGA that is used
*              to implement output expressions.
*
**********************************************************************/


/* System headers */
#include "dag_platform.h"

/* Endace headers */
#include "dagapi.h"
#include "dagutil.h"
#include "dagreg.h"
#include "dag_config.h"
#include "dag_component.h"

/* DSM headers */
#include "dagdsm.h"
#include "dsm_types.h"




static int dsm_get_dagx_interface_count(daginf_t * daginf);


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_is_dsm_supported
 
 DESCRIPTION:   Returns whether or not DSM is supported by the specified
                card.
 
 PARAMETERS:    dagfd            IN      Unix style file descriptor to
                                         the card in question.
 
 RETURNS:       0 or 1 if the card is supported, -1 if DSM is not supported
                or an error occuried. Use dagdsm_get_last_error() to 
                query whether or not an error was the cause of the failure.

 HISTORY:       
dag_component_get_subcomponent
---------------------------------------------------------------------*/
int
dagdsm_is_dsm_supported(int dagfd)
{
	daginf_t   * daginf;
	dag_reg_t    regs[DAG_REG_MAX_ENTRIES];
	uint8_t    * iom;
	uint32_t     count;
	
	dagdsm_set_last_error (0);
	
    /* get card iom */
	iom = dag_iom(dagfd);
	if ( iom == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	/* get the card info */
	daginf = dag_info(dagfd);
	if (daginf == NULL)
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}

	
	/* determine if the card is supported */
	switch (daginf->device_code)
	{
		case PCI_DEVICE_ID_DAG6_20:
        /* removed the other card specific and added generic code in default */
        break;
		
		default:
        {    
            count = dag_reg_find((char*) iom, DAG_REG_HMM, regs);
            if (count > 0 )return 1;
            count = dag_reg_find((char*) iom, DAG_REG_DSMU, regs);
            if (count > 0) return 0;
            dagdsm_set_last_error (ENOENT);
			return -1;
        }
	}

	
	/* get the enumeration table and sanity check that the DSM is actually present */
	count = dag_reg_find((char*) iom, DAG_REG_HMM, regs);
	if (count > 0)
	{
		return 1;
	}
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_get_base_reg
 
 DESCRIPTION:   Returns the base register address of the filtering
                module. Uses the emuneration table to find the correct
                address.
 
 PARAMETERS:    dagfd            IN      Unix style file descriptor to
                                         the card in question.
 
 RETURNS:       Pointer to the base address of the DSM module on the
                card. May return NULL if the card handle is invalid.

 HISTORY:       

---------------------------------------------------------------------*/
volatile uint8_t *
dsm_get_base_reg (int dagfd)
{
	dag_reg_t    regs[DAG_REG_MAX_ENTRIES];
	uint8_t    * iom;
	uint32_t     count;
	
	
	/* get the enumeration table and sanity check that the DSM is actually present */
	iom = dag_iom(dagfd);
	if ( iom == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return NULL;
	}
	
	count = dag_reg_find((char*) iom, DAG_REG_HMM, regs);
	if (count < 1)
	{
		dagdsm_set_last_error (ENOENT);
		return NULL;
	}
	
	return (iom + regs[0].addr);
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_enable_erfmux_dropper
 
 DESCRIPTION:   Enables/disables the ERF MUX drop module in the cards,
                this is required to stop the ERF MUX from stalling when
                one of the receive streams is not being read from.
 
 PARAMETERS:    dagfd            IN      Card file file descriptor
                enable           IN      A non-zero value enables the
                                         drop module, a zero value disables
                                         the drop module.
 
 RETURNS:       0 if the dropper was enabled otherwise -1 if an error occuried.

 HISTORY:       

---------------------------------------------------------------------*/
static int
dsm_enable_erfmux_dropper (int dagfd, uint32_t enable)
{
	dag_reg_t    regs[DAG_REG_MAX_ENTRIES];
	uint8_t    * iom;
	uint32_t     count;
	uint32_t     ipf_value;
	
	
	
	/* get the enumeration table and sanity check that the DSM is actually present */
	iom = dag_iom(dagfd);
	if ( iom == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	count = dag_reg_find((char*) iom, DAG_REG_STEERING, regs);
	if (count < 1)
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}	
	
	
	/* update the address to map to the IPF steering reg */	
	ipf_value  = dag_readl (iom + regs[0].addr);
	
	if ( enable == 0 )
		ipf_value &= ~BIT15;
	else
		ipf_value |=  BIT15;
	
	dag_writel (ipf_value, (iom + regs[0].addr));
	

	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_enable_drop_module
 
 DESCRIPTION:   Enables/disables the ERF MUX drop module in the cards,
                this is required to stop the ERF MUX from stalling when
                one of the receive streams is not being read from.
 
 PARAMETERS:    dagfd            IN      Card file file descriptor
                enable           IN      A non-zero value enables the
                                         drop module, a zero value disables
                                         the drop module.
 
 RETURNS:       0 if the dropper was enabled otherwise -1 if an error occuried.

 HISTORY:       

---------------------------------------------------------------------*/
static int
dsm_enable_drop_module (int dagfd, uint32_t enable)
{
	char            dagname[DAGNAME_BUFSIZE];
	int             dagstream;
	char            str_dev[48];
	dag_card_ref_t  card_ref; 
	dag_component_t root_component;
	dag_component_t dropper;
	attr_uuid_t     drop_attr;
	daginf_t *      daginf;
	
	
	/* get the card info */
	daginf = dag_info(dagfd);
	assert(daginf != NULL);
	

	/* create the filename for the dag card */
	sprintf (str_dev, "%d", daginf->id);
	if (-1 == dag_parse_name(str_dev, dagname, DAGNAME_BUFSIZE, &dagstream))
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}

	
	/* open the config API */
	card_ref = dag_config_init (dagname);
	if ( card_ref == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	
	/* get a reference to the root component */
	root_component = dag_config_get_root_component (card_ref);
	if ( root_component == NULL )
	{
		dag_config_dispose(card_ref);
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	/* get a reference to the port component */
	dropper = dag_component_get_subcomponent(root_component, kComponentPbm, 0);
	if ( dropper == NULL )
	{
		dag_config_dispose(card_ref);
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	/* get the network mode */
	drop_attr = dag_component_get_attribute_uuid (dropper, kBooleanAttributeDrop);
	if ( drop_attr == 0 )
	{
		dag_config_dispose(card_ref);
		dagdsm_set_last_error (EBADF);
		return -1;
	}

	if ( dag_config_set_boolean_attribute (card_ref, drop_attr, 1) != kDagErrNone )
	{
		dag_config_dispose(card_ref);
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	

	/* clean up */
	dag_config_dispose(card_ref);
	
	
	return 0;	
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_set_crc_size
 
 DESCRIPTION:   Writes the CRC size into the DSM CSR's. DSM requires
                the CRC size to determine where the packet ends and
                therefore how to filter it.
 
 PARAMETERS:    dagfd            IN      Card file file descriptor
                crc_size         IN      An enumation that should have one
                                         of the following values;
                                         0 = no CRC present
                                         1 = CRC16 present
                                         2 = CRC32 present
 
 RETURNS:       0 if the CRC size was set or -1 if an error occuried.

 HISTORY:       

---------------------------------------------------------------------*/
static int
dsm_set_crc_size (int dagfd, uint32_t crc_size)
{
	volatile uint8_t * base_reg;
	uint32_t           csr_value;


	/* get a pointer to the DSM base register */
	base_reg = dsm_get_base_reg(dagfd);
	if ( base_reg == NULL )
		return -1;
			
	/* write the CRC length */
	csr_value = dag_readl (base_reg + DSM_REG_CSR_OFFSET);
	dsm_drb_delay ((volatile uint32_t *)(base_reg + DSM_REG_CSR_OFFSET));
			
	csr_value &= ~(BIT27 | BIT28);
	csr_value |= (((uint32_t)crc_size & 0x03) << 27);
			
	dag_writel (csr_value, (base_reg + DSM_REG_CSR_OFFSET));
	dsm_drb_delay ((volatile uint32_t *)(base_reg + DSM_REG_CSR_OFFSET));	
	
	return 0;	
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_set_ipf_steering_control
 
 DESCRIPTION:   Configures the IPF steering control module, this should
                probably be done by the config API but without it the
                DSM will not work.
 
 PARAMETERS:    dagfd            IN      Card file file descriptor
                value            IN      The value to set, possible values:
                                           0 : All memory hole 0
                                           1 : Use the colour field
                                           2 : Use the ethernet pad field
                                           3 : Use the interface number
 
 RETURNS:       1 if the card is configured for ethernet, 0 if configured
                for sonet and -1 if an error occuried.

 HISTORY:       

---------------------------------------------------------------------*/
int
dsm_set_ipf_steering_control (int dagfd, uint32_t value)
{
	dag_reg_t    regs[DAG_REG_MAX_ENTRIES];
	uint8_t    * iom;
	uint32_t     count;
	uint32_t     ipf_value;
	
	

	
	/* get the enumeration table and sanity check that the DSM is actually present */
	iom = dag_iom(dagfd);
	if ( iom == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	count = dag_reg_find((char*) iom, DAG_REG_STEERING, regs);
	if (count < 1)
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	
	/* update the address to map to the IPF steering reg */	
	ipf_value  = dag_readl (iom + regs[0].addr);
	
	ipf_value &= ~(0x03 << 12);
	ipf_value |= ((value & 0x3) << 12);
	
	dag_writel (ipf_value, (iom + regs[0].addr));
	

	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_get_dag6_2_crc_size
 
 DESCRIPTION:   Utility function that returns the size of the CRC on a
                DAG 6.2 card.
 
 PARAMETERS:    daginf           IN      Pointer to the dag info structure.
 
 RETURNS:       Positive number indicating the size of the CRC, possible
                values are:
                  0  -  0 byte CRC (stripped)
                  1  -  2 byte CRC
                  2  -  4 byte CRC
                 -1  -  an error occuried

 HISTORY:       

---------------------------------------------------------------------*/
static int
dsm_get_dag6_2_crc_size(daginf_t * daginf)
{
	char            dagname[DAGNAME_BUFSIZE];
	int             dagstream;
	char            str_dev[48];
	dag_card_ref_t  card_ref; 
	dag_component_t root_component;
	dag_component_t port;
	attr_uuid_t     crc_attr;
	crc_t           crc_value;
	attr_uuid_t     crc_strip_attr;
	
	assert(daginf->device_code == PCI_DEVICE_ID_DAG6_20);
	

	/* create the filename for the dag card */
	sprintf (str_dev, "%d", daginf->id);
	if (-1 == dag_parse_name(str_dev, dagname, DAGNAME_BUFSIZE, &dagstream))
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}

	
	/* open the config API */
	card_ref = dag_config_init (dagname);
	if ( card_ref == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	
	/* get a reference to the root component */
	root_component = dag_config_get_root_component (card_ref);
	if ( root_component == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	/* get a reference to the port component */
	port = dag_component_get_subcomponent(root_component, kComponentPort, 0);
	if ( port == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	/* get the crc mode */
	crc_attr  = dag_component_get_attribute_uuid (port, kUint32AttributeCrcSelect);
	crc_value = dag_config_get_uint32_attribute (card_ref, crc_attr);
	
	
	/* check for crc stripping */
	crc_strip_attr = dag_component_get_attribute_uuid  (port, kBooleanAttributeCrcStrip);
	if ( dag_config_get_boolean_attribute(card_ref, crc_strip_attr) )
		crc_value = kCrcOff;
	
	
	/* clean up */
	dag_config_dispose(card_ref);
	

	/* return the number of bytes used for the crc */
	switch (crc_value)
	{
		case kCrc16:     
			return 1;
		case kCrc32:     
			return 2;
			
		case kCrcOff:
		default:
			return 0;
	}
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_prepare_card
 
 DESCRIPTION:   Sets the card configuration so that it can be used by
                the DSM module. The card settings configured by this
                function are indirectly relelated to the DSM module,
                however they are not part of the DSM module. This
                function should be called after the card is configured.
 
 PARAMETERS:    dagfd            IN      Card file file descriptor
 
 RETURNS:       0 if the card was configured, -1 if an error occured,
                call dagdsm_get_last_error to retreive the errorcode.

 HISTORY:       

 NOTES:         This function sets the following card parameters
                 - ERF MUX Drop module enabled
                 - IPF Steering module is set to use the packet color
                 - On the 6.2 the size of the CRC is copied from the 
                   config API into the DWM control register.

---------------------------------------------------------------------*/
int
dagdsm_prepare_card(int dagfd)
{
	daginf_t  * daginf;
	int         crc_size = -1;
	int         ret1,ret2;

	dagdsm_set_last_error (0);
	

	/* get the card info and confirm it is a supported card */	
	daginf = dag_info(dagfd);
	if (daginf == NULL)
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	

	/* for all cards we need to set the IP filtering steering module */
	if ( dsm_set_ipf_steering_control(dagfd, 1) == -1 )
		return -1;

	
	crc_size=2; /* all ethernet card is 32 bit crc*/
	if( daginf->device_code==PCI_DEVICE_ID_DAG6_20 )
	{
		/* if a 6.2 card, get the size of the CRC */
		if ( (crc_size = dsm_get_dag6_2_crc_size(daginf)) == -1 )
			return -1;
	}

	/* write the CRC length */
	dsm_set_crc_size (dagfd, crc_size);

	/* If we can find a ermux dropper, we enable it*/
	ret1= dsm_enable_erfmux_dropper(dagfd, 1);
	
	/* If we can find a drop module, we should also enable it*/
	
	ret2= dsm_enable_drop_module(dagfd, 1);

	if(ret1==-1||ret2==-1)
	{
		/*we don't know which drop module we hvae, so we ignore the error*/ 
		dagdsm_set_last_error(0);
	}

	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_bypass_dsm
 
 DESCRIPTION:   Sets/resets bypass mode, in bypass mode all packets skip
                the DSM module and are supplied to the host directly.
                By default when the card boots the DSM is not bypassed.
 
 PARAMETERS:    dagfd            IN      Card file file descriptor
                bypass           IN      Non-zero value to bypass the
                                         DSM module, a zero value to
                                         disable the bypass.
                
 
 RETURNS:       0 if bypass mode was set/reset, -1 if an error occured,
                call dagdsm_get_last_error to retreive the errorcode.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_bypass_dsm(int dagfd, uint32_t bypass)
{
	volatile uint8_t * base_reg;
	uint32_t           csr_value;
	
	
	/* get a pointer to the base register */
	base_reg = dsm_get_base_reg(dagfd);
	if ( base_reg == NULL )
		return -1;
	
	
	
	/* Enable or disable the DSM module */
	csr_value = dag_readl (base_reg + DSM_REG_CSR_OFFSET);
	dsm_drb_delay ((volatile uint32_t *)(base_reg + DSM_REG_CSR_OFFSET));
	
	
	if ( bypass == 0 )
		csr_value &= ~BIT31;
	else
		csr_value |=  BIT31;
	
	dag_writel (csr_value, (base_reg + DSM_REG_CSR_OFFSET));
	dsm_drb_delay ((volatile uint32_t *)(base_reg + DSM_REG_CSR_OFFSET));
	
		
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_get_port_count
 
 DESCRIPTION:   Gets the number of ports on the card
 
 PARAMETERS:    dagfd            IN      Pointer to the dag info structure.
 
 RETURNS:       A positive number indicates the number of ports available.
                -1 is returned if an error occuried, call dagdsm_get_last_error()
                to get the error code.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_get_port_count (int dagfd)
{
	daginf_t         *daginf;
	
	
	dagdsm_set_last_error (0);
	
	/* get the card info and confirm it is a supported card */	
	daginf = dag_info(dagfd);
	if (daginf == NULL)
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}


	/* Use the card type to determine the number of devices */
	switch (daginf->device_code)
	{
		case PCI_DEVICE_ID_DAG6_20:
			return 1;

		case PCI_DEVICE_ID_DAG5_20E:
			return 1;

		case PCI_DEVICE_ID_DAG8_20E:
		case PCI_DEVICE_ID_DAG8_20Z:
			return 1;
		
		default:
        {
       /*  Removing card specific 'cases' and adding generic code in 'default' *
         *  Using config and status api to find the interface_count        *
        */
            return dsm_get_dagx_interface_count(daginf);
        }
	}
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_get_dsm_version
 
 DESCRIPTION:   return version of DSM
 
 PARAMETERS:    dagfd            IN      Pointer to the dag info structure.
 
 RETURNS:       DSM_VERSION0 
				DSM_VERSION1 
 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_get_dsm_version (int dagfd)
{
	dag_reg_t    regs[DAG_REG_MAX_ENTRIES];
	uint8_t    * iom;
	uint32_t     count;
	
	dagdsm_set_last_error (0);
	/* get the enumeration table and sanity check that the DSM is actually present */
	iom = dag_iom(dagfd);
	if ( iom == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}	
	count = dag_reg_find((char*) iom, DAG_REG_HMM, regs);
	if (count < 1)
	{
		dagdsm_set_last_error (ENOENT);
		return -1;
	}
	
	return regs[0].version;	
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_is_dag6_2_ethernet
 
 DESCRIPTION:   Utility function that uses the config API to query 
                the type of the DAG 6.2 card. It is assumed that the
                card is checked to make sure it is a 6.2 prior to being
                called.
 
 PARAMETERS:    daginf           IN      Pointer to the dag info structure.
 
 RETURNS:       1 if the card is configured for ethernet, 0 if configured
                for sonet and -1 if an error occuried.

 HISTORY:       

---------------------------------------------------------------------*/
static int
dsm_is_dag6_2_ethernet(daginf_t * daginf)
{
	char            dagname[DAGNAME_BUFSIZE];
	int             dagstream;
	char            str_dev[48];
	dag_card_ref_t  card_ref; 
	dag_component_t root_component;
	dag_component_t port;
	attr_uuid_t     network_attr;
	network_mode_t  net_mode;
	
	assert(daginf->device_code == PCI_DEVICE_ID_DAG6_20);
	

	/* create the filename for the dag card */
	sprintf (str_dev, "%d", daginf->id);
	if (-1 == dag_parse_name(str_dev, dagname, DAGNAME_BUFSIZE, &dagstream))
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}

	
	/* open the config API */
	card_ref = dag_config_init (dagname);
	if ( card_ref == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	
	/* get a reference to the root component */
	root_component = dag_config_get_root_component (card_ref);
	if ( root_component == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	/* get a reference to the port component */
	port = dag_component_get_subcomponent(root_component, kComponentPort, 0);
	if ( port == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	/* get the network mode */
	network_attr = dag_component_get_attribute_uuid (port, kUint32AttributeNetworkMode);
	net_mode = dag_config_get_uint32_attribute (card_ref, network_attr);
	

	/* clean up */
	dag_config_dispose(card_ref);
	
	
	if ( net_mode == kNetworkModeEth )
		return 1;
	else
		return 0;
}



static int
dsm_is_dagx_sonet(daginf_t * daginf)
{
	char            dagname[DAGNAME_BUFSIZE];
	int             dagstream;
	char            str_dev[48];
	dag_card_ref_t  card_ref; 
	dag_component_t root_component;
	dag_component_t port;
	
	assert(daginf->device_code != PCI_DEVICE_ID_DAG6_20);
	

	/* create the filename for the dag card */
	sprintf (str_dev, "%d", daginf->id);
	if (-1 == dag_parse_name(str_dev, dagname, DAGNAME_BUFSIZE, &dagstream))
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}

	
	/* open the config API */
	card_ref = dag_config_init (dagname);
	if ( card_ref == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	
	/* get a reference to the root component */
	root_component = dag_config_get_root_component (card_ref);
	if ( root_component == NULL )
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	/* get a reference to the Sonet component 
	* if the component does exist we assume that the card is in sonet mode
	* the assumption will brake in the future if we have both sonet and ethernet FW componets on a single FW image
	* it should work for all new cards 5.X 8.X
	*/
	port = dag_component_get_subcomponent(root_component, kComponentSonetPP, 0);
	if ( port != NULL )
	{
		/* clean up */
		dag_config_dispose(card_ref);
		return 1;
	} else {
	
	/* clean up */
	dag_config_dispose(card_ref);
	return 0;
	}

	return 0;
	
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_is_card_ethernet
 
 DESCRIPTION:   Return value indicates whether the card is ethernet or
                not. The supplied card indentifier must be to a card
                that supports DSM filtering.
 
 PARAMETERS:    dagfd            IN      Unix style file descriptor to
                                         the card in question.
 
 RETURNS:       1 if the card is ethernet, 0 if not ethernet and -1 if
                there was an error. This function updates the last error
                code using dagdsm_set_last_error().

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_is_card_ethernet(int dagfd)
{
	daginf_t         *daginf;
	int res;
	
	dagdsm_set_last_error (0);
	
	
	/* get the card info and confirm it is a supported card */	
	daginf = dag_info(dagfd);
	if (daginf == NULL)
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	
	/* determine what sort of card we have */
	switch (daginf->device_code)
	{
		case PCI_DEVICE_ID_DAG6_20:
			return dsm_is_dag6_2_ethernet(daginf);
			
		case PCI_DEVICE_ID_DAG4_52E:
		case PCI_DEVICE_ID_DAG4_52F:
		case PCI_DEVICE_ID_DAG4_5CF:
		case PCI_DEVICE_ID_DAG4_52Z:
		case PCI_DEVICE_ID_DAG4_52Z8:
			return 1;
		
		case PCI_DEVICE_ID_DAG4_54E:
			return 1;
		
		/*case PCI_DEVICE_ID_DAG5_20E:
			return 1; */
		
		case PCI_DEVICE_ID_DAG8_20E:
		case PCI_DEVICE_ID_DAG8_20Z:
			return 1;
		
		
		default:
            res = dsm_is_dagx_sonet(daginf);
			if ( res == 1 )       res = 0;
			else if ( res == 0 )  res = 1;
			if( res < 0 )
				dagdsm_set_last_error (ENOENT);
			return res;
			/*dagdsm_set_last_error (ENOENT);
			return -1;*/
	}
	
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_is_card_sonet
 
 DESCRIPTION:   Return value indicates whether the card is sonet or
                not.
 
 PARAMETERS:    dagfd            IN      Unix style file descriptor to
                                         the card in question.
 
 RETURNS:       1 if the card is sonet, 0 if not sonet and -1 if
                there was an error. This function updates the last error
                code using dagdsm_set_last_error().

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_is_card_sonet(int dagfd)
{
	daginf_t         *daginf;
	int              res;
	
	dagdsm_set_last_error (0);
	
	
	/* get the card info and confirm it is a supported card */	
	daginf = dag_info(dagfd);
	if (daginf == NULL)
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	
	
	/* determine what sort of card we have */
	switch (daginf->device_code)
	{
		case PCI_DEVICE_ID_DAG6_20:
			res = dsm_is_dag6_2_ethernet(daginf);
			if ( res == 1 )       res = 0;
			else if ( res == 0 )  res = 1;
			return res;
			
		case PCI_DEVICE_ID_DAG4_52E:
		case PCI_DEVICE_ID_DAG4_52F:
		case PCI_DEVICE_ID_DAG4_5CF:
		case PCI_DEVICE_ID_DAG4_52Z:
        case PCI_DEVICE_ID_DAG4_54E:
		case PCI_DEVICE_ID_DAG4_52Z8:
			return 0;
		
		case PCI_DEVICE_ID_DAG8_20E:
		case PCI_DEVICE_ID_DAG8_20Z:
			return 0;
		
		default:
            res = dsm_is_dagx_sonet(daginf);
			if ( res < 0 )
 				dagdsm_set_last_error (ENOENT);
			return res;
			/*dagdsm_set_last_error (ENOENT);
			return -1;*/
	}
	
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_is_filter_active
 
 DESCRIPTION:   Checks if a particular physical filter is active or not.
 
 PARAMETERS:    dagfd            IN      Unix style file descriptor to
                                         the card in question.
                filter           IN      The physical number of the
                                         filter.
 
 RETURNS:       1 if the filter is active, 0 if not active and -1 if
                there was an error. This function updates the last error
                code using dagdsm_set_last_error().

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_is_filter_active(int dagfd, uint32_t filter)
{
	volatile uint8_t * base_addr;
	
	dagdsm_set_last_error (0);
	
	
	/* sanity check the filter number */
	if ( filter >= FILTER_COUNT )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* get the base address of the dsm registers */
	if ( (base_addr = dsm_get_base_reg(dagfd)) == NULL )
		return -1;
	
	
	/* check if enabled (which is the same as active) */
	return dsm_is_filter_enabled(base_addr, filter);
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_activate_filter
 
 DESCRIPTION:   Activates a particular filter on the card
 
 PARAMETERS:    dagfd            IN      Unix style file descriptor to
                                         the card in question.
                filter           IN      The physical number of the
                                         filter.
                activate         IN      Non-zero value to activate and
                                         a zero value to deactivate.
 
 RETURNS:       0 if the filter was activated/deactivated, -1 if there was
                an error. This function updates the last error code using 
                dagdsm_set_last_error().

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_activate_filter(int dagfd, uint32_t filter, uint32_t activate)
{
	volatile uint8_t * base_addr;
	
	dagdsm_set_last_error (0);

	
	/* sanity check the filter number */
	if ( filter >= FILTER_COUNT )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* get the base address of the dsm registers */
	if ( (base_addr = dsm_get_base_reg(dagfd)) == NULL )
		return -1;
	
	
	/* check if enabled (which is the same as active) */
	return dsm_enable_filter(base_addr, filter, activate);
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_load_filter
 
 DESCRIPTION:   Loads a filter into the card, the card 
 
 PARAMETERS:    dagfd            IN      Unix style file descriptor to
                                         the card in question.
                filter           IN      The number of the filter to load
                                         value into. Warning this is the
                                         hardware filter number not the
                                         logical filter number as used
                                         by the dagdsm_filter_??? functions.
                term_depth       IN      Defines the first element to set
                                         early termination flag, if early
                                         termination is not required set
                                         this value to DSM_NO_EARLYTERM.
                value            IN      Array of bytes that contain the
                                         comparand of the filter.
                mask             IN      Array of bytes that contain the
                                         mask of the filter.
                size             IN      The size of the mask and value
                                         array in bytes.

 
 RETURNS:       0 if the filter was loaded, otherwise -1 if an error occurred.
                Use dagdsm_get_last_error() to retreive the error code.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_load_filter(int dagfd, uint32_t filter, uint32_t term_depth, const uint8_t * value, const uint8_t * mask, uint32_t size)
{
	volatile uint8_t * base_addr;
	uint8_t            actual_value[MATCH_DEPTH];
	uint8_t            actual_mask[MATCH_DEPTH];
	uint32_t           count;
	int                enabled;
	int                res;
	
	
	dagdsm_set_last_error (0);
	
	
	/* sanity check the filter number */
	if ( filter >= FILTER_COUNT )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ((size > 0) && (value == NULL || mask == NULL))
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* copy to a temporary array to ensure we have at least 64 bytes, because
	 * the lower level functions don't check the length of the buffers. 	*/
	count = (size < MATCH_DEPTH) ? size : MATCH_DEPTH;
	
	memset (actual_value, 0, MATCH_DEPTH);
	memcpy (actual_value, value, count);

	memset (actual_mask, 0, MATCH_DEPTH);
	memcpy (actual_mask, mask, count);

	
	
	/* get the base address of the dsm registers */
	if ( (base_addr = dsm_get_base_reg(dagfd)) == NULL )
		return -1;
	
	
	/* if the filter is currently enabled, disable, download and then enable again */
	if ( (enabled = dsm_is_filter_enabled(base_addr, filter)) == -1 )
		return -1;
	
	if ( enabled )
		dsm_enable_filter (base_addr, filter, 0);
	
	/* write the filter into the FPGA */
	res = dsm_write_filter(base_addr, filter, term_depth, actual_value, actual_mask);	
	
	if ( enabled )
		dsm_enable_filter (base_addr, filter, 1);

	return res;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_get_filter_stream_count
 
 DESCRIPTION:   Returns the number of possible receive streams.
 
 PARAMETERS:    dagfd            IN      Unix style file descriptor to
                                         the card in question.
 
 RETURNS:       The number of receive streams supported by the card in
                it's current configuration.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_get_filter_stream_count(int dagfd)
{
	int       streams;
	daginf_t *daginf;
	


	/* sanity check the card supports DSM */ 
	dagdsm_set_last_error (0);
	if ( dagdsm_is_dsm_supported(dagfd) != 1 )
	{
		dagdsm_set_last_error (ENOENT);
		return -1;
	}
	

	/* get the card info and confirm it is a supported card */	
	daginf = dag_info(dagfd);
	if (daginf == NULL)
	{
		dagdsm_set_last_error (EBADF);
		return -1;
	}
	

	/* todo: this should be removed soon and the config structure updated */
	streams = dag_rx_get_stream_count (dagfd);

	
	/* determine what sort of card we have */
	if (daginf->device_code == PCI_DEVICE_ID_DAG8_20E)
	{
		char second_core[32];
		int  dagstream;
		char dagname[DAGNAME_BUFSIZE];
		int  sec_dagfd;
		
		snprintf(second_core, 32, "dag%d", (int)(daginf->id + 1));
		if (-1 != dag_parse_name(second_core, dagname, DAGNAME_BUFSIZE, &dagstream) )
		{
			sec_dagfd = dag_open(dagname);
			if ( sec_dagfd != -1 )
			{
				daginf = dag_info(sec_dagfd);
				if (daginf != NULL && daginf->device_code == PCI_DEVICE_ID_DAG8_20F)
				{
					streams += dag_rx_get_stream_count (dagfd);
				}
				
				dag_close(sec_dagfd);
			}
		}
	}
	

	assert(streams<=STREAM_COUNT);

	return streams;
}

int dsm_get_dagx_interface_count(daginf_t *daginf)
{
    dag_component_t gpp;
    int gpp_iface_count = 0;
    attr_uuid_t any_attr ;
    char            dagname[DAGNAME_BUFSIZE];
    int             dagstream;
    char            str_dev[48];
    dag_card_ref_t  card_ref; 
    dag_component_t root_component;
    
    /* get the filename frm daginf */
    sprintf (str_dev, "%d", daginf->id);
    if (-1 == dag_parse_name(str_dev, dagname, DAGNAME_BUFSIZE, &dagstream))
    {
        dagdsm_set_last_error (EBADF);
        return -1;
    }
    
    
    /* open the config API */
    card_ref = dag_config_init (dagname);
    if ( card_ref == NULL )
    {
        dagdsm_set_last_error (EBADF);
        return -1;
    }
    
    
    /* get a reference to the root component */
    root_component = dag_config_get_root_component (card_ref);
    if (NULL == root_component )
    {
        dagdsm_set_last_error (EBADF);
        return -1;
    }
    gpp = dag_component_get_subcomponent(root_component, kComponentGpp,0);
    if ( NULL == gpp ) return -1;
    any_attr = dag_component_get_attribute_uuid(gpp, kUint32AttributeInterfaceCount);
    if ( kNullAttributeUuid ==  any_attr) return -1;
    gpp_iface_count = dag_config_get_uint32_attribute(card_ref, any_attr);
    return gpp_iface_count;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_get_active_lut
 
 DESCRIPTION:   Returns the number of the active COLUR, there are only
                2 banks of the COLUR therefore the active COLUR can be
                either 0 or 1.
 
 PARAMETERS:    dagfd            IN      Unix style file descriptor to
                                         the card in question.
 
 RETURNS:       The active bank of the COLUR 0 or 1, -1 may be returned
                if an error occurs. This function updates the last error
                code using dagdsm_set_last_error().

 HISTORY:       

---------------------------------------------------------------------*/
#if 0   /* Deprecated */
int
dagdsm_get_active_lut(int dagfd)
{
	volatile uint8_t * base_addr;
	
	dagdsm_set_last_error (0);

	
	/* get the base address of the dsm registers */
	if ( (base_addr = dsm_get_base_reg(dagfd)) == NULL )
		return -1;
	
	
	/* check if enabled (which is the same as active) */
	switch  ( dsm_get_inactive_colur_bank(base_addr) )
	{
		case 0:   return 1;	
		case 1:   return 0;	
		default:  return -1;
	}
}
#endif	


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_set_active_lut
 
 DESCRIPTION:   Sets the active lut
 
 PARAMETERS:    dagfd            IN      Unix style file descriptor to
                                         the card in question.
                lut              IN      The number of the lut to make
                                         the active one, this should be
                                         either 0 or 1.
 
 RETURNS:       If the lut was made active 0 is returned otherwise -1, 
                this function updates the last error code using
                dagdsm_set_last_error().

 HISTORY:       

---------------------------------------------------------------------*/
#if 0   /* Deprecated */
int
dagdsm_set_active_lut(int dagfd, uint32_t lut)
{
	volatile uint8_t * base_addr;
	
	dagdsm_set_last_error (0);

	
	/* sanity check the filter number */
	if ( lut != 0 && lut != 1 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		
	
	/* get the base address of the dsm registers */
	if ( (base_addr = dsm_get_base_reg(dagfd)) == NULL )
		return -1;
	
	
	/* check if enabled (which is the same as active) */
	dsm_set_active_colur_bank(base_addr, lut);
	
	return 0;
}
#endif	

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_load_lut
 
 DESCRIPTION:   Loads the supplied data into the COLUR, you should always
                load into the inactive COLUR (use dagdsm_get_active_lut
                to determine which is the inactive COLUR).
 
 PARAMETERS:    dagfd            IN      Unix style file descriptor to
                                         the card in question.
                lut              IN      The lut to read from.
                data_p           OUT     Buffer to store the table in
                size             IN      The number of bytes to read from
                                         the table.
 
 RETURNS:       If the lut was read 0 is returned otherwise -1, 
                this function updates the last error code using
                dagdsm_set_last_error().

 HISTORY:       

---------------------------------------------------------------------*/
#if 0   /* Deprecated */
int
dagdsm_load_lut(int dagfd, uint32_t lut, uint8_t * data_p, uint32_t size)
{
	volatile uint8_t * base_addr;
	
	dagdsm_set_last_error (0);
	
	
	/* sanity check the filter number */
	if ( lut != 0 && lut != 1 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		
	
	/* get the base address of the dsm registers */
	if ( (base_addr = dsm_get_base_reg(dagfd)) == NULL )
		return -1;
	
	
	/* check if enabled (which is the same as active) */
	return dsm_read_colur (base_addr, lut, (uint16_t*)data_p, (size/2));
}
#endif
