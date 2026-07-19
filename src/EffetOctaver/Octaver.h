#pragma once

#include "../includes/Utils.h"

#include <Arduino.h>
#include "AudioStream.h"

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

class OctaverEffect : public AudioStream {
    public:

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

    float current_ODswell;

    int effect_mode = 0;

    bool odOn = false;
    bool bypass = false;

    OctaverEffect(); 
    virtual void update() override;

    void setMix(float mix);                // Ctrl 2 (0.0 -> 1.0)
    void setVolume(float vol);             // Ctrl 1 (0.0 -> 1.0
    
    void setOctaveMode(int mode);          // 3-Way Switch 2 (0, 1, 2)

    void setParameter(int param_id, float value);

    // --- METHODES ET VARIABLES PARTAGEES ---
    void setEnabled(bool e) { active = e; }
    bool isEnabled() const  { return active; }

private:
    bool active = false; // effet actif ou non
    
    audio_block_t* inputQueueArray_[1];
};
