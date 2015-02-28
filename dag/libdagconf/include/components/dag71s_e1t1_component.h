#ifndef DAG71S_E1T1_COMPONENT_H
#define DAG71S_E1T1_COMPONENT_H

#include "dag_attribute_codes.h"
enum
{
    kE1T1Status = 0x40,
    kE1T1Config = 0x44,
    kE1T1LinkStatus = 0x48
};

ComponentPtr dag71s_get_new_e1t1(DagCardPtr card, int i);

#endif

