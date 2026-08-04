#pragma once

#include "../includes/Utils.h"
#include "../includes/Effect.h"
#include <Arduino.h>
#include "AudioStream.h"

class NoiseGateEffect 
#if !UtilBypassRoutage
: public AudioStream 
#else
: public Effect
#endif
{
public:
    NoiseGateEffect();
    ~NoiseGateEffect() = default;

#if !TEENSY
    virtual void update(const float** in, float** out, int idx) override;
#else
#if !UtilBypassRoutage
    virtual void update() override;
#else
    virtual void update(float* buffer, int numSamples) override;
#endif
#endif



    void setThreshold(float threshold); 
    void setAttack(float attackMs);
    void setRelease(float releaseMs);
    void setParameter(int param_id, float value);

private:
#if !UtilBypassRoutage
    audio_block_t* inputQueueArray_[1];
#endif

    float threshold_ = 0.01f;
    float attackMs_ = 1.0f;
    float releaseMs_ = 50.0f;
    float attackCoef_ = 0.0f;
    float releaseCoef_ = 0.0f;
    float envelope_ = 0.0f;
    float gain_ = 1.0f;

    void calculateCoefs();
};
