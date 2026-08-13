#include "Equalizer.h"

EqualizerEffect::EqualizerEffect() 
#if !UtilBypassRoutage
: AudioStream(1, inputQueueArray_)
#endif
{
    volume = 1.0f;
    for(int i = 0; i < 5; i++) {
        gains_db[i] = 0.0f;
    }
    
    // Initialise l'instance CMSIS DSP pour 5 stages
    arm_biquad_cascade_df1_init_f32(&iir_inst, 5, pCoeffs, pState);
    calculateCoeffs();
}

void EqualizerEffect::begin() {
    calculateCoeffs();
}

void EqualizerEffect::calculateCoeffs() {
    // Fréquences standards pour un EQ 5 bandes
    float frequencies[5] = {80.0f, 250.0f, 750.0f, 2200.0f, 6600.0f};
    float Q = 1.414f; 
    float Fs = 44100.0f; 
    
    for (int i = 0; i < 5; i++) {
        float f0 = frequencies[i];
        float gainDB = gains_db[i];
        
        float A = powf(10.0f, gainDB / 40.0f); // sqrt(10^(gainDB/20))
        float w0 = 2.0f * PI * f0 / Fs;
        float alpha = sinf(w0) / (2.0f * Q);
        
        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * cosf(w0);
        float b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * cosf(w0);
        float a2 = 1.0f - alpha / A;
        
        // Normalisation par a0
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
        
        // CMSIS DSP stocke : b0, b1, b2, -a1, -a2
        int idx = i * 5;
        pCoeffs[idx]     = b0;
        pCoeffs[idx + 1] = b1;
        pCoeffs[idx + 2] = b2;
        pCoeffs[idx + 3] = -a1;
        pCoeffs[idx + 4] = -a2;
    }
}

void EqualizerEffect::setBand(int band_index, float value_norm) {
    if (band_index >= 0 && band_index < 5) {
        // Mappe [0.0, 1.0] vers [-12dB, +12dB]
        gains_db[band_index] = (value_norm - 0.5f) * 24.0f;
        calculateCoeffs();
    }
}

void EqualizerEffect::setVolume(float vol) {
    volume = vol;
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 2.0f) volume = 2.0f;
}

void EqualizerEffect::setParameter(int param_id, float value) {
    if (param_id >= 0 && param_id <= 4) {
        setBand(param_id, value);
    } else if (param_id == 5) {
        setVolume(value * 2.0f);
    }
}

#if !TEENSY
void EqualizerEffect::update(const float** in, float** out, int idx) {
    // Non implémenté pour Daisy actuellement
}
#else
#if !UtilBypassRoutage
void EqualizerEffect::update() {
    audio_block_t* block = receiveWritable();
    if (!block) return;
    
    if (!active_) {
        transmit(block);
        release(block);
        return;
    }
    
    float f32_block[128];
    for (int i = 0; i < 128; i++) {
        f32_block[i] = (float)block->data[i] / 32768.0f;
    }
    
    float f32_out[128];
    arm_biquad_cascade_df1_f32(&iir_inst, f32_block, f32_out, 128);
    
    for (int i = 0; i < 128; i++) {
        float sample = f32_out[i] * volume;
        sample = clampf(sample, -1.0f, 1.0f);
        block->data[i] = (int16_t)(sample * 32767.0f);
    }
    
    transmit(block);
    release(block);
}
#else
void EqualizerEffect::update(float* buffer, int numSamples) {
    if (!active_) return;
    
    // Traitement in-place
    arm_biquad_cascade_df1_f32(&iir_inst, buffer, buffer, numSamples);
    
    for (int i = 0; i < numSamples; i++) {
        buffer[i] *= volume;
    }
}
#endif
#endif
