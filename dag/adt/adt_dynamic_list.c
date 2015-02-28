/* File header. */
#include "adt_dynamic_list.h"

/* ADT headers. */
#include "adt_debugging.h"
#include "adt_list_common.h"
#include "adt_magic_numbers.h"
#include "adt_memory.h"
#include "adt_memory_pool.h"

/* C Standard Library headers. */
#include <assert.h>
#include <stdlib.h>


typedef struct DynamicListHeader DynamicListHeader;
typedef DynamicListHeader* DynamicListHeaderPtr;

typedef struct DynamicIteratorHeader DynamicIteratorHeader;
typedef DynamicIteratorHeader* DynamicIteratorHeaderPtr;


struct DynamicListHeader
{
	adt_magic_number_t mMagicNumber;
	unsigned int mItems;

	DisposeRoutine mDisposer;
	ListNodePtr mHead;
	ListNodePtr mTail;
	MemoryPoolPtr mFree;
};

struct DynamicIteratorHeader
{
	adt_magic_number_t mMagicNumber;
	DynamicListHeaderPtr mList;
	ListNodePtr mCurrent;
};


/* Internal routines */
static unsigned int valid_header(DynamicListHeaderPtr header);
static unsigned int valid_iterator(DynamicIteratorHeaderPtr iterator);
static inline unsigned char iterator_advance( DynamicIteratorHeaderPtr iterator );
#ifndef NDEBUG
static void verify_header(DynamicListHeaderPtr header);
static void verify_iterator(DynamicIteratorHeaderPtr iterator);
#endif /* Debugging code. */
static DynamicIteratorHeaderPtr new_dynamic_iterator(DynamicListHeaderPtr header);


/* Implementation of internal routines. */
static unsigned int
valid_header(DynamicListHeaderPtr header)
{
	if (header && (header->mMagicNumber == kDynamicListMagicNumber))
	{
		return 1;
	}

	return 0;
}


static unsigned int
valid_iterator(DynamicIteratorHeaderPtr header)
{
	if (header && (header->mMagicNumber == kDynamicListIteratorMagicNumber))
	{
		return valid_header(header->mList);
	}

	return 0;
}


#ifndef NDEBUG
static void
verify_header(DynamicListHeaderPtr header)
{
	unsigned int forward_count = 0;
	unsigned int backward_count = 0;
	ListNodePtr node;

	adt_assert_valid_ptr(header, "", __FILE__, __LINE__);
	assert(header->mMagicNumber == kDynamicListMagicNumber);

	if (header->mItems == 0)
	{
		assert(header->mHead == NULL);
		assert(header->mTail == NULL);
	}
	else if (header->mItems == 1)
	{
		adt_assert_valid_ptr(header->mHead, "", __FILE__, __LINE__);
		adt_assert_valid_ptr(header->mTail, "", __FILE__, __LINE__);
		assert(header->mHead == header->mTail);
	}
	else
	{
		adt_assert_valid_ptr(header->mHead, "", __FILE__, __LINE__);
		adt_assert_valid_ptr(header->mTail, "", __FILE__, __LINE__);
		assert(header->mHead != header->mTail);
	}

	/* Check forwards traversal. */
	node = header->mHead;
	while (node)
	{
		adt_assert_valid_ptr(node, "", __FILE__, __LINE__);
		adt_assert_valid_ptr_nil(node->mData, "", __FILE__, __LINE__);
		adt_assert_valid_ptr_nil(node->mNext, "", __FILE__, __LINE__);
		adt_assert_valid_ptr_nil(node->mPrev, "", __FILE__, __LINE__);

		forward_count++;
		node = node->mNext;
	}
	assert(forward_count == header->mItems);

	/* Check backwards traversal. */
	node = header->mTail;
	while (node)
	{
		adt_assert_valid_ptr(node, "", __FILE__, __LINE__);
		adt_assert_valid_ptr_nil(node->mData, "", __FILE__, __LINE__);
		adt_assert_valid_ptr_nil(node->mNext, "", __FILE__, __LINE__);
		adt_assert_valid_ptr_nil(node->mPrev, "", __FILE__, __LINE__);

		backward_count++;
		node = node->mPrev;
	}
	assert(backward_count == header->mItems);
}


