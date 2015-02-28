/*
 * Copyright (c) 2005-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* File header. */
#include "include/card.h"

/* Endace headers. */
#include "dagapi.h"
#include "dagutil.h"
#include "dag_romutil.h"
#include "dagimg.h"
#include "dagname.h"
#include "dag_platform.h"

/* Internal project headers. */
#include "include/cards/card_initialization.h"
#include "include/component.h"
#include "include/util/utility.h"
#include "include/cards/dag3_constants.h"
#include "include/component.h"
#include "include/util/enum_string_table.h"


/* Chosen randomly. */
#define CARD_MAGIC 0x493e56f3
#define	XBUFSIZE 128
#define TIMEOUT_MAX 10000

#ifdef _WIN32
#define IMAGE_READ_FLAGS "rb"
#else  
#define IMAGE_READ_FLAGS "r"
#endif

 
/* Software implementation of a DAG card. */
struct DagCardObject
{
	/* Standard protection. */
	uint32_t mMagicNumber;
	uint32_t mChecksum;

	/* Members. */
	ComponentPtr mRootComponent;
	int mMode;
	int mFd;
	dag_err_t mLastError;
	dag_card_t mCardType;
	daginf_t* mInfo;
	dag_reg_t* mRegisters;
	volatile uint8_t* mIoMemory;
	void* mPrivateState; /* Storage for specific implementations to use as they please. */
    int mRxStreamCount;
    int mTxStreamCount;
    int mWriteBackSWID;
    uint8_t* mSWID;
    uint32_t mSWIDKey;
    char* mConfigurationString;
	
	/* Methods. */
	CardDisposeRoutine fDispose;
	CardPostInitializeRoutine fPostInitialize;
	CardResetRoutine fReset;
	CardDefaultRoutine fDefault;
	CardLoadFirmwareRoutine fLoadFirmware;
   CardLoadPPImageRoutine fLoadPPImage;
   //this can be used together with fLoadFirmware insteda as a seperated function 
   CardLoadEmbeddedRoutine fLoadEmbedded;
   
   CardUpdateRegisterBaseRoutine fUpdateRegisterBaseRoutine;
};


#ifndef MAX
#define MAX(X,Y) ((X)>(Y))?(X):(Y)
#endif


#if 0
typedef struct pbm_offsets
{
	uint32_t globalstatus;  // Global status
	uint32_t streambase;    // Offset of first stream
	uint32_t streamsize;    // Size of each stream
	uint32_t streamstatus;  // Control / Status
	uint32_t mem_addr;      // Mem hole base address
	uint32_t mem_size;      // Mem hole size
	uint32_t record_ptr;    // Record pointer
	uint32_t limit_ptr;     // Limit pointer
	uint32_t safetynet_cnt; // At limit event pointer
	uint32_t drop_cnt;      // Drop counter

} pbm_offsets_t;



static pbm_offsets_t PBM[3] = {
	/* PBM version 0. */
	{
		/*globalstatus:*/   0x1c,
		/*streambase:*/     0x0,
		/*streamsize:*/     0x60,
		/*streamstatus:*/   0x1c,
		/*mem_addr:*/       0x0,
		/*mem_size:*/       0x04,
		/*record_ptr:*/     0x18,
		/*limit_ptr:*/      0x10,
		/*safetynet_cnt:*/  0x14,
		/*drop_cnt:*/       0x0c,
	},
	
	/* PBM version 1. */
	{
		/*globalstatus:*/   0x0,
		/*streambase:*/     0x40,
		/*streamsize:*/     0x40,
		/*streamstatus:*/   0x0,
		/*mem_addr:*/       0x04,
		/*mem_size:*/       0x08,
		/*record_ptr:*/     0x0c,
		/*limit_ptr:*/      0x10,
		/*safetynet_cnt:*/  0x14,
		/*drop_cnt:*/       0x18,
	},
	{ //pbm v2 CSBM
	/*	globalstatus:*/		0x0,
	/*	streambase:*/		0x40,
	/*	streamsize:*/		0x40,
	/*	streamstatus:*/		0x30,
	/*	mem_addr:*/			0x00,
	/*	mem_size:*/			0x08,
	/*	record_ptr:*/		0x18,
	/*	limit_ptr:*/		0x20,
	/*	safetynet_cnt:*/	0x28,
	/*	drop_cnt:*/			0x80000000,	/* Not present in pbm v2!!! */	
	}
};
#endif

/* Internal routines. */
static uint32_t compute_card_checksum(DagCardPtr card);
static uint8_t is_pbm_existent_stream(DagCardPtr card, uint32_t stream_number);
static  int get_previous_valid_stream(DagCardPtr card, uint32_t stream_number);

	/* Default method implementations. */
static void generic_card_dispose(DagCardPtr card);
static int generic_card_post_initialize(DagCardPtr card);
static void generic_card_reset(DagCardPtr card);
static void generic_card_default(DagCardPtr card);
static dag_err_t generic_card_load_firmware(DagCardPtr card, uint8_t* image);
static dag_err_t generic_card_load_embedded(DagCardPtr card, uint8_t* image, int target_processor_region);


/* Implementation of internal routines. */
static uint32_t
compute_card_checksum(DagCardPtr card)
{
	/* Don't check validity here because the card may not be fully constructed yet. */
	if (card && card->mRootComponent)
		return compute_checksum(&card->mRootComponent, sizeof(*card) - 2 * sizeof(uint32_t));
	return 0;
}

static void
generic_card_dispose(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
    }
}


static int
generic_card_post_initialize(DagCardPtr card)
{
	/* Specific component implementations can override this to handle their own post initialisation. */
	if (1 == valid_card(card))
	{
        return 1;
	}
	
	return 0;
}


static void
generic_card_reset(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
	}
}


static void
generic_card_default(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
        card_default(card);
	}
}


