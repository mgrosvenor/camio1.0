#ifndef SONIC_COMPONENT_H
#define SONIC_COMPONENT_H

typedef struct
{
    uint32_t mIndex;
    uint32_t mSonicBase;
	uint32_t mStatusCache;
} sonic_state_t;

ComponentPtr sonic_get_new_component(DagCardPtr card, uint32_t index);

void sonic_dispose(ComponentPtr component);
void sonic_reset(ComponentPtr component);
void sonic_default(ComponentPtr component);
int sonic_post_initialize(ComponentPtr component);
dag_err_t sonic_update_register_base(ComponentPtr component);

#endif
