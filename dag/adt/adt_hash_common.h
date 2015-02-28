/* ------------------------------------------------------------------------------
 adt_hash_common from Koryn's Units.

 Support routines and types common to 

	adt_int_hashmap
	adt_linear_hash_table
	adt_quadratic_hash_table

 Copyright (c) 2004, Koryn Grant (koryn@clear.net.nz)
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

#ifndef ADT_HASH_COMMON_H
#define ADT_HASH_COMMON_H

/* Koryn's Units. */
#include "adt_disposal.h"
#include "adt_types.h"


typedef struct
{
	AdtPtr mElement;
	unsigned int mActive;
	unsigned int mKey;

} HashEntry, *HashEntryPtr;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void initialise_hash_common(void);
HashEntryPtr new_hash_entry(AdtPtr data);
HashEntryPtr new_keyed_hash_entry(unsigned int key, AdtPtr data);
void dispose_hash_entry(HashEntryPtr entry);

AdtPtr allocate_array(unsigned int size);
void empty_hash_table(HashEntryPtr* array, unsigned int length, DisposeRoutine disposer);

#if ADT_MEMORY_DEBUG
void adt_hash_common_note_ptrs(void);
#endif /* ADT_MEMORY_DEBUG */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_HASH_COMMON_H */