static void
verify_iterator(DynamicIteratorHeaderPtr iterator)
{
	ListNodePtr node;

	adt_assert_valid_ptr(iterator, "", __FILE__, __LINE__);
	assert(iterator->mMagicNumber == kDynamicListIteratorMagicNumber);

	/* List must always be valid. */
	verify_header(iterator->mList);

	/* Current position must be nil or a valid node. */
	adt_assert_valid_ptr_nil((AdtPtr) iterator->mCurrent, "", __FILE__, __LINE__);

	node = iterator->mList->mHead;
	if (node)
	{
		/* Mustn't be a node before the head node. */
		adt_assert_code(node->mPrev == NULL, "", __FILE__, __LINE__);

		while (node)
		{
			adt_assert_valid_ptr((AdtPtr) node, "", __FILE__, __LINE__);
			adt_assert_valid_ptr_nil((AdtPtr) node->mNext, "", __FILE__, __LINE__);

			node = node->mNext;
		}
	}

	node = iterator->mList->mTail;
	if (node)
	{
		/* Mustn't be a node after the tail node. */
		assert(node->mNext == NULL);
	}
}
#endif /* Debugging code. */


static DynamicIteratorHeaderPtr
new_dynamic_iterator(DynamicListHeaderPtr header)
{
	DynamicIteratorHeaderPtr itor = (DynamicIteratorHeaderPtr) ADT_XALLOCATE(sizeof(DynamicIteratorHeader));

	if (itor == NULL)
	{
		return NULL;
	}

	itor->mList = header;
	itor->mCurrent = NULL;
	itor->mMagicNumber = kDynamicListIteratorMagicNumber;

	return itor;
}


/* Implementation of dynamic list common routines. */
DynamicListPtr
adt_dynamic_list_init(DisposeRoutine disposer)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) ADT_XALLOCATE(sizeof(DynamicListHeader));
	if (header == NULL)
	{
		return NULL;
	}

	header->mDisposer = disposer;
	header->mHead = NULL;
	header->mTail = NULL;
	header->mFree = adt_memory_pool_init(sizeof(ListNode));
	header->mItems = 0;
	header->mMagicNumber = kDynamicListMagicNumber;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	return (DynamicListPtr) header;
}


DynamicListPtr
adt_dynamic_list_dispose(DynamicListPtr list)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;

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

	adt_dynamic_list_make_empty(list);

	(void) adt_memory_pool_dispose(header->mFree);
	(void) adt_dispose_ptr((AdtPtr) header);

	return NULL;
}


unsigned int
adt_dynamic_list_is_empty(DynamicListPtr list)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header) || (header->mHead == NULL))
	{
		return 1;
	}

	return 0;
}


void
adt_dynamic_list_make_empty(DynamicListPtr list)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return;
	}

	while (header->mHead)
	{
		ListNodePtr node = header->mHead;

		header->mHead = node->mNext;

		if (header->mDisposer)
		{
			header->mDisposer(node->mData);
		}

		adt_memory_pool_put(header->mFree, node);
	}

	header->mTail = NULL;
	header->mItems = 0;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */
}


#if ADT_MEMORY_DEBUG
void
adt_dynamic_list_note_ptrs(DynamicListPtr list)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;
	ListNodePtr current;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	adt_memory_note_ptr(header);
	adt_memory_pool_note_ptrs(header->mFree);
	
	current = header->mHead;
	while (current)
	{
		adt_memory_note_ptr(current);
		current = current->mNext;
	}
}
#endif /* ADT_MEMORY_DEBUG */


unsigned int
adt_dynamic_list_items(DynamicListPtr list)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		return header->mItems;
	}

	return 0;
}


AdtPtr
adt_dynamic_list_get_indexed_item(DynamicListPtr list, unsigned int index)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;
	
#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	assert(0 < index); /* List indices are one-based. */
	assert(index <= header->mItems);

	if (valid_header(header))
	{
		if ((0 < index) && (index <= header->mItems))
		{
			ListNodePtr node = header->mHead;
			unsigned int count = 1;
			
			while (node)
			{
				if (count == index)
				{
					return node->mData;
				}
		
				count++;
				node = node->mNext;
			}
		}
	
	}

	return NULL;
}


