#include "NoiseGate.h"

NoiseGateEffect::NoiseGateEffect()
#if !UtilBypassRoutage
: AudioStream(1, inputQueueArray_) 
#endif
{
    setAttack(1.0f);
    setRelease(50.0f);
    setThreshold(0.01f);
}

void NoiseGateEffect::calculateCoefs() {
    if (attackMs_ < 0.1f) attackMs_ = 0.1f;
    if (releaseMs_ < 0.1f) releaseMs_ = 0.1f;
    attackCoef_ = expf(-1.0f / (attackMs_ * 0.001f * AUDIO_SAMPLE_RATE_EXACT));
    releaseCoef_ = expf(-1.0f / (releaseMs_ * 0.001f * AUDIO_SAMPLE_RATE_EXACT));
}

void NoiseGateEffect::setThreshold(float threshold) {
    // Mapping from 0.0-1.0 to reasonable threshold levels
    // Typically threshold is between 0.0001 (very low) and 0.1 (high)
    // We can use a cubic curve for better resolution at low levels
    threshold_ = threshold * threshold * threshold * 0.1f; 
}

void NoiseGateEffect::setAttack(float attackMs) {
    attackMs_ = attackMs;
    calculateCoefs();
}

void NoiseGateEffect::setRelease(float releaseMs) {
    releaseMs_ = releaseMs;
    calculateCoefs();
}

void NoiseGateEffect::setParameter(int param_id, float value) {
    switch (param_id) {
        case 0: // Threshold
            setThreshold(value);
            break;
        case 1: // Attack
            setAttack(1.0f + value * 99.0f); // 1ms to 100ms
            break;
        case 2: // Release
            setRelease(10.0f + value * 990.0f); // 10ms to 1000ms
            break;
        default:
            break;
    }
}
#if !TEENSY
#else
#if !UtilBypassRoutage
void NoiseGateEffect::update() {
    audio_block_t* in = receiveReadOnly(0);
    if (!in) return;
    
    if (!active_) {
        transmit(in, 0);
        release(in);
        return;
    }

    audio_block_t* out = allocate();
    if (!out) {
        release(in);
        return;
    }

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float sample = (float)in->data[i] / 32768.0f;
        float absSample = fabsf(sample);

        // Simple envelope follower
        if (absSample > envelope_) {
            envelope_ = attackCoef_ * envelope_ + (1.0f - attackCoef_) * absSample;
        } else {
            envelope_ = releaseCoef_ * envelope_ + (1.0f - releaseCoef_) * absSample;
        }

        // Target gain based on threshold
        float targetGain = (envelope_ > threshold_) ? 1.0f : 0.0f;
        
        // Smooth gain transition to avoid clicks
        if (targetGain > gain_) {
            gain_ = attackCoef_ * gain_ + (1.0f - attackCoef_) * targetGain;
        } else {
            gain_ = releaseCoef_ * gain_ + (1.0f - releaseCoef_) * targetGain;
        }

        float output = sample * gain_;

        out->data[i] = (int16_t)(output * 32767.0f);
    }

    transmit(out, 0);
    release(out);
    release(in);
}
#else
void NoiseGateEffect::update(float* buffer, int numSamples) {
    if (!active_) return;

    for (int i = 0; i < numSamples; i++) {
        float sample = buffer[i];
        float absSample = fabsf(sample);

        // Simple envelope follower
        if (absSample > envelope_) {
            envelope_ = attackCoef_ * envelope_ + (1.0f - attackCoef_) * absSample;
        } else {
            envelope_ = releaseCoef_ * envelope_ + (1.0f - releaseCoef_) * absSample;
        }

        // Target gain based on threshold
        float targetGain = (envelope_ > threshold_) ? 1.0f : 0.0f;

        // Smooth gain transition to avoid clicks
        if (targetGain > gain_) {
            gain_ = attackCoef_ * gain_ + (1.0f - attackCoef_) * targetGain;
        } else {
            gain_ = releaseCoef_ * gain_ + (1.0f - releaseCoef_) * targetGain;
        }

        buffer[i] = sample * gain_;
    }
}
#endif 
#endif