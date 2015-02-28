#ifndef GPP_COMPONENT_H
#define GPP_COMPONENT_H

#include "../component_types.h"

ComponentPtr gpp_get_new_gpp(DagCardPtr card, uint32_t index);

typedef enum
{
    kGppLegacy = 0,
    kGppHDLCPOS,
    kGppEthernet,
    kGppATM,
    kGppAAL5,
    kGppMCHDLC,
    kGppMCRAW,
    kGppMCATM,
    kGppMCRAWChannel,
    kGppMCAAL5,
    kGppColorHDLCPOS,
    kGppColorEthernet
} gpp_record_type_t;

int gpp_record_type_set_value(ComponentPtr gpp, gpp_record_type_t record_type);
void gpp_active_set_value(AttributePtr attribute, void* value, int length);
void* gpp_active_get_value(AttributePtr attribute);

#endif


