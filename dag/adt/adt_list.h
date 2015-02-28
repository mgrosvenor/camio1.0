/* ------------------------------------------------------------------------------
 adt_list from Koryn's Units.

 Generic list interface.
 The objects stored in the list are generic AdtPtr pointers.

 When disposing a list that contains references to memory allocated on the
 heap, you'll need to iterate over the list and free the memory yourself
 OR provide the list with an appropriate disposal routine.

 Copyright (c) 2000-2004, Koryn Grant (koryn@clear.net.nz)
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 *	Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.
 *	Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution.
 *	Neither the name of the author nor the names of other contributors
	may be used to endorse or promote products derived from this software without
	specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
------------------------------------------------------------------------------ */

#ifndef ADT_LIST_H
#define ADT_LIST_H

/* Koryn's Units. */
#include "adt_disposal.h"


typedef struct List_ List_;
typedef List_* ListPtr; /* Type-safe opaque type. */

typedef struct Iterator_ Iterator_;
typedef Iterator_* IteratorPtr; /* Type-safe opaque type. */


/* Available interfaces. */
typedef enum
{
	kListInterfacePlain = 1,
	kListInterfaceIterator = 2

} list_interface_t;

/* Available representations. */
typedef enum
{
	kListRepresentationDynamic = 3,
	kListRepresentationArray = 4

} list_represent_t;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Generalised list ADT common routines. */
ListPtr adt_list_init(list_interface_t list_interface, list_represent_t representation, DisposeRoutine disposer);
ListPtr adt_list_dispose(ListPtr list);
unsigned int adt_list_is_empty(ListPtr list);
void adt_list_make_empty(ListPtr list);
unsigned int adt_list_items(ListPtr list);
AdtPtr adt_list_get_indexed_item(ListPtr list, unsigned int index);

/* Synonym for adt_list_get_indexed_item(), provided for consistency with Sets. */
AdtPtr adt_list_retrieve(ListPtr list, unsigned int index);

/* Plain interface list routines. */
unsigned int adt_list_add_first(ListPtr list, AdtPtr data);
unsigned int adt_list_add_last(ListPtr list, AdtPtr data);
unsigned int adt_list_add_at(ListPtr list, AdtPtr data, unsigned int index );
AdtPtr adt_list_get_first(ListPtr list);
AdtPtr adt_list_get_last(ListPtr list);
void adt_list_remove(ListPtr list, AdtPtr data);
AdtPtr adt_list_remove_at(ListPtr list, unsigned int index);
AdtPtr adt_list_remove_first(ListPtr list);
AdtPtr adt_list_remove_last(ListPtr list);
unsigned int adt_list_contains(ListPtr list, AdtPtr data);

/* Iterator interface list routines. */
IteratorPtr adt_list_get_iterator(ListPtr list);

/* Iterator routines. */
IteratorPtr adt_iterator_dispose(IteratorPtr itor);
unsigned int adt_iterator_is_in_list(IteratorPtr itor);
void adt_iterator_zeroth(IteratorPtr itor);
void adt_iterator_first(IteratorPtr itor);
void adt_iterator_last(IteratorPtr itor);
unsigned int adt_iterator_is_equal( IteratorPtr itor1, IteratorPtr itor2 );
void adt_iterator_advance(IteratorPtr itor);
unsigned char adt_iterator_advance_to( IteratorPtr itor1, IteratorPtr itor2 );
void adt_iterator_retreat(IteratorPtr itor);
AdtPtr adt_iterator_retrieve(IteratorPtr itor);
IteratorPtr adt_iterator_equate( IteratorPtr itor1, IteratorPtr itor2 );
unsigned int adt_iterator_find(IteratorPtr itor, AdtPtr data);
unsigned int adt_iterator_add(IteratorPtr itor, AdtPtr data);
AdtPtr adt_iterator_remove(IteratorPtr itor);
void adt_iterator_swap(IteratorPtr itor1, IteratorPtr itor2);
IteratorPtr adt_iterator_duplicate(IteratorPtr itor);

#ifdef ADT_MEMORY_DEBUG
#if ADT_MEMORY_DEBUG
void adt_list_note_ptrs(ListPtr list);
#endif /* ADT_MEMORY_DEBUG */
#endif /* ADT_MEMORY_DEBUG */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_LIST_H */
