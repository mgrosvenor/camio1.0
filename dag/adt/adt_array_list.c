/* File header */
#include "adt_array_list.h"

/* Koryn's Units */
#include "adt_debugging.h"
#include "adt_magic_numbers.h"
#include "adt_memory.h"

/* C Standard Library headers */
#include <assert.h>
#include <stdlib.h>


static const unsigned int kDefaultListArraySize = 8;
static const unsigned int kExpansionFactor = 2;


typedef struct ArrayListHeader ArrayListHeader;
typedef ArrayListHeader* ArrayListHeaderPtr;

typedef struct ArrayIteratorHeader ArrayIteratorHeader;
typedef ArrayIteratorHeader* ArrayIteratorHeaderPtr;


struct ArrayListHeader
{
	adt_magic_number_t mMagicNumber;
	DisposeRoutine mDisposer;
	unsigned int mItems;
	unsigned int mCurrentSize; /* Current size of the array. */
	AdtPtr* mArray; /* Array of AdtPtr pointers. */
};

struct ArrayIteratorHeader
{
	adt_magic_number_t mMagicNumber;
	ArrayListHeaderPtr mList;
	unsigned int mIsInList;
	unsigned int mCurrent;
};

/* Internal routines */
static unsigned int valid_header(ArrayListHeaderPtr header);
static unsigned int valid_iterator(ArrayIteratorHeaderPtr iterator);
static inline unsigned int default_advance( ArrayIteratorHeaderPtr iterator );
#ifndef NDEBUG
static void verify_header(ArrayListHeaderPtr header);
static void verify_iterator(ArrayIteratorHeaderPtr iterator);
#endif /* Debugging code. */
static void double_array(ArrayListHeaderPtr header, unsigned int new_index, AdtPtr new_item);
static ArrayIteratorHeaderPtr new_array_iterator(ArrayListHeaderPtr header);

/* Implementation of internal routines. */
static unsigned int
valid_header(ArrayListHeaderPtr header)
{
	if (header && (header->mMagicNumber == kArrayListMagicNumber))
	{
		return 1;
	}

	return 0;
}


static unsigned int
valid_iterator(ArrayIteratorHeaderPtr iterator)
{
	if (iterator && (iterator->mMagicNumber == kArrayListIteratorMagicNumber))
	{
		return valid_header(iterator->mList);
	}

	return 0;
}


#ifndef NDEBUG
static void
verify_header(ArrayListHeaderPtr header)
{
	unsigned int index;

	adt_assert_valid_ptr(header, "", __FILE__, __LINE__);
	assert(header->mMagicNumber == kArrayListMagicNumber);
	adt_assert_valid_ptr((AdtPtr) header->mArray, "", __FILE__, __LINE__);
	assert(header->mItems <= header->mCurrentSize);

	for (index = header->mItems; index < header->mCurrentSize; index++)
	{
		assert(header->mArray[index] == NULL);
	}
}


static void
verify_iterator(ArrayIteratorHeaderPtr iterator)
{
	ArrayListHeaderPtr header = iterator->mList;
	unsigned int index;

	/* Check the iterator header. */
	adt_assert_valid_ptr(iterator, "", __FILE__, __LINE__);
	assert(iterator->mMagicNumber == kArrayListIteratorMagicNumber);

	assert(iterator->mIsInList <= 1);

	/* List must always be valid. */
	adt_assert_valid_ptr((AdtPtr) header, "", __FILE__, __LINE__);

	/* Current position must be nil or a valid node ptr. */
	assert(iterator->mCurrent < header->mCurrentSize);

	adt_assert_valid_ptr((AdtPtr) header->mArray, "", __FILE__, __LINE__);

	if (iterator->mIsInList)
	{
		adt_assert_valid_ptr(header->mArray[iterator->mCurrent], "", __FILE__, __LINE__);
	}

	for (index = header->mItems; index < header->mCurrentSize; index++)
	{
		assert(header->mArray[index] == NULL);
	}
}
#endif /* Debugging code. */


