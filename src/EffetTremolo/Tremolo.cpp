#include "Tremolo.h"
 
TremoloEffect::TremoloEffect() : AudioStream(2, inputQueueArray_) {
    enabled = false;
    setMix(1.0f);
    setDepth(0.5f);
    setRate(10.0f);
    setWaveform(0);
    setVolume(1.0f);
    phase = 0.0f;
}
 
void TremoloEffect::setEnabled(bool state) {
    enabled = state;
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
    rate_hz = r;
    if (rate_hz < 0.0f) rate_hz = 0.0f;
    if (rate_hz > 20.0f) rate_hz = 20.0f;
    phaseIncrement = rate_hz / AUDIO_SAMPLE_RATE_EXACT;
}
 
void TremoloEffect::setWaveform(int mode) {
    waveform = mode;
}
 
void TremoloEffect::setVolume(float vol) {
    volume = vol;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
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
        case 5: setVolume(value); break;
        default: break;
    }
}
 
void TremoloEffect::update() {
    audio_block_t* inL = receiveReadOnly(0);
    audio_block_t* inR = receiveReadOnly(1);
 
    if (!inL && !inR) return;
 
    if (!enabled) {
        // Just pass through if disabled
        if (inL) transmit(inL, 0);
        if (inR) transmit(inR, 1);
        if (inL) release(inL);
        if (inR) release(inR);
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
 
        phase += phaseIncrement;
        if (phase >= 1.0f) phase -= 1.0f;
 
        float lfoValue = 0.0f;
        switch (waveform) {
            case 0: // Sine
                lfoValue = (sinf(phase * 2.0f * PI) + 1.0f) * 0.5f;
                break;
            case 1: // Tri
                if (phase < 0.5f) lfoValue = phase * 2.0f;
                else lfoValue = 2.0f - (phase * 2.0f);
                break;
            case 2: // Square
                lfoValue = (phase < 0.5f) ? 1.0f : 0.0f;
                break;
            case 3: // Saw
                lfoValue = phase;
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