static dag_err_t
generic_card_load_firmware(DagCardPtr card, uint8_t* image)
{
	assert(NULL != image);
	if (1 == valid_card(card))
	{
        romtab_t* rt = NULL;
        int fd;
        int saddr, eaddr;
        uint32_t ibuflen;
        uint8_t* ibuf;

		fd = card_get_fd(card);
		//CHECKME: the hold bit is always on for the PBI it should be used on 37t 
		rt = dagrom_init(fd, 0, 1); 
		/* program to the current half */
		saddr = rt->bstart;
		eaddr = saddr + rt->bsize;
//		ibuflen = rt->bsize;
		//open image file
		ibuf = dagutil_readfile((char *)image, IMAGE_READ_FLAGS, &ibuflen);
        
		if (ibuf == NULL)
		{
			free(ibuf);
			dagrom_free(rt);
			return kDagErrFirmwareLoadFailed;
		};
		
		//cheks if the file is not bigger then the bottum size of rom (Current version)
		if (ibuflen > rt->bsize )
		{
			//return error the size of the image is bigger then possible to load 
			free(ibuf);
			dagrom_free(rt);
			return kDagErrFirmwareLoadFailed;
		};
		
		//FIXME: check for the correct image for firmware upload 
		//img_idx = dag_get_img_idx(daginfo->device_code, rom_number);
		//dag_check_img_ptr(img_idx, -1, (char*) uInBuffer, uInBufferLength, daginfo->brd_rev) 
		//verify that the proper image is loaded into the card
		//if (rp->mpu_id == DAG_MPU_NONE)
		//{
		//	dag_check_img_ptr(img_idx, -1, (char*) uInBuffer, uInBufferLength, daginfo->brd_rev) 
		//	check_image(rom_number, daginfo);
		//};
		
		card_erase_rom(card, rt, saddr, eaddr);
		if (0 != dagrom_program(rt, ibuf, ibuflen, saddr, eaddr))
		{
			free(ibuf);
			dagrom_free(rt);
			return kDagErrFirmwareLoadFailed;
		};
		/* load the current half */
		dagrom_loadcurrent(fd, rt);
		if (1 == card->mWriteBackSWID)
		{
			dagswid_write(rt, card->mSWID, DAGSERIAL_SIZE, card->mSWIDKey);
			card->mWriteBackSWID = 0;
		};
		dagrom_free(rt);
		free(ibuf);
	} else {
		return kDagErrFirmwareLoadFailed;
	};
	return kDagErrNone;
};


static dag_err_t
generic_card_load_embedded(DagCardPtr card, uint8_t* image, int target_processor_region)
{
	assert(NULL != image);
	if (1 == valid_card(card))
	{
		unsigned int uHoldPbiBus = 0;
		romtab_t* rp = NULL;
		daginf_t *daginfo;
		int dagfd;
		int rom_number = 0;
		int start_address, end_address;
		uint32_t ibuflen,regionlen, obuflen;
		uint8_t* ibuf = NULL;
		uint8_t* obuf = NULL;

		dagfd = card_get_fd(card);
		daginfo = card_get_info(card);
		
		// If the card is a 3.7ti/3.7t4 hold the bus 
		if ( daginfo->device_code == PCI_DEVICE_ID_DAG3_70T || 
		     daginfo->device_code == PCI_DEVICE_ID_DAG3_7T4 )
			uHoldPbiBus = 1;
		
		
		rom_number = 0;
		rp = dagrom_init(dagfd, rom_number, uHoldPbiBus);
		//check for Embedded presens 
		if (rp->mpu_id == DAG_MPU_NONE)        
		{
			// not Embedded rom presented (MPU)
			return kDagErrFirmwareLoadFailed;
		};
		
		//switch to propper rom 
		if (rp->mpu_rom != rom_number)
		{
//			dagutil_verbose("Switching to rom number %d for processor write\n", rp->mpu_rom);
			rom_number = rp->mpu_rom;
			dagrom_free(rp);
			rp = dagrom_init(dagfd, rom_number, uHoldPbiBus);
		}


		//check if the rom is know 
		if (rp->romid == 0x0000)
		{
			if (1 == uHoldPbiBus)
			{
				dagrom_pbirelease(rp);
			};
			//dagutil_panic("0x%.4x unknown flash ROM type\n", rom_number);
			return kDagErrFirmwareLoadFailed;
		};

		/* Does this flash have a processor section defined? 
			CHECK ME: this might be duplication with DAG_MPU_NONE
			or more likly card with CPU with out memory for it 
		*/
		if (DAGROM_XSIZE == 0)
		{
			if (1 == uHoldPbiBus)
				dagrom_pbirelease(rp);
			return kDagErrFirmwareLoadFailed;
//			dagutil_panic("error -c option not supported by board\n");
		};

		
		start_address = DAGROM_XSTART + dagrom_procstart(rp->mpu_id, target_processor_region);
		end_address = start_address + dagrom_procsize(rp->mpu_id, target_processor_region);
		regionlen = dagrom_procsize(rp->mpu_id, target_processor_region);

		ibuf = dagutil_readfile((char *)image, IMAGE_READ_FLAGS, &ibuflen);
		if (ibuf == NULL || ibuflen == 0)
		{
			return kDagErrInvalidParameter;
		}

		/* allocates memmory for verify before write to optimize the flash rom usage and to shorten the time 
		in case no new image has been loaded */;
		
		obuf = malloc(regionlen); 
		if( obuf == NULL ) {
			return kDagErrInvalidParameter;
		}
		obuflen = ibuflen; 

		//check if the image is smaller then the space for the region 
		if (ibuflen > regionlen)
		{
			return kDagErrInvalidParameter;
		}

		/* Checks if the contents of the Flash is identical 
		/ If it is identical will return success with out erasing and programing again 
		/
		*/
			if( dagrom_verify(rp,  ibuf,  ibuflen,  obuf, obuflen, start_address, end_address) == 0 )
			{ 
				// clean the memory from the buffers and from the rom structure
				dagutil_verbose("the contents of the flash is identical\n");
				free(obuf);
				free(ibuf);
				dagrom_free(rp);
				return kDagErrNone;
			} 
		/* continue with the reprogramming */
		free(obuf);
		/* source image okay, check size of source image fits into target ROM, if so, then erase and write */
		card_erase_rom(card, rp, start_address, end_address);
//		dagrom_safe_erase(rp, start_address, end_address);
		if ( dagrom_program(rp, ibuf, ibuflen, start_address, end_address) == -1 ) 
		{
			//error in programming 
			return kDagErrFirmwareLoadFailed;		
		};
		
		if (1 == card->mWriteBackSWID)
		{
			dagswid_write(rp, card->mSWID, DAGSERIAL_SIZE, card->mSWIDKey);
			card->mWriteBackSWID = 0;
		}
		dagrom_free(rp);
		free(ibuf);
		
	} else 
		return kDagErrFirmwareLoadFailed;
	
	return kDagErrNone;
}




