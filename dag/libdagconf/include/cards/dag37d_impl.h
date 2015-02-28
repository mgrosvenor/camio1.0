#ifndef DAG37D_IMPL_H
#define DAG37D_IMPL_H

#include "../component.h"
typedef struct
{
	ComponentPtr mGpp;
	ComponentPtr mPbm;
	ComponentPtr *mStream;
	ComponentPtr mPort[2];
	ComponentPtr mPatternMatch[2];
    ComponentPtr mSC256;
    uint32_t mPhyBase;
    uint32_t mGppBase;
    uint32_t mGeneralBase;
} dag37d_state_t;
#endif

