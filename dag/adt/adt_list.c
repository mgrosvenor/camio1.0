/* File header. */
#include "adt_list.h"

/* ADT headers. */
#include "adt_adler32.h"
#include "adt_array_list.h"
#include "adt_debugging.h"
#include "adt_dynamic_list.h"
#include "adt_list_common.h"
#include "adt_magic_numbers.h"
#include "adt_memory.h"
#include "adt_memory_pool.h"

/* C Standard Library headers. */
#include <assert.h>
#include <stdlib.h>


typedef struct ListOperations
{
#ifndef NDEBUG
	uintptr_t mAdler32Checksum;
#endif /* Debugging code. */
	list_dispose_routine mDispose;
	list_is_empty_routine mIsEmpty;
	list_items_routine mItems;
	list_make_empty_routine mMakeEmpty;

#if ADT_MEMORY_DEBUG
	list_note_ptrs_routine mNotePtrs;
#endif /* ADT_MEMORY_DEBUG */

	list_add_first_routine mAddFirst;
	list_add_last_routine mAddLast;
	list_add_at_routine mAddAt;
	list_get_first_routine mGetFirst;
	list_get_last_routine mGetLast;
	list_remove_routine mRemove;
	list_remove_at_routine mRemoveAt;
	list_remove_first_routine mRemoveFirst;
	list_remove_last_routine mRemoveLast;
	list_contains_routine mContains;
	list_retrieve_routine mRetrieve;
	list_get_iterator_routine mGetIterator;
} ListOperations, * ListOperationsPtr;


typedef struct IteratorOperations
{
#ifndef NDEBUG
	uintptr_t mAdler32Checksum;
#endif /* Debugging code. */

	iterator_dispose_routine mIteratorDispose;
	iterator_is_in_list_routine mIteratorIsInList;
	iterator_zeroth_routine mIteratorZeroth;
	iterator_first_routine mIteratorFirst;
	iterator_last_routine mIteratorLast;
	iterator_is_equal_routine mIteratorIsEqual;
	iterator_advance_routine mIteratorAdvance;
	iterator_advance_to_routine mIteratorAdvanceTo;
	iterator_retreat_routine mIteratorRetreat;
	iterator_retrieve_routine mIteratorRetrieve;
	iterator_equate_routine mIteratorEquate;
	iterator_find_routine mIteratorFind;
	iterator_add_routine mIteratorAdd;
	iterator_remove_routine mIteratorRemove;
	iterator_swap_routine mIteratorSwap;
	iterator_duplicate_routine mIteratorDuplicate;

} IteratorOperations, * IteratorOperationsPtr;


static ListOperations uDynamicList =
{
#ifndef NDEBUG
	0,
#endif /* Debugging code. */
	(list_dispose_routine) adt_dynamic_list_dispose,
	(list_is_empty_routine) adt_dynamic_list_is_empty,
	(list_items_routine) adt_dynamic_list_items,
	(list_make_empty_routine) adt_dynamic_list_make_empty,

#if ADT_MEMORY_DEBUG
	(list_note_ptrs_routine) adt_dynamic_list_note_ptrs,
#endif /* ADT_MEMORY_DEBUG */

	(list_add_first_routine) adt_dynamic_list_add_first,
	(list_add_last_routine) adt_dynamic_list_add_last,
	(list_add_at_routine) adt_dynamic_list_add_at,
	(list_get_first_routine) adt_dynamic_list_get_first,
	(list_get_last_routine) adt_dynamic_list_get_last,
	(list_remove_routine) adt_dynamic_list_remove,
	(list_remove_at_routine) adt_dynamic_list_remove_at,
	(list_remove_first_routine) adt_dynamic_list_remove_first,
	(list_remove_last_routine) adt_dynamic_list_remove_last,
	(list_contains_routine) adt_dynamic_list_contains,
	(list_retrieve_routine) adt_dynamic_list_get_indexed_item,
	(list_get_iterator_routine) NULL,
};