static dag_err_t
generic_card_load_pp_image(DagCardPtr card, const char* filename, int which_pp)
{
	int img_idx;
	int seenerr = 0;
	uint32_t len;
	char *ntype = NULL;
	char *nfunc;
	daginf_t* daginf = NULL;
	int img_offset = 0;
	int copro_id = -1; /* For now ignore copros */
	int fpga_stat_cmd;
	int fpga_config_data;
	unsigned fpgap_base = 0;
	uint8_t *bp;
	uint8_t xnew[XBUFSIZE];
	char *av[5];
    uint32_t reg_val;
	dag_reg_t result[DAG_REG_MAX_ENTRIES];
    uint32_t	coprodet_base;
    char* iom = NULL;
    	

    if (card == NULL || filename == NULL)
    {
        return kDagErrInvalidParameter;
    }

	fpgap_base = card_get_register_address(card, DAG_REG_FPGAP, 0);
	
	fpga_stat_cmd = fpgap_base + 8*which_pp;
	fpga_config_data = fpgap_base+ 8*which_pp+ 4;

	daginf = card_get_info(card);
	if (daginf == NULL)
	{
		return kDagErrInvalidParameter;
	}
			
	bp = dagutil_readfile((char*)filename, IMAGE_READ_FLAGS, &len);
	if (bp == NULL || len == 0)
	{
		return kDagErrInvalidParameter;
	}
	
	memcpy(xnew, bp, XBUFSIZE);

	ntype = dag_xrev_parse(xnew, XBUFSIZE, av)[1];
	nfunc = av[0];

    iom = (char*) card_get_iom_address(card);

	
	img_offset = dag_reg_find( iom, DAG_REG_ROM, result);
	if (img_offset < 0)
		return kDagErrFirmwareLoadFailed;
    /* img_offset has to be 0 ( No DAG_REG_ROM present ) or 1 ( atleast 1 DAG_REG_ROM present ) 
       i,e, making it conistent  with dagld */
    if ( img_offset >= 1 ) img_offset = 1;
	dagutil_verbose("img_offset=%d\n", img_offset); /* XXX */

    /* Detect copro support and type if fitted. */
	/* Find COPRO_DETECT. */
	if (dag_reg_find((char*) iom, DAG_REG_COPRO, result) < 1)
	{
		copro_id = -1;
	}
	else
	{
        coprodet_base = DAG_REG_ADDR(*result);
	
		copro_id = *(volatile unsigned *)(iom + coprodet_base);
		if ((copro_id & 0xff00) == 0xff00)
		{
			/* no support */
			copro_id = -1;
		}
		else
		{
			int timeout = TIMEOUT_MAX;
			
			/* read copro_id */
			*(volatile unsigned *)(iom + coprodet_base) = 0;
			while (0 == (*(volatile unsigned *)(iom + coprodet_base) & BIT31))
			{
				usleep(1000);
				timeout--;
				if (0 == timeout)
				{
					dagutil_panic("timeout reading Coprocessor ID.");
				}
			}
			
			/* Read result of type probe */
			copro_id = (*(volatile unsigned *)(iom + coprodet_base)) & 0xff;
		}
	}
	dagutil_verbose("Coprocessor ID = %d\n", copro_id); 

	/* Check image is correct for location */
	/* find location */
	img_idx = dag_get_img_idx(daginf->device_code, img_offset + which_pp);
	dagutil_verbose("dag%d dev 0x%04x pos %d got idx %d\n",
		daginf->id, daginf->device_code, img_offset + which_pp, img_idx); // XXX
	/* check against image */
	if (img_idx < 0) 
	{
		dagutil_verbose("Unkown image. Not loading\n");
		return kDagErrFirmwareLoadFailed;
	}
		
	switch (dag_check_img_ptr(img_idx, copro_id, (char*) xnew, XBUFSIZE, daginf->brd_rev)) 
	{
		case 0:
			break;

		case 1:
			dagutil_verbose("Image type mismatch\n");
			return kDagErrFirmwareLoadFailed;
			
		case 2:
			dagutil_verbose("Image type and device mismatch, not loading!\nEnsure you are loading the correct firmware for this device.\n");
			return kDagErrFirmwareLoadFailed;

		default:
			return kDagErrFirmwareLoadFailed;
	}

    /* 7.1s cards need to be put in reset while loading PP images */
    if(card->mCardType == kDag71s)
    {
	// put AMCC to eql
	reg_val = card_read_iom(card, 0x360);
	card_write_iom(card, 0x360, reg_val | BIT5);
	// put AMCC to fcl
	reg_val = card_read_iom(card, 0x360);
	card_write_iom(card, 0x360, reg_val | BIT4);
	// reset AMCC 
        reg_val = card_read_iom(card, 0x174);
        card_write_iom(card, 0x174, reg_val | 0x1);
        usleep(1000000);
    }



	/* pre-charge programming logic (for 3.8s and 4.3s) */
	card_write_iom(card, fpga_stat_cmd, 0x20);
	card_write_iom(card, fpga_stat_cmd, 0x10);
	dagutil_microsleep(100000);

	/*
	* Programming loop here
	*/
	if ((card_read_iom(card, fpga_stat_cmd) == 0xffffffff) || (card_read_iom(card, fpga_stat_cmd) == 0x2))
		dagutil_panic("Xilinx%d is not available!\n", which_pp+1);

	card_write_iom(card, fpga_stat_cmd, 0x20); /* erase fpga */
	dagutil_microsleep(100000);
	
	dagutil_verbose ("Waiting for Xilinx%d (%s %s) to program ... \n",
		which_pp+1, nfunc, ntype);
	fflush(stdout);

	while (0 == (card_read_iom(card, fpga_stat_cmd) & 0x4))
	{
		/* wait for INIT_N, ready to program */
	}
   dagutil_microsleep(100);
	dagutil_verbose("FPGA Initialized.\nStarting to program \n");
	seenerr = 0;
	while (len-- > 0) {
		card_write_iom(card, fpga_config_data, *bp++);
		//delay may needed to ensure the FPGA bandwidth 
//		dagutil_nanosleep(200);
		if ((0 == seenerr) && (card_read_iom(card, fpga_stat_cmd) & 0x5) == 0)
		{
			seenerr = 1;
			dagutil_verbose("\nCRC error at byte %d\n", len);
		}
//		dagutil_nanosleep(200);
	}

    /* remove 7.1s from reset state */
    if(card->mCardType == kDag71s)
    {
        dagutil_microsleep(100000);
        reg_val = card_read_iom(card, 0x174);
        card_write_iom(card, 0x174, reg_val & 0xFFFFFFFE);
    }

	dagutil_verbose("\nFile loaded.\n");
	if (card_read_iom(card, fpga_stat_cmd) & 1)
	{
		dagutil_verbose ("Done.\n");
	}
	else
	{
		dagutil_verbose("Finished, but status 0x%x\n", card_read_iom(card, fpga_stat_cmd));
        return kDagErrFirmwareLoadFailed;
	}
    return kDagErrNone;
}