/* Implementation of dynamic list plain interface routines. */
unsigned int
adt_dynamic_list_add_first(DynamicListPtr list, AdtPtr data)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;
	ListNodePtr node;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (0 == valid_header(header))
	{
		return 0;
	}

	node = adt_memory_pool_get(header->mFree);
	if (NULL == node)
	{
		return 0;
	}

	/* Initialise the node. */
	node->mData = data;
	node->mNext = header->mHead;
	node->mPrev = NULL;

	/* Insert the node at the head of the list. */
	if (header->mHead)
	{
		header->mHead->mPrev = node;
	}

	header->mHead = node;
	if (NULL == header->mTail)
	{
		header->mTail = node;
	}

	header->mItems++;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	return 1;
}


unsigned int
adt_dynamic_list_add_last(DynamicListPtr list, AdtPtr data)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;
	ListNodePtr node;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return 0;
	}

	node = adt_memory_pool_get(header->mFree);
	if (!node)
	{
		return 0;
	}

	/* Initialise the node. */
	node->mData = data;
	node->mNext = NULL;
	node->mPrev = header->mTail;

	/* Insert the node at the tail of the list. */
	if (header->mTail)
	{
		header->mTail->mNext = node;
	}

	header->mTail = node;
	if (!header->mHead)
	{
		header->mHead = node;
	}

	header->mItems++;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	return 1;
}

unsigned int
adt_dynamic_list_add_at( DynamicListPtr list, AdtPtr data, unsigned int index )
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;
	unsigned int count = 0;
	ListNodePtr insert_node;
	ListNodePtr current, prev_node;
	
#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	assert(0 < index);
	assert(index < header->mItems);

	if (!valid_header(header))
	{
		return 0;
	}

	if (index >= header->mItems)
	{
		return 0;
	}
	
	insert_node = adt_memory_pool_get(header->mFree);
	if (NULL == insert_node)
	{
		return 0;
	}

	/* Initialise the node. */
	insert_node->mData = data;
	insert_node->mNext = header->mHead;
	insert_node->mPrev = NULL;
	
	current = header->mHead;
	
	while (current)
	{
		/*	Iterate as far as we can, or until we run out of entries
			to iterate through. */
		if (count == index)
		{
			break;
		}
		
		count++;
		current = current->mNext;
	}
	
	/*	NOTE: Due to the if condition checking for the index of the
		items, this should never actually be NULL. */
	if (current == NULL)
	{
		/* Insert the node at the tail of the list. */
		insert_node->mPrev = header->mTail;
		if (header->mTail)
		{
			header->mTail->mNext = insert_node;
		}
	
		header->mTail = insert_node;
		if (!header->mHead)
		{
			header->mHead = insert_node;
		}
	}
	else if (current == header->mTail)
	{
		/* Inserting as the tail of the list. */
		insert_node->mPrev = current;
		current->mNext = insert_node;
		header->mTail = insert_node;
	}
	else
	{
		/* Inserting in the middle of the list. We insert
		   before current. This ensures that the new element
		   will be inserted at the supplied index. */
		prev_node = current->mPrev;
		
		/* Put the new node before current */
		insert_node->mNext = current;
		current->mPrev = insert_node;
		
		/* and after prev_node. */
		prev_node->mNext = insert_node;
		insert_node->mPrev = prev_node;
	}
	
	header->mItems++;
	
#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	return 1;
}

AdtPtr
adt_dynamic_list_get_first(DynamicListPtr list)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (valid_header(header) && header->mHead)
	{
		return header->mHead->mData;
	}

	return NULL;
}


AdtPtr
adt_dynamic_list_get_last(DynamicListPtr list)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (valid_header(header) && header->mTail)
	{
		return header->mTail->mData;
	}

	return NULL;
}


void
adt_dynamic_list_remove(DynamicListPtr list, AdtPtr data)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;
	ListNodePtr node;

