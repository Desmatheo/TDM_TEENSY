#pragma once
#include <Arduino.h>
#include "AudioStream.h"
#include "arm_math.h"
 
class TremoloEffect : public AudioStream {
public:
    TremoloEffect();
    virtual void update() override;
 
    void setEnabled(bool state);
    void setMix(float mix);
    void setDepth(float depth);
    void setRate(float rate_hz);
    void setWaveform(int mode);
    void setVolume(float vol);
    void setParameter(int param_id, float value);
 
private:
    bool enabled;
    float wetMix;
    float dryMix;
    float depth;
    float rate_hz;
    int waveform; // 0=Sine, 1=Tri, 2=Square, 3=Saw
    float volume;
 
    float phase;
    float phaseIncrement;
 
    audio_block_t* inputQueueArray_[2];
};