DagCardPtr
card_init(dag_card_t card_type)
{
	DagCardPtr result;
	
	assert(kFirstDagCard <= card_type);
	assert(card_type <= kLastDagCard);
	
	result = (DagCardPtr) malloc(sizeof(*result));
	if (NULL != result)
	{
		memset(result, 0, sizeof(*result));
		
		/* Set default values. */
		result->mLastError = kDagErrNone;
		result->mCardType = card_type;
		
		/* Initialize virtual methods to default implementations. */
		result->fDispose = generic_card_dispose;
		result->fPostInitialize = generic_card_post_initialize;
		result->fReset = generic_card_reset;
		result->fDefault = generic_card_default;
		result->fLoadFirmware = generic_card_load_firmware;
		result->fLoadPPImage = generic_card_load_pp_image;
		result->fLoadEmbedded = generic_card_load_embedded;

		/* allocate memory for the SWID string */
		result->mSWID = (uint8_t*)malloc(DAGSERIAL_SIZE);

		/* Compute checksum over the object. */
		result->mChecksum = compute_card_checksum(result);
		result->mMagicNumber = CARD_MAGIC;
		
		(void) valid_card(result);

		/* Call the initialize() routine appropriate to the card_type. */
		if (kDagErrNone != initialize_card(card_type, result))
		{
			/* Initialization failed. */
			/* FIXME: need to communicate this back to the caller. */
			free(result);
			return NULL;
		}
		

		(void) valid_card(result);
	}
	
	return result;
}


/* Virtual method implementations. */
void
card_dispose(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		card->fDispose(card);
		if (card->mSWID != NULL)
		{
			free(card->mSWID);
			card->mSWID = NULL;
		}
		
		if (card->mPrivateState)
		{
			free(card->mPrivateState);
			card->mPrivateState = NULL;
		}
		
		assert(NULL == card->mPrivateState); /* Did the impl remember to clean up? */
		
		memset(card, 0, sizeof(*card));
		free(card);
	};
}


void
card_post_initialize(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		/* Call the post_initialize() routine registered by the card. */
		card->fPostInitialize(card);

		/* Call post_initialize() on the root component. */
		component_post_initialize(card->mRootComponent);
	}
}


void
card_reset(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		card->fReset(card);
	}
}


void
card_default(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		card->fDefault(card);
	}
}


dag_err_t
card_load_firmware(DagCardPtr card, uint8_t* image)
{
    dag_err_t error = kDagErrNone;
	assert(NULL != image);
    if (NULL == image)
    {
        return kDagErrInvalidParameter;
    }
	
	if (1 == valid_card(card))
	{
        if (card->fLoadFirmware == NULL)
            return kDagErrFirmwareLoadFailed;
		error = card->fLoadFirmware(card, image);
        if (error != kDagErrNone)
            return error;
        return error;
	}
    return kDagErrInvalidCardRef;
}

dag_err_t
card_load_embedded(DagCardPtr card, uint8_t* image, int target_processor_region)
{
dag_err_t error = kDagErrNone;

	assert(NULL != image);
    if (NULL == image)
    {
        return kDagErrInvalidParameter;
    }
	
	if (1 == valid_card(card))
	{
		if (card->fLoadEmbedded == NULL)
			return kDagErrFirmwareLoadFailed;
		error = card->fLoadEmbedded(card, image, target_processor_region);
		if (error != kDagErrNone)
			return error;
			return error;
	}
	return kDagErrInvalidCardRef;
}


/* Non-virtual method implementations. */
ComponentPtr
card_get_root_component(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		return card->mRootComponent;
	}
	
	return NULL;
}


void
card_set_root_component(DagCardPtr card, ComponentPtr root_component)
{
	if (1 == valid_card(card))
	{
		assert(NULL == card->mRootComponent); /* Root component should be immutable. */
		assert(kComponentRoot == component_get_component_code(root_component)); /* Only a kComponentRoot can be a root component. */
		
		card->mRootComponent = root_component;
		card->mChecksum = compute_card_checksum(card);
		
		/* Make sure nothing has broken. */
		(void) valid_card(card);
	}
}


volatile uint8_t*
card_get_iom_address(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		return card->mIoMemory;
	}
	
	return NULL;
}


void
card_set_iom_address(DagCardPtr card, volatile uint8_t* iom)
{
	assert(NULL != iom);
	
	if (1 == valid_card(card))
	{
		assert(NULL == card->mIoMemory); /* Should only be set once. */
		
		if (NULL == card->mIoMemory)
		{
			card->mIoMemory = iom;
		}
		card->mChecksum = compute_card_checksum(card);
	}
}