#ifndef NDEBUG
	adt_assert_valid_ptr(data, "", __FILE__, __LINE__);
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return;
	}

	node = header->mHead;
	while (node)
	{
		if (node->mData == data)
		{
			node->mData = NULL;

			/* Bypass the node in the list. */
			if (header->mHead == node)
			{
				header->mHead = node->mNext;
			}
			else
			{
				node->mPrev->mNext = node->mNext;
			}

			if (header->mTail == node)
			{
				header->mTail = node->mPrev;
			}
			else
			{
				node->mNext->mPrev = node->mPrev;
			}

			adt_memory_pool_put(header->mFree, node);
			header->mItems--;

		#ifndef NDEBUG
			verify_header(header);
		#endif /* Debugging code. */

			return;
		}

		node = node->mNext;
	}

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */
}


AdtPtr
adt_dynamic_list_remove_at(DynamicListPtr list, unsigned int index)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;
	unsigned int count = 0;
	AdtPtr data = NULL;
	ListNodePtr node;
	
#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	assert(0 < index);
	assert(index < header->mItems);

	if (!valid_header(header))
	{
		return NULL;
	}

	if (index >= header->mItems)
	{
		return NULL;
	}

	node = header->mHead;
	while (node)
	{
		if (count == index)
		{
			data = node->mData;
			node->mData = NULL;

			/* Bypass the node in the list. */
			if (header->mHead == node)
			{
				header->mHead = node->mNext;
			}
			else
			{
				node->mPrev->mNext = node->mNext;
			}

			if (header->mTail == node)
			{
				header->mTail = node->mPrev;
			}
			else
			{
				node->mNext->mPrev = node->mPrev;
			}

			adt_memory_pool_put(header->mFree, node);
			header->mItems--;

		#ifndef NDEBUG
			verify_header(header);
		#endif /* Debugging code. */

			return data;
		}

		count++;
		node = node->mNext;
	}

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	return NULL;
}


AdtPtr
adt_dynamic_list_remove_first(DynamicListPtr list)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;
	ListNodePtr node;
	AdtPtr data;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return NULL;
	}

	node = header->mHead;
	if (node == NULL)
	{
		return NULL;
	}
	else if (header->mItems == 1)
	{
		data = node->mData;
		node->mData = NULL;

		header->mHead = NULL;
		header->mTail = NULL;
		header->mItems = 0;
	}
	else
	{
		data = node->mData;
		node->mData = NULL;

		/* Bypass the node in the list. */
		header->mHead = node->mNext;
		header->mHead->mPrev = NULL;

		header->mItems--;
	}

	adt_memory_pool_put(header->mFree, node);

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	return data;
}


AdtPtr
adt_dynamic_list_remove_last(DynamicListPtr list)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;
	ListNodePtr node;
	AdtPtr data;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return NULL;
	}

	node = header->mTail;
	if (node == NULL)
	{
		return NULL;
	}
	else if (header->mItems == 1)
	{
		data = node->mData;
		node->mData = NULL;

		header->mHead = NULL;
		header->mTail = NULL;
		header->mItems = 0;
	}
	else
	{
		data = node->mData;
		node->mData = NULL;

		/* Bypass the node in the list. */
		header->mTail = node->mPrev;
		header->mTail->mNext = NULL;

		header->mItems--;
	}

	adt_memory_pool_put(header->mFree, node);

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	return data;
}


unsigned int
adt_dynamic_list_contains(DynamicListPtr list, AdtPtr data)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;
	ListNodePtr node;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (!valid_header(header))
	{
		return 0;
	}

	if (header->mItems == 0)
	{
		return 0;
	}

	node = header->mHead;
	while (node)
	{
		if (node->mData == data)
		{
			return 1;
		}

		node = node->mNext;
	}

	return 0;
}


/* Implementation of dynamic list iterator interface routines. */
IteratorPtr
adt_dynamic_list_get_iterator(DynamicListPtr list)
{
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) list;

#ifndef NDEBUG
	verify_header(header);
#endif /* Debugging code. */

	if (valid_header(header))
	{
		return (IteratorPtr) new_dynamic_iterator(header);
	}

	return NULL;
}


/* Implementation of dynamic list iterator routines. */
IteratorPtr
adt_dynamic_list_iterator_dispose(IteratorPtr itor)
{
	DynamicIteratorHeaderPtr iterator = (DynamicIteratorHeaderPtr) itor;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (valid_iterator(iterator))
	{
		(void) adt_dispose_ptr(iterator);
	}

	return NULL;
}