static void
double_array(ArrayListHeaderPtr header, unsigned int new_entry_index, AdtPtr new_item)
{
    AdtPtr* new_array = NULL;
	unsigned int source_index;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	/* Get the new size for the array. */
	header->mCurrentSize = header->mCurrentSize * kExpansionFactor;

	/* Allocate the new array. */
	new_array = (AdtPtr*) ADT_XALLOCATE(header->mCurrentSize * sizeof(AdtPtr));
	if (new_array)
	{
		unsigned int target_index = 0;

		/* Copy the elements across, zeroing the new positions. */
		for (source_index = 0; source_index < header->mItems; source_index++)
		{
			if (source_index == new_entry_index)
			{
				/* Put the new item there. */
				new_array[target_index] = new_item;

				/* Put the item from the old array in the next position. */
				target_index++;
				new_array[target_index] = header->mArray[source_index];
			}
			else
			{
				new_array[target_index] = header->mArray[source_index];
			}

			target_index++;
		}
		
		/* If the item was being added at the end of the list, it won't have been added yet. */
		if (new_entry_index == header->mItems)
		{
			new_array[new_entry_index] = new_item;
		}
		header->mItems++;

		for (target_index = header->mItems; target_index < header->mCurrentSize; target_index++)
		{
			new_array[target_index] = NULL;
		}
	}

	header->mArray = (AdtPtr*) adt_dispose_ptr(header->mArray);
	header->mArray = new_array;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */
}


static ArrayIteratorHeaderPtr
new_array_iterator(ArrayListHeaderPtr header)
{
	ArrayIteratorHeaderPtr itor = (ArrayIteratorHeaderPtr) ADT_XALLOCATE(sizeof(ArrayIteratorHeader));
	if (itor == NULL)
	{
		return NULL;
	}

	itor->mList = header;
	itor->mIsInList = 0;
	itor->mCurrent = 0;
	itor->mMagicNumber = kArrayListIteratorMagicNumber;

#ifndef NDEBUG
	verify_iterator(itor);
#endif /* Debugging code. */

	return itor;
}



/* Implementation of array list common routines. */
ArrayListPtr
adt_array_list_init(DisposeRoutine disposer)
{
	unsigned int index;
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) ADT_XALLOCATE(sizeof(ArrayListHeader));

	if (header == NULL)
	{
		return NULL;
	}

	header->mCurrentSize = kDefaultListArraySize;
	header->mArray = (AdtPtr*) ADT_XALLOCATE(header->mCurrentSize * sizeof(AdtPtr));
	if (header->mArray == NULL)
	{
		header = adt_dispose_ptr(header);
		return NULL;
	}

	for (index = 0; index < header->mCurrentSize; index++)
	{
		header->mArray[index] = NULL;
	}

	header->mDisposer = disposer;
	header->mItems = 0;
	header->mMagicNumber = kArrayListMagicNumber;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	return (ArrayListPtr) header;
}


ArrayListPtr
adt_array_list_dispose(ArrayListPtr list)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return NULL;
	}

#ifndef NDEBUG
	if (header->mItems && (header->mDisposer == NULL))
	{
		adt_assert_code(0, "Possible memory leak: non-empty list disposed", __FILE__, __LINE__);
	}
#endif /* Debugging code. */

	if (header->mArray)
	{
		if (header->mDisposer)
		{
			unsigned int index;
			for (index = 0; index < header->mItems; index++)
			{
				if (header->mArray[index])
				{
					header->mDisposer(header->mArray[index]);
				}
			}
		}

		(void) adt_dispose_ptr(header->mArray);
	}

	(void) adt_dispose_ptr(header);
	return NULL;
}


unsigned int
adt_array_list_is_empty(ArrayListPtr list)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (valid_header(header) && header->mItems)
	{
		return 0;
	}

	return 1;
}


void
adt_array_list_make_empty(ArrayListPtr list)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return;
	}

	if (header->mArray)
	{
		unsigned int index;
		for (index = header->mItems; index != 0; index--)
		{
			if ((header->mDisposer) && (header->mArray[index - 1]))
			{
				header->mDisposer(header->mArray[index - 1]);
			}

			header->mArray[index - 1] = NULL;
		}
	}

	header->mItems = 0;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */
}


#if ADT_MEMORY_DEBUG
void
adt_array_list_note_ptrs(ArrayListPtr list)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	adt_memory_note_ptr(header);
	adt_memory_note_ptr(header->mArray);
}
#endif /* ADT_MEMORY_DEBUG */


