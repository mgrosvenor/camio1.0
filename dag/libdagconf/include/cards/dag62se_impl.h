#ifndef DAG62SE_IMPL_H
#define DAG62SE_IMPL_H

#define SAFETY_ITERATIONS 5000
#define SAFETY_NANOSECONDS 100

typedef struct
{
    ComponentPtr mGpp;
    ComponentPtr mPbm;
    ComponentPtr mTerf;
    ComponentPtr *mStream;
    ComponentPtr mPort; /* Single Port */
    ComponentPtr mHardwareMonitor;
    

    /* cached addresses */
    uint32_t mPhyBase;
    uint32_t mGppBase;
    uint32_t mPbmBase;
    uint32_t mSteerBase;
    uint32_t mSMBBase;

} dag62se_state_t;

#endif