daginf_t*
card_get_info(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		return card->mInfo;
	}
	
	return NULL;
}


void
card_set_info(DagCardPtr card, daginf_t* info)
{
	assert(NULL != info);
	
	if (1 == valid_card(card))
	{
		assert(NULL == card->mInfo); /* Should only be set once. */
		
		if (NULL == card->mInfo)
		{
			card->mInfo = info;
		}
		card->mChecksum = compute_card_checksum(card);
	}
}


dag_reg_t*
card_get_registers(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		return card->mRegisters;
	}
	
	return NULL;
}


void
card_set_registers(DagCardPtr card, dag_reg_t* registers)
{
	assert(NULL != registers);
	
	if (1 == valid_card(card))
	{
		assert(NULL == card->mRegisters); /* Should only be set once. */
		
		if (NULL == card->mRegisters)
		{
			card->mRegisters = registers;
		}
		card->mChecksum = compute_card_checksum(card);
	}
}


int
card_get_fd(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		assert(-1 != card->mFd);
		
		return card->mFd;
	}
	
	return -1;
}


void
card_set_fd(DagCardPtr card, int fd)
{
	assert(-1 != fd);
	
	if (1 == valid_card(card))
	{
		card->mFd = fd;
		card->mChecksum = compute_card_checksum(card);
		
		/* Make sure nothing has broken. */
		(void) valid_card(card);
	}
}


uint32_t
card_read_iom(DagCardPtr card, uintptr_t address)
{
	if (1 == valid_card(card))
	{
        uint32_t value = *(volatile uint32_t*)((uintptr_t) card->mIoMemory + address);
        dagutil_verbose_level(5, "card_read_iom: Read 0x%08x from 0x%" PRIxPTR "\n", value, address);
        return value;
	}
	
	return 0;
}


void
card_write_iom(DagCardPtr card, uintptr_t address, uint32_t value)
{
	if (1 == valid_card(card))
	{
        dagutil_verbose_level(5, "card_write_iom: Writing 0x%08x to 0x%" PRIxPTR "\n", value, address);
		*(volatile uint32_t*)((uintptr_t) card->mIoMemory + address) = value;
	}
}


void*
card_get_private_state(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		return card->mPrivateState;
	}
	
	return NULL;
}


void
card_set_private_state(DagCardPtr card, void* state)
{
	if (1 == valid_card(card))
	{
		assert((NULL == card->mPrivateState) || (NULL == state));
		/* Potential memory leak if this is not true.
		 * Since only the specific implementation knows, we force it to zero
		 * mPrivateState before reusing it.  This should help remind the impl
		 * of the need to dispose existing state before overwriting the pointer.
		 */
		
		card->mPrivateState = state;
		card->mChecksum = compute_card_checksum(card);
		
		/* Make sure nothing has broken. */
		(void) valid_card(card);
	}
}


dag_err_t
card_get_last_error(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		return card->mLastError;
	}
	
	return kDagErrNone;
};


void
card_set_last_error(DagCardPtr card, dag_err_t error_code)
{
	if (1 == valid_card(card))
	{
		card->mLastError = error_code;
		card->mChecksum = compute_card_checksum(card);
		
		/* Make sure nothing has broken. */
		(void) valid_card(card);
	}
}


void
card_set_dispose_routine(DagCardPtr card, CardDisposeRoutine routine)
{
	assert(NULL != routine);
	
	if (1 == valid_card(card))
	{
		card->fDispose = routine;
		card->mChecksum = compute_card_checksum(card);
		
		/* Make sure nothing has broken. */
		(void) valid_card(card);
	}
}

void
card_set_post_initialize_routine(DagCardPtr card, CardPostInitializeRoutine routine)
{
	assert(NULL != routine);
	
	if (1 == valid_card(card))
	{
		card->fPostInitialize = routine;
		card->mChecksum = compute_card_checksum(card);
		
		/* Make sure nothing has broken. */
		(void) valid_card(card);
	}
}


void
card_set_reset_routine(DagCardPtr card, CardResetRoutine routine)
{
	assert(NULL != routine);
	
	if (1 == valid_card(card))
	{
		card->fReset = routine;
		card->mChecksum = compute_card_checksum(card);
		
		/* Make sure nothing has broken. */
		(void) valid_card(card);
	}
}


void
card_set_default_routine(DagCardPtr card, CardDefaultRoutine routine)
{
	assert(NULL != routine);
	
	if (1 == valid_card(card))
	{
		card->fDefault = routine;
		card->mChecksum = compute_card_checksum(card);
		
		/* Make sure nothing has broken. */
		(void) valid_card(card);
	}
}

void
card_set_update_register_base_routine(DagCardPtr card, CardUpdateRegisterBaseRoutine routine)
{
    if (1 == valid_card(card))
    {
        card->fUpdateRegisterBaseRoutine = routine;
        card->mChecksum = compute_card_checksum(card);
    }
}

void
card_set_load_firmware_routine(DagCardPtr card, CardLoadFirmwareRoutine routine)
{
	assert(NULL != routine);
	
	if (1 == valid_card(card))
	{
		card->fLoadFirmware = routine;
		card->mChecksum = compute_card_checksum(card);
		
		/* Make sure nothing has broken. */
		(void) valid_card(card);
	}
}


int
card_lock_stream(DagCardPtr card, int stream)
{
	if (1 == valid_card(card))
	{
		int lock = (stream << 16) | 1; /* lock */
		int dagfd = card->mFd;
	
	#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	
		if (ioctl(dagfd, DAGIOCLOCK, &lock) < 0)
		{
			return -1; /* errno set accordingly */
		}
	
	#elif defined(_WIN32)
	
		ULONG BytesTransfered;
	
		if (DeviceIoControl(dag_gethandle(dagfd),
			IOCTL_LOCK,
			&lock,
			sizeof(lock),
			NULL,
			0,
			&BytesTransfered,
			NULL) == FALSE) 
		{
			panic("DeviceIoControl IOCTL_LOCK\n");
		}
	
	#else
	#error Compiling on an unsupported platform - please contact <support@endace.com> for assistance.
	#endif /* Platform-specific code. */
	
		return 0;
	}
	
	return 1;
}


