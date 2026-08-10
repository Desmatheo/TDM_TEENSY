#pragma once

#include "../includes/Utils.h"
#include "../includes/Effect.h"
#include <Arduino.h>
#include "AudioStream.h"

class CompresseurEffect 
#if !UtilBypassRoutage
: public AudioStream 
#else
: public Effect
#endif
{
public:
    CompresseurEffect();
    ~CompresseurEffect() = default;

#if !TEENSY
    virtual void update(const float** in, float** out, int idx) override;
#else
#if !UtilBypassRoutage
    virtual void update() override;
#else
    virtual void update(float* buffer, int numSamples) override;
#endif
#endif

    void setThreshold(float thresholdDb); // Seuil en dB (ex: -20.0f)
    void setRatio(float ratio);           // Ratio (ex: 4.0f pour 4:1)
    void setAttack(float attackMs);       // Attaque en ms
    void setRelease(float releaseMs);     // Relâchement en ms
    void setMakeupGain(float gainDb);     // Gain de compensation en dB
    
    virtual void setParameter(int param_id, float value) override;

private:
#if !UtilBypassRoutage
    audio_block_t* inputQueueArray_[1];
#endif

    float thresholdLinear_ = 0.1f;
    float thresholdDb_ = -20.0f;
    float ratio_ = 4.0f;
    float attackMs_ = 10.0f;
    float releaseMs_ = 100.0f;
    float makeupGainLinear_ = 1.0f;

    float attackCoef_ = 0.0f;
    float releaseCoef_ = 0.0f;
    
    float envelope_ = 0.0f;

    void calculateCoefs();
    
    // Pour la conversion linéaire vers décibel et inversement
    float dbToLinear(float db) { return powf(10.0f, db / 20.0f); }
    float linearToDb(float lin) { return (lin > 0.00001f) ? 20.0f * log10f(lin) : -100.0f; }
};
