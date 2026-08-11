#pragma once

#include "../includes/Utils.h"
#include "../includes/Effect.h"

#include <Arduino.h>
#include "AudioStream.h"

#include <q/fx/biquad.hpp>
#include <vector>

#include "../includes/toneDaisySP/tone.h"

class DistoEffect 
#if !UtilBypassRoutage
: public AudioStream 
#else 
: public Effect
#endif
{

    public:

    static constexpr float preFilterCutoffBase = 140.0f;
    static constexpr float preFilterCutoffMax = 300.0f;
    static constexpr float postFilterCutoff = 8000.0f;
    static constexpr uint8_t overFactor = 16;

    DistoEffect(float sampleRate = AUDIO_SAMPLE_RATE_EXACT); 

#if !TEENSY
    virtual void update(const float** in, float** out, int idx) override;
#else
#if !UtilBypassRoutage
    virtual void update() override;
#else 
    virtual void update(float* buffer, int numSamples) override;
#endif
#endif

    void setGain(float gain);               // param_id 1
    void setDistoMode(int mode);            // param_id 2
    void setTone(float tone);               // param_id 3
    void setIntensity(float intensity);     // param_id 4
    void setOversamp(bool oversamp);        // param_id 5
    void setVolume(float vol);              // param_id 6
    
    void InitializeFilters();

    float ProcessTiltToneControl(float input);

    void setParameter(int param_id, float value);

private:
#if !UtilBypassRoutage
    audio_block_t* inputQueueArray_[1];
#endif
    float anti_denormal = 1e-9f;

    float samplerate;

    float volume = 1;
    float dryMix = 0.5f;
    float wetMix = 0.5f;

    float gain;
    float min_gain = 1.0f;
    float max_gain = 20.0f;

    float toneFreq;
    float toneAmount = 0.5f;
    bool oversamp;
    float intensity;

    int effect_mode = 0;

    daisysp::Tone2 tone;

    // Filters as member variables to prevent cross-channel memory sharing
    cycfi::q::highpass preFilter;
    cycfi::q::lowpass postFilter;
    cycfi::q::lowpass upsamplingLowpassFilter;

    // Helper functions for distortion and clipping
    float hardClipping(float input, float threshold);
    float diodeClipping(float input, float threshold);
    float softClipping(float input, float gainVal);
    float fuzzEffect(float input, float intensityVal);
    float tubeSaturation(float input, float gainVal);
    float multiStage(float sample, float drive, float intensityVal);

    float testDistortion(float input, float gainVal);
    float testOverDrive(float input);
    float testFuzz(float input, float gainVal);
    
    std::vector<float> upsample(const std::vector<float> &input, int factor, float sample_rate);
    std::vector<float> downsample(const std::vector<float> &input, int factor);
    void processDistortion(float &sample, const float &gainVal, const int &clippingType, const float &intensityVal);
    void normalizeVolume(float &sample, int clippingType);
};