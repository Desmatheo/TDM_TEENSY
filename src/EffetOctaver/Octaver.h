#pragma once

#include "../includes/Utils.h"
#include "../includes/Effect.h"

#if TEENSY
#include <Arduino.h>
#include "AudioStream.h"
#endif

#include "Util/Multirate.h"
#include "Util/OctaveGenerator.h"

#define eq_ON 0
#if eq_ON
#include <q/fx/biquad.hpp>
#include <q/support/literals.hpp>

namespace q = cycfi::q;
using namespace daisy;
using namespace daisysp;
#endif

class OctaverEffect
#if !UtilBypassRoutage
: public AudioStream 
#else 
: public Effect
#endif
{
    public:

    OctaverEffect(); 
#if !TEENSY
    virtual void update(const float** in, float** out, int idx) override;
#else
#if !UtilBypassRoutage
    virtual void update() override;
#else 
    virtual void update(float* buffer, int numSamples) override;
#endif
#endif

    void setMix(float mix);                // Ctrl 2 (0.0 -> 1.0)
    void setVolume(float vol);             // Ctrl 1 (0.0 -> 2.0, 1.0 par défaut)

    void setOctaveMode(int mode);          // 3-Way Switch 2 (0, 1, 2)

    void setParameter(int param_id, float value);

private:
#if !UtilBypassRoutage
    audio_block_t* inputQueueArray_[1];
#endif

    float dryMix;
    float wetMix;
    float volume = 1;

    Decimator2 decimate2;
    Interpolator interpolate;
    OctaveGenerator octave;
#if eq_ON
    q::highshelf eq1;
    q::lowshelf eq2;
    Overdrive overdrive;
#endif
    float buff[6];
    float buff_out[6];
    int bin_counter = 0;

    int effect_mode = 0;
};