int
card_unlock_stream(DagCardPtr card, int stream)
{
	if (1 == valid_card(card))
	{
		int lock = (stream << 16) | 0; /* unlock */
		int dagfd = card->mFd;
	
	#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	
		if (ioctl(dagfd, DAGIOCLOCK, &lock) < 0)
		{
			return -1; /* errno set accordingly */
		}
	
	#elif defined(_WIN32)
	
		ULONG BytesTransfered;
	
		if (DeviceIoControl(dag_gethandle(dagfd),
			IOCTL_LOCK,
			&lock,
			sizeof(lock),
			NULL,
			0,
			&BytesTransfered,
			NULL) == FALSE)
		{
			panic("DeviceIoControl IOCTL_LOCK\n");
		}
		
	#else
	#error Compiling on an unsupported platform - please contact <support@endace.com> for assistance.
	#endif /* Platform-specific code. */
	
		return 0;
	}
	
	return 1;
}


int
card_lock_all_streams(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		int c;
		
		for (c = 0; c < DAG_STREAM_MAX; c++)
		{
			if ( is_pbm_existent_stream( card, c) && card_lock_stream(card, c))
			{
				return -1;
			}
		}
		
		return 0;
	}
	
	return -1;
}


void
card_unlock_all_streams(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		int c;
		
		for (c = 0; c < DAG_STREAM_MAX; c++)
		{
			if (is_pbm_existent_stream( card, c) && card_unlock_stream(card, c))
			{
				dagutil_warning("Cannot release lock on stream %d: %s\n",  c, strerror(errno));
			}
		}
	}
}


uintptr_t
card_get_register_address(DagCardPtr card, dag_reg_module_t reg_code, uint32_t offset)
{
	if (1 == valid_card(card))
	{
		dag_reg_t result[DAG_REG_MAX_ENTRIES];
		uint32_t count = 0;
		
		assert(NULL != card->mRegisters);
		
		if ((dag_reg_table_find(card->mRegisters, 0, reg_code, result, &count)) || (0 == count))
		{
			return 0;
		}
		
		if (offset >= count)
		{
			return 0;
		}
		
		return (uintptr_t) result[offset].addr;
	}
	
	return 0;
}

/* Return the count of the modules indicated by code */
uint32_t 
card_get_register_count(DagCardPtr card, dag_reg_module_t code)
{
    if (1 == valid_card(card))
    {
        dag_reg_t result[DAG_REG_MAX_ENTRIES];
        uint32_t count = 0;
        dag_reg_table_find(card->mRegisters, 0, code, result, &count);
        return count;
    }
    return -1;
}

int
card_get_register_version(DagCardPtr card, dag_reg_module_t reg_code, uint32_t offset)
{
	if (1 == valid_card(card))
	{
		dag_reg_t result[DAG_REG_MAX_ENTRIES];
		uint32_t count = 0;
		
		assert(NULL != card->mRegisters);
		
		if ((dag_reg_table_find(card->mRegisters, 0, reg_code, result, &count)) || (0 == count))
		{
			return -1;
		}
		
		if (offset >= count)
		{
			return -1;
		}
		
		return result[offset].version;
	}
	
	return 0;
}
/* this function will return an index for specific register module with specified version and module index 
The reason to have this function is that all the interfaces to the register tables are expecting module index which is independant
from the version numbers. For some components can be usfull to have index which is specific for the version.
you still need to have the module indexe if you need to access the registers this is only for proper numbering in casees like SMBUS controllers

*/
int
card_get_register_index_by_version(DagCardPtr card, dag_reg_module_t reg_code, uint32_t version, uint32_t offset)
{
	if (1 == valid_card(card))
	{
		dag_reg_t result[DAG_REG_MAX_ENTRIES];
		uint32_t count = 0;
		int version_array[2]; //for the time being we assume two devices
		int version_count = 0;
		int i;
		
		assert(NULL != card->mRegisters);
		
		if ((dag_reg_table_find(card->mRegisters, 0, reg_code, result, &count)) || (0 == count))
		{
			return -3;
		}
		
		if (offset >= count)
		{
			return -2;
		}
		/*need to generalize the function.out of the result array the index version returned should be n the index of the reg code having version.where n is the offset*/
		for(i=0;i<=count;i++)
		{
			if(version == result[i].version)
			{
				version_array[version_count] = i;
				version_count++;
			}
		}
		return version_array[offset];
	}
	return 0;
}


void
card_reset_datapath(DagCardPtr card)
{
	int val = card_read_iom(card, DAG_CONFIG);
	
	val |= 0x80000000;
	card_write_iom(card, DAG_CONFIG, val);
}

const char*
card_get_card_type_as_string(DagCardPtr card)
{
    return card_type_to_string(card_get_card_type(card));
}

int
card_pbm_config_overlap(DagCardPtr card)
{
    int rxstreams, txstreams, streammax, stream, base, size;
	int retval = 0;
    daginf_t* daginf = card->mInfo;
    uint32_t pbm_base = card_get_register_address(card, DAG_REG_PBM, 0);
    int pbm_ver = card_get_register_version(card, DAG_REG_PBM, 0);
    pbm_offsets_t* PBM = NULL;
    
    PBM = dagutil_get_pbm_offsets();
    if( NULL == PBM )
    {
        dagutil_panic("Unable to get PBM offset in %s\n",__FUNCTION__);
        return 1;
    }
	if (card_lock_all_streams(card))
    {
		retval = EACCES;
	}
    else
    {
        rxstreams = dag_rx_get_stream_count(card->mFd);
        txstreams = dag_tx_get_stream_count(card->mFd);
        streammax = MAX(rxstreams*2-1, txstreams*2);

        if ((rxstreams<1)||(txstreams<1))
        {
            retval = ENODEV;
        }
        else
        {
            for (stream=0;stream<streammax;stream++)
            {
                if ((stream == 0) || (stream == 1)) {
                    base = daginf->phy_addr;
                    size = daginf->buf_size;
                } else {
                    base = 0;
                    size = 0;
                }
                    
                card_write_iom(card, pbm_base +
                        PBM[pbm_ver].streambase +
                        stream*PBM[pbm_ver].streamsize +
                        PBM[pbm_ver].mem_addr,
                        base);
                
                card_write_iom(card, pbm_base +
                        PBM[pbm_ver].streambase +
                        stream*PBM[pbm_ver].streamsize +
                        PBM[pbm_ver].mem_size,
                        size);
            }	
        }
        card_unlock_all_streams(card);
    }
	return retval;
}

