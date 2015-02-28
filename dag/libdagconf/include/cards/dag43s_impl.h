#ifndef DAG43S_IMPL_H
#define DAG43S_IMPL_H
typedef struct
{
    ComponentPtr mGpp;
    ComponentPtr mPbm;
    ComponentPtr mTerf;
    ComponentPtr *mStream;
    ComponentPtr mPort; /* Single Port */

    /* cached addresses */
    uint32_t mPhyBase;
    uint32_t mGppBase;
    uint32_t mPbmBase;
    uint32_t mSteerBase;

} dag43s_state_t;

#endif

