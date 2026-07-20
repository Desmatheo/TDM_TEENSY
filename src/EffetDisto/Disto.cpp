// Disto Reverbscape

#include "Disto.h"

#include <span>
#include <vector>
#include <cmath>
#include <algorithm>

DistoEffect::DistoEffect(float sampleRate)
    : AudioStream(1, inputQueueArray_),
      preFilter(preFilterCutoffBase, sampleRate),
      postFilter(postFilterCutoff, sampleRate),
      upsamplingLowpassFilter(0.0f, sampleRate)
{
    tone.Init(sampleRate);

    // Pivot between 500 Hz and 2 kHz as the tone amount changes
    tone.SetFreq(toneFreq);

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

float DistoEffect::hardClipping(float input, float threshold) { return std::clamp(input, -threshold, threshold); }

float DistoEffect::diodeClipping(float input, float threshold) {
    if (input > threshold)
        return threshold - std::exp(-(input - threshold));
    else if (input < -threshold)
        return -threshold + std::exp(input + threshold);
    return input;
}

float DistoEffect::softClipping(float input, float gainVal) { return std::tanh(input * gainVal); }

float DistoEffect::fuzzEffect(float input, float intensityVal) {
    // Symmetrical clipping with extreme compression
    float fuzzed = softClipping(input, intensityVal);

    // Introduce a slight asymmetry for a classic fuzz character and adds harmonic content
    fuzzed += 0.05f * std::sin(input * 20.0f);

    // Dynamic response: Adjust the intensity based on the input signal's amplitude
    const float dynamicIntensity = intensityVal * (1.0f + 0.5f * std::abs(input));
    fuzzed = softClipping(fuzzed, dynamicIntensity);

    return fuzzed;
}

float DistoEffect::tubeSaturation(float input, float gainVal) { return std::atan(input * gainVal); }

float DistoEffect::multiStage(float sample, float drive, float intensityVal) {
    // First stage
    const float stage1 = softClipping(sample, drive * intensityVal * 2.0f);

    // Second stage
    const float stage2 = softClipping(stage1, drive * intensityVal);

    // Power amp, mimic second tube clipping, possibly negative feedback
    const float result = tubeSaturation(stage2, drive * intensityVal);

    return result;
}

float DistoEffect::dynamicPreFilterCutoff(float inputEnergy) {
    return preFilterCutoffBase + (preFilterCutoffMax - preFilterCutoffBase) * std::tanh(inputEnergy);
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
    sample *= gainVal;

    switch (clippingType) {
    case 0: // Hard Clipping
        {
            float threshold = 1.0f - intensityVal * 0.9f;
            sample = hardClipping(sample, threshold) / threshold;
        }
        break;
    case 1: // Soft Clipping
        sample = softClipping(sample, 1.0f);
        break;
    case 2: // Fuzz
        sample = fuzzEffect(sample, 1.0f + intensityVal * 9.0f);
        break;
    case 3: // Tube Saturation
        sample = tubeSaturation(sample, 1.0f + intensityVal * 9.0f);
        break;
    case 4: // Multi-stage
        sample = multiStage(sample, 1.0f, 1.0f + intensityVal * 9.0f);
        break;
    case 5: // Diode Clipping
        {
            float threshold = 1.0f - intensityVal * 0.9f;
            sample = diodeClipping(sample, threshold) / threshold;
        }
        break;
    }
}

void DistoEffect::normalizeVolume(float &sample, int clippingType) {
    switch (clippingType) {
    case 0: // Hard Clipping
        sample *= 1.0f;
        break;
    case 1: // Soft Clipping
        sample *= 1.0f;
        break;
    case 2: // Fuzz
        sample *= 1.0f;
        break;
    case 3: // Tube Saturation
        sample *= 1.0f;
        break;
    case 4: // Multi-stage
        sample *= 1.0f;
        break;
    case 5: // Diode Clipping
        sample *= 1.0f;
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
        inputL += 1e-9f; // Anti-denormal

        float distorted = inputL;

        // Apply high-pass filter to remove excessive low frequencies
        const float energy = std::abs(distorted);
        preFilter.config(dynamicPreFilterCutoff(energy), samplerate);
        distorted = preFilter(distorted);

        const float computed_gain = min_gain + (this->gain * (max_gain - min_gain));

        // Reduce signal amplitude before clipping
        distorted = distorted * 0.5f;

        if (oversamp) {
            // Prepare signal for oversampling
            std::vector<float> monoInput = {distorted};
            std::vector<float> oversampledInput = upsample(monoInput, overFactor, samplerate);

            // Apply gain and distortion processing
            for (float &sample : oversampledInput) {
                processDistortion(sample, computed_gain, effect_mode, intensity);

                // Post-filter: Low-pass to smooth out harsh high frequencies
                sample = postFilter(sample);
            }

            // Downsample back to original sample rate
            const std::vector<float> downsampledOutput = downsample(oversampledInput, overFactor);
            distorted = downsampledOutput[0];
        } else {
            processDistortion(distorted, computed_gain, effect_mode, intensity);

            // Post-filter: Low-pass to smooth out harsh high frequencies
            distorted = postFilter(distorted);
        }

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
    // freq = freq * 1500.0f;
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
            setGain(value);
            break;
        case 1 :  
            if (value >= 0.82f) setDistoMode(0);
            else if ((0.82f > value) && (value > 0.66f)) setDistoMode(1);
            else if ((0.66f > value) && (value > 0.49f)) setDistoMode(2);
            else if ((0.49f > value) && (value > 0.33f)) setDistoMode(3);
            else if ((0.33f > value) && (value > 0.16f)) setDistoMode(4);
            else setDistoMode(5);
            break; 
        case 2 : 
            setTone(value);
            break;
        case 3 : 
            setIntensity(value);
            break;
        case 4 : 
            setOversamp(value);
            break;
        case 5 : 
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