int
card_pbm_is_overlapped(DagCardPtr card, uint8_t* is_overlapped)
{
    int rx_base = 0;
    int tx_base = 0;
    int retval = 0;
    uint32_t pbm_base = card_get_register_address(card, DAG_REG_PBM, 0);
    int pbm_ver = card_get_register_version(card, DAG_REG_PBM, 0);
    pbm_offsets_t *PBM = NULL;

    PBM = dagutil_get_pbm_offsets();
    if( NULL == PBM )
    {
        dagutil_panic("Unable to get PBM offset in %s\n",__FUNCTION__);
        return 1;
    }
    *is_overlapped = 0;
	/* Test for overlapped memory usage if the rx and tx streams both start at the same base address */
    rx_base = card_read_iom(card, pbm_base + PBM[pbm_ver].streambase + PBM[pbm_ver].mem_addr);
    tx_base = card_read_iom(card, pbm_base + PBM[pbm_ver].streambase + PBM[pbm_ver].streamsize + PBM[pbm_ver].mem_addr);
    *is_overlapped = (rx_base == tx_base);
    return retval;
}

int
card_pbm_get_stream_mem_size(DagCardPtr card, uint32_t stream)
{
	int retval = 0;
    pbm_offsets_t *PBM = NULL;
    PBM = dagutil_get_pbm_offsets();
    if( NULL == PBM )
    {
        dagutil_panic("Unable to get PBM offset in %s\n",__FUNCTION__);
        return -1;
    }
	/*
	if (card_lock_all_streams(card))
	{
		retval = EACCES;
	}
	else
	*/
	{
		uint32_t pbm_base = card_get_register_address(card, DAG_REG_PBM, 0);
		int pbm_ver = card_get_register_version(card, DAG_REG_PBM, 0);

        /* read the size */
        retval = card_read_iom(card, pbm_base + PBM[pbm_ver].streambase +
                    stream*PBM[pbm_ver].streamsize + PBM[pbm_ver].mem_size);
	}

	//card_unlock_all_streams(card);

	return retval;
}

int
card_pbm_config_stream(DagCardPtr card, uint32_t stream, uint32_t size)
{
    daginf_t* daginf = card->mInfo;
	int retval = 0;

    pbm_offsets_t *PBM = NULL;
    PBM = dagutil_get_pbm_offsets();
    if( NULL == PBM )
    {
        dagutil_panic("Unable to get PBM offset in %s\n",__FUNCTION__);
        return 1;
    }

    /* If the stream is not existent in pbm, return without doing anything */
    if ( !is_pbm_existent_stream( card, stream) )
    {
        dagutil_verbose_level(2,"Non-existent stream%u. Not setting the size\n",stream);
        return retval;
    }
	if (card_lock_all_streams(card))
	{
		retval = EACCES;
	}
	else
	{
		uint32_t pbm_base = card_get_register_address(card, DAG_REG_PBM, 0);
		int pbm_ver = card_get_register_version(card, DAG_REG_PBM, 0);
        int prev_address = 0;
        int prev_size = 0;
        int previous_valid_stream = -1;

        /* Find out the address and size of the previous stream, then we know
         * at which address to start this stream
         */
        
        if (stream > 0)
        {
            previous_valid_stream = get_previous_valid_stream(card, stream);
            if ( previous_valid_stream < 0 )
            {
                dagutil_error("Failed to get a previous valid stream for %u\n",stream);
                return 0;
            }
            prev_address = card_read_iom(card, pbm_base + PBM[pbm_ver].streambase +
                (previous_valid_stream)*PBM[pbm_ver].streamsize + PBM[pbm_ver].mem_addr);
            prev_size = card_read_iom(card, pbm_base + PBM[pbm_ver].streambase +
                (previous_valid_stream)*PBM[pbm_ver].streamsize + PBM[pbm_ver].mem_size);
        }
        else
        {
            prev_address = daginf->phy_addr;
            prev_size = 0;
        }
		
        /* write the base address */
        card_write_iom(card, pbm_base + PBM[pbm_ver].streambase +
                stream*PBM[pbm_ver].streamsize + PBM[pbm_ver].mem_addr,
                prev_address + prev_size);
                
        /* write the size given in bytes*/
        card_write_iom(card, pbm_base + PBM[pbm_ver].streambase +
                stream*PBM[pbm_ver].streamsize + PBM[pbm_ver].mem_size,
                size);
	}

	card_unlock_all_streams(card);

	return retval;
}

int
card_get_tx_stream_count(DagCardPtr card)
{
    return card->mTxStreamCount;
}

void
card_set_tx_stream_count(DagCardPtr card, int val)
{
    if (1 == valid_card(card))
	{
		card->mTxStreamCount = val;
		card->mChecksum = compute_card_checksum(card);
		
		/* Make sure nothing has broken. */
		(void) valid_card(card);
	}
}

int
card_get_rx_stream_count(DagCardPtr card)
{
    return card->mRxStreamCount;
}

