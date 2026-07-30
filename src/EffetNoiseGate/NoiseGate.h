#pragma once

#include "../includes/Utils.h"
#include <Arduino.h>
#include "AudioStream.h"

class NoiseGateEffect : public AudioStream {
public:
    NoiseGateEffect();
    ~NoiseGateEffect() = default;

    virtual void update() override;

    void setEnabled(bool e) { active = e; }
    bool isEnabled() const { return active; }

    void setThreshold(float threshold); 
    void setAttack(float attackMs);
    void setRelease(float releaseMs);
    void setParameter(int param_id, float value);

private:
    bool active = false;
    audio_block_t* inputQueueArray_[1];

    float threshold_ = 0.01f;
    float attackMs_ = 1.0f;
    float releaseMs_ = 50.0f;
    float attackCoef_ = 0.0f;
    float releaseCoef_ = 0.0f;
    float envelope_ = 0.0f;
    float gain_ = 1.0f;

    void calculateCoefs();
};