static ListOperations uArrayList =
{
#ifndef NDEBUG
	0,
#endif /* Debugging code. */
	(list_dispose_routine) adt_array_list_dispose,
	(list_is_empty_routine) adt_array_list_is_empty,
	(list_items_routine) adt_array_list_items,
	(list_make_empty_routine) adt_array_list_make_empty,

#if ADT_MEMORY_DEBUG
	(list_note_ptrs_routine) adt_array_list_note_ptrs,
#endif /* ADT_MEMORY_DEBUG */

	(list_add_first_routine) adt_array_list_add_first,
	(list_add_last_routine) adt_array_list_add_last,
	(list_add_at_routine) adt_array_list_add_at,
	(list_get_first_routine) adt_array_list_get_first,
	(list_get_last_routine) adt_array_list_get_last,
	(list_remove_routine) adt_array_list_remove,
	(list_remove_at_routine) adt_array_list_remove_at,
	(list_remove_first_routine) adt_array_list_remove_first,
	(list_remove_last_routine) adt_array_list_remove_last,
	(list_contains_routine) adt_array_list_contains,
	(list_retrieve_routine) adt_array_list_get_indexed_item,
	(list_get_iterator_routine) NULL,
};


static ListOperations uDynamicIteratorList =
{
#ifndef NDEBUG
	0,
#endif /* Debugging code. */
	(list_dispose_routine) adt_dynamic_list_dispose,
	(list_is_empty_routine) adt_dynamic_list_is_empty,
	(list_items_routine) adt_dynamic_list_items,
	(list_make_empty_routine) adt_dynamic_list_make_empty,

#if ADT_MEMORY_DEBUG
	(list_note_ptrs_routine) adt_dynamic_list_note_ptrs,
#endif /* ADT_MEMORY_DEBUG */

	(list_add_first_routine) NULL,
	(list_add_last_routine) NULL,
	(list_add_at_routine) NULL,
	(list_get_first_routine) NULL,
	(list_get_last_routine) NULL,
	(list_remove_routine) NULL,
	(list_remove_at_routine) NULL,
	(list_remove_first_routine) NULL,
	(list_remove_last_routine) NULL,
	(list_contains_routine) NULL,
	(list_retrieve_routine) adt_dynamic_list_get_indexed_item,
	(list_get_iterator_routine) adt_dynamic_list_get_iterator
};


static IteratorOperations uDynamicIterator =
{
#ifndef NDEBUG
	0,
#endif /* Debugging code. */

	(iterator_dispose_routine) adt_dynamic_list_iterator_dispose,
	(iterator_is_in_list_routine) adt_dynamic_list_iterator_is_in_list,
	(iterator_zeroth_routine) adt_dynamic_list_iterator_zeroth,
	(iterator_first_routine) adt_dynamic_list_iterator_first,
	(iterator_last_routine) adt_dynamic_list_iterator_last,
	(iterator_is_equal_routine) adt_dynamic_list_iterator_is_equal,
	(iterator_advance_routine) adt_dynamic_list_iterator_advance,
	(iterator_advance_to_routine) adt_dynamic_list_iterator_advance_to,
	(iterator_retreat_routine) adt_dynamic_list_iterator_retreat,
	(iterator_retrieve_routine) adt_dynamic_list_iterator_retrieve,
	(iterator_equate_routine) adt_dynamic_list_iterator_equate,
	(iterator_find_routine) adt_dynamic_list_iterator_find,
	(iterator_add_routine) adt_dynamic_list_iterator_add,
	(iterator_remove_routine) adt_dynamic_list_iterator_remove,
	(iterator_swap_routine) adt_dynamic_list_iterator_swap,
	(iterator_duplicate_routine) adt_dynamic_list_iterator_duplicate
};


