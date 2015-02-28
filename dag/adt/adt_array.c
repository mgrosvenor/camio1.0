/* File header */
#include "adt_array.h"

/* Koryn's Units */
#include "adt_debugging.h"
#include "adt_magic_numbers.h"
#include "adt_memory.h"

/* C Standard Library headers. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct ArrayHeader ArrayHeader;
typedef ArrayHeader* ArrayHeaderPtr;


struct ArrayHeader
{
	adt_magic_number_t mMagicNumber;
	DisposeRoutine mDisposer;
	unsigned int mItems;
	AdtPtr* mArray; /* Array of Ptrs */
};


/* Internal routines */
static unsigned int valid_array(ArrayHeaderPtr header);
#ifndef NDEBUG
static void verify_array(ArrayHeaderPtr header);
#endif /* Debugging code. */
static unsigned int increase_array(ArrayHeaderPtr header, unsigned int location);


/* Implementation of internal routines. */
static unsigned int
valid_array(ArrayHeaderPtr header)
{
	if (header && (header->mMagicNumber == kArrayMagicNumber))
	{
		return 1;
	}

	return 0;
}


#ifndef NDEBUG
static void
verify_array(ArrayHeaderPtr header)
{
	unsigned int index;

	ADT_ASSERT_PTR(header, "");
	assert(header->mMagicNumber == kArrayMagicNumber);

	for (index = 0; index < header->mItems; index++)
	{
		ADT_ASSERT_PTR(header->mArray[index], "");
	}
}
#endif /* Debugging code. */


static unsigned int
increase_array(ArrayHeaderPtr header, unsigned int location)
{
	AdtPtr* new_array = (AdtPtr*) ADT_XALLOCATE(1 + 2 * sizeof(AdtPtr) * location);

	if (!new_array)
	{
		return 0;
	}

	memset(new_array, 0, 1 + 2 * sizeof(AdtPtr) * location);
	if (header->mArray)
	{
		/* Copy previous contents over. */
		memcpy(new_array, header->mArray, sizeof(AdtPtr) * header->mItems);
		
		/* Dispose previous array. */
		header->mArray = (AdtPtr*) adt_dispose_ptr(header->mArray);
	}
	
	header->mArray = new_array;
	header->mItems = 1 + 2 * location;
	
	return 1;
}


/* Array ADT routines. */
ArrayPtr
adt_array_init(DisposeRoutine disposer, unsigned int size)
{
	ArrayHeaderPtr result = (ArrayHeaderPtr) ADT_XALLOCATE(sizeof(ArrayHeader));

	memset(result, 0, sizeof(ArrayHeader));
	result->mDisposer = disposer;
	
	if (size)
	{
		result->mItems = size;

		result->mArray = (AdtPtr*) ADT_XALLOCATE(sizeof(AdtPtr) * size);
		memset(result->mArray, 0, sizeof(AdtPtr) * size);
	}

	result->mMagicNumber = kArrayMagicNumber;

#ifndef NDEBUG
	verify_array(result);
#endif /* Debugging code. */

	return (ArrayPtr) result;
}


ArrayPtr
adt_array_dispose(ArrayPtr array)
{
	ArrayHeaderPtr header = (ArrayHeaderPtr) array;
	
#ifndef NDEBUG
	verify_array(header);
#endif /* Debugging code. */

	if (!valid_array(header))
	{
		return NULL;
	}

	if (header->mDisposer)
	{
		unsigned int index;
		for (index = 0; index < header->mItems; index++)
		{
		#ifndef NDEBUG
			ADT_ASSERT_PTR_NIL(header->mArray[index], "");
		#endif /* Debugging code. */
		
			if (header->mArray[index])
			{
				header->mDisposer(header->mArray[index]);
			}
		}
	}

	header->mMagicNumber = 0;
	(void) adt_dispose_ptr(header->mArray);
	(void) adt_dispose_ptr((AdtPtr) header);

	return NULL;
}


void
adt_array_make_empty(ArrayPtr array)
{
	ArrayHeaderPtr header = (ArrayHeaderPtr) array;
	unsigned int index;
	
#ifndef NDEBUG
	verify_array(header);
#endif /* Debugging code. */

	if (valid_array(header))
	{
		for (index = 0; index < header->mItems; index++)
		{
			if (header->mDisposer)
			{
				header->mDisposer(header->mArray[index]);
			}
		}
	
		memset(header->mArray, 0, sizeof(AdtPtr) * header->mItems);
	}
}


unsigned int
adt_array_put(ArrayPtr array, unsigned int location, AdtPtr data)
{
	ArrayHeaderPtr header = (ArrayHeaderPtr) array;
	
#ifndef NDEBUG
	verify_array(header);
#endif /* Debugging code. */

	if (!valid_array(header))
	{
		return 0;
	}
	
	/* Ensure array is large enough to take the item. */
	if (location >= header->mItems)
	{
		if (!increase_array(header, location))
		{
			return 0;
		}
	}

	/* Put the item in the array, disposing of anything there previously. */
	if (header->mArray[location] && header->mDisposer)
	{
		header->mDisposer(header->mArray[location]);
	}

	header->mArray[location] = data;
	return 1;
}


AdtPtr
adt_array_get(ArrayPtr array, unsigned int location)
{
	ArrayHeaderPtr header = (ArrayHeaderPtr) array;

#ifndef NDEBUG
	verify_array(header);
#endif /* Debugging code. */

	assert(location < header->mItems);
	if (valid_array(header) && (location < header->mItems))
	{
		return header->mArray[location];
	}
	
	return NULL;
}


unsigned int
adt_array_items(ArrayPtr array)
{
	ArrayHeaderPtr header = (ArrayHeaderPtr) array;

#ifndef NDEBUG
	verify_array(header);
#endif /* Debugging code. */

	if (valid_array(header))
	{
		return header->mItems;
	}

	return 0;
}


void
adt_array_iterate(ArrayPtr array, IterationCallback iterator, AdtPtr user_data)
{
	ArrayHeaderPtr header = (ArrayHeaderPtr) array;

#ifndef NDEBUG
	verify_array(header);
#endif /* Debugging code. */

	assert(iterator != NULL);
	if (valid_array(header))
	{
		unsigned int count = header->mItems;
		unsigned int index;
		
		for (index = 0; index < count; index++)
		{
			if (!iterator(header->mArray[index], user_data))
			{
				return;
			}
		}
	}
}


#if ADT_MEMORY_DEBUG
void
adt_array_note_ptrs(ArrayPtr array)
{
	ArrayHeaderPtr header = (ArrayHeaderPtr) array;

#ifndef NDEBUG
	verify_array(header);
#endif /* Debugging code. */

	adt_memory_note_ptr(header);
	adt_memory_note_ptr(header->mArray);
}
#endif /* ADT_MEMORY_DEBUG */
