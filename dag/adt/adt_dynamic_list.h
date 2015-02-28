/* ------------------------------------------------------------------------------
 adt_dynamic_list from Koryn's Units.

 Doubly-linked, dynamically allocated list.
 The objects stored in the list are generic AdtPtr pointers.

 When disposing a list that contains references to memory allocated on the
 heap, you'll need to iterate over the list and free the memory yourself.

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

#ifndef ADT_DYNAMIC_LIST_H
#define ADT_DYNAMIC_LIST_H

/* Koryn's Units. */
#include "adt_disposal.h"
#include "adt_list.h"


typedef struct DynamicList_ DynamicList_;
typedef DynamicList_* DynamicListPtr; /* Type-safe opaque type. */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Dynamic list common routines. */
DynamicListPtr adt_dynamic_list_init(DisposeRoutine disposer);
DynamicListPtr adt_dynamic_list_dispose(DynamicListPtr list);
unsigned int adt_dynamic_list_is_empty(DynamicListPtr list);
void adt_dynamic_list_make_empty(DynamicListPtr list);
unsigned int adt_dynamic_list_items(DynamicListPtr list);
AdtPtr adt_dynamic_list_get_indexed_item(DynamicListPtr list, unsigned int index);

#if ADT_MEMORY_DEBUG
void adt_dynamic_list_note_ptrs(DynamicListPtr list);
#endif /* ADT_MEMORY_DEBUG */

/* Dynamic list plain interface routines. */
unsigned int adt_dynamic_list_add_first(DynamicListPtr list, AdtPtr data);
unsigned int adt_dynamic_list_add_last(DynamicListPtr list, AdtPtr data);
unsigned int adt_dynamic_list_add_at( DynamicListPtr list, AdtPtr data, unsigned int index );
AdtPtr adt_dynamic_list_get_first(DynamicListPtr list);
AdtPtr adt_dynamic_list_get_last(DynamicListPtr list);
void adt_dynamic_list_remove(DynamicListPtr list, AdtPtr data);
AdtPtr adt_dynamic_list_remove_at(DynamicListPtr list, unsigned int index);
AdtPtr adt_dynamic_list_remove_first(DynamicListPtr list);
AdtPtr adt_dynamic_list_remove_last(DynamicListPtr list);
unsigned int adt_dynamic_list_contains(DynamicListPtr list, AdtPtr data);

/* Dynamic list iterator interface routines. */
IteratorPtr adt_dynamic_list_get_iterator(DynamicListPtr list);

/* Dynamic list iterator routines. */
IteratorPtr adt_dynamic_list_iterator_dispose(IteratorPtr itor);
unsigned int adt_dynamic_list_iterator_is_in_list(IteratorPtr itor);
void adt_dynamic_list_iterator_zeroth(IteratorPtr itor);
void adt_dynamic_list_iterator_first(IteratorPtr itor);
void adt_dynamic_list_iterator_last(IteratorPtr itor);
unsigned int adt_dynamic_list_iterator_is_equal(IteratorPtr itor1, IteratorPtr itor2);
void adt_dynamic_list_iterator_advance(IteratorPtr itor);
unsigned char adt_dynamic_list_iterator_advance_to(IteratorPtr itor1, IteratorPtr itor2);
void adt_dynamic_list_iterator_retreat(IteratorPtr itor);
AdtPtr adt_dynamic_list_iterator_retrieve(IteratorPtr itor);
IteratorPtr adt_dynamic_list_iterator_equate(IteratorPtr itor1, IteratorPtr itor2);
unsigned int adt_dynamic_list_iterator_find(IteratorPtr itor, AdtPtr data);
unsigned int adt_dynamic_list_iterator_add(IteratorPtr itor, AdtPtr data);
AdtPtr adt_dynamic_list_iterator_remove(IteratorPtr itor);
void adt_dynamic_list_iterator_swap(IteratorPtr itor1, IteratorPtr itor2);
IteratorPtr adt_dynamic_list_iterator_duplicate(IteratorPtr itor);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_DYNAMIC_LIST_H */
