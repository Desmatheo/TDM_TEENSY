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
    void setLocalRate(float rate_hz);
    void setWaveform(int mode);
    void setVolume(float vol);
    void setParameter(int param_id, float value);
 
    // Phase modes: 0=Sync, 1=Dephased, 2=Custom
    enum PhaseMode { SYNC = 0, DEPHASED = 1, CUSTOM = 2 };
    void setPhaseMode(int mode);
    void setPhaseOffset(float offset);
    static void setGlobalRate(float rate_hz);

private:
    float wetMix;
    float dryMix;
    float depth;
    int waveform; // 0=Sine, 1=Tri, 2=Square, 3=Saw
    float volume;
 
    float phase;
    float phaseOffset;
    float localPhaseIncrement;
    int phaseMode; // 0=Sync, 1=Dephased, 2=Custom
    
    static TremoloEffect* instances[6];
    static int num_instances;
    static float masterPhaseArray[128];
    static float masterPhase;
    static float globalPhaseIncrement;
 
#if !UtilBypassRoutage
    audio_block_t* inputQueueArray_[2];
#endif
};