// Disto Reverbscape

#include "Disto.h"

#include <span>
#include <vector>
#include <cmath>
#include <algorithm>

inline float fast_tanh(float x) {
    if (x <= -3.0f) return -1.0f;
    if (x >= 3.0f) return 1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float fast_atan(float x) {
    // Fast overdrive approximation replacing atan
    return x / (1.0f + std::abs(x));
}


DistoEffect::DistoEffect(float sampleRate)
    : AudioStream(1, inputQueueArray_),
      preFilter(preFilterCutoffBase, sampleRate),
      postFilter(postFilterCutoff, sampleRate),
      upsamplingLowpassFilter(0.0f, sampleRate)
{
    tone.Init(sampleRate);

    // Pivot between 500 Hz and 2 kHz as the tone amount changes
    tone.SetFreq(500.0f + 1500.0f * toneAmount);

    samplerate = sampleRate;

    setDistoMode(1);
    setTone(0.5f);
    setVolume(1.0f);
    setGain(1.0f);
    setIntensity(0.5f);
    setOversamp(false);

    InitializeFilters();
}

void DistoEffect::InitializeFilters() {
    preFilter.config(preFilterCutoffBase, samplerate);

    if (oversamp) {
        postFilter.config(postFilterCutoff, samplerate * overFactor);
    } else {
        postFilter.config(postFilterCutoff, samplerate);
    }

    upsamplingLowpassFilter.config(samplerate / (2.0f * static_cast<float>(overFactor)), samplerate);
}

float DistoEffect::hardClipping(float input, float threshold) { 
    return std::clamp(input, -threshold, threshold); 
}

float DistoEffect::diodeClipping(float input, float threshold) {
    // Soft exponential knee mimicking a diode curve
    if (input > threshold)
        return threshold + (1.0f - expf(-(input - threshold)));
    else if (input < -threshold)
        return -threshold - (1.0f - expf(input + threshold));
    return input;
}

float DistoEffect::softClipping(float input, float gainVal) { 
    return fast_tanh(input * gainVal); 
}

float DistoEffect::fuzzEffect(float input, float intensityVal) {
    // Fuzz faces are often heavily asymmetrical
    float driven = input * (1.0f + intensityVal * 20.0f); // Massive gain
    
    if (driven > 0.0f) {
        return fast_tanh(driven);
    } else {
        // Hard clip the negative side for strong even harmonics
        return std::clamp(driven, -1.0f, 0.0f);
    }
}

float DistoEffect::tubeSaturation(float input, float gainVal) { 
    // Atan is a softer curve than tanh, good for tube warmth
    return fast_atan(input * gainVal); 
}

float DistoEffect::multiStage(float sample, float drive, float intensityVal) {
    // Stage 1: Soft clip
    float s1 = fast_tanh(sample * drive);
    // Stage 2: Boost and soft clip again
    float s2 = fast_tanh(s1 * (1.0f + intensityVal * 5.0f));
    return s2;
}

float DistoEffect::testDistortion(float input, float gainVal){
    float g = input * gainVal;
    float sign = (g < 0.0f) ? -1.0f : 1.0f;
    return sign * (1.0f - expf(-std::abs(g)));
}

float DistoEffect::testOverDrive(float input){
    // Standard cubic soft clipper (mathematically continuous without glitches)
    // f(x) = x - x^3/3 for -1 < x < 1
    if (input >= 1.0f) return 0.666666f;
    if (input <= -1.0f) return -0.666666f;
    return input - (input * input * input) / 3.0f;
}

float DistoEffect::testFuzz(float input, float gainVal){
    return input;
}

// Helper functions for oversampling
std::vector<float> DistoEffect::upsample(const std::vector<float> &input, int factor, float sample_rate) {
    std::vector<float> output(input.size() * factor, 0.0f);

    for (size_t i = 0; i < input.size(); ++i) {
        // Insert input samples, leaving zeros in between. Scale by factor to preserve energy after lowpass.
        output[i * factor] = input[i] * factor;
    }

    // Apply the low-pass filter to smooth interpolated samples
    for (size_t i = 0; i < output.size(); ++i) {
        output[i] = upsamplingLowpassFilter(output[i]);
    }

    return output;
}

std::vector<float> DistoEffect::downsample(const std::vector<float> &input, int factor) {
    std::vector<float> output(input.size() / factor);
    for (size_t i = 0; i < output.size(); ++i) {
        output[i] = input[i * factor]; // Take every nth sample
    }
    return output;
}

void DistoEffect::processDistortion(float &sample,           // Sample to process
                                    const float &gainVal,       // Gain
                                    const int &clippingType, // Clipping type
                                    const float &intensityVal)  // Intensity
{
    // Do NOT multiply sample *= gainVal here, otherwise we get double gain!
    
    switch (clippingType) {
    case 0: // Hard Clipping
        {
            float driven = sample * gainVal;
            float thresh = 1.0f - (intensityVal * 0.9f); // 1.0 to 0.1
            sample = hardClipping(driven, thresh) / thresh; // Normalize volume
        }
        break;
    case 1: // Soft Clipping
        // Use intensity to add extra drive
        sample = softClipping(sample, gainVal * (1.0f + intensityVal * 4.0f));
        break;
    case 2: // Fuzz
        sample = fuzzEffect(sample * gainVal, intensityVal);
        break;
    case 3: // Tube Saturation
        sample = tubeSaturation(sample, gainVal * (1.0f + intensityVal * 4.0f));
        break;
    case 4: // Multi-stage
        sample = multiStage(sample, gainVal, intensityVal);
        break;
    case 5: // Diode Clipping
        {
            float driven = sample * gainVal;
            float thresh = 1.0f - (intensityVal * 0.9f);
            sample = diodeClipping(driven, thresh) / thresh;
        }
        break;
    case 6: // Test Distortion (Exponential soft clip)
        sample = testDistortion(sample, gainVal * (1.0f + intensityVal * 4.0f));
        break;
    case 7: // Test Overdrive (Cubic soft clip)
        {
            float driven = sample * gainVal * (1.0f + intensityVal * 4.0f);
            sample = testOverDrive(driven) * 1.5f; // 1.5 normalizes 2/3 output to 1.0
        }
        break;
    }
}

void DistoEffect::normalizeVolume(float &sample, int clippingType) {
    // Balance perceived loudness between different distortion algorithms
    // A square wave (hard clip) sounds much louder than a rounded wave peaking at the same level.
    switch (clippingType) {
    case 0: // Hard Clipping (very loud)
        sample *= 0.5f;
        break;
    case 1: // Soft Clipping
        sample *= 0.7f;
        break;
    case 2: // Fuzz (very loud)
        sample *= 0.5f;
        break;
    case 3: // Tube Saturation (softer)
        sample *= 0.9f;
        break;
    case 4: // Multi-stage (loud)
        sample *= 0.6f;
        break;
    case 5: // Diode Clipping
        sample *= 0.5f;
        break;
    case 6: // Test Distortion
        sample *= 0.7f;
        break;
    case 7: // Test Overdrive (Cubic)
        sample *= 0.75f;
        break;
    }
}

float DistoEffect::ProcessTiltToneControl(float input) {
    // Process input with one-pole low-pass
    const float lp = tone.Process(input);

    // Compute the high-passed portion
    const float hp = input - lp;

    // Crossfade: toneAmount=0 => all LP (more bass), toneAmount=1 => all HP (more treble)
    return lp * (1.f - toneAmount) + hp * toneAmount;
}

void DistoEffect::update() {
    audio_block_t* in = receiveReadOnly(0);
    if (!in) return;

    if (!active) {
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
        float inputL = (float)in->data[i] / 32768.0f;
        
        // Bruit de Nyquist (alterné) : +1e-9f, -1e-9f, +1e-9f...
        anti_denormal = -anti_denormal;
        inputL += anti_denormal;

        float distorted = inputL;

        // Apply high-pass filter to remove excessive low frequencies
        distorted = preFilter(distorted);

        const float computed_gain = min_gain + (this->gain * (max_gain - min_gain));

        // Reduce signal amplitude before clipping
        distorted = distorted * 0.5f;

        processDistortion(distorted, computed_gain, effect_mode, intensity);

        // Post-filter: Low-pass to smooth out harsh high frequencies
        distorted = postFilter(distorted);

        // Normalize the volume between the types of distortion
        normalizeVolume(distorted, effect_mode);

        // Apply tilt-tone filter
        const float effect_output = ProcessTiltToneControl(distorted);

        // Pas de mixage dry/wet, signal 100% effet
        float output = effect_output * volume;

        if (output > 1.0f) output = 1.0f;
        if (output < -1.0f) output = -1.0f;
        out->data[i] = (int16_t)(output * 32767.0f);
    }

    transmit(out, 0);
    release(out);
    release(in);
}

// --- Implémentation des Setters Spécifiques ---

void DistoEffect::setGain(float val) {
    gain = clampf(val, 0.0f, 10.0f);
}

void DistoEffect::setTone(float freq) {
    toneAmount = clampf(freq, 0.0f, 1.0f);
    toneFreq = 500.0f + toneAmount * 1500.0f ;
    tone.SetFreq(toneFreq);
}

void DistoEffect::setVolume(float vol){
    volume = clampf(vol, 0.0f, 1.0f);
}

void DistoEffect::setOversamp(bool tmp){
    oversamp = tmp;
}

void DistoEffect::setDistoMode(int mode) {
    effect_mode = mode;
}

void DistoEffect::setIntensity(float val) {
    intensity = clampf(val, 0.0f, 1.0f);
}

void DistoEffect::setParameter(int param_id, float value) {
    switch (param_id){
        case 0 : 
            // Mix supprimé (toujours 100% wet)
            break;
        case 1 : 
            setGain(value);
            break;
        case 2 :  
            // Les seuils (thresholds) correspondent aux bascules exactes entre les crans sur le GUI
            if (value >= 0.929f)      setDistoMode(7); // Mode 8 (Test OD)   - MIDI 118 à 127
            else if (value >= 0.786f) setDistoMode(6); // Mode 7 (Test)      - MIDI 100 à 117
            else if (value >= 0.643f) setDistoMode(5); // Mode 6 (Diode)     - MIDI 82 à 99
            else if (value >= 0.500f) setDistoMode(4); // Mode 5 (Multi)     - MIDI 64 à 81
            else if (value >= 0.357f) setDistoMode(3); // Mode 4 (Tube)      - MIDI 46 à 63
            else if (value >= 0.214f) setDistoMode(2); // Mode 3 (Fuzz)      - MIDI 28 à 45
            else if (value >= 0.071f) setDistoMode(1); // Mode 2 (Soft Clip) - MIDI 10 à 27
            else                      setDistoMode(0); // Mode 1 (Hard Clip) - MIDI 0 à 9
            break;
        case 3 : 
            setTone(value);
            break;
        case 4 : 
            setIntensity(value);
            break;
        case 5 : 
            setOversamp(value);
            break;
        case 6 : 
            setVolume(value);
            break;
        default:
#if !USE_DAISY
            Serial.print("Parametre invalide: ");
            Serial.println(param_id);
#endif
            break;
    };
}