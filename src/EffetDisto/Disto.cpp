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


DistoEffect::DistoEffect(float sampleRate) :
#if !UtilBypassRoutage
      AudioStream(1, inputQueueArray_),
#endif
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
        return threshold + (1.0f - fastexp(-(input - threshold)));
    else if (input < -threshold)
        return -threshold - (1.0f - fastexp(input + threshold));
    return input;
}

float DistoEffect::softClipping(float input, float gainVal) { 
    return fast_tanh(input * gainVal); 
}

float DistoEffect::fuzzEffect(float input, float intensityVal) {
    // Symmetrical clipping with extreme compression
    float fuzzed = softClipping(input, intensity);

    // Introduce a slight asymmetry for a classic fuzz character and adds harmonic content
    fuzzed += 0.05f * std::sin(input * 20.0f);

    // Dynamic response: Adjust the intensity based on the input signal's amplitude
    const float dynamicIntensity = intensity * (1.0f + 0.5f * std::abs(input));
    fuzzed = softClipping(fuzzed, dynamicIntensity);

    return fuzzed;
}

float DistoEffect::tubeSaturation(float input, float gainVal) { 
    // Atan is a softer curve than tanh, good for tube warmth
    return fast_atan(input * gainVal); 
}

float DistoEffect::multiStage(float sample, float drive, float intensityVal) {
    // First stage
    const float stage1 = softClipping(sample, drive * intensity * 2.0f);

    // Second stage
    const float stage2 = softClipping(stage1, drive * intensity);

    // Power amp, mimic second tube clipping, possibly negative feedback
    const float result = tubeSaturation(stage2, drive * intensity);

    return result;
}

float DistoEffect::testDistortion(float input, float gainVal){
    float g = input * gainVal;
    float sign = (g < 0.0f) ? -1.0f : 1.0f;
    return sign * (1.0f - fastexp(-std::abs(g)));
}

float DistoEffect::testOverDrive(float input){
    float threshold = 1.0f - intensity;
    if (threshold < 0.0001f) threshold = 0.0001f;
    
    float abs_input = std::abs(input);
    float sign = (input > 0.0f) ? 1.0f : ((input < 0.0f) ? -1.0f : 0.0f);
 
    if(abs_input < threshold){
        return 2.0f * input;
    }
    else if (abs_input > 2.0f * threshold){
        return sign * 1.0f;
    }
    else {
        float tmp = 2.0f - abs_input * 3.0f;
        return sign * ((3.0f - tmp * tmp) / 3.0f);      
    }
}

float DistoEffect::testFuzz(float input, float gainVal){

    // Dans une fuzz, le gain appliqué en amont est souvent énorme (ex: x50 ou x100)
    
    // Si le signal est positif, on le force au maximum (1.0)
    if (input > 0.05f) {
        return 1.0f;
    } 
    // S'il est négatif, on le force au minimum (-1.0)
    else if (input < -0.05f) {
        return -1.0f;
    }
    // Zone de transition très fine pour garder un tout petit peu d'attaque
    else {
        return input * gainVal * 2.0f; // Amplification pour la zone de transition
    }
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
        // sample = fuzzEffect(sample * gainVal, intensityVal * 10.0f);
        sample = testFuzz(sample, gainVal);
        break;
    case 3: // Tube Saturation
        sample = tubeSaturation(sample, gainVal * (1.0f + intensityVal * 10.0f));
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

// Utilisation si l'effet est utilisé dans le chainage
#if !TEENSY
void DistoEffect::update(const float** in, float** out, int idx) {
    float inputL;
    float inputR;

    // Bruit de Nyquist (alterné) : +1e-9f, -1e-9f, +1e-9f...
    // Contrairement au courant continu (DC), ce bruit traverse le filtre passe-haut (preFilter)
    // et empêche tous les filtres suivants (postFilter) de crasher sur des nombres sous-normaux.
    anti_denormal = -anti_denormal;

    inputL = inputR = in[0][idx] + anti_denormal;

    float distorted = inputL;

    // Apply high-pass filter to remove excessive low frequencies
    // (preFilter cutoff is now fixed in InitializeFilters to prevent audio-rate modulation crash)
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

    // Mixage final dry/wet pour cet effet de corde
    out[0][idx] = (inputL * dryMix + effect_output * wetMix) * volume;
    out[1][idx] = out[0][idx];
}
#else
#if !UtilBypassRoutage
void DistoEffect::update() {
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

#else
// Utilisation si l'effet est dans le bypass
void DistoEffect::update(float* buffer, int numSamples) {
    if (!active_) return;

    for (int i = 0; i < numSamples; i++) {
        float inputL = buffer[i];

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

        // Signal 100% effet
        float output = effect_output * volume;

        if (output > 1.0f) output = 1.0f;
        if (output < -1.0f) output = -1.0f;
        buffer[i] = output;
    }
}
#endif
#endif

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
            if (value < 0.125f) setDistoMode(0);
            else if (value < 0.25f) setDistoMode(1);
            else if (value < 0.375f) setDistoMode(2);
            else if (value < 0.5f) setDistoMode(3);
            else if (value < 0.625f) setDistoMode(4);
            else if (value < 0.75f) setDistoMode(5);
            else if (value < 0.875f) setDistoMode(6);
            else setDistoMode(7);
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