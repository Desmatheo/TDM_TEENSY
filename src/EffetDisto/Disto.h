#pragma once

#include "../includes/Utils.h"

#include <Arduino.h>
#include "AudioStream.h"

#include <q/fx/biquad.hpp>
#include <vector>

#include "../includes/toneDaisySP/tone.h"

class DistoEffect : public AudioStream {

    public:

    static constexpr float preFilterCutoffBase = 140.0f;
    static constexpr float preFilterCutoffMax = 300.0f;
    static constexpr float postFilterCutoff = 8000.0f;
    static constexpr uint8_t overFactor = 16;

    float samplerate;

    float dryMix;
    float wetMix;
    float volume = 1;

    float gain;
    float min_gain = 1.0f;
    float max_gain = 20.0f;

    float toneFreq;
    bool oversamp;
    float intensity;

    int effect_mode = 0;

    daisysp::Tone2 tone;

    // Filters as member variables to prevent cross-channel memory sharing
    cycfi::q::highpass preFilter;
    cycfi::q::lowpass postFilter;
    cycfi::q::lowpass upsamplingLowpassFilter;

    DistoEffect(float sampleRate = AUDIO_SAMPLE_RATE_EXACT); 

    virtual void update() override;

    void setMix(float mix);                 // Ctrl 2 (0.0 -> 1.0)
    void setVolume(float vol);              // Ctrl 1 (0.0 -> 1.0)
    
    void setDistoMode(int mode);            // 3-Way Switch 2 (0, 1, 2)

    void setTone(float tone);               // Ctrl 3 (0.0 -> 1.0)
    void setIntensity(float intensity);     // Ctrl 4 (0.0 -> 1.0)
    void setGain(float gain);               // Ctrl 5 (0.0 -> 10.0)
    void setOversamp(bool oversamp);        // Switch 1 (0, 1)
    
    void InitializeFilters();

    float ProcessTiltToneControl(float input);

    void setParameter(int param_id, float value);

    // --- METHODES ET VARIABLES PARTAGEES ---
    void setEnabled(bool e) { active = e; }
    bool isEnabled() const  { return active; }

private:
    bool active = false; // effet actif ou non
    audio_block_t* inputQueueArray_[1];

    // Helper functions for distortion and clipping
    float hardClipping(float input, float threshold);
    float diodeClipping(float input, float threshold);
    float softClipping(float input, float gainVal);
    float fuzzEffect(float input, float intensityVal);
    float tubeSaturation(float input, float gainVal);
    float multiStage(float sample, float drive, float intensityVal);
    float dynamicPreFilterCutoff(float inputEnergy);
    
    std::vector<float> upsample(const std::vector<float> &input, int factor, float sample_rate);
    std::vector<float> downsample(const std::vector<float> &input, int factor);
    void processDistortion(float &sample, const float &gainVal, const int &clippingType, const float &intensityVal);
    void normalizeVolume(float &sample, int clippingType);
};