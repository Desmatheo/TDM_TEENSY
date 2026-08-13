#pragma once

#include "../includes/Utils.h"
#include "../includes/Effect.h"

#include <Arduino.h>
#include "AudioStream.h"
#include "arm_math.h"

class EqualizerEffect 
#if !UtilBypassRoutage
: public AudioStream 
#else 
: public Effect
#endif
{
public:
    EqualizerEffect();

#if !TEENSY
    virtual void update(const float** in, float** out, int idx) override;
#else
#if !UtilBypassRoutage
    virtual void update() override;
#else
    virtual void update(float* buffer, int numSamples) override;
#endif
#endif

    void begin();

    // Règle le gain d'une bande spécifique (0..4) avec une valeur normalisée [0.0, 1.0]
    void setBand(int band_index, float value_norm);
    
    // Définit le volume global de l'effet
    void setVolume(float vol); // volume de 0.0 à 2.0 (1.0 par défaut)
    
    virtual void setParameter(int param_id, float value) override;

private:
    void calculateCoeffs();

    float volume;
    float gains_db[5];

    arm_biquad_casd_df1_inst_f32 iir_inst;
    float pCoeffs[5 * 5];
    float pState[5 * 4];

#if !UtilBypassRoutage
    audio_block_t* inputQueueArray_[1];
#endif
};
