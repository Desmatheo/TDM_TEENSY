#pragma once

#include "../includes/Utils.h"
#include "../includes/Effect.h"

#include <Arduino.h>
#include "AudioStream.h"
#include "arm_math.h"
 
class TremoloEffect 
#if !UtilBypassRoutage
: public AudioStream 
#else 
: public Effect
#endif
{
public:
    TremoloEffect();

#if !TEENSY
    virtual void update(const float** in, float** out, int idx) override;
#else
#if !UtilBypassRoutage
    virtual void update() override;
#else
    virtual void update(float* buffer, int numSamples) override;
#endif
#endif
 
 
    void setMix(float mix);
    void setDepth(float depth);
    void setRate(float rate_hz);
    void setWaveform(int mode);
    void setVolume(float vol);
    void setParameter(int param_id, float value);
 
private:
    float wetMix;
    float dryMix;
    float depth;
    float rate_hz;
    int waveform; // 0=Sine, 1=Tri, 2=Square, 3=Saw
    float volume;
 
    float phase;
    float phaseIncrement;
 
#if !UtilBypassRoutage
    audio_block_t* inputQueueArray_[2];
#endif
};