unsigned int
adt_dynamic_list_iterator_is_in_list(IteratorPtr itor)
{
	DynamicIteratorHeaderPtr iterator = (DynamicIteratorHeaderPtr) itor;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (valid_iterator(iterator) && iterator->mCurrent)
	{
		return 1;
	}

	return 0;
}


void
adt_dynamic_list_iterator_zeroth(IteratorPtr itor)
{
	/*
	 * Don't check consistency on entry, because it's possible that the list may
	 * have been affected by another iterator and mCurrent is invalid.
	 * However, the list must be consistent before returning.
	 */

	DynamicIteratorHeaderPtr iterator = (DynamicIteratorHeaderPtr) itor;

	if (valid_iterator(iterator))
	{
		iterator->mCurrent = NULL;
	}

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */
}


void
adt_dynamic_list_iterator_first(IteratorPtr itor)
{
	/*
	 * Don't check consistency on entry, because it's possible that the list may
	 * have been affected by another iterator and mCurrent is invalid.
	 * However, the list must be consistent before exiting.
	 */

	DynamicIteratorHeaderPtr iterator = (DynamicIteratorHeaderPtr) itor;

	if (valid_iterator(iterator))
	{
		iterator->mCurrent = iterator->mList->mHead;
	}

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */
}


void 
adt_dynamic_list_iterator_last(IteratorPtr itor)
{


	DynamicIteratorHeaderPtr iterator = (DynamicIteratorHeaderPtr) itor;
	
	if (valid_iterator(iterator))
	{
		iterator->mCurrent = iterator->mList->mTail;
	}

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */
}

unsigned int
adt_dynamic_list_iterator_is_equal(IteratorPtr itor1, IteratorPtr itor2)
{
	DynamicIteratorHeaderPtr iterator1 = (DynamicIteratorHeaderPtr) itor1;
	DynamicIteratorHeaderPtr iterator2 = (DynamicIteratorHeaderPtr) itor2;
	
#ifndef NDEBUG
	verify_iterator(iterator1);
	verify_iterator(iterator2);
#endif /* Debugging code. */
	
	if (valid_iterator(iterator1) && valid_iterator(iterator2))
	{
		return iterator1->mCurrent == iterator2->mCurrent;
	}
	
	return 0;
}

void
adt_dynamic_list_iterator_advance(IteratorPtr itor)
{
	DynamicIteratorHeaderPtr iterator = (DynamicIteratorHeaderPtr) itor;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (valid_iterator(iterator))
	{
		if (iterator->mCurrent)
		{
			iterator->mCurrent = iterator->mCurrent->mNext;
		}
		else
		{
			iterator->mCurrent = iterator->mList->mHead;
		}
	}

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */
}

/*	
	Similar to standard advance, but we only advance as far as the 2nd
	iterator supplied. Otherwise we terminate in a similar way to the 
	standard advance function call.
*/
unsigned char
adt_dynamic_list_iterator_advance_to( IteratorPtr itor1, IteratorPtr itor2 )
{
	DynamicIteratorHeaderPtr iterator1 = (DynamicIteratorHeaderPtr) itor1;
	DynamicIteratorHeaderPtr iterator2 = (DynamicIteratorHeaderPtr) itor2;
	unsigned char res = 0;

#ifndef NDEBUG
	verify_iterator( iterator1 );
#endif /* Debugging code. */
	
	if( valid_iterator( iterator1 ))
	{
		if( valid_iterator( iterator2 ))
		{
#ifndef NDEBUG
			verify_iterator( iterator2 );
#endif
			if( iterator1->mCurrent == iterator2->mCurrent )
			{
				return 0;
			}
		}
		
		res = iterator_advance( iterator1 );
	}

#ifndef NDEBUG
	verify_iterator(iterator1);
#endif /* Debugging code. */
	
	return res;
}

static inline unsigned char
iterator_advance( DynamicIteratorHeaderPtr iterator1 )
{
	if( iterator1->mCurrent )
	{
		iterator1->mCurrent = iterator1->mCurrent->mNext;
	}
	else
	{
		iterator1->mCurrent = iterator1->mList->mHead;
	}
	
	return 1;
}