unsigned int
adt_array_list_items(ArrayListPtr list)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		return header->mItems;
	}

	return 0;
}

/*
 * list index is 1-based 
 */
AdtPtr
adt_array_list_get_indexed_item(ArrayListPtr list, unsigned int index)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	assert(0 < index); /* List indices are one-based. */
	assert(index < header->mItems);

	if (valid_header(header) && (index > 0))
	{
		return header->mArray[index - 1];
	}

	return NULL;
}

#if 0
/*
 * list is 0-based 
 */
AdtPtr
adt_array_list_get_indexed_item(ArrayListPtr list, unsigned int index)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	assert(0 <= index); /* List indices are zero-based. */
	assert(index < header->mItems);

	if (valid_header(header) && (index < header->mItems))
	{
		return header->mArray[index];
	}

	return NULL;
}
#endif

/* Implementation of array list plain interface routines. */
unsigned int
adt_array_list_add_first(ArrayListPtr list, AdtPtr data)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return 0;
	}

	if (header->mItems == header->mCurrentSize)
	{
		double_array(header, 0, data);
	}
	else
	{
		unsigned int index;
		for (index = header->mItems; index != 0; index--)
		{
			header->mArray[index] = header->mArray[index - 1];
		}

		header->mArray[0] = data;
		header->mItems++;
	}

	return 1;
}


unsigned int
adt_array_list_add_last(ArrayListPtr list, AdtPtr data)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return 0;
	}

	if (header->mItems == header->mCurrentSize)
	{
		double_array(header, header->mItems, data);
	}
	else
	{
		header->mArray[header->mItems] = data;
		header->mItems++;
	}

	return 1;
}

/* XXX/JNM: This is a flawed routine, it places several restrictions on
 * how we can add elements. Lack of time.
 */
unsigned int 
adt_array_list_add_at(ArrayListPtr list, AdtPtr data, unsigned int index )
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;
	int x;
    int err = 0;
	
#ifndef NDEBUG
	verify_header(header);
#endif
	
	if (!valid_header(header))
	{
		return err;
	}
	
    /*
    if( index > header->mItems )
	{
		return 0;
	}
    */

    /* XXX/JNM: We restrict adding elements to the array if 
     * adding it will require more then 1 doubling of the array. 
     */
	if (header->mItems == header->mCurrentSize)
	{
        if (index <= header->mCurrentSize * kExpansionFactor)
        {
            double_array(header, header->mItems, data);
        }
        else
        {
            return err;
        }
    }

    /* Special case for if we add at the start and at the end */
    if (header->mItems == 0 )
    {
        err = adt_array_list_add_first((ArrayListPtr)header, data);
    }
    else if (header->mItems == index)
    {
        err = adt_array_list_add_last((ArrayListPtr)header, data);
    }
    else
    {
        /* If we are inserting between the start and end elements, 
         * then we need to move items over to make room. */
        if (index < header->mItems )
        {
            /* Start from the end and start shifting elements over until
        	   we find the index we want to insert. */
        	for( x = header->mItems; x > 0; x-- )
    	    {
	        	header->mArray[x] = header->mArray[x - 1];
		        if(( x - 1 ) == index )
        		{   
	        		header->mArray[x - 1] = data;
		        	break;
    		    }
        	}

	        header->mItems++;
        }
        else
        {
            /* Greater then items, so insert directly, we assume that
             * the item can fit, i.e. index < mCurrenSize. We have to 
             * increase the item count by the difference between 
             * the index and the current item count. */
            header->mArray[index] = data;
            
            header->mItems+=(index-(header->mItems))+1;
        }
    }
	
	
#ifndef NDEBUG
	 verify_header(header);
#endif
	
	return 0;
}

#if 0
/* XXX: This is a flawed routine, it places several restrictions on
 * how we can add elements. Lack of time.
 */
unsigned int 
adt_array_list_add_at(ArrayListPtr list, AdtPtr data, unsigned int index )
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;
	int x;
	
#ifndef NDEBUG
	verify_header(header);
