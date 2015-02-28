/* File header. */
#include "adt_disposal.h"


/* ADT headers. */
#include "adt_debugging.h"
#include "adt_memory.h"


/* Standard disposer that calls adt_dispose_ptr() on the item. */
void
adt_std_disposer(AdtPtr data)
{
#ifndef NDEBUG
	adt_assert_valid_ptr(data, "", __FILE__, __LINE__);
#endif /* Debugging code. */

	(void) adt_dispose_ptr(data);
}


void
adt_null_disposer(AdtPtr data)
{
#ifndef NDEBUG
	adt_assert_valid_ptr(data, "", __FILE__, __LINE__);
#endif /* Debugging code. */

	/* Do nothing. */
}
