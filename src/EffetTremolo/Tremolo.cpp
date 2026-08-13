#include "Tremolo.h"
 
float TremoloEffect::globalPhaseIncrement = 0.0f;
TremoloEffect* TremoloEffect::instances[6] = {nullptr};
int TremoloEffect::num_instances = 0;
float TremoloEffect::masterPhaseArray[128] = {0.0f};
float TremoloEffect::masterPhase = 0.0f;

TremoloEffect::TremoloEffect() 
#if !UtilBypassRoutage
    : AudioStream(2, inputQueueArray_) 
#endif
{
    if (num_instances < 6) {
        instances[num_instances++] = this;
    }
    active_ = true;
    setMix(1.0f);
    setDepth(0.5f);
    setPhaseOffset(0.0f);
    phaseMode = SYNC;
    localPhaseIncrement = 0.0f;
    setRate(10.0f);
    setWaveform(0);
    setVolume(1.0f);
    phase = 0.0f;
}
 

 
void TremoloEffect::setMix(float mix) {
    float clamped = mix;
    if (clamped < 0.0f) clamped = 0.0f;
    if (clamped > 1.0f) clamped = 1.0f;
    wetMix = clamped;
    dryMix = 1.0f - clamped;
}
 
void TremoloEffect::setDepth(float d) {
    depth = d;
    if (depth < 0.0f) depth = 0.0f;
    if (depth > 1.0f) depth = 1.0f;
}
 
void TremoloEffect::setRate(float r) {
    if (phaseMode == CUSTOM) {
        setLocalRate(r);
    } else {
        setGlobalRate(r);
    }
}

void TremoloEffect::setLocalRate(float r) {
    float clamped = r;
    if (clamped < 0.0f) clamped = 0.0f;
    if (clamped > 20.0f) clamped = 20.0f;
    localPhaseIncrement = clamped / AUDIO_SAMPLE_RATE_EXACT;
}

void TremoloEffect::setGlobalRate(float r) {
    float clamped_rate = r;
    if (clamped_rate < 0.0f) clamped_rate = 0.0f;
    if (clamped_rate > 20.0f) clamped_rate = 20.0f;
    globalPhaseIncrement = clamped_rate / AUDIO_SAMPLE_RATE_EXACT;
}

void TremoloEffect::setPhaseOffset(float offset) {
    phaseOffset = offset;
    while (phaseOffset >= 1.0f) phaseOffset -= 1.0f;
    while (phaseOffset < 0.0f) phaseOffset += 1.0f;
}

void TremoloEffect::setPhaseMode(int mode) {
    if (mode < 0) mode = 0;
    if (mode > 2) mode = 2;
    if (mode == CUSTOM && phaseMode != CUSTOM) {
        localPhaseIncrement = globalPhaseIncrement;
    }
    phaseMode = mode;
}
 
void TremoloEffect::setWaveform(int mode) {
    waveform = mode;
}
 
void TremoloEffect::setVolume(float vol) {
    volume = vol;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 2.0f) volume = 2.0f;
}
 
void TremoloEffect::setParameter(int param_id, float value) {
    switch(param_id) {
        case 0: setMix(value); break;
        case 1: setDepth(value); break;
        case 2: setRate(value * 20.0f); break; // value is 0.0 to 1.0, map to 0-20 Hz
        case 3:
            if (value < 0.2f) setWaveform(0);
            else if (value < 0.5f) setWaveform(1);
            else if (value < 0.8f) setWaveform(2);
            else setWaveform(3);
            break;
        case 4: 
            if (value < 0.33f) setPhaseMode(SYNC);
            else if (value < 0.66f) setPhaseMode(DEPHASED);
            else setPhaseMode(CUSTOM);
            break;
        case 5: setVolume(value * 2.0f); break;
        case 6: setPhaseOffset(value); break;
        default: break;
    }
}
 