#endif
	
	if (!valid_header(header))
	{
		return 0;
	}
	
	if( index >= header->mItems )
	{
		return 0;
	}
	
	if (header->mItems == header->mCurrentSize)
	{
		double_array(header, header->mItems, data);
	}
	
	/*	Start from the end and start shifting elements over until
		we find the index we want to insert. */
	for( x = header->mItems; x > 0; x-- )
	{
		header->mArray[x] = header->mArray[x - 1];
		if(( x - 1 ) == index )
		{
			header->mArray[x - 1] = data;
			break;
		}
	}
	
	header->mItems++;
	
#ifndef NDEBUG
	 verify_header(header);
#endif
	
	return 0;
}
#endif

AdtPtr
adt_array_list_get_first(ArrayListPtr list)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		return header->mArray[0];
	}

	return NULL;
}


AdtPtr
adt_array_list_get_last(ArrayListPtr list)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		return header->mArray[header->mItems - 1];
	}

	return NULL;
}


void
adt_array_list_remove(ArrayListPtr list, AdtPtr data)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;
	unsigned int index;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return;
	}

	for (index = 0; index < header->mItems; index++)
	{
		if (header->mArray[index] == data)
		{
			unsigned int replace_index;
			for (replace_index = index; replace_index < header->mItems - 1; replace_index++)
			{
				header->mArray[replace_index] = header->mArray[replace_index + 1];
			}

			header->mItems--;
			header->mArray[header->mItems] = NULL;
			return;
		}
	}
}


AdtPtr
adt_array_list_remove_at(ArrayListPtr list, unsigned int index)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;
	AdtPtr data = NULL;
	unsigned int replace_index;

#ifndef NDEBUG
	verify_header(header);
	adt_assert_code(index < header->mItems, "", __FILE__, __LINE__);
#endif /* Debugging code. */

	if ((!valid_header(header)) || (index >= header->mItems))
	{
		return NULL;
	}

	data = header->mArray[index];
	for (replace_index = index; replace_index < header->mItems - 1; replace_index++)
	{
		header->mArray[replace_index] = header->mArray[replace_index + 1];
	}

	header->mItems--;
	header->mArray[header->mItems] = NULL;

	return data;
}


AdtPtr
adt_array_list_remove_first(ArrayListPtr list)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;
	unsigned int replace_index;
	AdtPtr data;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if ((!valid_header(header)) || (header->mItems == 0))
	{
		return NULL;
	}

	data = header->mArray[0];
	for (replace_index = 0; replace_index < header->mItems - 1; replace_index++)
	{
		header->mArray[replace_index] = header->mArray[replace_index + 1];
	}

	header->mItems--;
	header->mArray[header->mItems] = NULL;

	return data;
}


AdtPtr
adt_array_list_remove_last(ArrayListPtr list)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if ((valid_header(header)) && (header->mItems))
	{
		AdtPtr data = header->mArray[header->mItems - 1];

		header->mItems--;
		header->mArray[header->mItems] = NULL;

		return data;
	}

	return NULL;
}


unsigned int
adt_array_list_contains(ArrayListPtr list, AdtPtr data)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		unsigned int index;
		for (index = 0; index < header->mItems; index++)
		{
			if (header->mArray[index] == data)
			{
				return 1;
			}
		}
	}

	return 0;
}


/* Implementation of array list iterator interface routines. */
IteratorPtr
adt_array_list_get_iterator(ArrayListPtr list)
{
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		return (IteratorPtr) new_array_iterator(header);
	}

	return NULL;
}


/* Implementation of array list iterator routines. */
IteratorPtr
adt_array_list_iterator_dispose(IteratorPtr itor)
{
	ArrayIteratorHeaderPtr iterator = (ArrayIteratorHeaderPtr) itor;

	/*
	 * Don't check consistency on entry, because it's possible that the list may
	 * have been affected by another iterator and mCurrent is invalid.
	 */

	if (valid_iterator(iterator))
	{
	#ifndef NDEBUG
		ArrayListHeaderPtr header = iterator->mList;
	
		verify_header(header);
	#endif /* Debugging code. */
		
		iterator = (ArrayIteratorHeaderPtr) adt_dispose_ptr((AdtPtr) iterator);
	}

	return NULL;
}


