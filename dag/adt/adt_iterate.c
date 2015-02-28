/* File header */
#include "adt_iterate.h"


/* Standard iteration function */
unsigned int
adt_iterate_counter(AdtPtr data, AdtPtr userData)
{
	/* Increment the int pointed at by userData */
	(*((unsigned int *) userData))++;

	return 1;
}