static ListOperations uArrayIteratorList =
{
#ifndef NDEBUG
	0,
#endif /* Debugging code. */
	(list_dispose_routine) adt_array_list_dispose,
	(list_is_empty_routine) adt_array_list_is_empty,
	(list_items_routine) adt_array_list_items,
	(list_make_empty_routine) adt_array_list_make_empty,

#if ADT_MEMORY_DEBUG
	(list_note_ptrs_routine) adt_array_list_note_ptrs,
#endif /* ADT_MEMORY_DEBUG */

	(list_add_first_routine) NULL,
	(list_add_last_routine) NULL,
	(list_add_at_routine) NULL,
	(list_get_first_routine) NULL,
	(list_get_last_routine) NULL,
	(list_remove_routine) NULL,
	(list_remove_at_routine) NULL,
	(list_remove_first_routine) NULL,
	(list_remove_last_routine) NULL,
	(list_contains_routine) NULL,
	(list_retrieve_routine) adt_array_list_get_indexed_item,
	(list_get_iterator_routine) adt_array_list_get_iterator
};


static IteratorOperations uArrayIterator =
{
#ifndef NDEBUG
	0,
#endif /* Debugging code. */

	(iterator_dispose_routine) adt_array_list_iterator_dispose,
	(iterator_is_in_list_routine) adt_array_list_iterator_is_in_list,
	(iterator_zeroth_routine) adt_array_list_iterator_zeroth,
	(iterator_first_routine) adt_array_list_iterator_first,
	(iterator_last_routine) adt_array_list_iterator_last,
	(iterator_is_equal_routine) adt_array_list_iterator_is_equal,
	(iterator_advance_routine) adt_array_list_iterator_advance,
	(iterator_advance_to_routine) adt_array_list_iterator_advance_to,
	(iterator_retreat_routine) adt_array_list_iterator_retreat,
	(iterator_retrieve_routine) adt_array_list_iterator_retrieve,
	(iterator_equate_routine) adt_array_list_iterator_equate,
	(iterator_find_routine) adt_array_list_iterator_find,
	(iterator_add_routine) adt_array_list_iterator_add,
	(iterator_remove_routine) adt_array_list_iterator_remove,
	(iterator_swap_routine) adt_array_list_iterator_swap,
	(iterator_duplicate_routine) adt_array_list_iterator_duplicate
};


typedef struct ListHeader ListHeader;
typedef ListHeader* ListHeaderPtr;

struct ListHeader
{
	adt_magic_number_t mMagicNumber;
	AdtPtr mList;
	ListOperationsPtr mListOperations;
	IteratorOperationsPtr mIteratorOperations;
};


typedef struct IteratorHeader IteratorHeader;
typedef IteratorHeader* IteratorHeaderPtr;

struct IteratorHeader
{
	adt_magic_number_t mMagicNumber;
	AdtPtr mIterator;
	IteratorOperationsPtr mIteratorOperations;
};


/* Internal routines */
static unsigned int valid_list_header(ListHeaderPtr header);
static unsigned int valid_iterator_header(IteratorHeaderPtr header);
#ifndef NDEBUG
static void verify_list(ListHeaderPtr header);
static void verify_iterator(IteratorHeaderPtr header);
#endif /* Debugging code. */


/* Implementation of internal routines. */
static unsigned int
valid_list_header(ListHeaderPtr header)
{
	if (header && (header->mMagicNumber == kListMagicNumber))
	{
		return 1;
	}

	return 0;
}


static unsigned int
valid_iterator_header(IteratorHeaderPtr header)
{
	if (header && (header->mMagicNumber == kIteratorMagicNumber))
	{
		return 1;
	}

	return 0;
}


#ifndef NDEBUG
static void
verify_list(ListHeaderPtr header)
{
	/* Compute and compare checksum over function pointers. */
	uint32_t checksum = adt_adler32((AdtPtr) &header->mListOperations->mDispose, sizeof(ListOperations) - sizeof(uintptr_t));

	assert(checksum == header->mListOperations->mAdler32Checksum);

	if (header->mIteratorOperations)
	{
		checksum = adt_adler32((AdtPtr) &header->mIteratorOperations->mIteratorDispose, sizeof(IteratorOperations) - sizeof(uintptr_t));
		assert(checksum == header->mIteratorOperations->mAdler32Checksum);
	}
}