void
adt_dynamic_list_iterator_retreat(IteratorPtr itor)
{
	DynamicIteratorHeaderPtr iterator = (DynamicIteratorHeaderPtr) itor;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (valid_iterator(iterator))
	{
		if (iterator->mCurrent)
		{
			iterator->mCurrent = iterator->mCurrent->mPrev;
		}
		else
		{
			iterator->mCurrent = iterator->mList->mTail;
		}
	}

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */
}


AdtPtr
adt_dynamic_list_iterator_retrieve(IteratorPtr itor)
{
	DynamicIteratorHeaderPtr iterator = (DynamicIteratorHeaderPtr) itor;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (valid_iterator(iterator) && (NULL != iterator->mCurrent))
	{
		return iterator->mCurrent->mData;
	}

	return NULL;
}

IteratorPtr 
adt_dynamic_list_iterator_equate(IteratorPtr itor1, IteratorPtr itor2)
{
	DynamicIteratorHeaderPtr iterator1 = (DynamicIteratorHeaderPtr) itor1;
	DynamicIteratorHeaderPtr iterator2 = (DynamicIteratorHeaderPtr) itor2;	

#ifndef NDEBUG
	verify_iterator(iterator1);
	verify_iterator(iterator2);
#endif /* Debugging code. */
	
	if ( (valid_iterator(iterator1) && (NULL != iterator1->mCurrent )) &&
		 (valid_iterator(iterator2) && (NULL != iterator2->mCurrent )))
	{
		iterator1->mCurrent = iterator2->mCurrent;
	}
	
	return itor1;
}

IteratorPtr 
adt_dynamic_list_iterator_duplicate(IteratorPtr itor)
{
	DynamicIteratorHeaderPtr iterator_new = NULL;
	DynamicIteratorHeaderPtr iterator = (DynamicIteratorHeaderPtr) itor;
	DynamicListHeaderPtr header = (DynamicListHeaderPtr) iterator->mList;

#ifndef NDEBUG
	verify_iterator(iterator);
	verify_header(header);
#endif /* Debugging code. */

	if (valid_iterator(iterator) && valid_header(header))
	{
		iterator_new = new_dynamic_iterator(header);
		iterator_new->mCurrent = iterator->mCurrent;
		return (IteratorPtr) iterator_new;
	}

	return NULL;
}


unsigned int
adt_dynamic_list_iterator_find(IteratorPtr itor, AdtPtr data)
{
	/*
	 * Finds the node containing the given data after the current position,
	 * and advances the current position to be that node (if found).
	 */

	unsigned int result = 0;
	ListNodePtr node;
	DynamicIteratorHeaderPtr iterator = (DynamicIteratorHeaderPtr) itor;

#ifndef NDEBUG
	verify_iterator(iterator);
	adt_assert_valid_ptr(data, "", __FILE__, __LINE__);
#endif /* Debugging code. */

	if (!valid_iterator(iterator))
	{
		return 0;
	}

	node = iterator->mList->mHead;
	while (node)
	{
		if (node->mData == data)
		{
			result = 1;
			iterator->mCurrent = node;
			break;
		}

		node = node->mNext;
	}

#ifndef NDEBUG
	verify_iterator(iterator);
	if (result == 1)
	{
		adt_assert_valid_ptr(iterator->mCurrent, "", __FILE__, __LINE__);
	}
	else
	{
		adt_assert_valid_ptr_nil(iterator->mCurrent, "", __FILE__, __LINE__);
	}
#endif /* Debugging code. */

	return result;
}