void
card_set_rx_stream_count(DagCardPtr card, int val)
{
    if (1 == valid_card(card))
	{
		card->mRxStreamCount = val;
		card->mChecksum = compute_card_checksum(card);
		
		/* Make sure nothing has broken. */
		(void) valid_card(card);
	}
}
#if 0
//this one was replaced by the component default method 
int
card_pbm_default(DagCardPtr card)
{
	daginf_t* daginf = card->mInfo;
	int dagfd = card->mFd;
	int rxstreams;
	int txstreams;
	int total;
	int used = 0;
	int streammax;
	int retval = 0;

	total = daginf->buf_size / 1024 / 1024;
	rxstreams = dag_rx_get_stream_count(dagfd);
	txstreams = dag_tx_get_stream_count(dagfd);
	streammax = MAX(rxstreams*2 - 1, txstreams*2);

	if (card_lock_all_streams(card))
	{
		retval = EACCES;
	}
	else
	{
		int rxsize = (total - txstreams * PBM_TX_DEFAULT_SIZE) / rxstreams;
		int stream;
		uint32_t pbm_base = card_get_register_address(card, DAG_REG_PBM, 0);
		int pbm_ver = card_get_register_version(card, DAG_REG_PBM, 0);
		
		for (stream = 0; stream < streammax; stream++)
		{
			int val = 0;
			
			if (0 == (stream & 1))
			{
				if (stream < rxstreams*2)
				{
					val = rxsize;
				}
			}
            else
            {
                /* transmit stream */
                if (stream - 1 < txstreams * 2)
                {
                    val = PBM_TX_DEFAULT_SIZE;
                }
            }
			
			/* assignment checks out, set up pbm stream. */
			card_write_iom(card, pbm_base + PBM[pbm_ver].streambase +
					stream*PBM[pbm_ver].streamsize + PBM[pbm_ver].mem_addr,
					daginf->phy_addr + used*1024*1024);
					
			card_write_iom(card, pbm_base + PBM[pbm_ver].streambase +
					stream*PBM[pbm_ver].streamsize + PBM[pbm_ver].mem_size,
					val*1024*1024);
					
			/* update for next loop */
			used += val;
		}
	}

	card_unlock_all_streams(card);

	return retval;
}
#endif


/* Verification routine. */
int
valid_card(DagCardPtr card)
{
	uint32_t checksum;
	
	assert(NULL != card);
	assert(CARD_MAGIC == card->mMagicNumber);
	
	checksum = compute_card_checksum(card);
	assert(checksum == card->mChecksum);
	
	if ((NULL != card) && (CARD_MAGIC == card->mMagicNumber) && (checksum == card->mChecksum))
	{
		return 1;
	}
	
	return 0;
}


void
card_erase_rom(DagCardPtr card, romtab_t* rp, uint32_t saddr, uint32_t eaddr)
{
    int iret;
    /* look for a swid */
    if (dagswid_isswid(rp) < 0 || (eaddr != (DAGROM_BSTART + DAGROM_BSIZE)))
    {
        dagrom_erase(rp, saddr, eaddr);
    }
    else
    {
        if(rp->device_code == 0x430e)  /* DAG4.3ge */
        {

        /* Read the key and SWID */
        iret = dagswid_read(rp, card->mSWID, DAGSERIAL_SIZE);
        if (iret < 0)
        {
            dagutil_verbose("In erase, SWID read failed, errno : %d\n", iret);
        }
        
        iret = dagswid_readkey(rp, &card->mSWIDKey);
        if (iret < 0)
        {
            dagutil_verbose("In erase, SWID read key failed, errno : %d\n", iret);
        }
        
        card->mWriteBackSWID = 1;
        
        /* Now we can safely perform the erase operation. */
        dagrom_erase(rp, saddr, eaddr);

        /*write the swid back straight away, in case programming fails */
        dagswid_write(rp, card->mSWID, DAGSERIAL_SIZE, card->mSWIDKey);

        } 
        else 
        {
            dagrom_erase(rp, saddr, eaddr - DAGROM_SECTOR);

        }


    }
}

void
card_set_card_type(DagCardPtr card, dag_card_t type)
{
    card->mCardType = type;
    card->mChecksum = compute_card_checksum(card);
}

dag_card_t
card_get_card_type(DagCardPtr card)
{
    return card->mCardType;
}

dag_err_t
card_load_pp_image(DagCardPtr card, const char* filename, int which_pp)
{
    dag_err_t error = kDagErrNone;
	assert(NULL != filename);
    if (NULL == filename)
    {
        return kDagErrInvalidParameter;
    }
	
	if (1 == valid_card(card))
	{
		error = card->fLoadPPImage(card, filename, which_pp);
        if (error != kDagErrNone)
            return error;
        return error;
	}
    return kDagErrInvalidCardRef;
}

/* Use this function to update the register base on all components of the card. Useful
 * if the firmware is changed.*/
dag_err_t
card_update_register_base(DagCardPtr card)
{
    dag_err_t error = kDagErrNone;
    ComponentPtr root = NULL;
    uint32_t count = 0;
    uint32_t i = 0;
    ComponentPtr any_component = NULL;

    if (1 == valid_card(card))
    {
        if (card->fUpdateRegisterBaseRoutine)
        {
            error = card->fUpdateRegisterBaseRoutine(card);
            DAG_ERR_RETURN_WV(error, error);
        }

        root = card_get_root_component(card);
        count = component_get_subcomponent_count(root);
        for (i = 0; i < count; i++)
        {
            any_component = component_get_indexed_subcomponent(root, i);
            error = component_update_register_base(any_component);
            DAG_ERR_RETURN_WV(error, error);
        }
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}
/* checks whether the current stream number is valid (existing) in burst manager 
    Verifies the stream number against rx and tx stream count 
*/
uint8_t is_pbm_existent_stream(DagCardPtr card, uint32_t stream_number)
{
    if ( stream_number & 1 )
    {
        return ( stream_number < ( 2 * card_get_tx_stream_count(card) ));
    }
    else
    {
        return ( stream_number < ( 2 * card_get_rx_stream_count(card)));
    }
}
/*  Gets the pervious valid stream number 
    The previous stream number is needed to calculate  the mem address of the stream
*/
int get_previous_valid_stream(DagCardPtr card, uint32_t stream_number)
{
    int cur_stream = stream_number - 1;
    while ( (cur_stream >= 0) && !is_pbm_existent_stream(card,cur_stream) )
        cur_stream--;
    return cur_stream ;
}
