#pragma once

#include <Arduino.h>
#include "AudioStream.h"
#include "arm_math.h"
#include "../includes/Utils.h"
#include "../includes/Effect.h"

class BypassEffect : public AudioStream {
public:

    BypassEffect() : AudioStream(2, inputQueueArray_) {}

    int StringIndex = 0;
    float preampVal = 1.0f;

    void setStringIndex(int index) { 
        StringIndex = index;
        switch (StringIndex) {
            case 0:
                preampVal = 0.75f * 10.0f;
                break;
            case 1:
                preampVal = 0.75f * 14.0f * 1.0f;
                break;
            case 2:
                preampVal = 0.75f * 12.0f;
                break;
            case 3:
                preampVal = 0.75f * 12.0f;
                break;
            case 4:
                preampVal = 0.75f * 14.0f;
                break;
            case 5:
                preampVal = 0.75f * 12.0f * 2.0f;
                break;
            default: 
                preampVal = 0.75f * 14.0f;
                break;
        }
    }

    // IDs des effets — NONE=0 est réservé (pas d'effet)
    enum EffectID { NONE = 0, DELAY = 1, DISTO = 2, OCTAVER = 3, TREMOLO = 4, NOISEGATE = 5, EQUALIZER = 6, COMPRESSEUR = 7, NUM_EFFECT_IDS = 8 };

    // Tableau de pointeurs polymorphiques indexé par EffectID
    // effects_[0] (NONE) reste nullptr
    Effect* effects_[NUM_EFFECT_IDS] = {};

    void setEffect(int effectID, Effect* fx) {
        if (effectID > 0 && effectID < NUM_EFFECT_IDS)
            effects_[effectID] = fx;
    }

    // 4 slots dynamiques (contrôlés par CC 20, 21, 22, 23 depuis Python)
    int slots_[4] = {NONE, NONE, NONE, NONE}; // Défaut

    void setSlot(int slotIndex, int effectID) {
        slots_[slotIndex] = effectID;
    }

#if PeakAnalysage
    // Retourne le volume RMS lissé de cette corde (0.0 à 1.0)
    float getVolume() const { return volume_; }
#endif

    virtual void update() override; // Implémenté dans Bypass.cpp

private:
    audio_block_t* inputQueueArray_[2];

#if PeakAnalysage
    float volume_ = 0.0f;  // Volume RMS lissé (0.0 à 1.0)
#endif
};