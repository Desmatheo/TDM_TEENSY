// Earth Reverbscape

#include "Octaver.h"

#include <span>

#if eq_ON
namespace q = cycfi::q;
using namespace q::literals;

using namespace daisy;
using namespace daisysp;
#endif



OctaverEffect::OctaverEffect()
    : AudioStream(1, inputQueueArray_),
      octave(AUDIO_SAMPLE_RATE_EXACT / resample_factor)
#if eq_ON
      , eq1(-11, 140_Hz, sampleRate),
      eq2(5, 160_Hz, sampleRate)
#endif
{
    for (int j = 0; j < 6; ++j) {
        buff[j] = 0.0f;
        buff_out[j] = 0.0f;
    }

    setMix(1.0f);
    setOctaveMode(1);

}

void OctaverEffect::update() {

    audio_block_t* in = receiveReadOnly(0);
    if (!in) return;

    // Pour éviter de faire le gros calcul DSP quand l'effet n'est pas actif, bypass complet
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

        buff[bin_counter] = inputL;
        
        if (bin_counter > 4) {
            std::span<const float, resample_factor> in_chunk(&(buff[0]), resample_factor);
            const auto sample = decimate2(in_chunk); 

            float octave_mix = 0.0f;
            octave.update(sample, effect_mode);

            if (effect_mode == 1) octave_mix += octave.up1()    * 6.0f;
            if (effect_mode == 2) octave_mix += octave.down1()  * 6.0f;
            if (effect_mode == 3) octave_mix += octave.down2()  * 6.0f;

            auto out_chunk = interpolate(octave_mix);
            for (size_t j = 0; j < out_chunk.size(); ++j) {
#if eq_ON
                buff_out[j] = eq2(eq1(out_chunk[j]));
#else
                buff_out[j] = out_chunk[j];
#endif
            }
        } 
        // else {
        //     for (size_t j = 0; j < 6; ++j) {
        //         buff_out[j] = 0.0f;
        //     }
        // }

        bin_counter += 1;
        if (bin_counter > 5) bin_counter = 0;

        float octave_signal = buff_out[bin_counter];

        float output = ((inputL * dryMix) + (octave_signal * wetMix)) * volume;

        if (output > 1.0f) output = 1.0f;
        if (output < -1.0f) output = -1.0f;
        out->data[i] = (int16_t)(output * 32767.0f);
    }

    transmit(out, 0);
    release(out);
    release(in);
}

// --- Implémentation des Setters Spécifiques ---

void OctaverEffect::setMix(float mix) {
    dryMix = 1.0f - mix;
    wetMix = mix;
}

void OctaverEffect::setVolume(float vol)
{
    volume = clampf(vol, 0.0f, 1.0f);
}

void OctaverEffect::setOctaveMode(int mode) {
    effect_mode = mode;
}


void OctaverEffect::setParameter(int param_id, float value) {
    switch (param_id){
        case 0 : 
            setMix(value);
            break;
        case 1 :  
            if (value > 0.66f) setOctaveMode(1);
            else if ((0.66f > value) && (value > 0.33f)) setOctaveMode(2);
            else setOctaveMode(3);
            break; 
        case 5 : 
            setVolume(value);
            break;
        default:
            Serial.print("Parametre invalide: ");
            Serial.println(param_id);
            break;
    };
}