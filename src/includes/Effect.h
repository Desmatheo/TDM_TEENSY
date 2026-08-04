#pragma once

#include "Utils.h"

#if !TEENSY
#include "daisy_seed.h"
#endif

#define CPU_LoadEffect 1
#define CPU_LoadAll 1

class Effect {
public:

// #if CPU_LoadEffect
//     uint32_t profiled_ticks = 0;
//     uint32_t last_profiled_ticks = 0;
// #endif


    virtual ~Effect() = default;
#if TEENSY
    virtual void update(float* buffer, int numSamples) = 0;
#else
    virtual void update(const float** in, float** out, int idx) = 0;
#endif
    // virtual float updateTest(const float in, float out, int idx) = 0;
    virtual void setParameter(int param_id, float value) = 0;

    // --- Enabled/Bypass commun à tous les effets ---
    void setEnabled(bool e) { active_ = e; }
    bool isEnabled() const  { return active_; }

protected:
    bool active_ = true;
};