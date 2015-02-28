/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#include "dag_config.h"
#include "../include/modules/generic_read_write.h"
#include "../include/util/utility.h"
#include "../include/modules/raw_smbus_module.h"
#include "../include/modules/latch.h"

#include "dagutil.h"
#include "dagreg.h"

/*Randomly choosen number*/
#define LATCH_MAGIC 0xFAC7D63D

struct LatchObject
{
   	/* Standard protection. */
	uint32_t mMagicNumber;
	uint32_t mChecksum;

    /* Every member from here onwards is checksummed */
    uint32_t mAddress;
    DagCardPtr mCard;
    uint32_t mMask;
    GenericReadWritePtr mGRW;
};

/* Implementation of internal routines. */
static uint32_t
compute_latch_checksum(LatchPtr latch)
{
	/* Don't check validity here with valid_attribute() because the attribute may not be fully constructed. */
    if (latch != NULL)
    {
	    return compute_checksum(&latch->mAddress, sizeof(*latch) - 2 * sizeof(uint32_t));
    }
    return 0;
}

static int
valid_latch(LatchPtr latch)
{
    uint32_t checksum;
	
	assert(latch);
	assert(LATCH_MAGIC == latch->mMagicNumber);
	
	checksum = compute_latch_checksum(latch);
	assert(checksum == latch->mChecksum);

	if ((NULL != latch) && (LATCH_MAGIC == latch->mMagicNumber) && (checksum == latch->mChecksum))
	{
		return 1;
	}
	
	return 0;
}


LatchPtr
latch_init(DagCardPtr card, uint32_t address, uint32_t mask)
{
    LatchPtr result = malloc(sizeof(*result));
    memset(result, 0, sizeof(*result));    
    result->mMagicNumber = LATCH_MAGIC;
    result->mAddress = address;
    result->mCard = card;
    result->mMask = mask;
    result->mGRW = grw_init(card, address, grw_iom_read, grw_iom_write);
    result->mChecksum = compute_latch_checksum(result);
    (void)valid_latch(result);
    return result;
}

void
latch_dispose(LatchPtr latch)
{
    if (1 == valid_latch(latch))
    {
        grw_dispose(latch->mGRW);
    }
}

void
latch_set(LatchPtr latch)
{
    if (1 == valid_latch(latch))
    {
        uint32_t reg_val = grw_read(latch->mGRW);
        reg_val |= latch->mMask;
        grw_write(latch->mGRW, reg_val);
    }
}

void
latch_clear(LatchPtr latch)
{
    if (1 == valid_latch(latch))
    {
        uint32_t reg_val = grw_read(latch->mGRW);
        reg_val &= ~latch->mMask;
        grw_write(latch->mGRW, reg_val);
    }
}