static void
verify_iterator(IteratorHeaderPtr header)
{
	/* Compute and compare checksum over function pointers. */
	uint32_t checksum = adt_adler32((AdtPtr) &header->mIteratorOperations->mIteratorDispose, sizeof(IteratorOperations) - sizeof(uintptr_t));

	assert(checksum == header->mIteratorOperations->mAdler32Checksum);
}
#endif /* Debugging code. */


/* List API. */
ListPtr
adt_list_init(unsigned int list_interface, unsigned int representation, DisposeRoutine disposer)
{
	ListHeaderPtr header = (ListHeaderPtr) ADT_XALLOCATE(sizeof(ListHeader));
	if (header == NULL)
	{
		return NULL;
	}

#ifndef NDEBUG
	/* Initialise checksums. */
	if (uDynamicList.mAdler32Checksum == 0)
	{
		uDynamicList.mAdler32Checksum = adt_adler32((AdtPtr) &uDynamicList.mDispose, sizeof(ListOperations) - sizeof(uintptr_t));
	}

	if (uDynamicIteratorList.mAdler32Checksum == 0)
	{
		uDynamicIteratorList.mAdler32Checksum = adt_adler32((AdtPtr) &uDynamicIteratorList.mDispose, sizeof(ListOperations) - sizeof(uintptr_t));
	}

	if (uDynamicIterator.mAdler32Checksum == 0)
	{
		uDynamicIterator.mAdler32Checksum = adt_adler32((AdtPtr) &uDynamicIterator.mIteratorDispose, sizeof(IteratorOperations) - sizeof(uintptr_t));
	}

	if (uArrayList.mAdler32Checksum == 0)
	{
		uArrayList.mAdler32Checksum = adt_adler32((AdtPtr) &uArrayList.mDispose, sizeof(ListOperations) - sizeof(uintptr_t));
	}

	if (uArrayIteratorList.mAdler32Checksum == 0)
	{
		uArrayIteratorList.mAdler32Checksum = adt_adler32((AdtPtr) &uArrayIteratorList.mDispose, sizeof(ListOperations) - sizeof(uintptr_t));
	}

	if (uArrayIterator.mAdler32Checksum == 0)
	{
		uArrayIterator.mAdler32Checksum = adt_adler32((AdtPtr) &uArrayIterator.mIteratorDispose, sizeof(IteratorOperations) - sizeof(uintptr_t));
	}
#endif /* Debugging code. */

	/* Determine the appropriate list. */
	if ((list_interface == kListInterfacePlain) && (representation == kListRepresentationDynamic))
	{
		header->mList = adt_dynamic_list_init(disposer);
		header->mListOperations = &uDynamicList;
		header->mIteratorOperations = NULL;
	}
	else if ((list_interface == kListInterfaceIterator) && (representation == kListRepresentationDynamic))
	{
		header->mList = adt_dynamic_list_init(disposer);
		header->mListOperations = &uDynamicIteratorList;
		header->mIteratorOperations = &uDynamicIterator;
	}
	else if ((list_interface == kListInterfacePlain) && (representation == kListRepresentationArray))
	{
		header->mList = adt_array_list_init(disposer);
		header->mListOperations = &uArrayList;
		header->mIteratorOperations = NULL;
	}
	else if ((list_interface == kListInterfaceIterator) && (representation == kListRepresentationArray))
	{
		header->mList = adt_array_list_init(disposer);
		header->mListOperations = &uArrayIteratorList;
		header->mIteratorOperations = &uArrayIterator;
	}
	else
	{
		/* Unsupported interface/representation combination. */
		(void) adt_dispose_ptr(header);
		return NULL;
	}

	header->mMagicNumber = kListMagicNumber;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	return (ListPtr) header;
}


ListPtr
adt_list_dispose(ListPtr list)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		(void) header->mListOperations->mDispose(header->mList);
		(void) adt_dispose_ptr(header);
	}

	return NULL;
}


unsigned int
adt_list_is_empty(ListPtr list)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	return header->mListOperations->mIsEmpty(header->mList);
}