unsigned int
adt_array_list_iterator_is_in_list(IteratorPtr itor)
{
	ArrayIteratorHeaderPtr iterator = (ArrayIteratorHeaderPtr) itor;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (valid_iterator(iterator))
	{
		return iterator->mIsInList;
	}

	return 0;
}


void
adt_array_list_iterator_zeroth(IteratorPtr itor)
{
	ArrayIteratorHeaderPtr iterator = (ArrayIteratorHeaderPtr) itor;

	/*
	 * Don't check consistency on entry, because it's possible that the list may
	 * have been affected by another iterator and mCurrent is invalid.
	 * However, the list must be consistent before exiting.
	 */

	if (valid_iterator(iterator))
	{
	#ifndef NDEBUG
		ArrayListHeaderPtr header = iterator->mList;
	
		verify_header(header);
	#endif /* Debugging code. */
		
		iterator->mIsInList = 0;
		iterator->mCurrent = 0;
	}

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */
}


void
adt_array_list_iterator_first(IteratorPtr itor)
{
	ArrayIteratorHeaderPtr iterator = (ArrayIteratorHeaderPtr) itor;

	/*
	 * Don't check consistency on entry, because it's possible that the list may
	 * have been affected by another iterator and mCurrent is invalid.
	 * However, the list must be consistent before exiting.
	 */

	if (valid_iterator(iterator))
	{
		ArrayListHeaderPtr header = iterator->mList;
	
	#ifndef NDEBUG
		verify_header(header);
	#endif /* Debugging code. */
		
		/* Set the current position to the head of the list. */
		if (0 == header->mItems)
		{
			iterator->mIsInList = 0;
		}
		else
		{
			iterator->mIsInList = 1;
		}
		iterator->mCurrent = 0;
	}

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */
}


void
adt_array_list_iterator_last(IteratorPtr itor)
{
	ArrayIteratorHeaderPtr iterator = (ArrayIteratorHeaderPtr) itor;

	/*
	 * Don't check consistency on entry, because it's possible that the list may
	 * have been affected by another iterator and mCurrent is invalid.
	 * However, the list must be consistent before exiting.
	 */

	if (valid_iterator(iterator))
	{
		ArrayListHeaderPtr header = iterator->mList;
	
	#ifndef NDEBUG
		verify_header(header);
	#endif /* Debugging code. */
		
		/* Set the current position to the head of the list. */
		if (0 == header->mItems)
		{
			iterator->mIsInList = 0;
		}
		else
		{
			iterator->mIsInList = 1;
		}

		iterator->mCurrent = (header->mItems - 1);
	}

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */
}

unsigned int 
adt_array_list_iterator_is_equal( IteratorPtr itor1, IteratorPtr itor2 )
{
	ArrayIteratorHeaderPtr iterator1 = (ArrayIteratorHeaderPtr) itor1;
	ArrayIteratorHeaderPtr iterator2 = (ArrayIteratorHeaderPtr) itor2;

#ifndef NDEBUG
	verify_iterator(iterator1);
	verify_iterator(iterator2);
#endif /* Debugging code. */

	/*
	 * Don't check consistency on entry, because it's possible that the list may
	 * have been affected by another iterator and mCurrent is invalid.
	 * However, the list must be consistent before exiting.
	 */
	if (valid_iterator(iterator1) && valid_iterator(iterator2))
	{
		return iterator1->mCurrent == iterator2->mCurrent;
	}
	
	return 0;
}

void
adt_array_list_iterator_advance(IteratorPtr itor)
{
	ArrayIteratorHeaderPtr iterator = (ArrayIteratorHeaderPtr) itor;
	ArrayListHeaderPtr header;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (!valid_iterator(iterator))
	{
		return;
	}

	if (!iterator->mIsInList)
	{
		return;
	}

	header = iterator->mList;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return;
	}

	iterator->mCurrent++;
	if (iterator->mCurrent == header->mItems)
	{
		iterator->mIsInList = 0;
		iterator->mCurrent = 0;
	}
}