unsigned int
adt_dynamic_list_iterator_add(IteratorPtr itor, AdtPtr data)
{
	/*
	 * Adds a node containing the given data after the current position,
	 * and advances the current position to be the node inserted.
	 */

	ListNodePtr head_node;
	ListNodePtr insert_node;
	ListNodePtr next_node = NULL;
	DynamicIteratorHeaderPtr iterator = (DynamicIteratorHeaderPtr) itor;

#ifndef NDEBUG
	verify_iterator(iterator);
	adt_assert_valid_ptr(data, "", __FILE__, __LINE__);
#endif /* Debugging code. */

	if (!valid_iterator(iterator))
	{
		return 0;
	}

	/* Get the new node, either from the free list or by creating it. */
	insert_node = adt_memory_pool_get(iterator->mList->mFree);
	if (insert_node == NULL)
	{
		/* Couldn't allocate a new node. */
		return 0;
	}
	insert_node->mData = data;
	insert_node->mNext = NULL;
	insert_node->mPrev = NULL;

	if (iterator->mCurrent == NULL)
	{
		/* Inserting as the head of the list. */
		head_node = iterator->mList->mHead;
		if (head_node == NULL)
		{
			/* Create the first node in the list. */
			iterator->mList->mTail = insert_node;
		}
		else
		{
			insert_node->mNext = head_node;
			head_node->mPrev = insert_node;
		}

		iterator->mList->mHead = insert_node;
	}
	else if (iterator->mCurrent == iterator->mList->mTail)
	{
		/* Inserting as the tail of the list. */
		insert_node->mPrev = iterator->mCurrent;
		iterator->mCurrent->mNext = insert_node;
		iterator->mList->mTail = insert_node;
	}
	else
	{
		/* Inserting in the middle of the list. */
		next_node = iterator->mCurrent->mNext;

		/* Put the new node before the next node. */
		insert_node->mNext = next_node;
		next_node->mPrev = insert_node;

		/* And after mCurrent. */
		iterator->mCurrent->mNext = insert_node;
		insert_node->mPrev = iterator->mCurrent;
	}

	iterator->mCurrent = insert_node;
	iterator->mList->mItems++;

#ifndef NDEBUG
	verify_header(iterator->mList);
	verify_iterator(iterator);
	adt_assert_code(iterator->mCurrent->mData == data, "", __FILE__, __LINE__);
#endif /* Debugging code. */

	return 1;
}


AdtPtr
adt_dynamic_list_iterator_remove(IteratorPtr itor)
{
	/*
	 * Removes the node at the current position (if current position is valid)
	 * and returns the data from the node removed (NULL if the current position was invalid).
	 */

	DynamicIteratorHeaderPtr iterator = (DynamicIteratorHeaderPtr) itor;
	ListNodePtr node;
	AdtPtr data = NULL;

#ifndef NDEBUG
	verify_iterator(iterator);
#endif /* Debugging code. */

	if (!valid_iterator(iterator))
	{
		return NULL;
	}

	if (!iterator->mCurrent)
	{
		return NULL;
	}

	node = iterator->mCurrent;
	data = node->mData;
	node->mData = NULL;
	iterator->mCurrent = node->mNext;

	/* Adjust head and tail pointers. */
	if (node == iterator->mList->mHead)
	{
		iterator->mList->mHead = node->mNext;
	}

	if (node == iterator->mList->mTail)
	{
		iterator->mList->mTail = node->mPrev;
	}

	/* Detach node from list. */
	if (node->mNext)
	{
		node->mNext->mPrev = node->mPrev;
	}

	if (node->mPrev)
	{
		node->mPrev->mNext = node->mNext;
	}

	/* Dispose of node. */
	iterator->mList->mItems--;
	adt_memory_pool_put(iterator->mList->mFree, node);

#ifndef NDEBUG
	if (iterator->mCurrent)
	{
		adt_assert_valid_ptr((AdtPtr) iterator->mCurrent, "", __FILE__, __LINE__);
	}
	verify_iterator(iterator);
#endif /* Debugging code. */

	return data;
}




void
adt_dynamic_list_iterator_swap(IteratorPtr itor1, IteratorPtr itor2)
{
	DynamicIteratorHeaderPtr iterator1 = (DynamicIteratorHeaderPtr) itor1;
	DynamicIteratorHeaderPtr iterator2 = (DynamicIteratorHeaderPtr) itor2;
	AdtPtr tmp_data;

#ifndef NDEBUG
	verify_iterator(iterator1);
	verify_iterator(iterator2);
#endif /* Debugging code. */
	
	if ( (valid_iterator(iterator1) && (NULL != iterator1->mCurrent )) &&
		 (valid_iterator(iterator2) && (NULL != iterator2->mCurrent )))
	{
		tmp_data = iterator1->mCurrent->mData;
		iterator1->mCurrent->mData = iterator2->mCurrent->mData;
		iterator2->mCurrent->mData = tmp_data;
	}
	
}