void
adt_list_make_empty(ListPtr list)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		header->mListOperations->mMakeEmpty(header->mList);
	}
}


#if ADT_MEMORY_DEBUG
void
adt_list_note_ptrs(ListPtr list)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		adt_memory_note_ptr(header);
		header->mListOperations->mNotePtrs(header->mList);
	}
}
#endif /* ADT_MEMORY_DEBUG */


unsigned int
adt_list_add_first(ListPtr list, AdtPtr data)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		return header->mListOperations->mAddFirst(header->mList, data);
	}

	return 0;
}


unsigned int
adt_list_add_last(ListPtr list, AdtPtr data)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		return header->mListOperations->mAddLast(header->mList, data);
	}

	return 0;
}

unsigned int
adt_list_add_at(ListPtr list, AdtPtr data, unsigned int index )
{
	ListHeaderPtr header = (ListHeaderPtr) list;
	
#ifndef NDEBUG
	verify_list(header);
#endif
	
	if( valid_list_header(header))
	{
		return header->mListOperations->mAddAt(header->mList, data, index );
	}
	
	return 0;
}

AdtPtr
adt_list_get_first(ListPtr list)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		return header->mListOperations->mGetFirst(header->mList);
	}

	return 0;
}


AdtPtr
adt_list_get_last(ListPtr list)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		return header->mListOperations->mGetLast(header->mList);
	}

	return 0;
}


void
adt_list_remove(ListPtr list, AdtPtr data)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		header->mListOperations->mRemove(header->mList, data);
	}
}


AdtPtr
adt_list_remove_at(ListPtr list, unsigned int index)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		return header->mListOperations->mRemoveAt(header->mList, index);
	}

	return NULL;
}


AdtPtr
adt_list_remove_first(ListPtr list)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		return header->mListOperations->mRemoveFirst(header->mList);
	}

	return NULL;
}


AdtPtr
adt_list_remove_last(ListPtr list)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		return header->mListOperations->mRemoveLast(header->mList);
	}

	return NULL;
}


unsigned int
adt_list_contains(ListPtr list, AdtPtr data)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		return header->mListOperations->mContains(header->mList, data);
	}

	return 0;
}


unsigned int
adt_list_items(ListPtr list)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		return header->mListOperations->mItems(header->mList);
	}

	return 0;
}


AdtPtr
adt_list_get_indexed_item(ListPtr list, unsigned int index)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		return header->mListOperations->mRetrieve(header->mList, index);
	}

	return NULL;
}


AdtPtr
adt_list_retrieve(ListPtr list, unsigned int index)
{
	ListHeaderPtr header = (ListHeaderPtr) list;

#ifndef NDEBUG
	verify_list(header);
#endif /* Debugging code. */

	if (valid_list_header(header))
	{
		return header->mListOperations->mRetrieve(header->mList, index);
	}

	return NULL;
}



IteratorPtr
adt_list_get_iterator(ListPtr list)
{
	ListHeaderPtr list_header = (ListHeaderPtr) list;
	IteratorHeaderPtr iterator_header;

#ifndef NDEBUG
	verify_list(list_header);
#endif /* Debugging code. */

	if (!valid_list_header(list_header))
	{
		return NULL;
	}

	if (list_header->mIteratorOperations == NULL)
	{
		/* List does not support an iterator. */
		return NULL;
	}

	iterator_header = ADT_XALLOCATE(sizeof(IteratorHeader));
	if (!iterator_header)
	{
		return NULL;
	}

	iterator_header->mIteratorOperations = list_header->mIteratorOperations;
	iterator_header->mIterator = list_header->mListOperations->mGetIterator(list_header->mList);
	iterator_header->mMagicNumber = kIteratorMagicNumber;

#ifndef NDEBUG
	verify_iterator(iterator_header);
#endif /* Debugging code. */

	return (IteratorPtr) iterator_header;
}



