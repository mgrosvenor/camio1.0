#ifndef VSC3312_COMPONENT_H
#define VSC3312_COMPONENT_H

typedef struct
{
	uint32_t mIndex;
}vsc3312_state_t;

ComponentPtr
vsc3312_get_new_component(DagCardPtr card, uint32_t index);

GenericReadWritePtr get_grw_object(ComponentPtr component,uint32_t register_offset);

#endif
