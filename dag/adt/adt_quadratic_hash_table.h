/* ------------------------------------------------------------------------------
 adt_quadratic_hash_table from Koryn's Units.

 Implementation of a quadratic probing hash table.
 The objects stored in the table are generic Ptrs.

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

#ifndef ADT_QUADRATIC_HASH_TABLE_H
#define ADT_QUADRATIC_HASH_TABLE_H

/* Koryn's Units */
#include "adt_comparison.h"
#include "adt_disposal.h"
#include "adt_hash.h"
#include "adt_iterate.h"


typedef struct QuadHashTable_ QuadHashTable_;
typedef QuadHashTable_* QuadHashTablePtr; /* Type-safe opaque type. */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Quadratic probing hash table ADT */
QuadHashTablePtr adt_quad_hash_table_init(CompareRoutine comparer, DisposeRoutine disposer, HashRoutine hasher);
QuadHashTablePtr adt_quad_hash_table_dispose(QuadHashTablePtr table);
void adt_quad_hash_table_make_empty(QuadHashTablePtr table);
unsigned int adt_quad_hash_table_items(QuadHashTablePtr table);
unsigned int adt_quad_hash_table_insert(QuadHashTablePtr table, AdtPtr data);
void adt_quad_hash_table_remove(QuadHashTablePtr table, AdtPtr data, unsigned int dispose);
AdtPtr adt_quad_hash_table_find(QuadHashTablePtr table, AdtPtr data);
AdtPtr adt_quad_hash_table_retrieve(QuadHashTablePtr table, unsigned int count);
void adt_quad_hash_table_iterate(QuadHashTablePtr table, IterationCallback iterator, AdtPtr context);

#if ADT_MEMORY_DEBUG
void adt_quad_hash_table_note_ptrs(QuadHashTablePtr table);
#endif /* ADT_MEMORY_DEBUG */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_QUADRATIC_HASH_TABLE_H */