IteratorPtr
adt_iterator_dispose(IteratorPtr iterator)
{
	IteratorHeaderPtr header = (IteratorHeaderPtr) iterator;

#ifndef NDEBUG
	verify_iterator(header);
#endif /* Debugging code. */

	if (valid_iterator_header(header))
	{
		header->mIterator = header->mIteratorOperations->mIteratorDispose(header->mIterator);
		(void) adt_dispose_ptr(header);
	}

	return NULL;
}


unsigned int
adt_iterator_is_in_list(IteratorPtr iterator)
{
	IteratorHeaderPtr header = (IteratorHeaderPtr) iterator;

#ifndef NDEBUG
	verify_iterator(header);
#endif /* Debugging code. */

	if (valid_iterator_header(header))
	{
		return header->mIteratorOperations->mIteratorIsInList(header->mIterator);
	}

	return 0;
}


void
adt_iterator_zeroth(IteratorPtr iterator)
{
	IteratorHeaderPtr header = (IteratorHeaderPtr) iterator;

#ifndef NDEBUG
	verify_iterator(header);
#endif /* Debugging code. */

	if (valid_iterator_header(header))
	{
		header->mIteratorOperations->mIteratorZeroth(header->mIterator);
	}
}


void
adt_iterator_first(IteratorPtr iterator)
{
	IteratorHeaderPtr header = (IteratorHeaderPtr) iterator;

#ifndef NDEBUG
	verify_iterator(header);
#endif /* Debugging code. */

	if (valid_iterator_header(header))
	{
		header->mIteratorOperations->mIteratorFirst(header->mIterator);
	}
}


void
adt_iterator_last(IteratorPtr iterator)
{
	IteratorHeaderPtr header = (IteratorHeaderPtr) iterator;

#ifndef NDEBUG
	verify_iterator(header);
#endif /* Debugging code. */

	if (valid_iterator_header(header))
	{
		header->mIteratorOperations->mIteratorLast(header->mIterator);
	}
}

unsigned int
adt_iterator_is_equal( IteratorPtr itor1, IteratorPtr itor2 )
{
	IteratorHeaderPtr header1 = (IteratorHeaderPtr) itor1;
	IteratorHeaderPtr header2 = (IteratorHeaderPtr) itor2;

#ifndef NDEBUG
	verify_iterator(header1);
	verify_iterator(header2);
#endif /* Debugging code */

	if ( valid_iterator_header(header1) && 
		 valid_iterator_header(header2))
	{	
		return header1->mIteratorOperations->mIteratorIsEqual( header1->mIterator, header2->mIterator );
	}
	
	return 0;
}

void
adt_iterator_advance(IteratorPtr iterator)
{
	IteratorHeaderPtr header = (IteratorHeaderPtr) iterator;

#ifndef NDEBUG
	verify_iterator(header);
#endif /* Debugging code. */

	if (valid_iterator_header(header))
	{
		header->mIteratorOperations->mIteratorAdvance(header->mIterator);
	}
}

unsigned char
adt_iterator_advance_to( IteratorPtr iterator1, IteratorPtr iterator2 )
{
	IteratorHeaderPtr header1 = (IteratorHeaderPtr) iterator1;
	IteratorHeaderPtr header2 = (IteratorHeaderPtr) iterator2;
	
#ifndef NDEBUG
	verify_iterator( header1 );
#endif
	
	/*	If only 1 of the iterators is valid, we call the simple advance 
		function. */
	if( valid_iterator_header( header1 ))
	{
		if( valid_iterator_header( header2 ))
		{
#ifndef NDEBUG
			verify_iterator( header2 );
#endif
			return header1->mIteratorOperations->mIteratorAdvanceTo( header1->mIterator, header2->mIterator );
		}
		
		header1->mIteratorOperations->mIteratorAdvance( header1->mIterator );
		
		return 1;
	}
	
	return 0;
}

void
adt_iterator_retreat(IteratorPtr iterator)
{
	IteratorHeaderPtr header = (IteratorHeaderPtr) iterator;

#ifndef NDEBUG
	verify_iterator(header);
#endif /* Debugging code. */

	if (valid_iterator_header(header))
	{
		header->mIteratorOperations->mIteratorRetreat(header->mIterator);
	}
}

