/* File header */
#include "adt_array_stack.h"

/* Koryn's Units */
#include "adt_debugging.h"
#include "adt_magic_numbers.h"
#include "adt_memory.h"

/* C Standard Library headers */
#include <stdlib.h>


const static unsigned int kDefaultStackArraySize = 8;
const static int kEmptyStackPosition = -1;
const static unsigned int kExpansionFactor = 2;


typedef struct ArrayStackHeader ArrayStackHeader;
typedef ArrayStackHeader* ArrayStackHeaderPtr;

struct ArrayStackHeader
{
    adt_magic_number_t mMagicNumber;
    unsigned int mCurrentSize;
    int mTopOfStack;
    /*@only@*/ AdtPtr* mArray; /* Array of Ptrs. */
};


/* Internal routines */
static unsigned int valid_header(ArrayStackHeaderPtr header);
#ifndef NDEBUG
static void verify_array_stack(ArrayStackHeaderPtr header);
#endif /* Debugging code. */
static void double_array(ArrayStackHeaderPtr header);


/* Implementation of internal routines. */
static unsigned int
valid_header(ArrayStackHeaderPtr header)
{
    if (header && (header->mMagicNumber == kArrayStackMagicNumber))
    {
		return 1;
    }

	return 0;
}


static void
double_array(ArrayStackHeaderPtr header)
{
    AdtPtr* new_array;
    unsigned int old_size;

    if (!valid_header(header))
    {
		return;
    }

#ifndef NDEBUG
    verify_array_stack(header);
#endif /* Debugging code. */

    old_size = header->mCurrentSize;
    header->mCurrentSize *= kExpansionFactor;

    new_array = (AdtPtr*) ADT_XALLOCATE(sizeof(AdtPtr) * header->mCurrentSize);
    if (new_array)
    {
		int index;

		for (index = 0; index <= header->mTopOfStack; index++)
		{
		    new_array[index] = header->mArray[index];
		}

		for (index = header->mTopOfStack + 1; index < (int)header->mCurrentSize; index++)
		{
		    new_array[index] = NULL;
		}

		(void) adt_dispose_ptr((AdtPtr) header->mArray);
		header->mArray = new_array;
	}
	else
	{
		/* FIXME: what to do when we run out of memory? */
    }

#ifndef NDEBUG
    verify_array_stack(header);
#endif /* Debugging code. */
}


#ifndef NDEBUG
static void
verify_array_stack(ArrayStackHeaderPtr header)
{
    int index;

    if (!valid_header(header))
    {
		return;
    }

    /* Check that mTopOfStack is valid. */
    adt_assert_code(kEmptyStackPosition <= header->mTopOfStack, "", __FILE__, __LINE__);
    adt_assert_code(header->mTopOfStack < (int)header->mCurrentSize, "", __FILE__, __LINE__);

    /* Check that the array is a valid pointer. */
    adt_assert_valid_ptr((AdtPtr) header->mArray, "", __FILE__, __LINE__);
    if (header->mTopOfStack != kEmptyStackPosition)
    {
		adt_assert_valid_ptr_nil((AdtPtr) header->mArray[header->mTopOfStack], "", __FILE__, __LINE__);
    }

    /* Check that the array is a consistent size. */
#if USE_MAC_MEMORY
    adt_assert_code(GetPtrSize((AdtPtr) header->mArray) == sizeof(AdtPtr) * header->mCurrentSize, "", __FILE__, __LINE__);
#endif /* USE_MAC_MEMORY */

    for (index = 0; index <= header->mTopOfStack; index++)
    {
		adt_assert_valid_ptr(header->mArray[index], "", __FILE__, __LINE__);
    }

    adt_assert_code(-1 <= header->mTopOfStack, "", __FILE__, __LINE__);
    adt_assert_code(header->mTopOfStack < (int)header->mCurrentSize, "", __FILE__, __LINE__);
}
#endif /* Debugging code. */


ArrayStackPtr
adt_array_stack_init(void)
{
    ArrayStackHeaderPtr header = (ArrayStackHeaderPtr) ADT_XALLOCATE(sizeof(ArrayStackHeader));
    unsigned int index;

    if (!header)
    {
		return NULL;
    }

    header->mCurrentSize = kDefaultStackArraySize;
    header->mTopOfStack = kEmptyStackPosition;
    /*@ignore@*/
    header->mArray = (AdtPtr*) ADT_XALLOCATE(sizeof(AdtPtr) * kDefaultStackArraySize);
    /*@end@*/
    if (header->mArray == NULL)
    {
		(void) adt_dispose_ptr((AdtPtr) header);
		return NULL;
    }

    for (index = 0; index < kDefaultStackArraySize; index++)
    {
		header->mArray[index] = NULL;
    }

    header->mMagicNumber = kArrayStackMagicNumber;

#ifndef NDEBUG
    verify_array_stack(header);
#endif /* Debugging code. */

    return (ArrayStackPtr) header;
}


