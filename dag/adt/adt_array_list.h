/* ------------------------------------------------------------------------------
 adt_array_list from Koryn's Units.

 An array-based implementation of a plain list.

 Copyright (c) 2001-2004, Koryn Grant (koryn@clear.net.nz)
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

#ifndef ADT_ARRAY_LIST_H
#define ADT_ARRAY_LIST_H

/* Koryn's Units. */
#include "adt_disposal.h"
#include "adt_list.h"


typedef struct ArrayList_ ArrayList_;
typedef ArrayList_* ArrayListPtr; /* Type-safe opaque type. */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Array list common routines. */
ArrayListPtr adt_array_list_init(DisposeRoutine disposer);
ArrayListPtr adt_array_list_dispose(ArrayListPtr list);
unsigned int adt_array_list_is_empty(ArrayListPtr list);
void adt_array_list_make_empty(ArrayListPtr list);
unsigned int adt_array_list_items(ArrayListPtr list);
AdtPtr adt_array_list_get_indexed_item(ArrayListPtr list, unsigned int index);

#if ADT_MEMORY_DEBUG
void adt_array_list_note_ptrs(ArrayListPtr list);
#endif /* ADT_MEMORY_DEBUG */

/* Array list plain interface routines. */
unsigned int adt_array_list_add_first(ArrayListPtr list, AdtPtr data);
unsigned int adt_array_list_add_last(ArrayListPtr list, AdtPtr data);
unsigned int adt_array_list_add_at(ArrayListPtr list, AdtPtr data, unsigned int index );
AdtPtr adt_array_list_get_first(ArrayListPtr list);
AdtPtr adt_array_list_get_last(ArrayListPtr list);
void adt_array_list_remove(ArrayListPtr list, AdtPtr data);
AdtPtr adt_array_list_remove_at(ArrayListPtr list, unsigned int index);
AdtPtr adt_array_list_remove_first(ArrayListPtr list);
AdtPtr adt_array_list_remove_last(ArrayListPtr list);
unsigned int adt_array_list_contains(ArrayListPtr list, AdtPtr data);

/* Array list iterator interface routines. */
IteratorPtr adt_array_list_get_iterator(ArrayListPtr list);

/* Array list iterator routines. */
IteratorPtr adt_array_list_iterator_dispose(IteratorPtr itor);
unsigned int adt_array_list_iterator_is_in_list(IteratorPtr itor);
void adt_array_list_iterator_zeroth(IteratorPtr itor);
void adt_array_list_iterator_first(IteratorPtr itor);
void adt_array_list_iterator_last(IteratorPtr itor);
unsigned int adt_array_list_iterator_is_equal( IteratorPtr itor1, IteratorPtr itor2 );
void adt_array_list_iterator_advance(IteratorPtr itor);
unsigned char adt_array_list_iterator_advance_to( IteratorPtr itor1, IteratorPtr itor2 );
void adt_array_list_iterator_retreat(IteratorPtr itor);
AdtPtr adt_array_list_iterator_retrieve(IteratorPtr itor);
IteratorPtr adt_array_list_iterator_equate(IteratorPtr itor1, IteratorPtr itor2);
unsigned int adt_array_list_iterator_find(IteratorPtr itor, AdtPtr data);
unsigned int adt_array_list_iterator_add(IteratorPtr itor, AdtPtr data);
void adt_array_list_iterator_remove(IteratorPtr itor);
void adt_array_list_iterator_swap(IteratorPtr itor1, IteratorPtr itor2);
IteratorPtr adt_array_list_iterator_duplicate(IteratorPtr itor);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_ARRAY_LIST_H */