/*	
	Similar to standard advance, but we only advance as far as the 2nd
	iterator supplied. Otherwise we terminate in a similar way to the 
	standard advance function call.
*/
unsigned char
adt_array_list_iterator_advance_to( IteratorPtr itor1, IteratorPtr itor2 )
{
	ArrayIteratorHeaderPtr iterator1 = (ArrayIteratorHeaderPtr) itor1;
	ArrayIteratorHeaderPtr iterator2 = (ArrayIteratorHeaderPtr) itor2;
	ArrayListHeaderPtr header;
	ArrayListHeaderPtr header2;

#ifndef NDEBUG
	verify_iterator( iterator1 );
#endif /* Debugging code. */
	
	if( !valid_iterator( iterator1 ))
	{
		return 0;
	}
	
	if( !iterator1->mIsInList )
	{
		return 0;
	}
	
	header = iterator1->mList;
	
#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */
	
	if (!valid_header(header))
	{
		return 0;
	}
	
	/*	If the 2nd iterator supplied is valid, then we use it to determine
		if we should stop iterating. */
	if( valid_iterator( iterator2 ) && 
		iterator2->mIsInList )
	{
		header2 = iterator2->mList;
		
#ifndef NDEBUG
		verify_header( header2 );
		
		/*	If we don't have a valid iterator2, perform the default
			iterator advance.
		*/
		if( !valid_header( header2 ))
		{
			return default_advance( iterator1 );
		}
#endif
		
		/*	We have a valid target iterator, check iterator1 against it,
			that is, test to see if iterator1 is equal to iterator2. */
		if( iterator1->mCurrent == iterator2->mCurrent )
		{
			return 0;
		}
	}
	
	return default_advance( iterator1 );
}

static inline unsigned int
default_advance( ArrayIteratorHeaderPtr iterator )
{
	iterator->mCurrent++;
	if( iterator->mCurrent == iterator->mList->mItems )
	{
		iterator->mIsInList = 0;
		iterator->mCurrent = 0;
	}
	
	return 1;
}

void
adt_array_list_iterator_retreat(IteratorPtr itor)
{
	ArrayIteratorHeaderPtr iterator = (ArrayIteratorHeaderPtr) itor;
	ArrayListHeaderPtr header;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (!valid_iterator(iterator))
	{
		return;
	}

	if (!iterator->mIsInList)
	{
		return;
	}

	header = iterator->mList;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return;
	}

	if (iterator->mCurrent == 0 || iterator->mCurrent >= header->mItems)
	{
		iterator->mIsInList = 0;
		iterator->mCurrent = 0;
	}
	else
	{
		iterator->mIsInList = 1;
		iterator->mCurrent--;
	}
}


AdtPtr
adt_array_list_iterator_retrieve(IteratorPtr itor)
{
	ArrayIteratorHeaderPtr iterator = (ArrayIteratorHeaderPtr) itor;
ArrayListHeaderPtr header;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (!valid_iterator(iterator))
	{
		return NULL;
	}

	if (!iterator->mIsInList)
	{
		return NULL;
	}

	header = iterator->mList;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	return header->mArray[iterator->mCurrent];
}


IteratorPtr 
adt_array_list_iterator_equate(IteratorPtr itor1, IteratorPtr itor2)
{
	ArrayIteratorHeaderPtr iterator1 = (ArrayIteratorHeaderPtr) itor1;
	ArrayIteratorHeaderPtr iterator2 = (ArrayIteratorHeaderPtr) itor2;
	
#ifndef NDEBUG
	verify_iterator(iterator1);
	verify_iterator(iterator2);
#endif /* Debugging code. */
	
	if(!valid_iterator(iterator1) || !valid_iterator(iterator2))
	{
		return NULL;
	}
	
	if(!iterator1->mIsInList || !iterator2->mIsInList )
	{
		return NULL;
	}
	
	iterator1->mCurrent = iterator2->mCurrent;
	
	return itor1;
}

IteratorPtr 
adt_array_list_iterator_duplicate(IteratorPtr itor)
{
	ArrayIteratorHeaderPtr iterator_new = NULL;
	ArrayIteratorHeaderPtr iterator = (ArrayIteratorHeaderPtr) itor;
	ArrayListHeaderPtr header = (ArrayListHeaderPtr) iterator->mList;

#ifndef NDEBUG
	verify_iterator(iterator);
	verify_header(header);
#endif /* Debugging code. */

	if (valid_iterator(iterator) && valid_header(header))
	{
		iterator_new = new_array_iterator(header);
		iterator_new->mCurrent = iterator->mCurrent;
		return (IteratorPtr) iterator_new;
	}

	return NULL;
}

