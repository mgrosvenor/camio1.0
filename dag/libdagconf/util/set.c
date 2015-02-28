/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* File header. */
#include "../include/util/set.h"

/* C Standard Library headers. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>


typedef struct SetHeader
{
	int mCount;
	int mArraySize;
	void** mArray; /* Array of pointers to stored elements. */

} SetHeader, *SetHeaderPtr;

#define DEFAULT_ARRAY_SIZE 8


/* Internal routines. */
static void verify_header(SetHeaderPtr header);
static void double_array(SetHeaderPtr header);


/* Implementation of internal routines. */
static void
verify_header(SetHeaderPtr header)
{
	int index;
	
	assert(NULL != header);
	assert(NULL != header->mArray);
	assert(DEFAULT_ARRAY_SIZE <= header->mArraySize);
	
	for (index = 0; index < header->mArraySize; index++)
	{
		if (index < header->mCount)
		{
			assert(NULL != header->mArray[index]);
		}
		else
		{
			assert(NULL == header->mArray[index]);
		}
	}
}


static void
double_array(SetHeaderPtr header)
{
	int new_array_bytes = 2 * header->mArraySize * sizeof(void*);
	void** new_array = (void**) malloc(new_array_bytes);
	int index;
	
	/* Zero the new array. */
	memset(new_array, 0, new_array_bytes);
	
	/* Transfer old elements across. */
	for (index = 0; index < header->mCount; index++)
	{
		new_array[index] = header->mArray[index];
	}
	free(header->mArray);
	
	header->mArray = new_array;
	header->mArraySize *= 2;
}



SetPtr
set_init(void)
{
	SetHeaderPtr header = (SetHeaderPtr) malloc(sizeof(*header));
	
	if (NULL != header)
	{
		header->mCount = 0;
		header->mArraySize = DEFAULT_ARRAY_SIZE;
		header->mArray = (void**) malloc(DEFAULT_ARRAY_SIZE * sizeof(void*));
		
		if (NULL == header->mArray)
		{
			free(header);
			return NULL;
		}
		
		memset(header->mArray, 0, DEFAULT_ARRAY_SIZE * sizeof(void*));
		verify_header(header);
	}
	
	return (SetPtr) header;
}


void
set_dispose(SetPtr set)
{
	SetHeaderPtr header = (SetHeaderPtr) set;

	verify_header(header);
	
	free(header->mArray);

#ifndef NDEBUG
	memset(header, 0, sizeof(*header));
#endif /* NDEBUG */

	free(header);
}


int
set_get_count(SetPtr set)
{
	SetHeaderPtr header = (SetHeaderPtr) set;

	verify_header(header);
	
	return header->mCount;
}


void
set_add(SetPtr set, void* item)
{
	SetHeaderPtr header = (SetHeaderPtr) set;

	verify_header(header);
	assert(NULL != item);
	
	if (header->mCount == header->mArraySize)
	{
		double_array(header);
	}
	
	header->mArray[header->mCount] = item;
	header->mCount++;
}


void*
set_retrieve(SetPtr set, int index)
{
	SetHeaderPtr header = (SetHeaderPtr) set;

	verify_header(header);
	assert(0 <= index);
	assert(index < header->mCount);
	
	return header->mArray[index];
}


void
set_delete(SetPtr set, void* item)
{
    SetHeaderPtr header = (SetHeaderPtr)set;
    int i = 0;
    int j = 0;
    verify_header(header);
    assert(NULL != item);

    for (i = 0; i < header->mCount; i++)
    {
        if (header->mArray[i] == item)
        {
            /* Remove the item */
            header->mArray[i] = NULL;
            /* Shift the items ahead of the removed item down a place */
            for (j = i; j < header->mCount-1; j++)
            {
                header->mArray[j] = header->mArray[j+1];
            }
            header->mArray[j] = NULL;
            header->mCount--;
            break;
        }
    }
}