#if !TEENSY
#else
#if !UtilBypassRoutage
void TremoloEffect::update() {
    if (this == instances[0]) {
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            masterPhase += globalPhaseIncrement;
            if (masterPhase >= 1.0f) masterPhase -= 1.0f;
            masterPhaseArray[i] = masterPhase;
        }
    }

    audio_block_t* inL = receiveReadOnly(0);
    audio_block_t* inR = receiveReadOnly(1);
 
    if (!inL && !inR) return;
 
    if (!active_) {
        // Just pass through if disabled
        if (inL) transmit(inL, 0);
        if (inR) transmit(inR, 1);
        if (inL) release(inL);
        if (inR) release(inR);
        
        if (phaseMode == CUSTOM) {
            phase += localPhaseIncrement * AUDIO_BLOCK_SAMPLES;
            while (phase >= 1.0f) phase -= 1.0f;
        }
        return;
    }
 
    audio_block_t* outL = allocate();
    audio_block_t* outR = allocate();
 
    if (!outL || !outR) {
        if (outL) release(outL);
        if (outR) release(outR);
        if (inL) release(inL);
        if (inR) release(inR);
        return;
    }
 
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float inputL = inL ? ((float)inL->data[i] / 32768.0f) : 0.0f;
        float inputR = inR ? ((float)inR->data[i] / 32768.0f) : inputL;
 
        float currentPhase = 0.0f;
        if (phaseMode == CUSTOM) {
            phase += localPhaseIncrement;
            if (phase >= 1.0f) phase -= 1.0f;
            currentPhase = phase;
        } else if (phaseMode == SYNC) {
            currentPhase = masterPhaseArray[i];
            phase = currentPhase; // Keep synced just in case
        } else if (phaseMode == DEPHASED) {
            currentPhase = masterPhaseArray[i] + phaseOffset;
            if (currentPhase >= 1.0f) currentPhase -= 1.0f;
            phase = currentPhase; // Keep synced just in case
        }

        float lfoValue = 0.0f;
        switch (waveform) {
            case 0: // Sine
                lfoValue = (sinf(currentPhase * 2.0f * PI) + 1.0f) * 0.5f;
                break;
            case 1: // Tri
                if (currentPhase < 0.5f) lfoValue = currentPhase * 2.0f;
                else lfoValue = 2.0f - (currentPhase * 2.0f);
                break;
            case 2: // Square
                lfoValue = (currentPhase < 0.5f) ? 1.0f : 0.0f;
                break;
            case 3: // Saw
                lfoValue = currentPhase;
                break;
        }
 
        float mod = (1.0f - depth) + (lfoValue * depth);
 
        float processedL = inputL * mod;
        float processedR = inputR * mod;
 
        float finalL = (inputL * dryMix + processedL * wetMix) * volume;
        float finalR = (inputR * dryMix + processedR * wetMix) * volume;
 
        // Clip
        if (finalL > 0.999f) finalL = 0.999f;
        if (finalL < -0.999f) finalL = -0.999f;
        if (finalR > 0.999f) finalR = 0.999f;
        if (finalR < -0.999f) finalR = -0.999f;
 
        outL->data[i] = (int16_t)(finalL * 32767.0f);
        outR->data[i] = (int16_t)(finalR * 32767.0f);
    }
 
    transmit(outL, 0);
    transmit(outR, 1);
   
    release(outL);
    release(outR);
    if (inL) release(inL);
    if (inR) release(inR);
}

#else
void TremoloEffect::update(float* buffer, int numSamples) {
    if (this == instances[0]) {
        for (int i = 0; i < numSamples; i++) {
            masterPhase += globalPhaseIncrement;
            if (masterPhase >= 1.0f) masterPhase -= 1.0f;
            if (i < 128) masterPhaseArray[i] = masterPhase;
        }
    }

    if (!active_) {
        if (phaseMode == CUSTOM) {
            phase += localPhaseIncrement * numSamples;
            while (phase >= 1.0f) phase -= 1.0f;
        }
        return;
    }
 
    for (int i = 0; i < numSamples; i++) {
        float inputL = buffer[i];
 
        float currentPhase = 0.0f;
        if (phaseMode == CUSTOM) {
            phase += localPhaseIncrement;
            if (phase >= 1.0f) phase -= 1.0f;
            currentPhase = phase;
        } else if (phaseMode == SYNC) {
            currentPhase = (i < 128) ? masterPhaseArray[i] : masterPhase;
            phase = currentPhase;
        } else if (phaseMode == DEPHASED) {
            currentPhase = ((i < 128) ? masterPhaseArray[i] : masterPhase) + phaseOffset;
            if (currentPhase >= 1.0f) currentPhase -= 1.0f;
            phase = currentPhase;
        }

        float lfoValue = 0.0f;
        switch (waveform) {
            case 0: // Sine
                lfoValue = (sinf(currentPhase * 2.0f * PI) + 1.0f) * 0.5f;
                break;
            case 1: // Tri
                if (currentPhase < 0.5f) lfoValue = currentPhase * 2.0f;
                else lfoValue = 2.0f - (currentPhase * 2.0f);
                break;
            case 2: // Square
                lfoValue = (currentPhase < 0.5f) ? 1.0f : 0.0f;
                break;
            case 3: // Saw
                lfoValue = currentPhase;
                break;
        }
 
        float mod = (1.0f - depth) + (lfoValue * depth);
        float processedL = inputL * mod;
        float finalL = (inputL * dryMix + processedL * wetMix) * volume;

        if (finalL > 0.999f) finalL = 0.999f;
        if (finalL < -0.999f) finalL = -0.999f;
        buffer[i] = finalL;
    }
}
#endif
#endif