ArrayStackPtr
adt_array_stack_dispose(ArrayStackPtr stack)
{
    ArrayStackHeaderPtr header = (ArrayStackHeaderPtr) stack;

#ifndef NDEBUG
    verify_array_stack(header);
#endif /* Debugging code. */

    if (!valid_header(header))
    {
		/*@ignore@*/
		return NULL;
		/*@end@*/
    }

    (void) adt_dispose_ptr((AdtPtr) header->mArray);
    (void) adt_dispose_ptr((AdtPtr) header);
    return NULL;
}


unsigned int
adt_array_stack_is_empty(ArrayStackPtr stack)
{
    ArrayStackHeaderPtr header = (ArrayStackHeaderPtr) stack;

#ifndef NDEBUG
    verify_array_stack(header);
#endif /* Debugging code. */

    if (!valid_header(header))
    {
		return 1;
    }

    return (header->mTopOfStack == kEmptyStackPosition);
}


void
adt_array_stack_make_empty(ArrayStackPtr stack)
{
    ArrayStackHeaderPtr header = (ArrayStackHeaderPtr) stack;
    int index;

#ifndef NDEBUG
    verify_array_stack(header);
#endif /* Debugging code. */

    if (!valid_header(header))
    {
		return;
    }

    for (index = 0; index <= header->mTopOfStack; index++)
    {
		header->mArray[index] = NULL;
    }

    header->mTopOfStack = kEmptyStackPosition;

#ifndef NDEBUG
	/*@ignore@*/
    verify_array_stack(header);
    /*@end@*/
#endif /* Debugging code. */
}


void
adt_array_stack_push(ArrayStackPtr stack, AdtPtr data)
{
    ArrayStackHeaderPtr header = (ArrayStackHeaderPtr) stack;

#ifndef NDEBUG
    verify_array_stack(header);
#endif /* Debugging code. */

    if (!valid_header(header))
    {
		return;
    }

    if (header->mTopOfStack == header->mCurrentSize - 1)
    {
		double_array(header);
    }

    header->mTopOfStack++;
    header->mArray[header->mTopOfStack] = data;

#ifndef NDEBUG
    verify_array_stack(header);
#endif /* Debugging code. */
}


void
adt_array_stack_pop(ArrayStackPtr stack)
{
    ArrayStackHeaderPtr header = (ArrayStackHeaderPtr) stack;

#ifndef NDEBUG
    verify_array_stack(header);
#endif /* Debugging code. */

    if (!valid_header(header))
    {
		return;
    }

    if (header->mTopOfStack != kEmptyStackPosition)
    {
		header->mArray[header->mTopOfStack] = NULL;
		header->mTopOfStack--;
    }

#ifndef NDEBUG
	/*@ignore@*/
    verify_array_stack(header);
    /*@end@*/
#endif /* Debugging code. */
}


AdtPtr
adt_array_stack_top(ArrayStackPtr stack)
{
    ArrayStackHeaderPtr header = (ArrayStackHeaderPtr) stack;

#ifndef NDEBUG
    verify_array_stack(header);
#endif /* Debugging code. */

    if (!valid_header(header))
    {
		return NULL;
    }

    if (header->mTopOfStack != kEmptyStackPosition)
    {
		return header->mArray[header->mTopOfStack];
    }

    return NULL;
}


AdtPtr
adt_array_stack_top_and_pop(ArrayStackPtr stack)
{
    ArrayStackHeaderPtr header = (ArrayStackHeaderPtr) stack;
    AdtPtr result = NULL;

#ifndef NDEBUG
    verify_array_stack(header);
#endif /* Debugging code. */

    if (!valid_header(header))
    {
		return NULL;
    }

    if (header->mTopOfStack != kEmptyStackPosition)
    {
		/* Top */
		result = header->mArray[header->mTopOfStack];

		/* Pop */
		header->mArray[header->mTopOfStack] = NULL;
		header->mTopOfStack--;
    }

    return result;
}
