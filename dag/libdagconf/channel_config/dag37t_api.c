/*
 * Copyright (c) 2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *  $Id: dag37t_api.c,v 1.6 2009/06/18 00:06:50 vladimir Exp $
 */




#include "dag_platform.h"
#include "dagutil.h"
#include "dagapi.h"



#include "../include/channel_config/dag37t_chan_cfg.h"
#include "../include/channel_config/dag37t_map_functions.h"

extern const int phy_to_logical_line[16];

/* This file contains all of the dag 3.7T api functions which do not fit 
 * nicely into the add channel or delete channel category  
 */

uint32_t dag_set_mux(int dagfd, 
					 uint32_t host_mode, 
					 uint32_t line_mode, 
					 uint32_t iop_mode)
{
	uint8_t* dagiom = dag_iom(dagfd);

	iom_write(dagiom, MUX_HOST, 0xFFFFFFFF, host_mode);
	iom_write(dagiom, MUX_LINE, 0xFFFFFFFF, line_mode);
	iom_write(dagiom, MUX_IOP,  0xFFFFFFFF, iop_mode);


	return 0;
} /* dag_set_mux */





int dag_ifc_hec (int dagfd, 
				 uint32_t iface, 
				 uint32_t option)
{

	modules_t module, module1, module2;
	uint8_t* dagiom = dag_iom(dagfd);
	uint32_t line = 0;
	uint32_t bit_val = 0;
	daginf_t*       daginf;

        daginf = dag_info(dagfd);
	if(daginf->device_code == PCI_DEVICE_ID_DAG3_7T4)
        {
        }
        else if(daginf->device_code == PCI_DEVICE_ID_DAG3_70T)
        {}
        line = phy_to_logical_line[iface];

	module1.type = UNDEFINED;
	module2.type = UNDEFINED;

	/* check the ATM_RX Module is loaded */
	get_firmware_modules(dagfd, &module1, &module2);


	if(module1.type != MOD_ATM_RX )
	{
		if( module2.type != MOD_ATM_RX )
		{
			dagutil_verbose_level(1, "ATM receive is not loaded, Header correction is not available\n");
			return -1;
		}
		else 
			module = module2;
	}
	else
		module = module1;


	if(option)
		bit_val = (1 << (16 + line));
		

	iom_write(dagiom, module.addr+ LINE_FEATURE_CTRL_REG, (1 << (16 + line)), bit_val);

	return 0;

}

int get_dag_ifc_hec (int dagfd, 
                     uint32_t iface)
{

	modules_t module, module1, module2;
	uint8_t* dagiom = dag_iom(dagfd);
	uint32_t line = 0;
	uint32_t reg_val = 0;
	uint32_t bit_val = 0;
	daginf_t*       daginf;

        daginf = dag_info(dagfd);
	if(daginf->device_code == PCI_DEVICE_ID_DAG3_7T4)
        {
        }
        else if(daginf->device_code == PCI_DEVICE_ID_DAG3_70T)
        { }

        line = phy_to_logical_line[iface];

	module1.type = UNDEFINED;
	module2.type = UNDEFINED;

	/* check the ATM_RX Module is loaded */
	get_firmware_modules(dagfd, &module1, &module2);


	if(module1.type != MOD_ATM_RX )
	{
		if( module2.type != MOD_ATM_RX )
		{
			dagutil_verbose_level(1, "ATM receive is not loaded, Header correction is not available\n");
			return -1;
		}
		else 
			module = module2;
	}
	else
		module = module1;

	reg_val = iom_read(dagiom, module.addr+ LINE_FEATURE_CTRL_REG);

	bit_val = (1 << (16 + line));
		
    if(bit_val & reg_val)
        return 1;

	return 0;

}



int dag_ifc_cell_scrambling (int dagfd, 
							 uint32_t iface, 
							 uint32_t option)
{

	modules_t module, module1, module2;
	uint8_t* dagiom = dag_iom(dagfd);
	uint32_t line = 0;
	uint32_t bit_val = 0;
	daginf_t*       daginf;

        daginf = dag_info(dagfd);
	if(daginf->device_code == PCI_DEVICE_ID_DAG3_7T4)
        {
        }
        else if(daginf->device_code == PCI_DEVICE_ID_DAG3_70T)
        {};

        line = phy_to_logical_line[iface];

	module1.type = UNDEFINED;
	module2.type = UNDEFINED;

	/* check the ATM_RX Module is loaded */
	get_firmware_modules(dagfd, &module1, &module2);

	if(module1.type != MOD_ATM_RX )
	{
		if( module2.type != MOD_ATM_RX )
		{
			dagutil_verbose_level(1, "ATM receive is not loaded, cell scrambling is not available\n");
			return -1;
		}
		else 
			module = module2;
	}
	else
		module = module1;

	if(option)
		bit_val = (1 << (line));
		

	iom_write(dagiom, module.addr+ LINE_FEATURE_CTRL_REG, (1 << (line)), bit_val);

	return 0;

}

int get_dag_ifc_cell_scrambling (int dagfd, 
                                 uint32_t iface)
{

	modules_t module, module1, module2;
	uint8_t* dagiom = dag_iom(dagfd);
	uint32_t line = 0;
	uint32_t reg_val = 0;
	uint32_t bit_val = 0;
	daginf_t*       daginf;

        daginf = dag_info(dagfd);
        #if 0 
	if(daginf->device_code == PCI_DEVICE_ID_DAG3_7T4)
        {
        }
        else if(daginf->device_code == PCI_DEVICE_ID_DAG3_70T)
        {};
	#endif 
        line = phy_to_logical_line[iface];

	module1.type = UNDEFINED;
	module2.type = UNDEFINED;

	/* check the ATM_RX Module is loaded */
	get_firmware_modules(dagfd, &module1, &module2);

	if(module1.type != MOD_ATM_RX )
	{
		if( module2.type != MOD_ATM_RX )
		{
			dagutil_verbose_level(1, "ATM receive is not loaded, cell scrambling is not available\n");
			return -1;
		}
		else 
			module = module2;
	}
	else
		module = module1;

	bit_val = (1 << (line));
		

	reg_val = iom_read(dagiom, module.addr+ LINE_FEATURE_CTRL_REG);
    

    if(reg_val & bit_val)
        return 1;

	return 0;

}


int dag_reset_37t(int dagfd)
{

	dagutil_verbose_level(1, "This function \"dag_reset_37t\" has been deprecated and will be removed\n");

	return 0;
}

void dag_channel_verbosity(uint32_t verb)
{
	dagutil_verbose_level(1, "This function \"dag_channel_verbosity\" has been deprecated and will be removed.  Please use dagutil_set_verbosity instead\n");
	dagutil_set_verbosity(verb);
}