unsigned int
adt_array_list_iterator_find(IteratorPtr itor, AdtPtr data)
{
	ArrayIteratorHeaderPtr iterator = (ArrayIteratorHeaderPtr) itor;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (valid_iterator(iterator))
	{
		ArrayListHeaderPtr header = iterator->mList;
		unsigned int index;
	
	#ifndef NDEBUG
		verify_header(header);
	#endif /* Debugging code. */
	
		if (header->mItems == 0)
		{
			/* List is empty. */
			return 0;
		}
	
		for (index = 0; index < header->mItems; index++)
		{
			if (header->mArray[index] == data)
			{
				return 1;
			}
		}
	}

	return 0;
}


/* Add after the current position and advance current position. */
unsigned int
adt_array_list_iterator_add(IteratorPtr itor, AdtPtr data)
{
	ArrayIteratorHeaderPtr iterator = (ArrayIteratorHeaderPtr) itor;
	ArrayListHeaderPtr header;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (!valid_iterator(iterator))
	{
		return 0;
	}

	header = iterator->mList;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!iterator->mIsInList)
	{
		/* Insert at front. */
		iterator->mIsInList = 1;
		iterator->mCurrent = 0;
	}
	else
	{
		/* Insert later on. */
		iterator->mCurrent++;
	}

	if (header->mItems == 0)
	{
		header->mArray[0] = data;
		header->mItems++;
	}
	else if (header->mItems == header->mCurrentSize)
	{
		/* Enlarge the array as the new item is added. */
		double_array(header, iterator->mCurrent, data);

		if (header->mArray == NULL)
		{
			/* Failed to allocate an array, probably out of memory. */
			return 0;
		}
	}
	else
	{
		unsigned int index;

		/* Shift items down to make room for new item. */
		for (index = header->mItems; index != iterator->mCurrent; index--)
		{
			header->mArray[index] = header->mArray[index - 1];

		}

		header->mArray[iterator->mCurrent] = data;
		header->mItems++;
	}

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	return 1;
}


void
adt_array_list_iterator_remove(IteratorPtr itor)
{
	ArrayIteratorHeaderPtr iterator = (ArrayIteratorHeaderPtr) itor;
	ArrayListHeaderPtr header;
	unsigned int index;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (!valid_iterator(iterator))
	{
		return;
	}

	header = iterator->mList;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if ((header->mItems == 0) || (!iterator->mIsInList))
	{
		return;
	}

	/* Dispose of the item to be removed. */
	if (header->mDisposer)
	{
		header->mDisposer(header->mArray[iterator->mCurrent]);
	}

	/* Shift the other items up. */
	for (index = iterator->mCurrent; index < header->mItems - 1; index++)
	{
		header->mArray[index] = header->mArray[index + 1];
	}

	/* Delete the (duplicate) last item. */
	header->mItems--;
	header->mArray[header->mItems] = NULL;

#ifdef DEBUG_VERSION
	verify_iterator(iterator);
#endif /* Debugging code. */
}


void
adt_array_list_iterator_swap(IteratorPtr itor1, IteratorPtr itor2)
{
	ArrayIteratorHeaderPtr iterator1 = (ArrayIteratorHeaderPtr) itor1;
	ArrayIteratorHeaderPtr iterator2 = (ArrayIteratorHeaderPtr) itor2;
	ArrayListHeaderPtr header;
	AdtPtr tmp_data;
	
	
	header = iterator1->mList;
	
#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */
	
	if ( iterator1->mList != iterator2->mList )
	{
		return;
	}
	
	
	#ifndef NDEBUG
	verify_iterator(iterator1);
	verify_iterator(iterator2);
#endif /* Debugging code. */
	
	
	if(!valid_iterator(iterator1) || !valid_iterator(iterator2))
	{
		return;
	}
	
	if(!iterator1->mIsInList || !iterator2->mIsInList )
	{
		return;
	}
	
	
	tmp_data = header->mArray[iterator1->mCurrent];
	header->mArray[iterator1->mCurrent] = header->mArray[iterator2->mCurrent];
	header->mArray[iterator2->mCurrent] = tmp_data;
	
}