AdtPtr
adt_iterator_retrieve(IteratorPtr iterator)
{
	IteratorHeaderPtr header = (IteratorHeaderPtr) iterator;

#ifndef NDEBUG
	verify_iterator(header);
#endif /* Debugging code. */

	if (valid_iterator_header(header))
	{
		return header->mIteratorOperations->mIteratorRetrieve(header->mIterator);
	}

	return NULL;
}

IteratorPtr
adt_iterator_equate( IteratorPtr itor1, IteratorPtr itor2 )
{
	IteratorHeaderPtr header1 = (IteratorHeaderPtr) itor1;
	IteratorHeaderPtr header2 = (IteratorHeaderPtr) itor2;

#ifndef NDEBUG
	verify_iterator(header1);
	verify_iterator(header2);
#endif /* Debugging code */

	if ( valid_iterator_header(header1) && 
		 valid_iterator_header(header2))
	{	
		return header1->mIteratorOperations->mIteratorEquate(header1->mIterator, header2->mIterator);
	}

	return NULL;
}

IteratorPtr 
adt_iterator_duplicate( IteratorPtr itor )
{
	IteratorHeaderPtr src_iterator_header = (IteratorHeaderPtr) itor;
	IteratorHeaderPtr dst_iterator_header;

#ifndef NDEBUG
	verify_iterator(src_iterator_header);
#endif /* Debugging code. */


	dst_iterator_header = ADT_XALLOCATE(sizeof(IteratorHeader));
	if (!dst_iterator_header)
	{
		return NULL;
	}

	dst_iterator_header->mIteratorOperations = src_iterator_header->mIteratorOperations;
	dst_iterator_header->mIterator = src_iterator_header->mIteratorOperations->mIteratorDuplicate(src_iterator_header->mIterator);
	dst_iterator_header->mMagicNumber = kIteratorMagicNumber;

#ifndef NDEBUG
	verify_iterator(dst_iterator_header);
#endif /* Debugging code. */

	return (IteratorPtr) dst_iterator_header;
}


unsigned int
adt_iterator_find(IteratorPtr iterator, AdtPtr data)
{
	IteratorHeaderPtr header = (IteratorHeaderPtr) iterator;

#ifndef NDEBUG
	verify_iterator(header);
#endif /* Debugging code. */

	if (valid_iterator_header(header))
	{
		return header->mIteratorOperations->mIteratorFind(header->mIterator, data);
	}

	return 0;
}

unsigned int
adt_iterator_add(IteratorPtr iterator, AdtPtr data)
{
	IteratorHeaderPtr header = (IteratorHeaderPtr) iterator;

#ifndef NDEBUG
	verify_iterator(header);
#endif /* Debugging code. */

	if (valid_iterator_header(header))
	{
		return header->mIteratorOperations->mIteratorAdd(header->mIterator, data);
	}

	return 0;
}

AdtPtr
adt_iterator_remove(IteratorPtr iterator)
{
	IteratorHeaderPtr header = (IteratorHeaderPtr) iterator;

#ifndef NDEBUG
	verify_iterator(header);
#endif /* Debugging code. */

	if (valid_iterator_header(header))
	{
		return header->mIteratorOperations->mIteratorRemove(header->mIterator);
	}

	return NULL;
}

void
adt_iterator_swap( IteratorPtr itor1, IteratorPtr itor2 )
{
	IteratorHeaderPtr header1 = (IteratorHeaderPtr) itor1;
	IteratorHeaderPtr header2 = (IteratorHeaderPtr) itor2;

#ifndef NDEBUG
	verify_iterator(header1);
	verify_iterator(header2);
#endif /* Debugging code */

	if ( valid_iterator_header(header1) && 
		 valid_iterator_header(header2))
	{	
		header1->mIteratorOperations->mIteratorSwap( header1->mIterator, header2->mIterator );
	}
	
}
