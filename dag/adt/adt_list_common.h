/* ------------------------------------------------------------------------------
 adt_list_common from Koryn's Units.

 Data types that the adt_list and adt_iterator_list have in common.

 Copyright (c) 2001-2004, Koryn Grant <koryn@clear.net.nz>.
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

#ifndef ADT_LIST_COMMON_H
#define ADT_LIST_COMMON_H


typedef struct ListNode ListNode;
typedef ListNode* ListNodePtr;

struct ListNode
{
	AdtPtr mData;
	ListNodePtr mNext;
	ListNodePtr mPrev;
};


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Common list operations. */
typedef AdtPtr (*list_dispose_routine)(AdtPtr header);
typedef unsigned int (*list_is_empty_routine)(AdtPtr header);
typedef unsigned int (*list_items_routine)(AdtPtr header);
typedef AdtPtr (*list_retrieve_routine)(AdtPtr header, unsigned int index);
typedef void (*list_make_empty_routine)(AdtPtr header);
typedef void (*list_note_ptrs_routine)(AdtPtr header);


/* List plain interface operations. */
typedef unsigned int (*list_add_first_routine)(AdtPtr header, AdtPtr data);
typedef unsigned int (*list_add_last_routine)(AdtPtr header, AdtPtr data);
typedef unsigned int (*list_add_at_routine)(AdtPtr header, AdtPtr data, unsigned int index );
typedef AdtPtr (*list_get_first_routine)(AdtPtr header);
typedef AdtPtr (*list_get_last_routine)(AdtPtr header);
typedef void (*list_remove_routine)(AdtPtr header, AdtPtr data);
typedef AdtPtr (*list_remove_at_routine)(AdtPtr header, unsigned int index);
typedef AdtPtr (*list_remove_first_routine)(AdtPtr header);
typedef AdtPtr (*list_remove_last_routine)(AdtPtr header);
typedef unsigned int (*list_contains_routine)(AdtPtr header, AdtPtr data);

/* List iterator interface operations. */
typedef AdtPtr (*list_get_iterator_routine)(AdtPtr header);

/* Iterator operations. */
typedef AdtPtr (*iterator_dispose_routine)(AdtPtr header);
typedef unsigned int (*iterator_is_in_list_routine)(AdtPtr header);
typedef unsigned int (*iterator_is_last_routine)(AdtPtr header);
typedef void (*iterator_zeroth_routine)(AdtPtr header);
typedef void (*iterator_first_routine)(AdtPtr header);
typedef void (*iterator_last_routine)(AdtPtr header);
typedef unsigned int (*iterator_is_equal_routine)( AdtPtr header1, AdtPtr header2 );
typedef void (*iterator_advance_routine)(AdtPtr header);
typedef unsigned int (*iterator_advance_to_routine)(AdtPtr header1, AdtPtr header2 );
typedef void (*iterator_retreat_routine)(AdtPtr header);
typedef AdtPtr (*iterator_retrieve_routine)(AdtPtr header);
typedef AdtPtr (*iterator_equate_routine)(AdtPtr header1, AdtPtr header2);
typedef unsigned int (*iterator_find_routine)(AdtPtr header, AdtPtr data);
typedef unsigned int (*iterator_add_routine)(AdtPtr header, AdtPtr data);
typedef AdtPtr (*iterator_remove_routine)(AdtPtr header);
typedef void (*iterator_swap_routine)(AdtPtr header1, AdtPtr header2);
typedef AdtPtr (*iterator_duplicate_routine)(AdtPtr header);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ADT_LIST_COMMON_H */
