/* File header */
#include "adt_comparison.h"

/* C Standard Library headers. */
#include "dag_platform.h"  //#include <inttypes.h>
#include <string.h>


/* Standard comparison functions */
comparison_t inline
adt_ptr_compare(AdtPtr first, AdtPtr second)
{
	if ((uintptr_t) first > (uintptr_t) second)
	{
		return kComparisonFirstIsBigger;
	}
	else if ((uintptr_t) first < (uintptr_t) second)
	{
		return kComparisonFirstIsSmaller;
	}

	return kComparisonEqual;
}


comparison_t inline
adt_string_compare(const char* first, const char* second)
{
	int result = strcmp((char*) first, (char*) second);

	if (result > 0)
	{
		return kComparisonFirstIsBigger;
	}
	else if (result < 0)
	{
		return kComparisonFirstIsSmaller;
	}

	return kComparisonEqual;
}
